#include "wasm.hpp"

#include <cmath> // ceil, trunc, isnan, ...
#include <cstring> // memset
#include <filesystem>

#include "alloc.hpp"
#include "bit.hpp" // rotl, rotr
#include "bitutil.hpp"
#include "Exception.hpp"
#include "MemoryRefReader.hpp"
#include "Reader.hpp"

#define DEBUG_LOAD false
#define DEBUG_VM false
#define DEBUG_API false

#if DEBUG_LOAD || DEBUG_VM || DEBUG_API
#include <iostream>
#include "string.hpp"
#endif

// Good resources:
// - https://webassembly.github.io/wabt/demo/wat2wasm/
// - https://github.com/sunfishcode/wasm-reference-manual/blob/master/WebAssembly.md

/*
Spec tests (https://github.com/WebAssembly/spec/tree/20dc91f64194580a542a302b7e1ab1b003d21617/test/core)
> Use wast2json from wabt then run `soup wast [file]`
> Results:
- address: pass
- align: FAIL (doesn't fail on some malformed modules)
- binary: FAIL (doesn't fail on some malformed modules)
- binary-leb128: FAIL
- block: pass
- br: pass
- br_if: pass
- br_table: pass
- bulk: FAIL
- call: pass
- call_indirect: FAIL
- comments: pass
- const: pass
- conversions: pass
- custom: FAIL (due to missing support for multiple memories?)
- data: FAIL
- elem: FAIL
- endianness:  pass
- exports: FAIL
- f32: pass
- f32_bitwise: pass
- f32_cmp: pass
- f64: pass
- f64_bitwise: pass
- f64_cmp: pass
- fac: pass
- float_exprs: pass
- float_literals: pass
- float_memory: pass
- float_misc: pass
- forward: pass
- func: pass
- func_ptrs: FAIL
- global: FAIL
- i32: pass
- i64: pass
- if: pass
- imports: FAIL
- inline-module: pass
- int_exprs: pass
- int_literals: pass
- labels: pass
- left-to-right: pass
- linking: FAIL (imports are not type-checked; missing support for global imports)
- load: pass
- local_get: pass
- local_set: pass
- local_tee: pass
- loop: pass
- memory: pass
- memory_copy: pass
- memory_fill: pass
- memory_grow: pass
- memory_init: FAIL (missing support for passive data segments)
- memory_redundancy: pass
- memory_size: pass
- memory_trap: pass
- names: FAIL
- nop: pass
- obsolete-keywords: pass
- ref_func: pass
- ref_is_null: WARN (Soup considers an externref with value 0 to be null)
- ref_null: pass
- return: pass
- select: pass
- skip-stack-guard-page: pass
- stack: pass
- start: pass
- store: pass
- switch: pass
- table: FAIL (due to missing support for table imports)
- table-sub: pass (Soup doesn't do static validation)
- table_copy: FAIL
- table_fill: FAIL
- table_get: FAIL
- table_grow: FAIL
- table_init: FAIL
- table_set: FAIL
- table_size: FAIL
- token: FAIL (due to missing support for passive data segments, lol)
- traps: pass
- type: pass
- unreachable: pass
- unreached-invalid: pass (Soup doesn't do static validation)
- unreached-valid: pass
- unwind: pass
- utf8-custom-section-id: FAIL
- utf8-import-field: pass (probably not for the right reason, but lol)
- utf8-import-module: pass (probably not for the right reason, but lol)
- utf8-invalid-encoding: pass (probably not for the right reason, but lol)
*/

NAMESPACE_SOUP
{
	WasmType wasm_type_from_string(const std::string& str) noexcept
	{
		if (str == "i32") { return WASM_I32; }
		if (str == "i64") { return WASM_I64; }
		if (str == "f32") { return WASM_F32; }
		if (str == "f64") { return WASM_F64; }
		if (str == "funcref") { return WASM_FUNCREF; }
		if (str == "externref") { return WASM_EXTERNREF; }
		return static_cast<WasmType>(0);
	}

	std::string wasm_type_to_string(WasmType type) SOUP_EXCAL
	{
		switch (type)
		{
		case WASM_I32: return "i32";
		case WASM_I64: return "i64";
		case WASM_F32: return "f32";
		case WASM_F64: return "f64";
		case WASM_FUNCREF: return "funcref";
		case WASM_EXTERNREF: return "externref";
		}
		return std::to_string(type);
	}

	// WasmScript::Memory

	WasmScript::Memory::~Memory() noexcept
	{
		if (data != nullptr)
		{
			soup::free(data);
		}
	}

	std::string WasmScript::Memory::readString(size_t addr, size_t size) SOUP_EXCAL
	{
		SOUP_IF_LIKELY (auto ptr = getView(addr, size))
		{
			return std::string((const char*)ptr, size);
		}
		return std::string();
	}

	std::string WasmScript::Memory::readNullTerminatedString(size_t addr) SOUP_EXCAL
	{
		std::string str;
		for (; addr < this->size; ++addr)
		{
			const auto c = static_cast<char>(this->data[addr]);
			SOUP_IF_UNLIKELY(!c)
			{
				break;
			}
			str.push_back(c);
		}
		return str;
	}

	bool WasmScript::Memory::write(size_t addr, const void* src, size_t size) noexcept
	{
		SOUP_IF_LIKELY (auto ptr = getView(addr, size))
		{
			memcpy(ptr, src, size);
			return true;
		}
		return false;
	}

	bool WasmScript::Memory::write(const WasmValue& addr, const void* src, size_t size) noexcept
	{
		return write(decodeUPTR(addr), src, size);
	}

	/*intptr_t WasmScript::Memory::decodeIPTR(const WasmValue& in) noexcept
	{
		return memory64 ? in.i64 : in.i32;
	}*/

	size_t WasmScript::Memory::decodeUPTR(const WasmValue& in) noexcept
	{
		return memory64 ? static_cast<uint64_t>(in.i64) : static_cast<uint32_t>(in.i32);
	}

	/*void WasmScript::Memory::encodeIPTR(WasmValue& out, intptr_t in) noexcept
	{
		if (memory64)
		{
			out = static_cast<int64_t>(in);
		}
		else
		{
			out = static_cast<int32_t>(in);
		}
	}*/

	void WasmScript::Memory::encodeUPTR(WasmValue& out, size_t in) noexcept
	{
		if (memory64)
		{
			out = static_cast<uint64_t>(in);
		}
		else
		{
			out = static_cast<uint32_t>(in);
		}
	}

	size_t WasmScript::Memory::grow(size_t delta_pages) noexcept
	{
		const auto delta_bytes = delta_pages * 0x10'000;
		auto nmem = (((this->size + delta_bytes) / 0x10'000) <= this->page_limit)
			? (uint8_t*)::realloc(this->data, this->size + delta_bytes)
			: nullptr
			;
		size_t old_size_pages = -1;
		if (nmem != nullptr)
		{
			memset(&nmem[this->size], 0, delta_bytes);
			old_size_pages = this->size / 0x10'000;
			this->data = nmem;
			this->size += delta_bytes;
		}
		return old_size_pages;
	}
	
	// WasmScript

	bool WasmScript::load(const std::string& data) SOUP_EXCAL
	{
		MemoryRefReader r(data);
		return load(r);
	}

	bool WasmScript::readConstant(Reader& r, WasmValue& out) noexcept
	{
		uint8_t op;
		r.u8(op);
		switch (op)
		{
		case 0x41: // i32.const
			r.soml(out.i32);
			out.type = WASM_I32;
			break;

		case 0x42: // i64.const
			r.soml(out.i64);
			out.type = WASM_I64;
			break;

		case 0x43: // f32.const
			r.f32(out.f32);
			out.type = WASM_F32;
			break;

		case 0x44: // f64.const
			r.f64(out.f64);
			out.type = WASM_F64;
			break;

		case 0xd0: // ref.null
			out.i64 = 0;
			r.u8(reinterpret_cast<uint8_t&>(out.type)); static_assert(sizeof(WasmType) == sizeof(uint8_t));
			break;

		case 0xd2: // ref.func
			r.oml(out.i32);
			out.i64 |= 0x1'0000'0000;
			out.type = WASM_FUNCREF;
			break;

		default:
#if DEBUG_LOAD
			std::cout << "unexpected op for constant/initialisation: " << string::hex(op) << "\n";
#endif
			return false;
		}
		r.u8(op);
		SOUP_IF_UNLIKELY (op != 0x0b) // end
		{
#if DEBUG_LOAD
			std::cout << "missing end for constant/initialisation\n";
#endif
			return false;
		}
		return true;
	}

	bool WasmScript::load(Reader& r) SOUP_EXCAL
	{
		uint32_t u;
		r.u32_le(u);
		if (u != 1836278016) // magic - '\0asm' in little-endian
		{
			return false;
		}
		r.u32_le(u);
		if (u != 1) // version
		{
			return false;
		}
		while (r.hasMore())
		{
			uint8_t section_type;
			r.u8(section_type);
			size_t section_size;
			r.oml(section_size);
			switch (section_type)
			{
			default:
#if DEBUG_LOAD
				std::cout << "Ignoring section of type " << (int)section_type << " (size: " << section_size << ")\n";
#endif
				SOUP_IF_UNLIKELY (section_size == 0)
				{
					return false;
				}
				r.skip(section_size);
				break;

			case 1: // Type
				{
					size_t num_types;
					r.oml(num_types);
#if DEBUG_LOAD
					std::cout << num_types << " type(s)\n";
#endif
					while (num_types--)
					{
						uint8_t type_type;
						r.u8(type_type);
						switch (type_type)
						{
						default:
#if DEBUG_LOAD
							std::cout << "unknown type: " << string::hex(type_type) << "\n";
#endif
							return false;

						case 0x60: // func
							{
#if DEBUG_LOAD
								std::cout << "- function with ";
#endif
								uint32_t size;
								r.oml(size);
#if DEBUG_LOAD
								std::cout << size << " parameter(s) and ";
#endif
								std::vector<WasmType> parameters;
								parameters.reserve(size);
								for (uint32_t i = 0; i != size; ++i)
								{
									uint8_t type;
									r.u8(type);
									parameters.emplace_back(static_cast<WasmType>(type));
								}

								r.oml(size);
#if DEBUG_LOAD
								std::cout << size << " return value(s)\n";
#endif
								std::vector<WasmType> results;
								results.reserve(size);
								for (uint32_t i = 0; i != size; ++i)
								{
									uint8_t type;
									r.u8(type);
									results.emplace_back(static_cast<WasmType>(type));
								}

								types.emplace_back(WasmFunctionType{ std::move(parameters), std::move(results) });
							}
							break;
						}
					}
				}
				break;

			case 2: // Import
				{
					size_t num_imports;
					r.oml(num_imports);
#if DEBUG_LOAD
					std::cout << num_imports << " import(s)\n";
#endif
					while (num_imports--)
					{
						size_t module_name_len; r.oml(module_name_len);
						std::string module_name; r.str(module_name_len, module_name);
						size_t field_name_len; r.oml(field_name_len);
						std::string field_name; r.str(field_name_len, field_name);
#if DEBUG_LOAD
						std::cout << "- " << module_name << ":" << field_name << "\n";
#endif
						uint8_t kind; r.u8(kind);
						if (kind == 0) // function
						{
							uint32_t type_index; r.oml(type_index);
							function_imports.emplace_back(FunctionImport{ std::move(module_name), std::move(field_name), nullptr, {}, type_index, (uint32_t)-1 });
						}
						/*else if (kind == 1) // table
						{
							uint8_t type; r.u8(type);
							uint8_t flags; r.u8(flags);
							size_t size; r.oml(size);
							if (flags & 1)
							{
								r.oml(size);
							}
						}*/
						else
						{
#if DEBUG_LOAD
							std::cout << "unknown import kind: " << string::hex(kind) << "\n";
#endif
							return false;
						}
					}
				}
				break;

			case 3: // Function
				{
					size_t num_functions;
					r.oml(num_functions);
#if DEBUG_LOAD
					std::cout << num_functions << " function(s)\n";
#endif
					functions.reserve(num_functions);
					while (num_functions--)
					{
						uint32_t type_index;
						r.oml(type_index);
						functions.emplace_back(type_index);
					}
				}
				break;

			case 4: // Table
				{
					size_t num_tables; r.oml(num_tables);
#if DEBUG_LOAD
					std::cout << num_tables << " table(s)\n";
#endif
					tables.reserve(num_tables);
					while (num_tables--)
					{
						uint8_t type;
						r.u8(type);
						uint8_t flags;
						r.u8(flags);
						size_t initial;
						r.oml(initial);
						if (flags & 1)
						{
							size_t maximum;
							r.oml(maximum);
						}
						auto& tbl = tables.emplace_back(static_cast<WasmType>(type));
						while (initial != tbl.values.size())
						{
							tbl.values.emplace_back();
						}
					}
				}
				break;

			case 5: // Memory
				{
					size_t num_memories; r.oml(num_memories);
					SOUP_IF_UNLIKELY (memory.data != nullptr || num_memories != 1)
					{
#if DEBUG_LOAD
						std::cout << "Too many memories\n";
#endif
						return false;
					}
					uint8_t flags; r.u8(flags);
					size_t pages; r.oml(pages);
					if (flags & 1)
					{
						uint64_t page_limit;
						r.oml(page_limit);
						memory.page_limit = page_limit;
					}
					if (flags & 4)
					{
						memory.memory64 = true;
					}
					if (pages == 0)
					{
						memory.data = (uint8_t*)soup::malloc(1);
						memory.size = 1;
					}
					else
					{
						memory.data = (uint8_t*)soup::malloc(pages * 0x10'000);
						memory.size = pages * 0x10'000;
					}
					memset(memory.data, 0, memory.size);
#if DEBUG_LOAD
					std::cout << "Memory consists of " << pages << " pages, totalling " << memory.size << " bytes\n";
#endif
				}
				break;

			case 6: // Global
				{
					size_t num_globals;
					r.oml(num_globals);
#if DEBUG_LOAD
					std::cout << num_globals << " global(s)\n";
#endif
					globals.reserve(num_globals);
					while (num_globals--)
					{
						uint8_t type; r.u8(type);
						r.skip(1); // mutability
						WasmValue& value = globals.emplace_back();
						SOUP_RETHROW_FALSE(readConstant(r, value));
						SOUP_IF_UNLIKELY (value.type != type)
						{
#if DEBUG_LOAD
							std::cout << "constant's type differs from global's type\n";
#endif
							return false;
						}
					}
				}
				break;

			case 7: // Export
				{
					size_t num_exports;
					r.oml(num_exports);
#if DEBUG_LOAD
					std::cout << num_exports << " export(s)\n";
#endif
					while (num_exports--)
					{
						size_t name_len;
						r.oml(name_len);
						std::string name;
						r.str(name_len, name);
#if DEBUG_LOAD
						std::cout << "- " << name << "\n";
#endif
						uint8_t kind; r.u8(kind);
						uint32_t index; r.oml(index);
						if (kind == 0) // function 
						{
							export_map.emplace(std::move(name), index);
						}
						// kind 2 = memory
					}
				}
				break;

			case 8: // Start
				SOUP_IF_UNLIKELY (start_func_idx != -1)
				{
#if DEBUG_LOAD
					std::cout << "Too many start sections\n";
#endif
					return false;
				}
				r.oml(start_func_idx);
#if DEBUG_LOAD
				std::cout << "start_func_idx: " << start_func_idx << "\n";
#endif
				break;

			case 9: // Elem
				{
					size_t num_segments;
					r.oml(num_segments);
#if DEBUG_LOAD
					std::cout << num_segments << " element segment(s)\n";
#endif
					while (num_segments--)
					{
						uint8_t flags;
						r.u8(flags);
						uint32_t tblidx = 0;
						if (flags & 2)
						{
							r.oml(tblidx);
						}
						if (flags & 1)
						{
#if DEBUG_LOAD
							std::cout << "skipping over passive/declarative elements\n";
#endif
							size_t num_elements;
							r.oml(num_elements);
							while (num_elements--)
							{
								uint32_t function_index;
								r.oml(function_index);
							}
						}
						else
						{
#if DEBUG_LOAD
							std::cout << "- elements for table " << tblidx << "\n";
#endif
							SOUP_IF_UNLIKELY (tblidx >= tables.size())
							{
#if DEBUG_LOAD
								std::cout << "no such table\n";
#endif
								return false;
							}
							auto& table = tables[tblidx];
							uint8_t op;
							r.u8(op);
							SOUP_IF_UNLIKELY (op != 0x41) // i32.const
							{
#if DEBUG_LOAD
								std::cout << "unexpected op for element initialisation: " << string::hex(op) << "\n";
#endif
								return false;
							}
							uint32_t index; r.oml(index);
							r.u8(op);
							SOUP_IF_UNLIKELY (op != 0x0b) // end
							{
#if DEBUG_LOAD
								std::cout << "missing end in element initialisation\n";
#endif
								return false;
							}
							if (flags & 2)
							{
								r.skip(1); // reserved
							}
							size_t num_elements;
							r.oml(num_elements);
							SOUP_IF_UNLIKELY (index + num_elements > table.values.size())
							{
#if DEBUG_LOAD
								std::cout << "elem: " << index << " + " << num_elements << " > " << table.values.size() << "\n";
#endif
								return false;
							}
							while (num_elements--)
							{
								uint32_t function_index;
								r.oml(function_index);
								if (table.type == WASM_FUNCREF)
								{
									table.values[index++] = 0x1'0000'0000 | function_index;
								}
							}
						}
					}
				}
				break;

			case 10: // Code
				{
					size_t num_functions;
					r.oml(num_functions);
#if DEBUG_LOAD
					std::cout << num_functions << " function(s)\n";
#endif
					while (num_functions--)
					{
						size_t body_size;
						r.oml(body_size);
						SOUP_IF_UNLIKELY (body_size == 0)
						{
							return false;
						}
						std::string body;
						r.str(body_size, body);
#if DEBUG_LOAD
						std::cout << "- " << string::bin2hex(body) << "\n";
#endif
						code.emplace_back(std::move(body));
					}
				}
				break;

			case 11: // Data
				{
					size_t num_segments;
					r.oml(num_segments);
#if DEBUG_LOAD
					std::cout << num_segments << " data segment(s)\n";
#endif
					while (num_segments--)
					{
						uint8_t flags;
						r.u8(flags);
						SOUP_IF_UNLIKELY (flags != 0)
						{
#if DEBUG_LOAD
							std::cout << "unexpected data segment flags: " << string::hex(flags) << "\n";
#endif
							return false;
						}
						WasmValue base;
						SOUP_RETHROW_FALSE(readConstant(r, base));
						SOUP_IF_UNLIKELY (base.type != (memory.memory64 ? WASM_I64 : WASM_I32))
						{
#if DEBUG_LOAD
							std::cout << "unexpected type for data initialisation: " << string::hex(static_cast<uint8_t>(base.type)) << "\n";
#endif
							return false;
						}
						size_t size; r.oml(size);
						auto ptr = memory.getView(memory.decodeUPTR(base), size);
						SOUP_IF_UNLIKELY (!ptr)
						{
#if DEBUG_LOAD
							std::cout << "data segment exceeds memory range: " << memory.decodeUPTR(base) << " + " << size << " > " << memory.size << "\n";
#endif
							return false;
						}
						r.raw(ptr, size);
					}
				}
				break;
			}
			if (section_size == 0)
			{
				r.oml(section_size);
#if DEBUG_LOAD
				std::cout << "FIXUP section_size=" << section_size << "\n";
#endif
			}
		}
		return true;
	}

	bool WasmScript::instantiate()
	{
		if (start_func_idx != -1)
		{
			return this->call(start_func_idx);
		}
		return true;
	}

	WasmScript::FunctionImport* WasmScript::getImportedFunction(const std::string& module_name, const std::string& function_name) noexcept
	{
		for (auto& fi : function_imports)
		{
			if (fi.function_name == function_name
				&& fi.module_name == module_name
				)
			{
				return &fi;
			}
		}
		return nullptr;
	}

	void WasmScript::importFromModule(const std::string& module_name, const SharedPtr<WasmScript>& other)
	{
		for (auto& fi : function_imports)
		{
			if (fi.module_name == module_name)
			{
				if (auto e = other->export_map.find(fi.function_name); e != other->export_map.end())
				{
					fi.source = other;
					fi.func_index = e->second;
				}
			}
		}
	}

	const std::string* WasmScript::getExportedFuntion(const std::string& name, const WasmFunctionType** optOutType) const noexcept
	{
		if (auto e = export_map.find(name); e != export_map.end())
		{
			const size_t i = (e->second - function_imports.size());
			if (i < code.size() && i < functions.size())
			{
				if (optOutType)
				{
					const auto type_index = functions[i];
					SOUP_IF_UNLIKELY (type_index >= types.size())
					{
						return nullptr;
					}
					*optOutType = &types[type_index];
				}
				return &code[i];
			}
		}
		return nullptr;
	}

	uint32_t WasmScript::getExportedFuntion2(const std::string& name) const noexcept
	{
		if (auto e = export_map.find(name); e != export_map.end())
		{
			return e->second;
		}
		return -1;
	}

	uint32_t WasmScript::getTypeIndexForFunction(uint32_t func_index) const noexcept
	{
		if (func_index < function_imports.size())
		{
			return function_imports[func_index].type_index;
		}
		func_index -= function_imports.size();
		if (func_index < functions.size())
		{
			return functions[func_index];
		}
		return -1;
	}

#define API_CHECK_STACK(x) SOUP_IF_UNLIKELY (vm.stack.size() < x) { throw Exception("Insufficient values on stack for function call"); }

	// https://github.com/WebAssembly/wasi-libc/blob/d02bdc21afc4d835383b006c11e285c4a7c78439/libc-bottom-half/headers/public/wasi/wasip1.h#L106
	enum WasiErrno : int32_t
	{
		WASI_ERRNO_SUCCESS = 0,
		WASI_ERRNO_BADF = 8,
		WASI_ERRNO_NAMETOOLONG = 37,
		WASI_ERRNO_NOENT = 44,
	};

	// The first 'fd' value that refers to an index in WasiData::files.
	static constexpr uint32_t WASI_FD_FILES_BASE = 10000;

	struct WasiData
	{
		std::vector<std::string> args;
		std::vector<FILE*> files;

		~WasiData() noexcept
		{
			for (const auto& fd : files)
			{
				fclose(fd);
			}
		}
	};

	void WasmScript::linkWasiPreview1(std::vector<std::string> args) noexcept
	{
		{
			WasiData& wd = custom_data.getStructFromMap(WasiData);
			wd.args = std::move(args);
		}

		// Resources:
		// - Barebones "Hello World": https://github.com/bytecodealliance/wasmtime/blob/main/docs/WASI-tutorial.md#web-assembly-text-example
		// - How the XCC compiler uses WASI:
		//   - https://github.com/tyfkda/xcc/blob/main/libsrc/_wasm/wasi.h
		//   - https://github.com/tyfkda/xcc/blob/main/libsrc/_wasm/crt0/_start.c#L41

		if (auto fi = getImportedFunction("wasi_snapshot_preview1", "args_sizes_get"))
		{
			fi->ptr = [](WasmVm& vm, uint32_t func_index, const WasmFunctionType&)
			{
				API_CHECK_STACK(2);
				auto plen = vm.stack.top().i32; vm.stack.pop();
				auto pargc = vm.stack.top().i32; vm.stack.pop();
				WasiData& wd = vm.script.custom_data.getStructFromMapConst(WasiData);
				if (auto pLen = vm.script.memory.getPointer<int32_t>(plen))
				{
					*pLen = 0;
					for (const auto& arg : wd.args)
					{
						*pLen += arg.size() + 1;
					}
				}
				if (auto pArgc = vm.script.memory.getPointer<int32_t>(pargc))
				{
					*pArgc = wd.args.size();
				}
				vm.stack.push(WASI_ERRNO_SUCCESS);
			};
		}
		if (auto fi = getImportedFunction("wasi_snapshot_preview1", "args_get"))
		{
			fi->ptr = [](WasmVm& vm, uint32_t func_index, const WasmFunctionType&)
			{
				API_CHECK_STACK(2);
				auto pstr = vm.stack.top().i32; vm.stack.pop();
				auto pargv = vm.stack.top().i32; vm.stack.pop();
				WasiData& wd = vm.script.custom_data.getStructFromMapConst(WasiData);
				std::string argstr;
				for (uint32_t i = 0; i != wd.args.size(); ++i)
				{
					if (auto pArg = vm.script.memory.getPointer<int32_t>(pargv + i * 4))
					{
						*pArg = pstr + argstr.size();
					}
					argstr.append(wd.args[i].data(), wd.args[i].size() + 1);
				}
				vm.script.memory.write(pstr, argstr.data(), argstr.size());
				vm.stack.push(WASI_ERRNO_SUCCESS);
			};
		}
		if (auto fi = getImportedFunction("wasi_snapshot_preview1", "environ_sizes_get"))
		{
			fi->ptr = [](WasmVm& vm, uint32_t func_index, const WasmFunctionType&)
			{
				API_CHECK_STACK(2);
				auto out_environ_buf_size = vm.stack.top().i32; vm.stack.pop();
				auto out_environ_count = vm.stack.top().i32; vm.stack.pop();
				if (auto ptr = vm.script.memory.getPointer<int32_t>(out_environ_count))
				{
					*ptr = 0;
				}
				if (auto ptr = vm.script.memory.getPointer<int32_t>(out_environ_buf_size))
				{
					*ptr = 0;
				}
				vm.stack.push(WASI_ERRNO_SUCCESS);
			};
		}
		if (auto fi = getImportedFunction("wasi_snapshot_preview1", "proc_exit"))
		{
			fi->ptr = [](WasmVm& vm, uint32_t func_index, const WasmFunctionType&)
			{
				API_CHECK_STACK(1);
				auto code = vm.stack.top().i32; vm.stack.pop();
				exit(code);
			};
		}
		if (auto fi = getImportedFunction("wasi_snapshot_preview1", "fd_prestat_get"))
		{
			fi->ptr = [](WasmVm& vm, uint32_t func_index, const WasmFunctionType&)
			{
				API_CHECK_STACK(2);
				auto prestat = vm.stack.top().i32; vm.stack.pop();
				auto fd = vm.stack.top().i32; vm.stack.pop();
#if DEBUG_API
				std::cout << "prestat on fd " << fd << "\n";
#endif
				if (fd == 3)
				{
					if (auto pTag = vm.script.memory.getPointer<uint32_t>(prestat + 0))
					{
						*pTag = 0; // __WASI_PREOPENTYPE_DIR
					}
					if (auto pDirNameLen = vm.script.memory.getPointer<uint32_t>(prestat + 4))
					{
						*pDirNameLen = 1;
					}
					vm.stack.push(WASI_ERRNO_SUCCESS);
				}
				else
				{
					vm.stack.push(WASI_ERRNO_BADF);
				}
			};
		}
		if (auto fi = getImportedFunction("wasi_snapshot_preview1", "fd_prestat_dir_name"))
		{
			fi->ptr = [](WasmVm& vm, uint32_t func_index, const WasmFunctionType&)
			{
				API_CHECK_STACK(3);
				auto path_len = vm.stack.top().i32; vm.stack.pop();
				auto path = vm.stack.top().i32; vm.stack.pop();
				auto fd = vm.stack.top().i32; vm.stack.pop();
				if (fd == 3)
				{
					if (path_len >= 1)
					{
						vm.script.memory.write(path, ".", 1);
						vm.stack.push(WASI_ERRNO_SUCCESS);
					}
					else
					{
						vm.stack.push(WASI_ERRNO_NAMETOOLONG);
					}
				}
				else
				{
					vm.stack.push(WASI_ERRNO_BADF);
				}
			};
		}
		if (auto fi = getImportedFunction("wasi_snapshot_preview1", "fd_filestat_get"))
		{
			fi->ptr = [](WasmVm& vm, uint32_t func_index, const WasmFunctionType&)
			{
				API_CHECK_STACK(2);
				auto out = vm.stack.top().i32; vm.stack.pop();
				auto fd = vm.stack.top().i32; vm.stack.pop();
				SOUP_UNUSED(out);
				vm.stack.push(fd < 3 ? WASI_ERRNO_SUCCESS : WASI_ERRNO_BADF);
			};
		}
		if (auto fi = getImportedFunction("wasi_snapshot_preview1", "fd_fdstat_get"))
		{
			fi->ptr = [](WasmVm& vm, uint32_t func_index, const WasmFunctionType&)
			{
				API_CHECK_STACK(2);
				auto out = vm.stack.top().i32; vm.stack.pop(); // https://github.com/WebAssembly/wasi-libc/blob/d02bdc21afc4d835383b006c11e285c4a7c78439/libc-bottom-half/headers/public/wasi/wasip1.h#L945
				auto fd = vm.stack.top().i32; vm.stack.pop();
#if DEBUG_API
				std::cout << "fdstat on fd " << fd << "\n";
#endif
				if (fd == 3)
				{
					if (auto pFiletype = vm.script.memory.getPointer<uint8_t>(out + 0))
					{
						*pFiletype = 3; // directory, as per https://github.com/WebAssembly/wasi-libc/blob/d02bdc21afc4d835383b006c11e285c4a7c78439/libc-bottom-half/headers/public/wasi/wasip1.h#L785
					}
					if (auto pFlags = vm.script.memory.getPointer<uint16_t>(out + 2))
					{
						*pFlags = 0;
					}
					if (auto pRightsBase = vm.script.memory.getPointer<uint64_t>(out + 8))
					{
						*pRightsBase = -1;
					}
					if (auto pRightsInheriting = vm.script.memory.getPointer<uint64_t>(out + 16))
					{
						*pRightsInheriting = -1;
					}
					vm.stack.push(WASI_ERRNO_SUCCESS);
				}
				else
				{
					vm.stack.push(WASI_ERRNO_BADF);
				}
			};
		}
		if (auto fi = getImportedFunction("wasi_snapshot_preview1", "path_filestat_get"))
		{
			fi->ptr = [](WasmVm& vm, uint32_t func_index, const WasmFunctionType&)
			{
				API_CHECK_STACK(5);
				auto buf = vm.stack.top().i32; vm.stack.pop(); // https://github.com/WebAssembly/wasi-libc/blob/d02bdc21afc4d835383b006c11e285c4a7c78439/libc-bottom-half/headers/public/wasi/wasip1.h#L1064
				auto path_len = vm.stack.top().i32; vm.stack.pop();
				auto path = vm.stack.top().i32; vm.stack.pop();
				auto flags = vm.stack.top().i32; vm.stack.pop();
				auto fd = vm.stack.top().i32; vm.stack.pop();
				if (fd == 3)
				{
					auto path_str = vm.script.memory.readString(path, path_len);
#if DEBUG_API
					std::cout << "path_filestat_get: " << path_str << " (relative to .)\n";
#endif
					SOUP_UNUSED(flags);
					if (auto pFiletype = vm.script.memory.getPointer<uint8_t>(buf + 16))
					{
						*pFiletype = 4; // regular file, as per https://github.com/WebAssembly/wasi-libc/blob/d02bdc21afc4d835383b006c11e285c4a7c78439/libc-bottom-half/headers/public/wasi/wasip1.h#L785
					}
					if (auto pSize = vm.script.memory.getPointer<uint64_t>(buf + 32))
					{
						*pSize = std::filesystem::file_size(path_str);
#if DEBUG_API
						std::cout << "path_filestat_get: size=" << *pSize << "\n";
#endif
					}
					vm.stack.push(WASI_ERRNO_SUCCESS);
				}
				else
				{
					vm.stack.push(WASI_ERRNO_BADF);
				}
			};
		}
		if (auto fi = getImportedFunction("wasi_snapshot_preview1", "path_open"))
		{
			fi->ptr = [](WasmVm& vm, uint32_t func_index, const WasmFunctionType&)
			{
				API_CHECK_STACK(8);
				auto out_fd = vm.stack.top().i32; vm.stack.pop();
				auto fdflags = vm.stack.top().i32; vm.stack.pop();
				auto fs_rights_inheriting = vm.stack.top().i64; vm.stack.pop();
				auto fs_rights_base = vm.stack.top().i64; vm.stack.pop();
				auto oflags = vm.stack.top().i32; vm.stack.pop();
				auto path_len = vm.stack.top().i32; vm.stack.pop();
				auto path = vm.stack.top().i32; vm.stack.pop();
				auto dirflags = vm.stack.top().i32; vm.stack.pop();
				auto fd = vm.stack.top().i32; vm.stack.pop();
				if (fd == 3)
				{
					auto path_str = vm.script.memory.readString(path, path_len);
#if DEBUG_API
					std::cout << "path_open: " << path_str << " (relative to .)\n";
#endif
					SOUP_UNUSED(dirflags);
					SOUP_UNUSED(oflags);
					SOUP_UNUSED(fs_rights_base);
					SOUP_UNUSED(fs_rights_inheriting);
					SOUP_UNUSED(fdflags);
					if (auto f = fopen(path_str.c_str(), "rb"))
					{
						WasiData& wd = vm.script.custom_data.getStructFromMapConst(WasiData);
						if (auto pOutFd = vm.script.memory.getPointer<uint32_t>(out_fd))
						{
							*pOutFd = WASI_FD_FILES_BASE + wd.files.size();
						}
						wd.files.emplace_back(f);
						vm.stack.push(WASI_ERRNO_SUCCESS);
					}
					else
					{
						vm.stack.push(WASI_ERRNO_NOENT);
					}
				}
				else
				{
					vm.stack.push(WASI_ERRNO_BADF);
				}
			};
		}
		if (auto fi = getImportedFunction("wasi_snapshot_preview1", "fd_seek"))
		{
			fi->ptr = [](WasmVm& vm, uint32_t func_index, const WasmFunctionType&)
			{
				API_CHECK_STACK(4);
				auto out_off = vm.stack.top().i32; vm.stack.pop();
				auto whence = vm.stack.top().i32; vm.stack.pop();
				auto delta = vm.stack.top().i64; vm.stack.pop();
				auto fd = vm.stack.top().i32; vm.stack.pop();
#if DEBUG_API
				std::cout << "fd_seek: fd=" << fd << ", delta=" << delta << ", whence=" << whence << "\n";
#endif
				WasiData& wd = vm.script.custom_data.getStructFromMapConst(WasiData);
				if (fd >= WASI_FD_FILES_BASE && fd - WASI_FD_FILES_BASE < wd.files.size())
				{
					auto f = wd.files[fd - WASI_FD_FILES_BASE];
					fseek(f, delta, whence == 0 ? SEEK_SET : (whence == 1 ? SEEK_CUR : (whence == 2 ? SEEK_END : (SOUP_UNREACHABLE,SEEK_END))));
					if (auto pOutOff = vm.script.memory.getPointer<uint64_t>(out_off))
					{
						*pOutOff = ftell(f);
#if DEBUG_API
						std::cout << "fd_seek: offset is now " << *pOutOff << "\n";
#endif
					}
					vm.stack.push(WASI_ERRNO_SUCCESS);
				}
				else
				{
					vm.stack.push(WASI_ERRNO_BADF);
				}
			};
		}
		if (auto fi = getImportedFunction("wasi_snapshot_preview1", "fd_read"))
		{
			fi->ptr = [](WasmVm& vm, uint32_t func_index, const WasmFunctionType&)
			{
				API_CHECK_STACK(4);
				auto out_nread = vm.stack.top().i32; vm.stack.pop();
				auto iovs_len = vm.stack.top().i32; vm.stack.pop();
				auto iovs = vm.stack.top().i32; vm.stack.pop();
				auto fd = vm.stack.top().i32; vm.stack.pop();
				WasiData& wd = vm.script.custom_data.getStructFromMapConst(WasiData);
				FILE* f = nullptr;
				if (fd == 0)
				{
					f = stdin;
				}
				else if (fd >= WASI_FD_FILES_BASE && fd - WASI_FD_FILES_BASE < wd.files.size())
				{
					f = wd.files[fd - WASI_FD_FILES_BASE];
				}
				if (f)
				{
					int32_t nread = 0;
					while (iovs_len--)
					{
						int32_t iov_base = 0;
						if (auto ptr = vm.script.memory.getPointer<int32_t>(iovs))
						{
							iov_base = *ptr;
						}
						iovs += 4;
						int32_t iov_len = 0;
						if (auto ptr = vm.script.memory.getPointer<int32_t>(iovs))
						{
							iov_len = *ptr;
						}
						iovs += 4;
						if (auto ptr = vm.script.memory.getView(iov_base, iov_len))
						{
							const auto ret = fread(ptr, 1, iov_len, f);
							if (ret >= 0)
							{
								nread += ret;
							}
						}
					}
					if (auto pOut = vm.script.memory.getPointer<int32_t>(out_nread))
					{
#if DEBUG_API
						std::cout << "read " << nread << " bytes from fd " << fd << "\n";
#endif
						*pOut = nread;
					}
					vm.stack.push(WASI_ERRNO_SUCCESS);
				}
				else
				{
					vm.stack.push(WASI_ERRNO_BADF);
				}
			};
		}
		if (auto fi = getImportedFunction("wasi_snapshot_preview1", "fd_write"))
		{
			fi->ptr = [](WasmVm& vm, uint32_t func_index, const WasmFunctionType&)
			{
				API_CHECK_STACK(4);
				auto out_nwritten = vm.stack.top().i32; vm.stack.pop();
				auto iovs_len = vm.stack.top().i32; vm.stack.pop();
				auto iovs = vm.stack.top().i32; vm.stack.pop();
				auto fd = vm.stack.top().i32; vm.stack.pop();
				//std::cout << "fd_write on fd " << fd << "\n";
				FILE* f = nullptr;
				if (fd == 1)
				{
					f = stdout;
				}
				else if (fd == 2)
				{
					f = stderr;
				}
				if (f)
				{
					int32_t nwritten = 0;
					while (iovs_len--)
					{
						int32_t iov_base = 0;
						if (auto ptr = vm.script.memory.getPointer<int32_t>(iovs))
						{
							iov_base = *ptr;
						}
						iovs += 4;
						int32_t iov_len = 0;
						if (auto ptr = vm.script.memory.getPointer<int32_t>(iovs))
						{
							iov_len = *ptr;
						}
						iovs += 4;
						if (auto ptr = vm.script.memory.getView(iov_base, iov_len))
						{
							const auto ret = fwrite(ptr, 1, iov_len, f);
							if (ret >= 0)
							{
								nwritten += ret;
							}
						}
					}
					if (auto pOut = vm.script.memory.getPointer<int32_t>(out_nwritten))
					{
						*pOut = nwritten;
					}
					vm.stack.push(WASI_ERRNO_SUCCESS);
				}
				else
				{
					vm.stack.push(WASI_ERRNO_BADF);
				}
			};
		}
		if (auto fi = getImportedFunction("wasi_snapshot_preview1", "fd_close"))
		{
			fi->ptr = [](WasmVm& vm, uint32_t func_index, const WasmFunctionType&)
			{
				API_CHECK_STACK(1);
				auto fd = vm.stack.top().i32; vm.stack.pop();
#if DEBUG_API
				std::cout << "close fd " << fd << "\n";
#endif
				SOUP_UNUSED(fd);
				vm.stack.push(WASI_ERRNO_SUCCESS);
			};
		}
	}

	void WasmScript::linkSpectestShim() noexcept
	{
		if (auto fi = getImportedFunction("spectest", "print_i32"))
		{
			fi->ptr = [](WasmVm& vm, uint32_t func_index, const WasmFunctionType&)
			{
				API_CHECK_STACK(1);
				vm.stack.pop();
			};
		}
		if (auto fi = getImportedFunction("spectest", "print"))
		{
			fi->ptr = [](WasmVm& vm, uint32_t func_index, const WasmFunctionType&)
			{
				// This function is apparently overloaded, so in theory it might have to pop a variable number of arguments.
			};
		}
	}

	bool WasmScript::call(uint32_t func_index, std::vector<WasmValue>&& args, std::stack<WasmValue>* out)
	{
		WasmVm vm(*this);
		if (func_index < function_imports.size())
		{
			const auto& imp = function_imports[func_index];
#if DEBUG_LOAD || DEBUG_API
			std::cout << "Calling into " << imp.module_name << ":" << imp.function_name << "\n";
#endif
			for (auto& arg : args)
			{
				vm.stack.emplace(std::move(arg));
			}
			if (imp.ptr)
			{
				SOUP_IF_UNLIKELY (imp.type_index >= types.size())
				{
#if DEBUG_LOAD || DEBUG_API
					std::cout << "call: type is out-of-bounds\n";
#endif
				}
				imp.ptr(vm, func_index, types[imp.type_index]);
			}
			else
			{
				SOUP_IF_UNLIKELY (!imp.source)
				{
#if DEBUG_LOAD || DEBUG_API
					std::cout << "call: unresolved function import\n";
#endif
					return false;
				}
				SOUP_IF_UNLIKELY (!vm.doCall(imp.source->getTypeIndexForFunction(imp.func_index), imp.func_index))
				{
					return false;
				}
			}
		}
		else
		{
			func_index -= function_imports.size();
			SOUP_IF_UNLIKELY (func_index >= this->code.size())
			{
#if DEBUG_LOAD || DEBUG_API
				std::cout << "call: function is out-of-bounds\n";
#endif
				return false;
			}
			vm.locals = std::move(args);
			SOUP_IF_UNLIKELY (!vm.run(this->code[func_index]))
			{
#if DEBUG_LOAD || DEBUG_API
				std::cout << "call: execution failed\n";
#endif
				return false;
			}
		}
		if (out)
		{
			*out = std::move(vm.stack);
		}
		return true;
	}

	// WasmVm

	bool WasmVm::run(const std::string& data, unsigned depth)
	{
		MemoryRefReader r(data);
		return run(r, depth);
	}

#if DEBUG_VM
#define WASM_CHECK_STACK(x) SOUP_IF_UNLIKELY (stack.size() < x) { /*__debugbreak();*/ std::cout << "Insufficient values on stack\n"; return false; }
#else
#define WASM_CHECK_STACK(x) SOUP_IF_UNLIKELY (stack.size() < x) { return false; }
#endif

	static constexpr float F32_I32_MIN = -2147483600.0f;
	static constexpr float F32_I32_MAX = 2147483500.0f;
	static constexpr float F32_U32_MIN = -0.99999994f;
	static constexpr float F32_U32_MAX = 4294967000.0f;
	static constexpr double F64_I32_MIN = -2147483648.9;
	static constexpr double F64_I32_MAX = 2147483647.9;
	static constexpr double F64_U32_MIN = -0.9999999999999999;
	static constexpr double F64_U32_MAX = 4294967295.9;
	static constexpr float F32_I64_MIN = -9223372000000000000.0f;
	static constexpr float F32_I64_MAX = 9223371500000000000.0f;
	static constexpr float F32_U64_MIN = -0.99999994f;
	static constexpr float F32_U64_MAX = 18446743000000000000.0f;
	static constexpr double F64_I64_MIN = -9223372036854776000.0;
	static constexpr double F64_I64_MAX = 9223372036854775000.0;
	static constexpr double F64_U64_MIN = -0.9999999999999999;
	static constexpr double F64_U64_MAX = 18446744073709550000.0;

	bool WasmVm::run(Reader& r, unsigned depth)
	{
		size_t local_decl_count;
		r.oml(local_decl_count);
		while (local_decl_count--)
		{
			size_t type_count;
			r.oml(type_count);
			uint8_t type;
			r.u8(type);
			while (type_count--)
			{
				locals.emplace_back(static_cast<WasmType>(type));
			}
		}

		std::stack<CtrlFlowEntry> ctrlflow{};

		uint8_t op;
		while (r.u8(op))
		{
			switch (op)
			{
			default:
#if DEBUG_VM
				std::cout << "Unsupported opcode: " << string::hex(op) << "\n";
#endif
				return false;

			case 0x00: // unreachable
#if DEBUG_VM
				std::cout << "unreachable\n";
#endif
				return false;

			case 0x01: // nop
				break;

			case 0x02: // block
				{
					int32_t result_type; r.soml(result_type);
					size_t stack_size = stack.size();
					uint32_t num_values = 0;
					if (result_type != -64)
					{
						if (result_type >= 0 && result_type < script.types.size())
						{
#if DEBUG_VM
							std::cout << "result type is a type index: " << script.types[result_type].parameters.size() << " + " << script.types[result_type].results.size() << "\n";
#endif
							stack_size -= script.types[result_type].parameters.size();
							num_values = script.types[result_type].results.size();
						}
						else
						{
							num_values = 1;
						}
					}
					ctrlflow.emplace(CtrlFlowEntry{ (std::streamoff)-1, stack_size, num_values });
#if DEBUG_VM
					std::cout << "block at position " << r.getPosition() << " with stack size " << stack.size() << " + " << num_values << "\n";
#endif
				}
				break;

			case 0x03: // loop
				{
					int32_t result_type; r.soml(result_type);
					size_t stack_size = stack.size();
					uint32_t num_values = 0;
					if (result_type != -64)
					{
						if (result_type >= 0 && result_type < script.types.size())
						{
#if DEBUG_VM
							std::cout << "result type is a type index: " << script.types[result_type].parameters.size() << " + " << script.types[result_type].results.size() << "\n";
#endif
							num_values = script.types[result_type].parameters.size();
							stack_size -= num_values;
						}
					}
					ctrlflow.emplace(CtrlFlowEntry{ r.getPosition(), stack_size, num_values });
#if DEBUG_VM
					std::cout << "loop at position " << r.getPosition() << " with stack size " << stack.size() << " + " << num_values << "\n";
#endif
				}
				break;

			case 0x04: // if
				{
					int32_t result_type; r.soml(result_type);
					size_t stack_size = stack.size();
					uint32_t num_values = 0;
					if (result_type != -64)
					{
						if (result_type >= 0 && result_type < script.types.size())
						{
#if DEBUG_VM
							std::cout << "result type is a type index: " << script.types[result_type].parameters.size() << " + " << script.types[result_type].results.size() << "\n";
#endif
							stack_size -= script.types[result_type].parameters.size();
							num_values = script.types[result_type].results.size();
						}
						else
						{
							num_values = 1;
						}
					}
					WASM_CHECK_STACK(1);
					auto value = stack.top(); stack.pop();
					//std::cout << "if: condition is " << (value.i32 ? "true" : "false") << "\n";
					if (value.i32)
					{
						ctrlflow.emplace(CtrlFlowEntry{ (std::streamoff)-1, stack_size, num_values });
					}
					else
					{
						if (skipOverBranch(r))
						{
							// we're in the 'else' branch
							ctrlflow.emplace(CtrlFlowEntry{ (std::streamoff)-1, stack_size, num_values });
						}
					}
				}
				break;

			case 0x05: // else
				//std::cout << "else: skipping over this branch\n";
				skipOverBranch(r);
				[[fallthrough]];
			case 0x0b: // end
				if (ctrlflow.empty())
				{
					return true;
				}
				ctrlflow.pop();
				//std::cout << "ctrlflow stack now has " << ctrlflow.size() << " entries\n";
				break;

			case 0x0c: // br
				{
					uint32_t depth;
					r.oml(depth);
					SOUP_IF_UNLIKELY (!doBranch(r, depth, ctrlflow))
					{
						return false;
					}
				}
				break;

			case 0x0d: // br_if
				{
					uint32_t depth;
					r.oml(depth);
					WASM_CHECK_STACK(1);
					auto value = stack.top(); stack.pop();
					if (value.i32)
					{
						SOUP_IF_UNLIKELY (!doBranch(r, depth, ctrlflow))
						{
							return false;
						}
					}
				}
				break;

			case 0x0e: // br_table
				{
					std::vector<uint32_t> table;
					uint32_t num_branches;
					r.oml(num_branches);
					table.reserve(num_branches);
					while (num_branches--)
					{
						uint32_t depth;
						r.oml(depth);
						table.emplace_back(depth);
					}
					uint32_t depth;
					r.oml(depth);
					WASM_CHECK_STACK(1);
					auto index = static_cast<uint32_t>(stack.top().i32); stack.pop();
					if (index < table.size())
					{
						depth = table.at(index);
					}
					SOUP_IF_UNLIKELY (!doBranch(r, depth, ctrlflow))
					{
						return false;
					}
				}
				break;

			case 0x0f: // return
				return true;

			case 0x10: // call
				{
					uint32_t function_index;
					r.oml(function_index);
					uint32_t type_index = script.getTypeIndexForFunction(function_index);
					SOUP_RETHROW_FALSE(doCall(type_index, function_index, depth));
				}
				break;

			case 0x11: // call_indirect
				{
					uint32_t type_index; r.oml(type_index);
					uint32_t table_index; r.oml(table_index);
					SOUP_IF_UNLIKELY (table_index >= script.tables.size())
					{
#if DEBUG_VM
						std::cout << "call: table is out-of-bounds\n";
#endif
						return false;
					}
					const auto& table = script.tables[table_index];
					SOUP_IF_UNLIKELY (table.type != WASM_FUNCREF)
					{
#if DEBUG_VM
						std::cout << "call: indexing non-funcref table\n";
#endif
						return false;
					}
					WASM_CHECK_STACK(1);
					auto element_index = static_cast<uint32_t>(stack.top().i32); stack.pop();
					SOUP_IF_UNLIKELY (element_index >= table.values.size())
					{
#if DEBUG_VM
						std::cout << "call: element is out-of-bounds\n";
#endif
						return false;
					}
					SOUP_IF_UNLIKELY (table.values[element_index] == 0)
					{
#if DEBUG_VM
						std::cout << "indirect call to null\n";
#endif
						return false;
					}
					uint32_t function_index = table.values[element_index] & 0xffff'ffff;
					SOUP_RETHROW_FALSE(doCall(type_index, function_index, depth));
				}
				break;

			case 0x1a: // drop
				stack.pop();
				break;

			case 0x1c: // select t
				r.skip(2);
				[[fallthrough]];
			case 0x1b: // select
				{
					WASM_CHECK_STACK(3);
					auto cond = stack.top(); stack.pop();
					auto fvalue = stack.top(); stack.pop();
					auto tvalue = stack.top(); stack.pop();
					stack.push(cond.i32 ? tvalue : fvalue);
				}
				break;

			case 0x20: // local.get
				{
					size_t local_index;
					r.oml(local_index);
					SOUP_IF_UNLIKELY (local_index >= locals.size())
					{
#if DEBUG_VM
						std::cout << "local.get: index is out-of-bounds: " << local_index << "\n";
#endif
						return false;
					}
					stack.push(locals.at(local_index));
				}
				break;

			case 0x21: // local.set
				{
					size_t local_index;
					r.oml(local_index);
					SOUP_IF_UNLIKELY (local_index >= locals.size())
					{
#if DEBUG_VM
						std::cout << "local.set: index is out-of-bounds: " << local_index << "\n";
#endif
						return false;
					}
					WASM_CHECK_STACK(1);
					locals.at(local_index) = stack.top(); stack.pop();
				}
				break;

			case 0x22: // local.tee
				{
					size_t local_index;
					r.oml(local_index);
					SOUP_IF_UNLIKELY (local_index >= locals.size())
					{
#if DEBUG_VM
						std::cout << "local.tee: index is out-of-bounds: " << local_index << "\n";
#endif
						return false;
					}
					WASM_CHECK_STACK(1);
					locals.at(local_index) = stack.top();
				}
				break;

			case 0x23: // global.get
				{
					size_t global_index;
					r.oml(global_index);
					SOUP_IF_UNLIKELY (global_index >= script.globals.size())
					{
#if DEBUG_VM
						std::cout << "global.get: index is out-of-bounds: " << global_index << "\n";
#endif
						return false;
					}
					stack.push(script.globals[global_index]);
				}
				break;

			case 0x24: // global.set
				{
					size_t global_index;
					r.oml(global_index);
					SOUP_IF_UNLIKELY (global_index >= script.globals.size())
					{
#if DEBUG_VM
						std::cout << "global.set: index is out-of-bounds: " << global_index << "\n";
#endif
						return false;
					}
					WASM_CHECK_STACK(1);
					script.globals.at(global_index) = stack.top(); stack.pop();
				}
				break;

			case 0x25: // table.get
				{
					size_t table_index;
					r.oml(table_index);
					SOUP_IF_UNLIKELY (table_index >= script.tables.size())
					{
#if DEBUG_VM
						std::cout << "table.get: table index " << table_index << " >= " << script.tables.size() << "\n";
#endif
						return false;
					}
					WASM_CHECK_STACK(1);
					auto elem_index = stack.top().i32; stack.pop();
					const auto& table = script.tables[table_index];
					SOUP_IF_UNLIKELY (elem_index >= table.values.size())
					{
#if DEBUG_VM
						std::cout << "table.get: element index " << elem_index << " >= " << table.values.size() << "\n";
#endif
						return false;
					}
					stack.emplace(table.type).i64 = table.values[elem_index];
				}
				break;

			case 0x26: // table.set
				{
					size_t table_index;
					r.oml(table_index);
					SOUP_IF_UNLIKELY (table_index >= script.tables.size())
					{
#if DEBUG_VM
						std::cout << "table.set: table index " << table_index << " >= " << script.tables.size() << "\n";
#endif
						return false;
					}
					WASM_CHECK_STACK(2);
					auto value = stack.top(); stack.pop();
					auto elem_index = stack.top().i32; stack.pop();
					auto& table = script.tables[table_index];
					SOUP_IF_UNLIKELY (elem_index >= table.values.size())
					{
#if DEBUG_VM
						std::cout << "table.set: element index " << elem_index << " >= " << table.values.size() << "\n";
#endif
						return false;
					}
					SOUP_IF_UNLIKELY (value.type != table.type)
					{
#if DEBUG_VM
						std::cout << "table.set: value type doesn't match table's element type\n";
#endif
						return false;
					}
					table.values[elem_index] = value.i64;
				}
				break;

			case 0x28: // i32.load
				{
					WASM_CHECK_STACK(1);
					auto base = stack.top(); stack.pop();
					r.skip(1); // memflags
					auto offset = readUPTR(r);
					if (auto ptr = script.memory.getPointer<int32_t>(base, offset))
					{
						stack.emplace(*ptr);
					}
					else
					{
#if DEBUG_VM
						std::cout << "memory access out of bounds\n";
#endif
						return false;
					}
				}
				break;

			case 0x29: // i64.load
				{
					WASM_CHECK_STACK(1);
					auto base = stack.top(); stack.pop();
					r.skip(1); // memflags
					auto offset = readUPTR(r);
					if (auto ptr = script.memory.getPointer<int64_t>(base, offset))
					{
						stack.emplace(*ptr);
					}
					else
					{
#if DEBUG_VM
						std::cout << "memory access out of bounds\n";
#endif
						return false;
					}
				}
				break;

			case 0x2a: // f32.load
				{
					WASM_CHECK_STACK(1);
					auto base = stack.top(); stack.pop();
					r.skip(1); // memflags
					auto offset = readUPTR(r);
					if (auto ptr = script.memory.getPointer<float>(base, offset))
					{
						stack.emplace(*ptr);
					}
					else
					{
#if DEBUG_VM
						std::cout << "memory access out of bounds\n";
#endif
						return false;
					}
				}
				break;

			case 0x2b: // f64.load
				{
					WASM_CHECK_STACK(1);
					auto base = stack.top(); stack.pop();
					r.skip(1); // memflags
					auto offset = readUPTR(r);
					if (auto ptr = script.memory.getPointer<double>(base, offset))
					{
						stack.emplace(*ptr);
					}
					else
					{
#if DEBUG_VM
						std::cout << "memory access out of bounds\n";
#endif
						return false;
					}
				}
				break;

			case 0x2c: // i32.load8_s
				{
					WASM_CHECK_STACK(1);
					auto base = stack.top(); stack.pop();
					r.skip(1); // memflags
					auto offset = readUPTR(r);
					if (auto ptr = script.memory.getPointer<int8_t>(base, offset))
					{
						stack.emplace(static_cast<int32_t>(*ptr));
					}
					else
					{
#if DEBUG_VM
						std::cout << "memory access out of bounds\n";
#endif
						return false;
					}
				}
				break;

			case 0x2d: // i32.load8_u
				{
					WASM_CHECK_STACK(1);
					auto base = stack.top(); stack.pop();
					r.skip(1); // memflags
					auto offset = readUPTR(r);
					if (auto ptr = script.memory.getPointer<uint8_t>(base, offset))
					{
						stack.emplace(static_cast<uint32_t>(*ptr));
					}
					else
					{
#if DEBUG_VM
						std::cout << "memory access out of bounds\n";
#endif
						return false;
					}
				}
				break;

			case 0x2e: // i32.load16_s
				{
					WASM_CHECK_STACK(1);
					auto base = stack.top(); stack.pop();
					r.skip(1); // memflags
					auto offset = readUPTR(r);
					if (auto ptr = script.memory.getPointer<int16_t>(base, offset))
					{
						stack.emplace(static_cast<uint32_t>(*ptr));
					}
					else
					{
#if DEBUG_VM
						std::cout << "memory access out of bounds\n";
#endif
						return false;
					}
				}
				break;

			case 0x2f: // i32.load16_u
				{
					WASM_CHECK_STACK(1);
					auto base = stack.top(); stack.pop();
					r.skip(1); // memflags
					auto offset = readUPTR(r);
					if (auto ptr = script.memory.getPointer<uint16_t>(base, offset))
					{
						stack.emplace(static_cast<uint32_t>(*ptr));
					}
					else
					{
#if DEBUG_VM
						std::cout << "memory access out of bounds\n";
#endif
						return false;
					}
				}
				break;

			case 0x30: // i64.load8_s
				{
					WASM_CHECK_STACK(1);
					auto base = stack.top(); stack.pop();
					r.skip(1); // memflags
					auto offset = readUPTR(r);
					if (auto ptr = script.memory.getPointer<int8_t>(base, offset))
					{
						stack.emplace(static_cast<int64_t>(*ptr));
					}
					else
					{
#if DEBUG_VM
						std::cout << "memory access out of bounds\n";
#endif
						return false;
					}
				}
				break;

			case 0x31: // i64.load8_u
				{
					WASM_CHECK_STACK(1);
					auto base = stack.top(); stack.pop();
					r.skip(1); // memflags
					auto offset = readUPTR(r);
					if (auto ptr = script.memory.getPointer<uint8_t>(base, offset))
					{
						stack.emplace(static_cast<uint64_t>(*ptr));
					}
					else
					{
#if DEBUG_VM
						std::cout << "memory access out of bounds\n";
#endif
						return false;
					}
				}
				break;

			case 0x32: // i64.load16_s
				{
					WASM_CHECK_STACK(1);
					auto base = stack.top(); stack.pop();
					r.skip(1); // memflags
					auto offset = readUPTR(r);
					if (auto ptr = script.memory.getPointer<int16_t>(base, offset))
					{
						stack.emplace(static_cast<int64_t>(*ptr));
					}
					else
					{
#if DEBUG_VM
						std::cout << "memory access out of bounds\n";
#endif
						return false;
					}
				}
				break;

			case 0x33: // i64.load16_u
				{
					WASM_CHECK_STACK(1);
					auto base = stack.top(); stack.pop();
					r.skip(1); // memflags
					auto offset = readUPTR(r);
					if (auto ptr = script.memory.getPointer<uint16_t>(base, offset))
					{
						stack.emplace(static_cast<uint64_t>(*ptr));
					}
					else
					{
#if DEBUG_VM
						std::cout << "memory access out of bounds\n";
#endif
						return false;
					}
				}
				break;

			case 0x34: // i64.load32_s
				{
					WASM_CHECK_STACK(1);
					auto base = stack.top(); stack.pop();
					r.skip(1); // memflags
					auto offset = readUPTR(r);
					if (auto ptr = script.memory.getPointer<int32_t>(base, offset))
					{
						stack.emplace(static_cast<int64_t>(*ptr));
					}
					else
					{
#if DEBUG_VM
						std::cout << "memory access out of bounds\n";
#endif
						return false;
					}
				}
				break;

			case 0x35: // i64.load32_u
				{
					WASM_CHECK_STACK(1);
					auto base = stack.top(); stack.pop();
					r.skip(1); // memflags
					auto offset = readUPTR(r);
					if (auto ptr = script.memory.getPointer<uint32_t>(base, offset))
					{
						stack.emplace(static_cast<uint64_t>(*ptr));
					}
					else
					{
#if DEBUG_VM
						std::cout << "memory access out of bounds\n";
#endif
						return false;
					}
				}
				break;


			case 0x36: // i32.store
				{
					WASM_CHECK_STACK(2);
					auto value = stack.top(); stack.pop();
					auto base = stack.top(); stack.pop();
					r.skip(1); // memflags
					auto offset = readUPTR(r);
					if (auto ptr = script.memory.getPointer<int32_t>(base, offset))
					{
						*ptr = value.i32;
					}
					else
					{
#if DEBUG_VM
						std::cout << "memory access out of bounds\n";
#endif
						return false;
					}
				}
				break;

			case 0x37: // i64.store
				{
					WASM_CHECK_STACK(2);
					auto value = stack.top(); stack.pop();
					auto base = stack.top(); stack.pop();
					r.skip(1); // memflags
					auto offset = readUPTR(r);
					if (auto ptr = script.memory.getPointer<int64_t>(base, offset))
					{
						*ptr = value.i64;
					}
					else
					{
#if DEBUG_VM
						std::cout << "memory access out of bounds\n";
#endif
						return false;
					}
				}
				break;

			case 0x38: // f32.store
				{
					WASM_CHECK_STACK(2);
					auto value = stack.top(); stack.pop();
					auto base = stack.top(); stack.pop();
					r.skip(1); // memflags
					auto offset = readUPTR(r);
					if (auto ptr = script.memory.getPointer<float>(base, offset))
					{
						*ptr = value.f32;
					}
					else
					{
#if DEBUG_VM
						std::cout << "memory access out of bounds\n";
#endif
						return false;
					}
				}
				break;

			case 0x39: // f64.store
				{
					WASM_CHECK_STACK(2);
					auto value = stack.top(); stack.pop();
					auto base = stack.top(); stack.pop();
					r.skip(1); // memflags
					auto offset = readUPTR(r);
					if (auto ptr = script.memory.getPointer<double>(base, offset))
					{
						*ptr = value.f64;
					}
					else
					{
#if DEBUG_VM
						std::cout << "memory access out of bounds\n";
#endif
						return false;
					}
				}
				break;

			case 0x3a: // i32.store8
				{
					WASM_CHECK_STACK(2);
					auto value = stack.top(); stack.pop();
					auto base = stack.top(); stack.pop();
					r.skip(1); // memflags
					auto offset = readUPTR(r);
					if (auto ptr = script.memory.getPointer<int8_t>(base, offset))
					{
						*ptr = static_cast<int8_t>(value.i32);
					}
					else
					{
#if DEBUG_VM
						std::cout << "memory access out of bounds\n";
#endif
						return false;
					}
				}
				break;

			case 0x3b: // i32.store16
				{
					WASM_CHECK_STACK(2);
					auto value = stack.top(); stack.pop();
					auto base = stack.top(); stack.pop();
					r.skip(1); // memflags
					auto offset = readUPTR(r);
					if (auto ptr = script.memory.getPointer<int16_t>(base, offset))
					{
						*ptr = static_cast<int16_t>(value.i32);
					}
					else
					{
#if DEBUG_VM
						std::cout << "memory access out of bounds\n";
#endif
						return false;
					}
				}
				break;

			case 0x3c: // i64.store8
				{
					WASM_CHECK_STACK(2);
					auto value = stack.top(); stack.pop();
					auto base = stack.top(); stack.pop();
					r.skip(1); // memflags
					auto offset = readUPTR(r);
					if (auto ptr = script.memory.getPointer<int8_t>(base, offset))
					{
						*ptr = static_cast<int8_t>(value.i64);
					}
					else
					{
#if DEBUG_VM
						std::cout << "memory access out of bounds\n";
#endif
						return false;
					}
				}
				break;

			case 0x3d: // i64.store16
				{
					WASM_CHECK_STACK(2);
					auto value = stack.top(); stack.pop();
					auto base = stack.top(); stack.pop();
					r.skip(1); // memflags
					auto offset = readUPTR(r);
					if (auto ptr = script.memory.getPointer<int16_t>(base, offset))
					{
						*ptr = static_cast<int16_t>(value.i64);
					}
					else
					{
#if DEBUG_VM
						std::cout << "memory access out of bounds\n";
#endif
						return false;
					}
				}
				break;

			case 0x3e: // i64.store32
				{
					WASM_CHECK_STACK(2);
					auto value = stack.top(); stack.pop();
					auto base = stack.top(); stack.pop();
					r.skip(1); // memflags
					auto offset = readUPTR(r);
					if (auto ptr = script.memory.getPointer<int32_t>(base, offset))
					{
						*ptr = static_cast<int32_t>(value.i64);
					}
					else
					{
#if DEBUG_VM
						std::cout << "memory access out of bounds\n";
#endif
						return false;
					}
				}
				break;

			case 0x3f: // memory.size
				{
					r.skip(1); // reserved
					script.memory.encodeUPTR(stack.emplace(), script.memory.size / 0x10'000);
				}
				break;

			case 0x40: // memory.grow
				{
					r.skip(1); // reserved
					WASM_CHECK_STACK(1);
					const auto old_size_pages = script.memory.grow(popUPTR());
					script.memory.encodeUPTR(stack.emplace(), old_size_pages);
				}
				break;

			case 0x41: // i32.const
				{
					int32_t value;
					r.soml(value);
					stack.push(value);
				}
				break;

			case 0x42: // i64.const
				{
					int64_t value;
					r.soml(value);
					stack.push(value);
				}
				break;

			case 0x43: // f32.const
				{
					float value;
					r.f32(value);
					stack.push(value);
				}
				break;

			case 0x44: // f64.const
				{
					double value;
					r.f64(value);
					stack.push(value);
				}
				break;

			case 0x45: // i32.eqz
				{
					WASM_CHECK_STACK(1);
					auto value = stack.top(); stack.pop();
					stack.push(value.i32 == 0);
				}
				break;

			case 0x46: // i32.eq
				{
					WASM_CHECK_STACK(2);
					auto b = stack.top(); stack.pop();
					auto a = stack.top(); stack.pop();
					stack.push(a.i32 == b.i32);
				}
				break;

			case 0x47: // i32.ne
				{
					WASM_CHECK_STACK(2);
					auto b = stack.top(); stack.pop();
					auto a = stack.top(); stack.pop();
					stack.push(a.i32 != b.i32);
				}
				break;

			case 0x48: // i32.lt_s
				{
					WASM_CHECK_STACK(2);
					auto b = stack.top(); stack.pop();
					auto a = stack.top(); stack.pop();
					stack.push(a.i32 < b.i32);
				}
				break;

			case 0x49: // i32.lt_u
				{
					WASM_CHECK_STACK(2);
					auto b = stack.top(); stack.pop();
					auto a = stack.top(); stack.pop();
					stack.push(static_cast<uint32_t>(a.i32) < static_cast<uint32_t>(b.i32));
				}
				break;

			case 0x4a: // i32.gt_s
				{
					WASM_CHECK_STACK(2);
					auto b = stack.top(); stack.pop();
					auto a = stack.top(); stack.pop();
					stack.push(a.i32 > b.i32);
				}
				break;

			case 0x4b: // i32.gt_u
				{
					WASM_CHECK_STACK(2);
					auto b = stack.top(); stack.pop();
					auto a = stack.top(); stack.pop();
					stack.push(static_cast<uint32_t>(a.i32) > static_cast<uint32_t>(b.i32));
				}
				break;

			case 0x4c: // i32.le_s
				{
					WASM_CHECK_STACK(2);
					auto b = stack.top(); stack.pop();
					auto a = stack.top(); stack.pop();
					stack.push(a.i32 <= b.i32);
				}
				break;

			case 0x4d: // i32.le_u
				{
					WASM_CHECK_STACK(2);
					auto b = stack.top(); stack.pop();
					auto a = stack.top(); stack.pop();
					stack.push(static_cast<uint32_t>(a.i32) <= static_cast<uint32_t>(b.i32));
				}
				break;

			case 0x4e: // i32.ge_s
				{
					WASM_CHECK_STACK(2);
					auto b = stack.top(); stack.pop();
					auto a = stack.top(); stack.pop();
					stack.push(a.i32 >= b.i32);
				}
				break;

			case 0x4f: // i32.ge_u
				{
					WASM_CHECK_STACK(2);
					auto b = stack.top(); stack.pop();
					auto a = stack.top(); stack.pop();
					stack.push(static_cast<uint32_t>(a.i32) >= static_cast<uint32_t>(b.i32));
				}
				break;

			case 0x50: // i64.eqz
				{
					WASM_CHECK_STACK(1);
					auto value = stack.top(); stack.pop();
					stack.push(value.i64 == 0);
				}
				break;

			case 0x51: // i64.eq
				{
					WASM_CHECK_STACK(2);
					auto b = stack.top(); stack.pop();
					auto a = stack.top(); stack.pop();
					stack.push(a.i64 == b.i64);
				}
				break;

			case 0x52: // i64.ne
				{
					WASM_CHECK_STACK(2);
					auto b = stack.top(); stack.pop();
					auto a = stack.top(); stack.pop();
					stack.push(a.i64 != b.i64);
				}
				break;

			case 0x53: // i64.lt_s
				{
					WASM_CHECK_STACK(2);
					auto b = stack.top(); stack.pop();
					auto a = stack.top(); stack.pop();
					stack.push(a.i64 < b.i64);
				}
				break;

			case 0x54: // i64.lt_u
				{
					WASM_CHECK_STACK(2);
					auto b = stack.top(); stack.pop();
					auto a = stack.top(); stack.pop();
					stack.push(static_cast<uint64_t>(a.i64) < static_cast<uint64_t>(b.i64));
				}
				break;

			case 0x55: // i64.gt_s
				{
					WASM_CHECK_STACK(2);
					auto b = stack.top(); stack.pop();
					auto a = stack.top(); stack.pop();
					stack.push(a.i64 > b.i64);
				}
				break;

			case 0x56: // i64.gt_u
				{
					WASM_CHECK_STACK(2);
					auto b = stack.top(); stack.pop();
					auto a = stack.top(); stack.pop();
					stack.push(static_cast<uint64_t>(a.i64) > static_cast<uint64_t>(b.i64));
				}
				break;

			case 0x57: // i64.le_s
				{
					WASM_CHECK_STACK(2);
					auto b = stack.top(); stack.pop();
					auto a = stack.top(); stack.pop();
					stack.push(a.i64 <= b.i64);
				}
				break;

			case 0x58: // i64.le_u
				{
					WASM_CHECK_STACK(2);
					auto b = stack.top(); stack.pop();
					auto a = stack.top(); stack.pop();
					stack.push(static_cast<uint64_t>(a.i64) <= static_cast<uint64_t>(b.i64));
				}
				break;

			case 0x59: // i64.ge_s
				{
					WASM_CHECK_STACK(2);
					auto b = stack.top(); stack.pop();
					auto a = stack.top(); stack.pop();
					stack.push(a.i64 >= b.i64);
				}
				break;

			case 0x5a: // i64.ge_u
				{
					WASM_CHECK_STACK(2);
					auto b = stack.top(); stack.pop();
					auto a = stack.top(); stack.pop();
					stack.push(static_cast<uint64_t>(a.i64) >= static_cast<uint64_t>(b.i64));
				}
				break;

			case 0x5b: // f32.eq
				{
					WASM_CHECK_STACK(2);
					auto b = stack.top(); stack.pop();
					auto a = stack.top(); stack.pop();
					stack.push(a.f32 == b.f32);
				}
				break;

			case 0x5c: // f32.ne
				{
					WASM_CHECK_STACK(2);
					auto b = stack.top(); stack.pop();
					auto a = stack.top(); stack.pop();
					stack.push(a.f32 != b.f32);
				}
				break;

			case 0x5d: // f32.lt
				{
					WASM_CHECK_STACK(2);
					auto b = stack.top(); stack.pop();
					auto a = stack.top(); stack.pop();
					stack.push(a.f32 < b.f32);
				}
				break;

			case 0x5e: // f32.gt
				{
					WASM_CHECK_STACK(2);
					auto b = stack.top(); stack.pop();
					auto a = stack.top(); stack.pop();
					stack.push(a.f32 > b.f32);
				}
				break;

			case 0x5f: // f32.le
				{
					WASM_CHECK_STACK(2);
					auto b = stack.top(); stack.pop();
					auto a = stack.top(); stack.pop();
					stack.push(a.f32 <= b.f32);
				}
				break;

			case 0x60: // f32.ge
				{
					WASM_CHECK_STACK(2);
					auto b = stack.top(); stack.pop();
					auto a = stack.top(); stack.pop();
					stack.push(a.f32 >= b.f32);
				}
				break;

			case 0x61: // f64.eq
				{
					WASM_CHECK_STACK(2);
					auto b = stack.top(); stack.pop();
					auto a = stack.top(); stack.pop();
					stack.push(a.f64 == b.f64);
				}
				break;

			case 0x62: // f64.ne
				{
					WASM_CHECK_STACK(2);
					auto b = stack.top(); stack.pop();
					auto a = stack.top(); stack.pop();
					stack.push(a.f64 != b.f64);
				}
				break;

			case 0x63: // f64.lt
				{
					WASM_CHECK_STACK(2);
					auto b = stack.top(); stack.pop();
					auto a = stack.top(); stack.pop();
					stack.push(a.f64 < b.f64);
				}
				break;

			case 0x64: // f64.gt
				{
					WASM_CHECK_STACK(2);
					auto b = stack.top(); stack.pop();
					auto a = stack.top(); stack.pop();
					stack.push(a.f64 > b.f64);
				}
				break;

			case 0x65: // f64.le
				{
					WASM_CHECK_STACK(2);
					auto b = stack.top(); stack.pop();
					auto a = stack.top(); stack.pop();
					stack.push(a.f64 <= b.f64);
				}
				break;

			case 0x66: // f64.ge
				{
					WASM_CHECK_STACK(2);
					auto b = stack.top(); stack.pop();
					auto a = stack.top(); stack.pop();
					stack.push(a.f64 >= b.f64);
				}
				break;

			case 0x67: // i32.clz
				WASM_CHECK_STACK(1);
				stack.top().i32 = bitutil::getNumLeadingZeros(static_cast<uint32_t>(stack.top().i32));
				break;

			case 0x68: // i32.ctz
				WASM_CHECK_STACK(1);
				stack.top().i32 = bitutil::getNumTrailingZeros(static_cast<uint32_t>(stack.top().i32));
				break;

			case 0x69: // i32.popcnt
				WASM_CHECK_STACK(1);
				stack.top().i32 = bitutil::getNumSetBits(static_cast<uint32_t>(stack.top().i32));
				break;

			case 0x6a: // i32.add
				{
					WASM_CHECK_STACK(2);
					auto b = stack.top(); stack.pop();
					auto a = stack.top(); stack.pop();
					stack.push(a.i32 + b.i32);
				}
				break;

			case 0x6b: // i32.sub
				{
					WASM_CHECK_STACK(2);
					auto b = stack.top(); stack.pop();
					auto a = stack.top(); stack.pop();
					stack.push(a.i32 - b.i32);
				}
				break;

			case 0x6c: // i32.mul
				{
					WASM_CHECK_STACK(2);
					auto b = stack.top(); stack.pop();
					auto a = stack.top(); stack.pop();
					stack.push(a.i32 * b.i32);
				}
				break;

			case 0x6d: // i32.div_s
				{
					WASM_CHECK_STACK(2);
					auto b = stack.top(); stack.pop();
					auto a = stack.top(); stack.pop();
					SOUP_IF_UNLIKELY (b.i32 == 0 || (a.i32 == INT32_MIN && b.i32 == -1))
					{
						return false;
					}
					stack.push(a.i32 / b.i32);
				}
				break;

			case 0x6e: // i32.div_u
				{
					WASM_CHECK_STACK(2);
					auto b = stack.top(); stack.pop();
					auto a = stack.top(); stack.pop();
					SOUP_IF_UNLIKELY (b.i32 == 0)
					{
						return false;
					}
					stack.push(static_cast<uint32_t>(a.i32) / static_cast<uint32_t>(b.i32));
				}
				break;

			case 0x6f: // i32.rem_s
				{
					WASM_CHECK_STACK(2);
					auto b = stack.top(); stack.pop();
					auto a = stack.top(); stack.pop();
					SOUP_IF_UNLIKELY (b.i32 == 0)
					{
						return false;
					}
					if (a.i32 == INT32_MIN && b.i32 == -1)
					{
						stack.push(0);
					}
					else
					{
						stack.push(a.i32 % b.i32);
					}
				}
				break;

			case 0x70: // i32.rem_u
				{
					WASM_CHECK_STACK(2);
					auto b = stack.top(); stack.pop();
					auto a = stack.top(); stack.pop();
					SOUP_IF_UNLIKELY (b.i32 == 0)
					{
						return false;
					}
					stack.push(static_cast<uint32_t>(a.i32) % static_cast<uint32_t>(b.i32));
				}
				break;

			case 0x71: // i32.and
				{
					WASM_CHECK_STACK(2);
					auto b = stack.top(); stack.pop();
					auto a = stack.top(); stack.pop();
					stack.push(a.i32 & b.i32);
				}
				break;

			case 0x72: // i32.or
				{
					WASM_CHECK_STACK(2);
					auto b = stack.top(); stack.pop();
					auto a = stack.top(); stack.pop();
					stack.push(a.i32 | b.i32);
				}
				break;

			case 0x73: // i32.xor
				{
					WASM_CHECK_STACK(2);
					auto b = stack.top(); stack.pop();
					auto a = stack.top(); stack.pop();
					stack.push(a.i32 ^ b.i32);
				}
				break;

			case 0x74: // i32.shl
				{
					WASM_CHECK_STACK(2);
					auto b = stack.top(); stack.pop();
					auto a = stack.top(); stack.pop();
					stack.push(a.i32 << (static_cast<uint32_t>(b.i32) % (sizeof(uint32_t) * 8)));
				}
				break;

			case 0x75: // i32.shr_s ("arithmetic right shift")
				{
					WASM_CHECK_STACK(2);
					auto b = stack.top(); stack.pop();
					auto a = stack.top(); stack.pop();
					stack.push(a.i32 >> (static_cast<uint32_t>(b.i32) % (sizeof(uint32_t) * 8)));
				}
				break;

			case 0x76: // i32.shr_u ("logical right shift")
				{
					WASM_CHECK_STACK(2);
					auto b = stack.top(); stack.pop();
					auto a = stack.top(); stack.pop();
					stack.push(static_cast<uint32_t>(a.i32) >> (static_cast<uint32_t>(b.i32) % (sizeof(uint32_t) * 8)));
				}
				break;

			case 0x77: // i32.rotl
				{
					WASM_CHECK_STACK(2);
					auto b = stack.top(); stack.pop();
					auto a = stack.top(); stack.pop();
					stack.push(soup::rotl<uint32_t>(a.i32, b.i32));
				}
				break;

			case 0x78: // i32.rotr
				{
					WASM_CHECK_STACK(2);
					auto b = stack.top(); stack.pop();
					auto a = stack.top(); stack.pop();
					stack.push(soup::rotr<uint32_t>(a.i32, b.i32));
				}
				break;

			case 0x79: // i64.clz
				WASM_CHECK_STACK(1);
				stack.top().i64 = bitutil::getNumLeadingZeros(static_cast<uint64_t>(stack.top().i64));
				break;

			case 0x7a: // i64.ctz
				WASM_CHECK_STACK(1);
				stack.top().i64 = bitutil::getNumTrailingZeros(static_cast<uint64_t>(stack.top().i64));
				break;

			case 0x7b: // i64.popcnt
				WASM_CHECK_STACK(1);
				stack.top().i64 = bitutil::getNumSetBits(static_cast<uint64_t>(stack.top().i64));
				break;

			case 0x7c: // i64.add
				{
					WASM_CHECK_STACK(2);
					auto b = stack.top(); stack.pop();
					auto a = stack.top(); stack.pop();
					stack.push(a.i64 + b.i64);
				}
				break;

			case 0x7d: // i64.sub
				{
					WASM_CHECK_STACK(2);
					auto b = stack.top(); stack.pop();
					auto a = stack.top(); stack.pop();
					stack.push(a.i64 - b.i64);
				}
				break;

			case 0x7e: // i64.mul
				{
					WASM_CHECK_STACK(2);
					auto b = stack.top(); stack.pop();
					auto a = stack.top(); stack.pop();
					stack.push(a.i64 * b.i64);
				}
				break;

			case 0x7f: // i64.div_s
				{
					WASM_CHECK_STACK(2);
					auto b = stack.top(); stack.pop();
					auto a = stack.top(); stack.pop();
					SOUP_IF_UNLIKELY (b.i64 == 0 || (a.i64 == INT64_MIN && b.i64 == -1))
					{
						return false;
					}
					stack.push(a.i64 / b.i64);
				}
				break;

			case 0x80: // i64.div_u
				{
					WASM_CHECK_STACK(2);
					auto b = stack.top(); stack.pop();
					auto a = stack.top(); stack.pop();
					SOUP_IF_UNLIKELY (b.i64 == 0)
					{
						return false;
					}
					stack.push(static_cast<uint64_t>(a.i64) / static_cast<uint64_t>(b.i64));
				}
				break;

			case 0x81: // i64.rem_s
				{
					WASM_CHECK_STACK(2);
					auto b = stack.top(); stack.pop();
					auto a = stack.top(); stack.pop();
					SOUP_IF_UNLIKELY (b.i32 == 0)
					{
						return false;
					}
					if (a.i64 == INT64_MIN && b.i64 == -1)
					{
						stack.push(static_cast<int64_t>(0));
					}
					else
					{
						stack.push(a.i64 % b.i64);
					}
				}
				break;

			case 0x82: // i64.rem_u
				{
					WASM_CHECK_STACK(2);
					auto b = stack.top(); stack.pop();
					auto a = stack.top(); stack.pop();
					SOUP_IF_UNLIKELY (b.i32 == 0)
					{
						return false;
					}
					stack.push(static_cast<uint64_t>(a.i64) % static_cast<uint64_t>(b.i64));
				}
				break;

			case 0x83: // i64.and
				{
					WASM_CHECK_STACK(2);
					auto b = stack.top(); stack.pop();
					auto a = stack.top(); stack.pop();
					stack.push(a.i64 & b.i64);
				}
				break;

			case 0x84: // i64.or
				{
					WASM_CHECK_STACK(2);
					auto b = stack.top(); stack.pop();
					auto a = stack.top(); stack.pop();
					stack.push(a.i64 | b.i64);
				}
				break;

			case 0x85: // i64.xor
				{
					WASM_CHECK_STACK(2);
					auto b = stack.top(); stack.pop();
					auto a = stack.top(); stack.pop();
					stack.push(a.i64 ^ b.i64);
				}
				break;

			case 0x86: // i64.shl
				{
					WASM_CHECK_STACK(2);
					auto b = stack.top(); stack.pop();
					auto a = stack.top(); stack.pop();
					stack.push(a.i64 << (static_cast<uint64_t>(b.i64) % (sizeof(uint64_t) * 8)));
				}
				break;

			case 0x87: // i64.shr_s ("arithmetic right shift")
				{
					WASM_CHECK_STACK(2);
					auto b = stack.top(); stack.pop();
					auto a = stack.top(); stack.pop();
					stack.push(a.i64 >> (static_cast<uint64_t>(b.i64) % (sizeof(uint64_t) * 8)));
				}
				break;

			case 0x88: // i64.shr_u ("logical right shift")
				{
					WASM_CHECK_STACK(2);
					auto b = stack.top(); stack.pop();
					auto a = stack.top(); stack.pop();
					stack.push(static_cast<uint64_t>(a.i64) >> (static_cast<uint64_t>(b.i64) % (sizeof(uint64_t) * 8)));
				}
				break;

			case 0x89: // i64.rotl
				{
					WASM_CHECK_STACK(2);
					auto b = stack.top(); stack.pop();
					auto a = stack.top(); stack.pop();
					stack.push(soup::rotl<uint64_t>(a.i64, static_cast<int>(b.i64)));
				}
				break;

			case 0x8a: // i64.rotr
				{
					WASM_CHECK_STACK(2);
					auto b = stack.top(); stack.pop();
					auto a = stack.top(); stack.pop();
					stack.push(soup::rotr<uint64_t>(a.i64, static_cast<int>(b.i64)));
				}
				break;

			case 0x8b: // f32.abs
				WASM_CHECK_STACK(1);
				stack.top().f32 = std::abs(stack.top().f32);
				break;

			case 0x8c: // f32.neg
				WASM_CHECK_STACK(1);
				stack.top().f32 = stack.top().f32 * -1.0f;
				break;

			case 0x8d: // f32.ceil
				WASM_CHECK_STACK(1);
				stack.top().f32 = std::ceil(stack.top().f32);
				break;

			case 0x8e: // f32.floor
				WASM_CHECK_STACK(1);
				stack.top().f32 = std::floor(stack.top().f32);
				break;

			case 0x8f: // f32.trunc
				WASM_CHECK_STACK(1);
				stack.top().f32 = std::trunc(stack.top().f32);
				break;

			case 0x90: // f32.nearest
				WASM_CHECK_STACK(1);
				stack.top().f32 = std::nearbyint(stack.top().f32);
				break;

			case 0x91: // f32.sqrt
				WASM_CHECK_STACK(1);
				stack.top().f32 = std::sqrt(stack.top().f32);
				break;

			case 0x92: // f32.add
				WASM_CHECK_STACK(2);
				{
					auto b = stack.top(); stack.pop();
					auto a = stack.top(); stack.pop();
					stack.push(a.f32 + b.f32);
				}
				break;

			case 0x93: // f32.sub
				WASM_CHECK_STACK(2);
				{
					auto b = stack.top(); stack.pop();
					auto a = stack.top(); stack.pop();
					stack.push(a.f32 - b.f32);
				}
				break;

			case 0x94: // f32.mul
				WASM_CHECK_STACK(2);
				{
					auto b = stack.top(); stack.pop();
					auto a = stack.top(); stack.pop();
					stack.push(a.f32 * b.f32);
				}
				break;

			case 0x95: // f32.div
				WASM_CHECK_STACK(2);
				{
					auto b = stack.top(); stack.pop();
					auto a = stack.top(); stack.pop();
					stack.push(a.f32 / b.f32);
				}
				break;

			case 0x96: // f32.min
				WASM_CHECK_STACK(2);
				{
					auto b = stack.top(); stack.pop();
					auto a = stack.top(); stack.pop();
					if (std::isnan(a.f32) || std::isnan(b.f32))
					{
						stack.push(std::numeric_limits<float>::quiet_NaN());
					}
					else if (a.f32 < b.f32 || (std::signbit(a.f32) && !std::signbit(b.f32)))
					{
						stack.push(a.f32);
					}
					else
					{
						stack.push(b.f32);
					}
				}
				break;

			case 0x97: // f32.max
				WASM_CHECK_STACK(2);
				{
					auto b = stack.top(); stack.pop();
					auto a = stack.top(); stack.pop();
					if (std::isnan(a.f32) || std::isnan(b.f32))
					{
						stack.push(std::numeric_limits<float>::quiet_NaN());
					}
					else if (a.f32 < b.f32 || (std::signbit(a.f32) && !std::signbit(b.f32)))
					{
						stack.push(b.f32);
					}
					else
					{
						stack.push(a.f32);
					}
				}
				break;

			case 0x98: // f32.copysign
				WASM_CHECK_STACK(2);
				{
					auto b = stack.top(); stack.pop();
					auto a = stack.top(); stack.pop();
					stack.push(std::copysign(a.f32, b.f32));
				}
				break;

			case 0x99: // f64.abs
				WASM_CHECK_STACK(1);
				stack.top().f64 = std::abs(stack.top().f64);
				break;

			case 0x9a: // f64.neg
				WASM_CHECK_STACK(1);
				stack.top().f64 = stack.top().f64 * -1.0;
				break;

			case 0x9b: // f64.ceil
				WASM_CHECK_STACK(1);
				stack.top().f64 = std::ceil(stack.top().f64);
				break;

			case 0x9c: // f64.floor
				WASM_CHECK_STACK(1);
				stack.top().f64 = std::floor(stack.top().f64);
				break;

			case 0x9d: // f64.trunc
				WASM_CHECK_STACK(1);
				stack.top().f64 = std::trunc(stack.top().f64);
				break;

			case 0x9e: // f64.nearest
				WASM_CHECK_STACK(1);
				stack.top().f64 = std::nearbyint(stack.top().f64);
				break;

			case 0x9f: // f64.sqrt
				WASM_CHECK_STACK(1);
				stack.top().f64 = std::sqrt(stack.top().f64);
				break;

			case 0xa0: // f64.add
				WASM_CHECK_STACK(2);
				{
					auto b = stack.top(); stack.pop();
					auto a = stack.top(); stack.pop();
					stack.push(a.f64 + b.f64);
				}
				break;

			case 0xa1: // f64.sub
				WASM_CHECK_STACK(2);
				{
					auto b = stack.top(); stack.pop();
					auto a = stack.top(); stack.pop();
					stack.push(a.f64 - b.f64);
				}
				break;

			case 0xa2: // f64.mul
				WASM_CHECK_STACK(2);
				{
					auto b = stack.top(); stack.pop();
					auto a = stack.top(); stack.pop();
					stack.push(a.f64 * b.f64);
				}
				break;

			case 0xa3: // f64.div
				WASM_CHECK_STACK(2);
				{
					auto b = stack.top(); stack.pop();
					auto a = stack.top(); stack.pop();
					stack.push(a.f64 / b.f64);
				}
				break;

			case 0xa4: // f64.min
				WASM_CHECK_STACK(2);
				{
					auto b = stack.top(); stack.pop();
					auto a = stack.top(); stack.pop();
					if (std::isnan(a.f64) || std::isnan(b.f64))
					{
						stack.push(std::numeric_limits<double>::quiet_NaN());
					}
					else if (a.f64 < b.f64 || (std::signbit(a.f64) && !std::signbit(b.f64)))
					{
						stack.push(a.f64);
					}
					else
					{
						stack.push(b.f64);
					}
				}
				break;

			case 0xa5: // f64.max
				WASM_CHECK_STACK(2);
				{
					auto b = stack.top(); stack.pop();
					auto a = stack.top(); stack.pop();
					if (std::isnan(a.f64) || std::isnan(b.f64))
					{
						stack.push(std::numeric_limits<double>::quiet_NaN());
					}
					else if (a.f64 < b.f64 || (std::signbit(a.f64) && !std::signbit(b.f64)))
					{
						stack.push(b.f64);
					}
					else
					{
						stack.push(a.f64);
					}
				}
				break;

			case 0xa6: // f64.copysign
				WASM_CHECK_STACK(2);
				{
					auto b = stack.top(); stack.pop();
					auto a = stack.top(); stack.pop();
					stack.push(std::copysign(a.f64, b.f64));
				}
				break;

			case 0xa7: // i32.wrap_i64
				WASM_CHECK_STACK(1);
				stack.top() = static_cast<int32_t>(stack.top().i64);
				break;

			case 0xa8: // i32.trunc_f32_s
				WASM_CHECK_STACK(1);
				SOUP_IF_UNLIKELY (std::isnan(stack.top().f32) || stack.top().f32 < F32_I32_MIN || stack.top().f32 > F32_I32_MAX)
				{
#if DEBUG_VM
					std::cout << "float cannot be represented as int\n";
#endif
					return false;
				}
				stack.top() = static_cast<int32_t>(stack.top().f32);
				break;

			case 0xa9: // i32.trunc_f32_u
				WASM_CHECK_STACK(1);
				SOUP_IF_UNLIKELY (std::isnan(stack.top().f32) || stack.top().f32 < F32_U32_MIN || stack.top().f32 > F32_U32_MAX)
				{
#if DEBUG_VM
					std::cout << "float cannot be represented as int\n";
#endif
					return false;
				}
				stack.top() = static_cast<uint32_t>(stack.top().f32);
				break;

			case 0xaa: // i32.trunc_f64_s
				WASM_CHECK_STACK(1);
				SOUP_IF_UNLIKELY (std::isnan(stack.top().f64) || stack.top().f64 < F64_I32_MIN || stack.top().f64 > F64_I32_MAX)
				{
#if DEBUG_VM
					std::cout << "float cannot be represented as int\n";
#endif
					return false;
				}
				stack.top() = static_cast<int32_t>(stack.top().f64);
				break;

			case 0xab: // i32.trunc_f64_u
				WASM_CHECK_STACK(1);
				SOUP_IF_UNLIKELY (std::isnan(stack.top().f64) || stack.top().f64 < F64_U32_MIN || stack.top().f64 > F64_U32_MAX)
				{
#if DEBUG_VM
					std::cout << "invalid value for i32.trunc_f64_u\n";
#endif
					return false;
				}
				stack.top() = static_cast<uint32_t>(stack.top().f64);
				break;

			case 0xac: // i64.extend_i32_s
				WASM_CHECK_STACK(1);
				stack.top() = static_cast<int64_t>(stack.top().i32);
				break;

			case 0xad: // i64.extend_i32_u
				WASM_CHECK_STACK(1);
				stack.top() = static_cast<int64_t>(static_cast<uint64_t>(static_cast<uint32_t>(stack.top().i32)));
				break;

			case 0xae: // i64.trunc_f32_s
				WASM_CHECK_STACK(1);
				SOUP_IF_UNLIKELY (std::isnan(stack.top().f32) || stack.top().f32 < F32_I64_MIN || stack.top().f32 > F32_I64_MAX)
				{
#if DEBUG_VM
					std::cout << "float cannot be represented as int\n";
#endif
					return false;
				}
				stack.top() = static_cast<int64_t>(stack.top().f32);
				break;

			case 0xaf: // i64.trunc_f32_u
				WASM_CHECK_STACK(1);
				SOUP_IF_UNLIKELY (std::isnan(stack.top().f32) || stack.top().f32 < F32_U64_MIN || stack.top().f32 > F32_U64_MAX)
				{
#if DEBUG_VM
					std::cout << "float cannot be represented as int\n";
#endif
					return false;
				}
				stack.top() = static_cast<uint64_t>(stack.top().f32);
				break;

			case 0xb0: // i64.trunc_f64_s
				WASM_CHECK_STACK(1);
				SOUP_IF_UNLIKELY (std::isnan(stack.top().f64) || stack.top().f64 < F64_I64_MIN || stack.top().f64 > F64_I64_MAX)
				{
#if DEBUG_VM
					std::cout << "float cannot be represented as int\n";
#endif
					return false;
				}
				stack.top() = static_cast<int64_t>(stack.top().f64);
				break;

			case 0xb1: // i64.trunc_f64_u
				WASM_CHECK_STACK(1);
				SOUP_IF_UNLIKELY (std::isnan(stack.top().f64) || stack.top().f64 < F64_U64_MIN || stack.top().f64 > F64_U64_MAX)
				{
#if DEBUG_VM
					std::cout << "float cannot be represented as int\n";
#endif
					return false;
				}
				stack.top() = static_cast<uint64_t>(stack.top().f64);
				break;

			case 0xb2: // f32.convert_i32_s
				WASM_CHECK_STACK(1);
				stack.top() = static_cast<float>(stack.top().i32);
				break;

			case 0xb3: // f32.convert_i32_u
				WASM_CHECK_STACK(1);
				stack.top() = static_cast<float>(static_cast<uint32_t>(stack.top().i32));
				break;

			case 0xb4: // f32.convert_i64_s
				WASM_CHECK_STACK(1);
				stack.top() = static_cast<float>(stack.top().i64);
				break;

			case 0xb5: // f32.convert_i64_u
				WASM_CHECK_STACK(1);
				stack.top() = static_cast<float>(static_cast<uint64_t>(stack.top().i64));
				break;

			case 0xb6: // f32.demote_f64
				WASM_CHECK_STACK(1);
				stack.top() = static_cast<float>(stack.top().f64);
				break;

			case 0xb7: // f64.convert_i32_s
				WASM_CHECK_STACK(1);
				stack.top() = static_cast<double>(stack.top().i32);
				break;

			case 0xb8: // f64.convert_i32_u
				WASM_CHECK_STACK(1);
				stack.top() = static_cast<double>(static_cast<uint32_t>(stack.top().i32));
				break;

			case 0xb9: // f64.convert_i64_s
				WASM_CHECK_STACK(1);
				stack.top() = static_cast<double>(stack.top().i64);
				break;

			case 0xba: // f64.convert_i64_u
				WASM_CHECK_STACK(1);
				stack.top() = static_cast<double>(static_cast<uint64_t>(stack.top().i64));
				break;

			case 0xbb: // f64.promote_f32
				WASM_CHECK_STACK(1);
				stack.top() = static_cast<double>(stack.top().f32);
				break;

			case 0xbc: // i32.reinterpret_f32
				WASM_CHECK_STACK(1);
				stack.top().type = WASM_I32;
				break;

			case 0xbd: // i64.reinterpret_f64
				WASM_CHECK_STACK(1);
				stack.top().type = WASM_I64;
				break;

			case 0xbe: // f32.reinterpret_i32
				WASM_CHECK_STACK(1);
				stack.top().type = WASM_F32;
				break;

			case 0xbf: // f64.reinterpret_i64
				WASM_CHECK_STACK(1);
				stack.top().type = WASM_F64;
				break;

			case 0xc0: // i32.extend8_s
				WASM_CHECK_STACK(1);
				stack.top().i32 = static_cast<int32_t>(static_cast<int8_t>(stack.top().i32));
				break;

			case 0xc1: // i32.extend16_s
				WASM_CHECK_STACK(1);
				stack.top().i32 = static_cast<int32_t>(static_cast<int16_t>(stack.top().i32));
				break;

			case 0xc2: // i64.extend8_s
				WASM_CHECK_STACK(1);
				stack.top().i64 = static_cast<int64_t>(static_cast<int8_t>(stack.top().i64));
				break;

			case 0xc3: // i64.extend16_s
				WASM_CHECK_STACK(1);
				stack.top().i64 = static_cast<int64_t>(static_cast<int16_t>(stack.top().i64));
				break;

			case 0xc4: // i64.extend32_s
				WASM_CHECK_STACK(1);
				stack.top().i64 = static_cast<int64_t>(static_cast<int32_t>(stack.top().i64));
				break;

			case 0xd0: // ref.null
				{
					uint8_t type;
					r.u8(type);
					stack.push(static_cast<WasmType>(type));
				}
				break;

			case 0xd1: // ref.is_null
				WASM_CHECK_STACK(1);
				stack.push(static_cast<int32_t>(stack.top().i64 == 0));
				break;

			case 0xd2: // ref.func
				{
					uint32_t idx;
					r.oml(idx);
					stack.push(WASM_FUNCREF);
					stack.top().i64 = 0x1'0000'0000 | idx;
				}
				break;

			case 0xfc:
				r.u8(op);
				switch (op)
				{
				case 0x00: // i32.trunc_sat_f32_s
					WASM_CHECK_STACK(1);
					if (std::isnan(stack.top().f32))
					{
						stack.top() = 0;
					}
					else if (stack.top().f32 < F32_I32_MIN)
					{
						stack.top() = INT32_MIN;
					}
					else if (stack.top().f32 > F32_I32_MAX)
					{
						stack.top() = INT32_MAX;
					}
					else
					{
						stack.top() = static_cast<int32_t>(stack.top().f32);
					}
					break;

				case 0x01: // i32.trunc_sat_f32_u
					WASM_CHECK_STACK(1);
					if (std::isnan(stack.top().f32))
					{
						stack.top() = 0;
					}
					else if (stack.top().f32 < F32_U32_MIN)
					{
						stack.top() = 0;
					}
					else if (stack.top().f32 > F32_U32_MAX)
					{
						stack.top() = UINT32_MAX;
					}
					else
					{
						stack.top() = static_cast<uint32_t>(stack.top().f32);
					}
					break;

				case 0x02: // i32.trunc_sat_f64_s
					WASM_CHECK_STACK(1);
					if (std::isnan(stack.top().f64))
					{
						stack.top() = 0;
					}
					else if (stack.top().f64 < F64_I32_MIN)
					{
						stack.top() = INT32_MIN;
					}
					else if (stack.top().f64 > F64_I32_MAX)
					{
						stack.top() = INT32_MAX;
					}
					else
					{
						stack.top() = static_cast<int32_t>(stack.top().f64);
					}
					break;

				case 0x03: // i32.trunc_sat_f64_u
					WASM_CHECK_STACK(1);
					if (std::isnan(stack.top().f64))
					{
						stack.top() = 0;
					}
					else if (stack.top().f64 < F64_U32_MIN)
					{
						stack.top() = 0;
					}
					else if (stack.top().f64 > F64_U32_MAX)
					{
						stack.top() = UINT32_MAX;
					}
					else
					{
						stack.top() = static_cast<uint32_t>(stack.top().f64);
					}
					break;

				case 0x04: // i64.trunc_sat_f32_s
					WASM_CHECK_STACK(1);
					if (std::isnan(stack.top().f32))
					{
						stack.top() = static_cast<int64_t>(0);
					}
					else if (stack.top().f32 < F32_I64_MIN)
					{
						stack.top() = INT64_MIN;
					}
					else if (stack.top().f32 > F32_I64_MAX)
					{
						stack.top() = INT64_MAX;
					}
					else
					{
						stack.top() = static_cast<int64_t>(stack.top().f32);
					}
					break;

				case 0x05: // i64.trunc_sat_f32_u
					WASM_CHECK_STACK(1);
					if (std::isnan(stack.top().f32))
					{
						stack.top() = static_cast<int64_t>(0);
					}
					else if (stack.top().f32 < F32_U64_MIN)
					{
						stack.top() = static_cast<int64_t>(0);
					}
					else if (stack.top().f32 > F32_U64_MAX)
					{
						stack.top() = UINT64_MAX;
					}
					else
					{
						stack.top() = static_cast<uint64_t>(stack.top().f32);
					}
					break;

				case 0x06: // i64.trunc_sat_f64_s
					WASM_CHECK_STACK(1);
					if (std::isnan(stack.top().f64))
					{
						stack.top() = static_cast<int64_t>(0);
					}
					else if (stack.top().f64 < F64_I64_MIN)
					{
						stack.top() = INT64_MIN;
					}
					else if (stack.top().f64 > F64_I64_MAX)
					{
						stack.top() = INT64_MAX;
					}
					else
					{
						stack.top() = static_cast<int64_t>(stack.top().f64);
					}
					break;

				case 0x07: // i64.trunc_sat_f64_u
					WASM_CHECK_STACK(1);
					if (std::isnan(stack.top().f64))
					{
						stack.top() = static_cast<int64_t>(0);
					}
					else if (stack.top().f64 < F64_U64_MIN)
					{
						stack.top() = static_cast<int64_t>(0);
					}
					else if (stack.top().f64 > F64_U64_MAX)
					{
						stack.top() = UINT64_MAX;
					}
					else
					{
						stack.top() = static_cast<uint64_t>(stack.top().f64);
					}
					break;

				case 0x0a: // memory.copy
					{
						r.skip(2); // reserved
						WASM_CHECK_STACK(3);
						auto size = popUPTR();
						auto src = popUPTR();
						auto dst = popUPTR();
						SOUP_IF_UNLIKELY (src + size > script.memory.size || dst + size > script.memory.size)
						{
#if DEBUG_VM
							std::cout << "out-of-bounds memory.copy\n";
#endif
							return false;
						}
						memcpy(&script.memory.data[dst], &script.memory.data[src], size);
					}
					break;

				case 0x0b: // memory.fill
					{
						r.skip(1); // reserved
						WASM_CHECK_STACK(3);
						auto size = popUPTR();
						auto value = stack.top().i32; stack.pop();
						auto addr = popUPTR();
						auto ptr = script.memory.getView(addr, size);
						SOUP_IF_UNLIKELY (!ptr)
						{
#if DEBUG_VM
							std::cout << "out-of-bounds memory.fill\n";
#endif
							return false;
						}
						memset(ptr, value, size);
					}
					break;

				default:
#if DEBUG_VM
					std::cout << "Unsupported opcode: " << string::hex(0xFC00 | op) << "\n";
#endif
					return false;
				}
				break;
			}
		}
		return true;
	}

	bool WasmVm::skipOverBranch(Reader& r, uint32_t depth) SOUP_EXCAL
	{
		uint8_t op;
		while (r.u8(op))
		{
			switch (op)
			{
			case 0x02: // block
			case 0x03: // loop
			case 0x04: // if
				r.skip(1); // result type
				++depth;
				break;

			case 0x05: // else
				if (depth == 0)
				{
					return true;
				}
				break;

			case 0x0b: // end
				if (depth == 0)
				{
					return false;
				}
				--depth;
				break;

			case 0x0c: // br
			case 0x0d: // br_if
			case 0x10: // call
			case 0x20: // local.get
			case 0x21: // local.set
			case 0x22: // local.tee
			case 0x23: // global.get
			case 0x24: // global.set
			case 0x25: // table.get
			case 0x26: // table.set
				{
					size_t imm;
					r.oml(imm);
				}
				break;

			case 0x0e: // br_table
				{
					uint32_t num_branches;
					r.oml(num_branches);
					while (num_branches--)
					{
						uint32_t depth;
						r.oml(depth);
					}
					uint32_t default_depth;
					r.oml(default_depth);
				}
				break;

			case 0x11: // call_indirect
				{
					uint32_t type_index; r.oml(type_index);
					uint32_t table_index; r.oml(table_index);
				}
				break;

			case 0x1c: // select t
				r.skip(2);
				break;

			case 0x28: // i32.load
			case 0x29: // i64.load
			case 0x2a: // f32.load
			case 0x2b: // f64.load
			case 0x2c: // i32.load8_s
			case 0x2d: // i32.load8_u
			case 0x2e: // i32.load16_s
			case 0x2f: // i32.load16_u
			case 0x30: // i64.load8_s
			case 0x31: // i64.load8_u
			case 0x32: // i64.load16_s
			case 0x33: // i64.load16_u
			case 0x34: // i64.load32_s
			case 0x35: // i64.load32_u
			case 0x36: // i32.store
			case 0x37: // i64.store
			case 0x38: // f32.store
			case 0x39: // f64.store
			case 0x3a: // i32.store8
			case 0x3b: // i32.store16
			case 0x3c: // i64.store8
			case 0x3d: // i64.store16
			case 0x3e: // i64.store32
				{
					r.skip(1); // memflags
					SOUP_UNUSED(readUPTR(r));
				}
				break;

			case 0x3f: // memory.size
			case 0x40: // memory.grow
				r.skip(1);
				break;

			case 0x41: // i32.const
				{
					int32_t value;
					r.soml(value);
				}
				break;

			case 0x42: // i64.const
				{
					int64_t value;
					r.soml(value);
				}
				break;

			case 0x43: // f32.const
				r.skip(4);
				break;

			case 0x44: // f64.const
				r.skip(8);
				break;

			case 0xd0: // ref.null
				r.skip(1);
				break;

			case 0xd2: // ref.func
				{
					uint32_t idx;
					r.oml(idx);
				}
				break;

#if DEBUG_VM
			case 0x00: // unreachable
			case 0x01: // nop
			case 0x0f: // return
			case 0x1a: // drop
			case 0x1b: // select
			case 0x45: // i32.eqz
			case 0x46: // i32.eq
			case 0x47: // i32.ne
			case 0x48: // i32.lt_s
			case 0x49: // i32.lt_u
			case 0x4a: // i32.gt_s
			case 0x4b: // i32.gt_u
			case 0x4c: // i32.le_s
			case 0x4d: // i32.le_u
			case 0x4e: // i32.ge_s
			case 0x4f: // i32.ge_u
			case 0x50: // i64.eqz
			case 0x51: // i64.eq
			case 0x52: // i64.ne
			case 0x53: // i64.lt_s
			case 0x54: // i64.lt_u
			case 0x55: // i64.gt_s
			case 0x56: // i64.gt_u
			case 0x57: // i64.le_s
			case 0x58: // i64.le_u
			case 0x59: // i64.ge_s
			case 0x5a: // i64.ge_u
			case 0x5b: // f32.eq
			case 0x5c: // f32.ne
			case 0x5d: // f32.lt
			case 0x5e: // f32.gt
			case 0x5f: // f32.le
			case 0x60: // f32.ge
			case 0x61: // f64.eq
			case 0x62: // f64.ne
			case 0x63: // f64.lt
			case 0x64: // f64.gt
			case 0x65: // f64.le
			case 0x66: // f64.ge
			case 0x67: // i32.clz
			case 0x68: // i32.ctz
			case 0x69: // i32.popcnt
			case 0x6a: // i32.add
			case 0x6b: // i32.sub
			case 0x6c: // i32.mul
			case 0x6d: // i32.div_s
			case 0x6e: // i32.div_u
			case 0x6f: // i32.rem_s
			case 0x70: // i32.rem_u
			case 0x71: // i32.and
			case 0x72: // i32.or
			case 0x73: // i32.xor
			case 0x74: // i32.shl
			case 0x75: // i32.shr_s ("arithmetic right shift")
			case 0x76: // i32.shr_u ("logical right shift")
			case 0x77: // i32.rotl
			case 0x78: // i32.rotr
			case 0x79: // i64.clz
			case 0x7a: // i64.ctz
			case 0x7b: // i64.popcnt
			case 0x7c: // i64.add
			case 0x7d: // i64.sub
			case 0x7e: // i64.mul
			case 0x7f: // i64.div_s
			case 0x80: // i64.div_u
			case 0x81: // i64.rem_s
			case 0x82: // i64.rem_u
			case 0x83: // i64.and
			case 0x84: // i64.or
			case 0x85: // i64.xor
			case 0x86: // i64.shl
			case 0x87: // i64.shr_s ("arithmetic right shift")
			case 0x88: // i64.shr_u ("logical right shift")
			case 0x89: // i64.rotl
			case 0x8a: // i64.rotr
			case 0x8b: // f32.abs
			case 0x8c: // f32.neg
			case 0x8d: // f32.ceil
			case 0x8e: // f32.floor
			case 0x8f: // f32.trunc
			case 0x90: // f32.nearest
			case 0x91: // f32.sqrt
			case 0x92: // f32.add
			case 0x93: // f32.sub
			case 0x94: // f32.mul
			case 0x95: // f32.div
			case 0x96: // f32.min
			case 0x97: // f32.max
			case 0x98: // f32.copysign
			case 0x99: // f64.abs
			case 0x9a: // f64.neg
			case 0x9b: // f64.ceil
			case 0x9c: // f64.floor
			case 0x9d: // f64.trunc
			case 0x9e: // f64.nearest
			case 0x9f: // f64.sqrt
			case 0xa0: // f64.add
			case 0xa1: // f64.sub
			case 0xa2: // f64.mul
			case 0xa3: // f64.div
			case 0xa4: // f64.min
			case 0xa5: // f64.max
			case 0xa6: // f64.copysign
			case 0xa7: // i32.wrap_i64
			case 0xa8: // i32.trunc_f32_s
			case 0xa9: // i32.trunc_f32_u
			case 0xaa: // i32.trunc_f64_s
			case 0xab: // i32.trunc_f64_u
			case 0xac: // i64.extend_i32_s
			case 0xad: // i64.extend_i32_u
			case 0xae: // i64.trunc_f32_s
			case 0xaf: // i64.trunc_f32_u
			case 0xb0: // i64.trunc_f64_s
			case 0xb1: // i64.trunc_f64_u
			case 0xb2: // f32.convert_i32_s
			case 0xb3: // f32.convert_i32_u
			case 0xb4: // f32.convert_i64_s
			case 0xb5: // f32.convert_i64_u
			case 0xb6: // f32.demote_f64
			case 0xb7: // f64.convert_i32_s
			case 0xb8: // f64.convert_i32_u
			case 0xb9: // f64.convert_i64_s
			case 0xba: // f64.convert_i64_u
			case 0xbb: // f64.promote_f32
			case 0xbc: // i32.reinterpret_f32
			case 0xbd: // i64.reinterpret_f64
			case 0xbe: // f32.reinterpret_i32
			case 0xbf: // f64.reinterpret_i64
			case 0xc0: // i32.extend8_s
			case 0xc1: // i32.extend16_s
			case 0xc2: // i64.extend8_s
			case 0xc3: // i64.extend16_s
			case 0xc4: // i64.extend32_s
			case 0xd1: // ref.is_null
				break;

			default:
				std::cout << "skipOverBranch: unknown instruction " << string::hex(op) << ", might cause problems\n";
				break;
#endif

			case 0xfc:
				r.u8(op);
				switch (op)
				{
				case 0x0a: // memory.copy
					r.skip(2);
					break;

				case 0x0b: // memory.fill
					r.skip(1);
					break;

#if DEBUG_VM
				case 0x00: // i32.trunc_sat_f32_s
				case 0x01: // i32.trunc_sat_f32_u
				case 0x02: // i32.trunc_sat_f64_s
				case 0x03: // i32.trunc_sat_f64_u
				case 0x04: // i64.trunc_sat_f32_s
				case 0x05: // i64.trunc_sat_f32_u
				case 0x06: // i64.trunc_sat_f64_s
				case 0x07: // i64.trunc_sat_f64_u
					break;

				default:
					std::cout << "skipOverBranch: unknown instruction " << string::hex(static_cast<uint16_t>(0xFC00 | op)) << ", might cause problems\n";
					break;
#endif
				}
				break;
			}
		}
#if DEBUG_VM
		std::cout << "skipOverBranch: end of stream reached\n";
#endif
		return false;
	}

	bool WasmVm::doBranch(Reader& r, uint32_t depth, std::stack<CtrlFlowEntry>& ctrlflow) SOUP_EXCAL
	{
#if DEBUG_VM
		std::cout << "branch with depth " << depth << " at position " << r.getPosition() << "\n";
#endif

		SOUP_IF_UNLIKELY (ctrlflow.empty())
		{
#if DEBUG_VM
			std::cout << "branch returns from function\n";
#endif
			r.seekEnd();
			return true;
		}
		for (uint32_t i = 0; i != depth; ++i)
		{
			ctrlflow.pop();
			SOUP_IF_UNLIKELY (ctrlflow.empty())
			{
#if DEBUG_VM
				std::cout << "branch returns from function\n";
#endif
				r.seekEnd();
				return true;
			}
		}

		if (ctrlflow.top().position == -1)
		{
			// branch forwards
			if (skipOverBranch(r, depth))
			{
				// also skip over 'else' branch
				skipOverBranch(r, depth);
			}
		}
		else
		{
			// branch backwards
			r.seek(ctrlflow.top().position);
		}
#if DEBUG_VM
		std::cout << "position after branch: " << r.getPosition() << "\n";
#endif

		std::vector<WasmValue> results;
		for (size_t i = 0; i != ctrlflow.top().num_values; ++i)
		{
			results.emplace_back(stack.top()); stack.pop();
		}
		while (stack.size() > ctrlflow.top().stack_size)
		{
			stack.pop();
		}
		for (size_t i = 0; i != ctrlflow.top().num_values; ++i)
		{
			stack.push(results[(results.size() - 1) - i]);
		}

#if DEBUG_VM
		std::cout << "stack size after branch: " << stack.size() << "\n";
#endif

		if (ctrlflow.top().position == -1)
		{
			ctrlflow.pop(); // we passed 'end', so need to pop here.
		}

		return true;
	}

	bool WasmVm::doCall(uint32_t type_index, uint32_t function_index, unsigned depth)
	{
		SOUP_IF_UNLIKELY (depth >= 200)
		{
#if DEBUG_VM
			std::cout << "call: c stack overflow\n";
#endif
			return false;
		}
		++depth;

		SOUP_IF_UNLIKELY (type_index >= script.types.size())
		{
#if DEBUG_VM
			std::cout << "call: type is out-of-bounds\n";
#endif
			return false;
		}
		const auto& type = script.types[type_index];

		if (function_index < script.function_imports.size())
		{
#if DEBUG_VM || DEBUG_API
			std::cout << "Calling into " << script.function_imports[function_index].module_name << ":" << script.function_imports[function_index].function_name << "\n";
#endif
			const auto& imp = script.function_imports[function_index];
			if (imp.ptr)
			{
				imp.ptr(*this, function_index, type);
				return true;
			}
			SOUP_IF_UNLIKELY (!imp.source)
			{
#if DEBUG_VM
				std::cout << "call: unresolved function import\n";
#endif
				return false;
			}
			WasmVm exvm(*imp.source);
			exvm.stack = std::move(this->stack);
			SOUP_RETHROW_FALSE(exvm.doCall(imp.source->getTypeIndexForFunction(imp.func_index), imp.func_index, depth));
			this->stack = std::move(exvm.stack);
			return true;
		}
		function_index -= script.function_imports.size();
		SOUP_IF_UNLIKELY (function_index >= script.code.size())
		{
#if DEBUG_VM
			std::cout << "call: function is out-of-bounds\n";
#endif
			return false;
		}

		WasmVm callvm(script);
		for (uint32_t i = 0; i != type.parameters.size(); ++i)
		{
			SOUP_IF_UNLIKELY (stack.empty())
			{
#if DEBUG_VM
				std::cout << "call: not enough values on the stack for parameters\n";
#endif
				return false;
			}
			//std::cout << "arg: " << script.memory.getPointer<const char>(stack.top()) << "\n";
			callvm.locals.insert(callvm.locals.begin(), stack.top()); stack.pop();
		}
#if DEBUG_VM
		//std::cout << "call: enter " << function_index << "\n";
		//std::cout << string::bin2hex(script.code[function_index]) << "\n";
#endif
		SOUP_RETHROW_FALSE(callvm.run(script.code[function_index], depth));
#if DEBUG_VM
		//std::cout << "call: leave " << function_index << "\n";
#endif
		if (type.results.size() < 2)
		{
			for (uint32_t i = 0; i != type.results.size(); ++i)
			{
				SOUP_IF_UNLIKELY (callvm.stack.empty())
				{
#if DEBUG_VM
					std::cout << "call: not enough values on the stack after return\n";
#endif
					return false;
				}
				//std::cout << "return value: " << callvm.stack.top() << "\n";
				stack.emplace(callvm.stack.top()); callvm.stack.pop();
			}
		}
		else
		{
			std::vector<WasmValue> results{};
			results.reserve(type.results.size());
			for (uint32_t i = 0; i != type.results.size(); ++i)
			{
				SOUP_IF_UNLIKELY (callvm.stack.empty())
				{
#if DEBUG_VM
					std::cout << "call: not enough values on the stack after return\n";
#endif
					return false;
				}
				//std::cout << "return value: " << callvm.stack.top() << "\n";
				results.emplace_back(callvm.stack.top()); callvm.stack.pop();
			}
			for (auto i = results.rbegin(); i != results.rend(); ++i)
			{
				stack.emplace(std::move(*i));
			}
		}
		return true;
	}

	/*intptr_t WasmVm::popIPTR() noexcept
	{
		const auto ptr = script.memory.decodeIPTR(stack.top());
		stack.pop();
		return ptr;
	}*/

	size_t WasmVm::popUPTR() noexcept
	{
		const auto ptr = script.memory.decodeUPTR(stack.top());
		stack.pop();
		return ptr;
	}

	size_t WasmVm::readUPTR(Reader& r) noexcept
	{
		size_t ptr;
		r.oml(ptr);
		return ptr;
	}
}
