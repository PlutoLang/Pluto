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
#if SOUP_WASM_PEDANTIC
#include "unicode.hpp"
#endif

#define DEBUG_LOAD false
#define DEBUG_VM false
#define DEBUG_BRANCHING false
#define DEBUG_API false

#if DEBUG_LOAD || DEBUG_VM || DEBUG_BRANCHING || DEBUG_API
#include <iostream>
#include "string.hpp"
#endif

// Useful resources:
// - https://webassembly.github.io/wabt/demo/wat2wasm/
// - https://github.com/sunfishcode/wasm-reference-manual/blob/master/WebAssembly.md
// - https://pengowray.github.io/wasm-ops/
// - https://webassembly.github.io/spec/versions/core/WebAssembly-2.0.pdf (not the latest version, but "WASM 2.0 minus SIMD plus Memory64" seems like a reasonable target for Soup right now)

/*
Spec tests (https://github.com/Sainan/wasm-spec/tree/wast2json/test/core)
> Use wast2json from wabt then run `soup wast [jsonfile]`
> Results:
- address: pass
- align: FAIL (Soup doesn't fail on some malformed modules)
- binary: FAIL (Soup doesn't fail on some malformed modules)
- binary-leb128: pass_pedantic
- block: pass
- br: pass
- br_if: pass
- br_table: pass
- bulk: pass
- call: pass
- call_indirect: pass_pedantic
- comments: pass
- const: pass
- conversions: pass
- custom: pass_pedantic
- data: FAIL (missing support for memory imports)
- elem: FAIL (missing support for table imports)
- endianness: pass
- exports: pass
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
- func_ptrs: pass
- global: FAIL (missing support for global.get in constants)
- i32: pass
- i64: pass
- if: pass
- imports: FAIL (missing support for assert_unlinkable/imports are not type-checked; missing support for table imports)
- inline-module: pass
- int_exprs: pass
- int_literals: pass
- labels: pass
- left-to-right: pass
- linking: FAIL (missing support for assert_unlinkable/imports are not type-checked; missing support for table imports)
- load: pass
- local_get: pass
- local_set: pass
- local_tee: pass
- loop: pass
- memory: pass
- memory_copy: pass
- memory_fill: pass
- memory_grow: pass
- memory_init: pass
- memory_redundancy: pass
- memory_size: pass
- memory_trap: pass
- names: pass
- nop: pass
- obsolete-keywords: pass
- ref_func: pass
- ref_is_null: pass
- ref_null: pass
- return: pass
- select: pass
- skip-stack-guard-page: pass
- stack: pass
- start: pass
- store: pass
- switch: pass
- table: FAIL (missing support for table imports)
- table-sub: pass (Soup doesn't do static validation)
- table_copy: pass
- table_fill: pass
- table_get: pass
- table_grow: pass
- table_init: pass
- table_set: pass
- table_size: pass
- token: pass
- traps: pass
- type: pass
- unreachable: pass
- unreached-invalid: pass (Soup doesn't do static validation)
- unreached-valid: pass
- unwind: pass
- utf8-custom-section-id: pass_pedantic
- utf8-import-field: pass_pedantic
- utf8-import-module: pass_pedantic
- utf8-invalid-encoding: pass (due to Soup not parsing .wat files)
- memory64/address64: pass
- memory64/align64: pass
- memory64/binary_leb128_64: pass_pedantic
- memory64/bulk64: pass
- memory64/call_indirect64: pass
- memory64/endianness64: pass
- memory64/float_memory64: pass
- memory64/load64: pass
- memory64/memory64: pass
- memory64/memory64-imports: FAIL (missing support for memory & table imports & exports)
- memory64/memory_copy64: pass
- memory64/memory_fill64: pass
- memory64/memory_grow64: pass
- memory64/memory_init64: pass
- memory64/memory_redundancy64: pass
- memory64/memory_trap64: pass
- memory64/table64: FAIL (missing support for table imports)
- memory64/table_copy64: pass
- memory64/table_copy_mixed: pass
- memory64/table_fill64: pass
- memory64/table_get64: pass
- memory64/table_grow64: pass
- memory64/table_init64: pass
- memory64/table_set64: pass
- memory64/table_size64: pass
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

	// WasmFunctionType

	std::string WasmFunctionType::toString() const SOUP_EXCAL
	{
		std::string str = "(param";
		for (const auto& t : parameters)
		{
			str.push_back(' ');
			str.append(wasm_type_to_string(t));
		}
		str.append(") (result");
		for (const auto& t : results)
		{
			str.push_back(' ');
			str.append(wasm_type_to_string(t));
		}
		str.push_back(')');
		return str;
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
		return write(addr.uptr(), src, size);
	}

	void WasmScript::Memory::encodeUPTR(WasmValue& out, size_t in) noexcept
	{
#if SOUP_WASM_MEMORY64
		if (memory64)
		{
			out = static_cast<uint64_t>(in);
		}
		else
#endif
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

	// WasmScript::Table

	size_t WasmScript::Table::grow(size_t delta, int64_t value) SOUP_EXCAL
	{
		size_t old_size = -1;
		if (can_add_without_overflow(values.size(), delta) && values.size() + delta <= limit)
		{
			old_size = values.size();
			values.reserve(old_size + delta);
			while (delta--)
			{
				values.emplace_back(value);
			}
		}
		return old_size;
	}

	bool WasmScript::Table::copy(const Table& src, size_t dst_offset, size_t src_offset, size_t size) noexcept
	{
		SOUP_IF_UNLIKELY (this->type != src.type)
		{
#if DEBUG_VM
			std::cout << "WasmScript::Table::copy: different element types\n";
#endif
			return false;
		}
		SOUP_IF_UNLIKELY (!can_add_without_overflow(dst_offset, size) || dst_offset + size > this->values.size() || !can_add_without_overflow(src_offset, size) || src_offset + size > src.values.size())
		{
#if DEBUG_VM
			std::cout << "out-of-bounds WasmScript::Table::copy: dst_offset=" << dst_offset << ", dst_size=" << this->values.size() << ", src_offset=" << src_offset << ", src_size=" << src.values.size() << ", size=" << size << "\n";
#endif
			return false;
		}
		if (this == &src && dst_offset > src_offset)
		{
			while (size--)
			{
				this->values[dst_offset + size] = src.values[src_offset + size];
			}
		}
		else
		{
			while (size--)
			{
				this->values[dst_offset++] = src.values[src_offset++];
			}
		}
		return true;
	}
	
	// WasmScript

#if SOUP_WASM_PEDANTIC
#define WASM_READ_OML(v) SOUP_RETHROW_FALSE(r.oml<decltype(v), true, true>(v));
#define WASM_READ_SOML(v) SOUP_RETHROW_FALSE(r.soml<decltype(v), true, true>(v));
#else
#define WASM_READ_OML(v) r.oml(v);
#define WASM_READ_SOML(v) r.soml(v);
#endif

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
			WASM_READ_SOML(out.i32);
			out.type = WASM_I32;
			break;

		case 0x42: // i64.const
			WASM_READ_SOML(out.i64);
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
			WASM_READ_OML(out.i32);
			out.hi32 = 1;
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
#if SOUP_WASM_PEDANTIC
		uint32_t data_count = -1;
#endif
		while (r.hasMore())
		{
			uint8_t section_type;
			r.u8(section_type);
			uint32_t section_size;
			WASM_READ_OML(section_size);
			switch (section_type)
			{
			default:
#if DEBUG_LOAD
				std::cout << "Unhandled section type: " << (int)section_type << " (size: " << section_size << ")\n";
#endif
#if !SOUP_WASM_PEDANTIC
				SOUP_IF_UNLIKELY (section_size == 0)
#endif
				{
					return false;
				}
				r.skip(section_size);
				break;

#if SOUP_WASM_PEDANTIC
			case 0: // Custom
				{
					SOUP_RETHROW_FALSE(section_size != 0);
					const auto section_end = r.getPosition() + section_size;
					uint32_t name_len;
					WASM_READ_OML(name_len);
					//std::cout << "custom section: name_len = " << name_len << "\n";
					SOUP_RETHROW_FALSE(name_len <= 0x1000);
					std::string name;
					r.str(name_len, name);
					auto name_utf32 = unicode::utf8_to_utf32(name);
					SOUP_RETHROW_FALSE(name_utf32.find(unicode::REPLACEMENT_CHAR) == std::string::npos); // UTF-8 must be valid
					SOUP_RETHROW_FALSE(unicode::utf32_to_utf8(name_utf32) == name); // UTF-8 must also be represented canonically (so, no overlong encodings)
					r.seekEnd();
					SOUP_RETHROW_FALSE(section_end <= r.getPosition());
					r.seek(section_end);
				}
				break;
#endif

			case 1: // Type
				{
					uint32_t num_types;
					WASM_READ_OML(num_types);
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
								WASM_READ_OML(size);
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

								WASM_READ_OML(size);
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
					uint32_t num_imports;
					WASM_READ_OML(num_imports);
#if DEBUG_LOAD
					std::cout << num_imports << " import(s)\n";
#endif
					while (num_imports--)
					{
						uint32_t module_name_len; WASM_READ_OML(module_name_len);
						std::string module_name; r.str(module_name_len, module_name);
						uint32_t field_name_len; WASM_READ_OML(field_name_len);
						std::string field_name; r.str(field_name_len, field_name);
#if DEBUG_LOAD
						std::cout << "- " << module_name << ":" << field_name << "\n";
#endif
#if SOUP_WASM_PEDANTIC
						{
							auto name_utf32 = unicode::utf8_to_utf32(module_name);
							SOUP_RETHROW_FALSE(name_utf32.find(unicode::REPLACEMENT_CHAR) == std::string::npos); // UTF-8 must be valid
							SOUP_RETHROW_FALSE(unicode::utf32_to_utf8(name_utf32) == module_name); // UTF-8 must also be represented canonically (so, no overlong encodings)
						}
						{
							auto name_utf32 = unicode::utf8_to_utf32(field_name);
							SOUP_RETHROW_FALSE(name_utf32.find(unicode::REPLACEMENT_CHAR) == std::string::npos); // UTF-8 must be valid
							SOUP_RETHROW_FALSE(unicode::utf32_to_utf8(name_utf32) == field_name); // UTF-8 must also be represented canonically (so, no overlong encodings)
						}
#endif
						uint8_t kind; r.u8(kind);
						if (kind == 0) // function
						{
							uint32_t type_index; WASM_READ_OML(type_index);
							SOUP_RETHROW_FALSE(type_index < types.size());
							function_imports.emplace_back(FunctionImport{ std::move(module_name), std::move(field_name), nullptr, {}, type_index, (uint32_t)-1 });
						}
						/*else if (kind == 1) // table
						{
							uint8_t type; r.u8(type);
							uint8_t flags; r.u8(flags);
							size_t size; WASM_READ_OML(size);
							if (flags & 1)
							{
								WASM_READ_OML(size);
							}
						}*/
						// 2 - memory
						else if (kind == 3) // global
						{
							uint8_t type; r.u8(type);
							r.skip(1); // mutability
							global_imports.emplace_back(Import{ std::move(module_name), std::move(field_name) });
							globals.emplace_back();
						}
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
					uint32_t num_functions;
					WASM_READ_OML(num_functions);
#if DEBUG_LOAD
					std::cout << num_functions << " function type(s)\n";
#endif
					functions.reserve(num_functions);
					while (num_functions--)
					{
						uint32_t type_index;
						WASM_READ_OML(type_index);
						SOUP_RETHROW_FALSE(type_index < types.size());
						functions.emplace_back(type_index);
					}
				}
				break;

			case 4: // Table
				{
					uint32_t num_tables; WASM_READ_OML(num_tables);
#if DEBUG_LOAD
					std::cout << num_tables << " table(s)\n";
#endif
					tables.reserve(num_tables);
					while (num_tables--)
					{
						uint8_t type;
						r.u8(type);
						auto& tbl = tables.emplace_back(static_cast<WasmType>(type));
						uint8_t flags;
						r.u8(flags);
#if SOUP_WASM_MEMORY64
						if (flags & 4)
						{
							tbl.table64 = true;
						}
#else
						SOUP_RETHROW_FALSE((flags & 0xfe) == 0);
#endif
						size_t initial;
						WASM_READ_OML(initial);
						if (flags & 1)
						{
							WASM_READ_OML(tbl.limit);
						}
						while (initial != tbl.values.size())
						{
							tbl.values.emplace_back();
						}
					}
				}
				break;

			case 5: // Memory
				{
					uint32_t num_memories; WASM_READ_OML(num_memories);
					if (num_memories != 0)
					{
						SOUP_IF_UNLIKELY (memory || num_memories != 1)
						{
#if DEBUG_LOAD
							std::cout << "Too many memories\n";
#endif
							return false;
						}
						uint8_t flags; r.u8(flags);
						size_t pages;
#if !SOUP_WASM_PEDANTIC
						WASM_READ_OML(pages);
#else
	#if SOUP_WASM_MEMORY64
						if (flags & 4)
						{
							uint64_t pages_u64;
							WASM_READ_OML(pages_u64);
							pages = pages_u64;
						}
	#endif
						else
						{
							uint32_t pages_u32;
							WASM_READ_OML(pages_u32);
							pages = pages_u32;
						}
#endif
						this->memory = soup::make_shared<Memory>();
						auto& memory = *this->memory;
						if (flags & 1)
						{
#if SOUP_WASM_MEMORY64
							if (flags & 4)
							{
								uint64_t page_limit;
								WASM_READ_OML(page_limit);
								memory.page_limit = page_limit;
							}
							else
							{
								uint32_t page_limit;
								WASM_READ_OML(page_limit);
								memory.page_limit = page_limit;
							}
#else
							WASM_READ_OML(memory.page_limit);
#endif
						}
#if SOUP_WASM_MEMORY64
						if (flags & 4)
						{
							memory.memory64 = true;
						}
#else
						SOUP_RETHROW_FALSE((flags & 0xfe) == 0);
#endif
						//std::cout << "pages = " << pages << "\n";
						//std::cout << "page_limit = " << memory.page_limit << "\n";
						if (pages == 0)
						{
							memory.data = (uint8_t*)soup::malloc(1);
							memory.size = 1;
						}
						else
						{
							SOUP_RETHROW_FALSE(pages <= memory.page_limit);
							memory.data = (uint8_t*)soup::malloc(pages * 0x10'000);
							memory.size = pages * 0x10'000;
						}
						memset(memory.data, 0, memory.size);
#if DEBUG_LOAD
						std::cout << "Memory consists of " << pages << " pages, totalling " << memory.size << " bytes\n";
#endif
					}
				}
				break;

			case 6: // Global
				{
					uint32_t num_globals;
					WASM_READ_OML(num_globals);
#if DEBUG_LOAD
					std::cout << num_globals << " global(s)\n";
#endif
					globals.reserve(globals.size() + num_globals);
					while (num_globals--)
					{
						uint8_t type; r.u8(type);
						r.skip(1); // mutability
						WasmValue& value = *globals.emplace_back(soup::make_shared<WasmValue>());
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
					uint32_t num_exports;
					WASM_READ_OML(num_exports);
#if DEBUG_LOAD
					std::cout << num_exports << " export(s)\n";
#endif
					export_map.reserve(num_exports);
					while (num_exports--)
					{
						uint32_t name_len;
						WASM_READ_OML(name_len);
						std::string name;
						r.str(name_len, name);
#if DEBUG_LOAD
						std::cout << "- " << name << "\n";
#endif
						uint8_t kind; r.u8(kind);
						uint32_t index; WASM_READ_OML(index);
						export_map.emplace(std::move(name), Export{ kind, index });
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
				WASM_READ_OML(start_func_idx);
#if DEBUG_LOAD
				std::cout << "start_func_idx: " << start_func_idx << "\n";
#endif
				break;

			case 9: // Elem
				{
					uint32_t num_segments;
					WASM_READ_OML(num_segments);
#if DEBUG_LOAD
					std::cout << num_segments << " element segment(s)\n";
#endif
					for (uint32_t i = 0; i != num_segments; ++i)
					{
						uint32_t flags;
						WASM_READ_OML(flags);
#if DEBUG_LOAD
						std::cout << "elem flags: " << flags << "\n";
#endif
						if (flags & 1)
						{
							uint8_t type = WASM_FUNCREF;
							if (flags & 0b100)
							{
								r.u8(type);
							}
							else
							{
								r.skip(1);
							}
#if DEBUG_LOAD
							std::cout << "passive elem segment of type " << (int)type << "\n";
#endif
							Table& vtbl = passive_elem_segments.emplace(i, static_cast<WasmType>(type)).first->second;
							uint32_t num_elements;
							WASM_READ_OML(num_elements);
							vtbl.values.reserve(num_elements);
							while (num_elements--)
							{
								if (flags & 0b100)
								{
									WasmValue value;
									readConstant(r, value);
									vtbl.values.emplace_back(/*vtbl.type == WASM_FUNCREF &&*/ value.type == vtbl.type ? value.i64 : 0);
								}
								else
								{
									uint32_t function_index;
									WASM_READ_OML(function_index);
									vtbl.values.emplace_back(vtbl.type == WASM_FUNCREF ? 0x1'0000'0000 | function_index : 0);
								}
							}
						}
						else
						{
							uint32_t tblidx = 0;
							if (flags & 0b10)
							{
								WASM_READ_OML(tblidx);
							}
#if DEBUG_LOAD
							std::cout << "- elements for table " << tblidx << "\n";
#endif
							Table scrap(WASM_FUNCREF);
							// an out-of-bounds table index is apparently valid...
							auto& table = tblidx >= tables.size() ? scrap : tables[tblidx];
							WasmValue base;
							SOUP_RETHROW_FALSE(readConstant(r, base));
							{
								WasmType index_type = WASM_I32;
#if SOUP_WASM_MEMORY64
								if (table.table64)
								{
									index_type = WASM_I64;
								}
#endif
								SOUP_IF_UNLIKELY (base.type != index_type)
								{
#if DEBUG_LOAD
									std::cout << "unexpected type for element initialisation: " << string::hex(static_cast<uint8_t>(base.type)) << "\n";
#endif
									return false;
								}
							}
							auto index = base.uptr();
							if (flags & 2)
							{
								r.skip(1); // reserved
							}
							uint32_t num_elements;
							WASM_READ_OML(num_elements);
							SOUP_IF_UNLIKELY (index + num_elements > table.values.size())
							{
#if DEBUG_LOAD
								std::cout << "elem: " << index << " + " << num_elements << " > " << table.values.size() << "\n";
#endif
								return false;
							}
							while (num_elements--)
							{
								if (flags & 0b100)
								{
									WasmValue value;
									readConstant(r, value);
									if (/*table.type == WASM_FUNCREF &&*/ value.type == table.type)
									{
										table.values[index++] = value.i64;
									}
								}
								else
								{
									uint32_t function_index;
									WASM_READ_OML(function_index);
									if (table.type == WASM_FUNCREF)
									{
										table.values[index++] = 0x1'0000'0000 | function_index;
									}
								}
							}
						}
					}
				}
				break;

#if SOUP_WASM_PEDANTIC
			case 12: // DataCount
				WASM_READ_OML(data_count);
				break;
#endif

			case 10: // Code
				{
					uint32_t num_functions;
					WASM_READ_OML(num_functions);
#if DEBUG_LOAD
					std::cout << num_functions << " function(s)\n";
#endif
					SOUP_RETHROW_FALSE(num_functions == functions.size());
					code.reserve(num_functions);
					while (num_functions--)
					{
						uint32_t body_size;
						WASM_READ_OML(body_size);
						SOUP_IF_UNLIKELY (body_size == 0)
						{
							return false;
						}
						std::string body;
						r.str(body_size, body);
#if DEBUG_LOAD
						std::cout << "- " << string::bin2hex(body) << "\n";
#endif
#if SOUP_WASM_PEDANTIC
						{
							MemoryRefReader body_reader(body);
							SOUP_RETHROW_FALSE(validateFunctionBody(body_reader));
						}
#endif
						code.emplace_back(std::move(body));
					}
				}
				break;

			case 11: // Data
				{
					uint32_t num_segments;
					WASM_READ_OML(num_segments);
#if DEBUG_LOAD
					std::cout << num_segments << " data segment(s)\n";
#endif
#if SOUP_WASM_PEDANTIC
					if (data_count != -1)
					{
						SOUP_RETHROW_FALSE(data_count == num_segments);
					}
#endif
					for (uint32_t i = 0; i != num_segments; ++i)
					{
						uint32_t flags;
						WASM_READ_OML(flags);
#if DEBUG_LOAD
						std::cout << "data flags: " << flags << "\n";
#endif
						if (flags & 1)
						{
							size_t size;
							WASM_READ_OML(size);
							std::string data;
							r.str(size, data);
							passive_data_segments.emplace(i, std::move(data));
						}
						else
						{
							SOUP_RETHROW_FALSE(memory);

							if (flags & 0b10)
							{
								size_t memidx; WASM_READ_OML(memidx);
							}

							WasmValue base;
							SOUP_RETHROW_FALSE(readConstant(r, base));
							{
								WasmType addr_type = WASM_I32;
#if SOUP_WASM_MEMORY64
								if (memory->memory64)
								{
									addr_type = WASM_I64;
								}
#endif
								SOUP_IF_UNLIKELY (base.type != addr_type)
								{
#if DEBUG_LOAD
									std::cout << "unexpected type for data initialisation: " << string::hex(static_cast<uint8_t>(base.type)) << "\n";
#endif
									return false;
								}
							}
							size_t size; WASM_READ_OML(size);
							auto ptr = memory->getView(base.uptr(), size);
							SOUP_IF_UNLIKELY (!ptr)
							{
#if DEBUG_LOAD
								std::cout << "data segment exceeds memory range: " << base.uptr() << " + " << size << " > " << memory.size << "\n";
#endif
								return false;
							}
							r.raw(ptr, size);
						}
					}
				}
				break;
			}
			if (section_size == 0)
			{
				WASM_READ_OML(section_size);
#if DEBUG_LOAD
				std::cout << "FIXUP section_size=" << section_size << "\n";
#endif
			}
		}
		return true;
	}

	bool WasmScript::validateFunctionBody(Reader& r) noexcept
	{
		size_t local_decl_count;
		WASM_READ_OML(local_decl_count);
		while (local_decl_count--)
		{
			size_t type_count;
			WASM_READ_OML(type_count);
			r.skip(1); // type
		}

		WasmVm::skipOverBranch(r, 0x8000'0000, *this, -1);
		const auto pos_after_branching = r.getPosition();
		r.seekEnd();
		SOUP_IF_UNLIKELY (r.getPosition() != pos_after_branching)
		{
#if DEBUG_LOAD
			std::cout << "load(pedantic): skipOverBranch bailed early\n";
#endif
			return false;
		}

		r.seek(r.getPosition() - 1);
		SOUP_IF_UNLIKELY (uint8_t byte; !r.u8(byte) || byte != 0x0b)
		{
#if DEBUG_LOAD
			std::cout << "load(pedantic): function body does not end on 'end' opcode\n";
#endif
			return false;
		}

		return true;
	}

	bool WasmScript::hasUnresolvedImports() const noexcept
	{
		for (const auto& fi : function_imports)
		{
			if (!(fi.ptr || fi.source))
			{
				return true;
			}
		}
		for (uint32_t i = 0; i != global_imports.size(); ++i)
		{
			if (!globals[i])
			{
				return true;
			}
		}
		return false;
	}

	void WasmScript::provideImportedFunction(const std::string& module_name, const std::string& function_name, wasm_ffi_func_t ptr) noexcept
	{
		for (auto& fi : function_imports)
		{
			if (fi.function_name == function_name
				&& fi.module_name == module_name
				)
			{
				fi.ptr = ptr;
				// Function may be imported multiple times so not breaking
			}
		}
	}

	void WasmScript::provideImportedFunctions(const std::string& module_name, const std::unordered_map<std::string, wasm_ffi_func_t>& map) noexcept
	{
		for (auto& fi : function_imports)
		{
			if (fi.module_name == module_name)
			{
				if (auto e = map.find(fi.function_name); e != map.end())
				{
					fi.ptr = e->second;
				}
			}
		}
	}

	void WasmScript::provideImportedGlobal(const std::string& module_name, const std::string& field_name, SharedPtr<WasmValue> value) noexcept
	{
		for (size_t i = 0; i != global_imports.size(); ++i)
		{
			const auto& gi = global_imports[i];
			if (gi.field_name == field_name
				&& gi.module_name == module_name
				)
			{
				globals[i] = value;
			}
		}
	}

	void WasmScript::provideImportedGlobals(const std::string& module_name, const std::unordered_map<std::string, SharedPtr<WasmValue>>& map) noexcept
	{
		for (size_t i = 0; i != global_imports.size(); ++i)
		{
			const auto& gi = global_imports[i];
			if (gi.module_name == module_name)
			{
				if (auto e = map.find(gi.field_name); e != map.end())
				{
					globals[i] = e->second;
				}
			}
		}
	}

	void WasmScript::importFromModule(const std::string& module_name, SharedPtr<WasmScript> other) noexcept
	{
		for (auto& fi : function_imports)
		{
			if (fi.module_name == module_name)
			{
				if (auto e = other->export_map.find(fi.function_name); e != other->export_map.end())
				{
					if (e->second.kind == Export::kFunction)
					{
						fi.source = other;
						fi.func_index = e->second.index;
					}
				}
			}
		}
		for (size_t i = 0; i != global_imports.size(); ++i)
		{
			const auto& gi = global_imports[i];
			if (gi.module_name == module_name)
			{
				if (auto e = other->export_map.find(gi.field_name); e != other->export_map.end())
				{
					if (e->second.kind == Export::kGlobal)
					{
						if (auto value = other->getGlobalByIndex(e->second.index))
						{
							globals[i] = value;
						}
					}
				}
			}
		}
	}

	bool WasmScript::instantiate()
	{
		if (start_func_idx != -1)
		{
			return this->call(start_func_idx);
		}
		return true;
	}

	const std::string* WasmScript::getExportedFuntion(const std::string& name, const WasmFunctionType** optOutType) const noexcept
	{
		if (auto e = export_map.find(name); e != export_map.end() && e->second.kind == Export::kFunction)
		{
			const size_t i = (e->second.index - function_imports.size());
			if (i < code.size()
#if false // already checked at load
				&& i < functions.size()
#endif
				)
			{
				if (optOutType)
				{
					const auto type_index = functions[i];
#if false // already checked at load
					SOUP_IF_UNLIKELY (type_index >= types.size())
					{
						return nullptr;
					}
#endif
					*optOutType = &types[type_index];
				}
				return &code[i];
			}
		}
		return nullptr;
	}

	uint32_t WasmScript::getExportedFuntion2(const std::string& name) const noexcept
	{
		if (auto e = export_map.find(name); e != export_map.end() && e->second.kind == Export::kFunction)
		{
			return e->second.index;
		}
		return -1;
	}

	// guaranteed to return an in-bounds type index if the function index is in-bounds (due to range checks at load)
	uint32_t WasmScript::getTypeIndexForFunction(uint32_t func_index) const noexcept
	{
		if (func_index < function_imports.size())
		{
			return function_imports[func_index].type_index;
		}
		func_index -= static_cast<uint32_t>(function_imports.size());
		if (func_index < functions.size())
		{
			return functions[func_index];
		}
		return -1;
	}

	WasmValue* WasmScript::getGlobalByIndex(uint32_t global_index) noexcept
	{
		if (global_index < globals.size())
		{
			return globals[global_index].get();
		}
		return nullptr;
	}

	WasmValue* WasmScript::getExportedGlobal(const std::string& name) noexcept
	{
		if (auto e = export_map.find(name); e != export_map.end() && e->second.kind == Export::kGlobal)
		{
			return getGlobalByIndex(e->second.index);
		}
		return nullptr;
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
		SOUP_IF_UNLIKELY (!memory)
		{
			return;
		}

		{
			WasiData& wd = custom_data.getStructFromMap(WasiData);
			wd.args = std::move(args);
		}

		// Resources:
		// - Barebones "Hello World": https://github.com/bytecodealliance/wasmtime/blob/main/docs/WASI-tutorial.md#web-assembly-text-example
		// - How the XCC compiler uses WASI:
		//   - https://github.com/tyfkda/xcc/blob/main/libsrc/_wasm/wasi.h
		//   - https://github.com/tyfkda/xcc/blob/main/libsrc/_wasm/crt0/_start.c#L41

		provideImportedFunction("wasi_snapshot_preview1", "args_sizes_get", [](WasmVm& vm, uint32_t func_index, const WasmFunctionType&)
		{
			API_CHECK_STACK(2);
			auto plen = vm.stack.back().i32; vm.stack.pop_back();
			auto pargc = vm.stack.back().i32; vm.stack.pop_back();
			WasiData& wd = vm.script.custom_data.getStructFromMapConst(WasiData);
			if (auto pLen = vm.script.memory->getPointer<int32_t>(plen))
			{
				*pLen = 0;
				for (const auto& arg : wd.args)
				{
					*pLen += arg.size() + 1;
				}
			}
			if (auto pArgc = vm.script.memory->getPointer<int32_t>(pargc))
			{
				*pArgc = wd.args.size();
			}
			vm.stack.emplace_back(WASI_ERRNO_SUCCESS);
		});
		provideImportedFunction("wasi_snapshot_preview1", "args_get", [](WasmVm& vm, uint32_t func_index, const WasmFunctionType&)
		{
			API_CHECK_STACK(2);
			auto pstr = vm.stack.back().i32; vm.stack.pop_back();
			auto pargv = vm.stack.back().i32; vm.stack.pop_back();
			WasiData& wd = vm.script.custom_data.getStructFromMapConst(WasiData);
			std::string argstr;
			for (uint32_t i = 0; i != wd.args.size(); ++i)
			{
				if (auto pArg = vm.script.memory->getPointer<int32_t>(pargv + i * 4))
				{
					*pArg = pstr + argstr.size();
				}
				argstr.append(wd.args[i].data(), wd.args[i].size() + 1);
			}
			vm.script.memory->write(pstr, argstr.data(), argstr.size());
			vm.stack.emplace_back(WASI_ERRNO_SUCCESS);
		});
		provideImportedFunction("wasi_snapshot_preview1", "environ_sizes_get", [](WasmVm& vm, uint32_t func_index, const WasmFunctionType&)
		{
			API_CHECK_STACK(2);
			auto out_environ_buf_size = vm.stack.back().i32; vm.stack.pop_back();
			auto out_environ_count = vm.stack.back().i32; vm.stack.pop_back();
			if (auto ptr = vm.script.memory->getPointer<int32_t>(out_environ_count))
			{
				*ptr = 0;
			}
			if (auto ptr = vm.script.memory->getPointer<int32_t>(out_environ_buf_size))
			{
				*ptr = 0;
			}
			vm.stack.emplace_back(WASI_ERRNO_SUCCESS);
		});
		provideImportedFunction("wasi_snapshot_preview1", "proc_exit", [](WasmVm& vm, uint32_t func_index, const WasmFunctionType&)
		{
			API_CHECK_STACK(1);
			auto code = vm.stack.back().i32; vm.stack.pop_back();
			exit(code);
		});
		provideImportedFunction("wasi_snapshot_preview1", "fd_prestat_get", [](WasmVm& vm, uint32_t func_index, const WasmFunctionType&)
		{
			API_CHECK_STACK(2);
			auto prestat = vm.stack.back().i32; vm.stack.pop_back();
			auto fd = vm.stack.back().i32; vm.stack.pop_back();
#if DEBUG_API
			std::cout << "prestat on fd " << fd << "\n";
#endif
			if (fd == 3)
			{
				if (auto pTag = vm.script.memory->getPointer<uint32_t>(prestat + 0))
				{
					*pTag = 0; // __WASI_PREOPENTYPE_DIR
				}
				if (auto pDirNameLen = vm.script.memory->getPointer<uint32_t>(prestat + 4))
				{
					*pDirNameLen = 1;
				}
				vm.stack.emplace_back(WASI_ERRNO_SUCCESS);
			}
			else
			{
				vm.stack.emplace_back(WASI_ERRNO_BADF);
			}
		});
		provideImportedFunction("wasi_snapshot_preview1", "fd_prestat_dir_name", [](WasmVm& vm, uint32_t func_index, const WasmFunctionType&)
		{
			API_CHECK_STACK(3);
			auto path_len = vm.stack.back().i32; vm.stack.pop_back();
			auto path = vm.stack.back().i32; vm.stack.pop_back();
			auto fd = vm.stack.back().i32; vm.stack.pop_back();
			if (fd == 3)
			{
				if (path_len >= 1)
				{
					vm.script.memory->write(path, ".", 1);
					vm.stack.emplace_back(WASI_ERRNO_SUCCESS);
				}
				else
				{
					vm.stack.emplace_back(WASI_ERRNO_NAMETOOLONG);
				}
			}
			else
			{
				vm.stack.emplace_back(WASI_ERRNO_BADF);
			}
		});
		provideImportedFunction("wasi_snapshot_preview1", "fd_filestat_get", [](WasmVm& vm, uint32_t func_index, const WasmFunctionType&)
		{
			API_CHECK_STACK(2);
			auto out = vm.stack.back().i32; vm.stack.pop_back();
			auto fd = vm.stack.back().i32; vm.stack.pop_back();
			SOUP_UNUSED(out);
			vm.stack.emplace_back(fd < 3 ? WASI_ERRNO_SUCCESS : WASI_ERRNO_BADF);
		});
		provideImportedFunction("wasi_snapshot_preview1", "fd_fdstat_get", [](WasmVm& vm, uint32_t func_index, const WasmFunctionType&)
		{
			API_CHECK_STACK(2);
			auto out = vm.stack.back().i32; vm.stack.pop_back(); // https://github.com/WebAssembly/wasi-libc/blob/d02bdc21afc4d835383b006c11e285c4a7c78439/libc-bottom-half/headers/public/wasi/wasip1.h#L945
			auto fd = vm.stack.back().i32; vm.stack.pop_back();
#if DEBUG_API
			std::cout << "fdstat on fd " << fd << "\n";
#endif
			if (fd == 3)
			{
				if (auto pFiletype = vm.script.memory->getPointer<uint8_t>(out + 0))
				{
					*pFiletype = 3; // directory, as per https://github.com/WebAssembly/wasi-libc/blob/d02bdc21afc4d835383b006c11e285c4a7c78439/libc-bottom-half/headers/public/wasi/wasip1.h#L785
				}
				if (auto pFlags = vm.script.memory->getPointer<uint16_t>(out + 2))
				{
					*pFlags = 0;
				}
				if (auto pRightsBase = vm.script.memory->getPointer<uint64_t>(out + 8))
				{
					*pRightsBase = -1;
				}
				if (auto pRightsInheriting = vm.script.memory->getPointer<uint64_t>(out + 16))
				{
					*pRightsInheriting = -1;
				}
				vm.stack.emplace_back(WASI_ERRNO_SUCCESS);
			}
			else
			{
				vm.stack.emplace_back(WASI_ERRNO_BADF);
			}
		});
		provideImportedFunction("wasi_snapshot_preview1", "path_filestat_get", [](WasmVm& vm, uint32_t func_index, const WasmFunctionType&)
		{
			API_CHECK_STACK(5);
			auto buf = vm.stack.back().i32; vm.stack.pop_back(); // https://github.com/WebAssembly/wasi-libc/blob/d02bdc21afc4d835383b006c11e285c4a7c78439/libc-bottom-half/headers/public/wasi/wasip1.h#L1064
			auto path_len = vm.stack.back().i32; vm.stack.pop_back();
			auto path = vm.stack.back().i32; vm.stack.pop_back();
			auto flags = vm.stack.back().i32; vm.stack.pop_back();
			auto fd = vm.stack.back().i32; vm.stack.pop_back();
			if (fd == 3)
			{
				auto path_str = vm.script.memory->readString(path, path_len);
#if DEBUG_API
				std::cout << "path_filestat_get: " << path_str << " (relative to .)\n";
#endif
				SOUP_UNUSED(flags);
				if (auto pFiletype = vm.script.memory->getPointer<uint8_t>(buf + 16))
				{
					*pFiletype = 4; // regular file, as per https://github.com/WebAssembly/wasi-libc/blob/d02bdc21afc4d835383b006c11e285c4a7c78439/libc-bottom-half/headers/public/wasi/wasip1.h#L785
				}
				if (auto pSize = vm.script.memory->getPointer<uint64_t>(buf + 32))
				{
					*pSize = std::filesystem::file_size(path_str);
#if DEBUG_API
					std::cout << "path_filestat_get: size=" << *pSize << "\n";
#endif
				}
				vm.stack.emplace_back(WASI_ERRNO_SUCCESS);
			}
			else
			{
				vm.stack.emplace_back(WASI_ERRNO_BADF);
			}
		});
		provideImportedFunction("wasi_snapshot_preview1", "path_open", [](WasmVm& vm, uint32_t func_index, const WasmFunctionType&)
		{
			API_CHECK_STACK(8);
			auto out_fd = vm.stack.back().i32; vm.stack.pop_back();
			auto fdflags = vm.stack.back().i32; vm.stack.pop_back();
			auto fs_rights_inheriting = vm.stack.back().i64; vm.stack.pop_back();
			auto fs_rights_base = vm.stack.back().i64; vm.stack.pop_back();
			auto oflags = vm.stack.back().i32; vm.stack.pop_back();
			auto path_len = vm.stack.back().i32; vm.stack.pop_back();
			auto path = vm.stack.back().i32; vm.stack.pop_back();
			auto dirflags = vm.stack.back().i32; vm.stack.pop_back();
			auto fd = vm.stack.back().i32; vm.stack.pop_back();
			if (fd == 3)
			{
				auto path_str = vm.script.memory->readString(path, path_len);
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
					if (auto pOutFd = vm.script.memory->getPointer<uint32_t>(out_fd))
					{
						*pOutFd = WASI_FD_FILES_BASE + wd.files.size();
					}
					wd.files.emplace_back(f);
					vm.stack.emplace_back(WASI_ERRNO_SUCCESS);
				}
				else
				{
					vm.stack.emplace_back(WASI_ERRNO_NOENT);
				}
			}
			else
			{
				vm.stack.emplace_back(WASI_ERRNO_BADF);
			}
		});
		provideImportedFunction("wasi_snapshot_preview1", "fd_seek", [](WasmVm& vm, uint32_t func_index, const WasmFunctionType&)
		{
			API_CHECK_STACK(4);
			auto out_off = vm.stack.back().i32; vm.stack.pop_back();
			auto whence = vm.stack.back().i32; vm.stack.pop_back();
			auto delta = vm.stack.back().i64; vm.stack.pop_back();
			auto fd = vm.stack.back().i32; vm.stack.pop_back();
#if DEBUG_API
			std::cout << "fd_seek: fd=" << fd << ", delta=" << delta << ", whence=" << whence << "\n";
#endif
			WasiData& wd = vm.script.custom_data.getStructFromMapConst(WasiData);
			if (fd >= WASI_FD_FILES_BASE && fd - WASI_FD_FILES_BASE < wd.files.size())
			{
				auto f = wd.files[fd - WASI_FD_FILES_BASE];
				fseek(f, delta, whence == 0 ? SEEK_SET : (whence == 1 ? SEEK_CUR : (whence == 2 ? SEEK_END : (SOUP_UNREACHABLE,SEEK_END))));
				if (auto pOutOff = vm.script.memory->getPointer<uint64_t>(out_off))
				{
					*pOutOff = ftell(f);
#if DEBUG_API
					std::cout << "fd_seek: offset is now " << *pOutOff << "\n";
#endif
				}
				vm.stack.emplace_back(WASI_ERRNO_SUCCESS);
			}
			else
			{
				vm.stack.emplace_back(WASI_ERRNO_BADF);
			}
		});
		provideImportedFunction("wasi_snapshot_preview1", "fd_read", [](WasmVm& vm, uint32_t func_index, const WasmFunctionType&)
		{
			API_CHECK_STACK(4);
			auto out_nread = vm.stack.back().i32; vm.stack.pop_back();
			auto iovs_len = vm.stack.back().i32; vm.stack.pop_back();
			auto iovs = vm.stack.back().i32; vm.stack.pop_back();
			auto fd = vm.stack.back().i32; vm.stack.pop_back();
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
					if (auto ptr = vm.script.memory->getPointer<int32_t>(iovs))
					{
						iov_base = *ptr;
					}
					iovs += 4;
					int32_t iov_len = 0;
					if (auto ptr = vm.script.memory->getPointer<int32_t>(iovs))
					{
						iov_len = *ptr;
					}
					iovs += 4;
					if (auto ptr = vm.script.memory->getView(iov_base, iov_len))
					{
						const auto ret = fread(ptr, 1, iov_len, f);
						if (ret >= 0)
						{
							nread += ret;
						}
					}
				}
				if (auto pOut = vm.script.memory->getPointer<int32_t>(out_nread))
				{
#if DEBUG_API
					std::cout << "read " << nread << " bytes from fd " << fd << "\n";
#endif
					*pOut = nread;
				}
				vm.stack.emplace_back(WASI_ERRNO_SUCCESS);
			}
			else
			{
				vm.stack.emplace_back(WASI_ERRNO_BADF);
			}
		});
		provideImportedFunction("wasi_snapshot_preview1", "fd_write", [](WasmVm& vm, uint32_t func_index, const WasmFunctionType&)
		{
			API_CHECK_STACK(4);
			auto out_nwritten = vm.stack.back().i32; vm.stack.pop_back();
			auto iovs_len = vm.stack.back().i32; vm.stack.pop_back();
			auto iovs = vm.stack.back().i32; vm.stack.pop_back();
			auto fd = vm.stack.back().i32; vm.stack.pop_back();
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
					if (auto ptr = vm.script.memory->getPointer<int32_t>(iovs))
					{
						iov_base = *ptr;
					}
					iovs += 4;
					int32_t iov_len = 0;
					if (auto ptr = vm.script.memory->getPointer<int32_t>(iovs))
					{
						iov_len = *ptr;
					}
					iovs += 4;
					if (auto ptr = vm.script.memory->getView(iov_base, iov_len))
					{
						const auto ret = fwrite(ptr, 1, iov_len, f);
						if (ret >= 0)
						{
							nwritten += ret;
						}
					}
				}
				if (auto pOut = vm.script.memory->getPointer<int32_t>(out_nwritten))
				{
					*pOut = nwritten;
				}
				vm.stack.emplace_back(WASI_ERRNO_SUCCESS);
			}
			else
			{
				vm.stack.emplace_back(WASI_ERRNO_BADF);
			}
		});
		provideImportedFunction("wasi_snapshot_preview1", "fd_close", [](WasmVm& vm, uint32_t func_index, const WasmFunctionType&)
		{
			API_CHECK_STACK(1);
			auto fd = vm.stack.back().i32; vm.stack.pop_back();
#if DEBUG_API
			std::cout << "close fd " << fd << "\n";
#endif
			SOUP_UNUSED(fd);
			vm.stack.emplace_back(WASI_ERRNO_SUCCESS);
		});
	}

	bool WasmScript::call(uint32_t func_index, std::vector<WasmValue>&& args, std::vector<WasmValue>* out)
	{
		WasmScript* script = this;
	_call_other_script:
		if (func_index < script->function_imports.size())
		{
			const auto& imp = script->function_imports[func_index];
#if DEBUG_LOAD || DEBUG_API
			std::cout << "Calling into " << imp.module_name << ":" << imp.function_name << "\n";
#endif
			if (imp.ptr)
			{
#if false // already checked at load
				SOUP_IF_UNLIKELY (imp.type_index >= script->types.size())
				{
#if DEBUG_LOAD || DEBUG_API
					std::cout << "call: type is out-of-bounds\n";
#endif
				}
#endif
				WasmVm vm(*this);
				vm.stack = std::move(args);
				imp.ptr(vm, func_index, script->types[imp.type_index]);
				if (out)
				{
					*out = std::move(vm.stack);
				}
				return true;
			}
			SOUP_IF_UNLIKELY (!imp.source)
			{
#if DEBUG_LOAD || DEBUG_API
				std::cout << "call: unresolved function import\n";
#endif
				return false;
			}
			script = imp.source.get();
			func_index = imp.func_index;
			goto _call_other_script;
		}
		func_index -= script->function_imports.size();
		SOUP_IF_UNLIKELY (func_index >= script->code.size())
		{
#if DEBUG_LOAD || DEBUG_API
			std::cout << "call: function is out-of-bounds\n";
#endif
			return false;
		}
		WasmVm vm(*script);
		vm.locals = std::move(args);
		SOUP_IF_UNLIKELY (!vm.run(script->code[func_index], 0, func_index))
		{
#if DEBUG_LOAD || DEBUG_API
			std::cout << "call: execution failed\n";
#endif
			return false;
		}
		if (out)
		{
			*out = std::move(vm.stack);
		}
		return true;
	}

	// WasmVm

	bool WasmVm::run(const std::string& data, unsigned depth, uint32_t func_index)
	{
		MemoryRefReader r(data);
		return run(r, depth, func_index);
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

	bool WasmVm::run(Reader& r, unsigned depth, uint32_t func_index)
	{
		size_t local_decl_count;
		WASM_READ_OML(local_decl_count);
		while (local_decl_count--)
		{
			size_t type_count;
			WASM_READ_OML(type_count);
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
					int32_t result_type; WASM_READ_SOML(result_type);
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
					int32_t result_type; WASM_READ_SOML(result_type);
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
					int32_t result_type; WASM_READ_SOML(result_type);
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
					auto value = stack.back(); stack.pop_back();
					//std::cout << "if: condition is " << (value.i32 ? "true" : "false") << "\n";
					if (value.i32)
					{
						ctrlflow.emplace(CtrlFlowEntry{ (std::streamoff)-1, stack_size, num_values });
					}
					else
					{
						if (skipOverBranch(r, 0, script, func_index))
						{
							// we're in the 'else' branch
							ctrlflow.emplace(CtrlFlowEntry{ (std::streamoff)-1, stack_size, num_values });
						}
					}
				}
				break;

			case 0x05: // else
				//std::cout << "else: skipping over this branch\n";
				skipOverBranch(r, 0, script, func_index);
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
					WASM_READ_OML(depth);
					SOUP_IF_UNLIKELY (!doBranch(r, depth, func_index, ctrlflow))
					{
						return false;
					}
				}
				break;

			case 0x0d: // br_if
				{
					uint32_t depth;
					WASM_READ_OML(depth);
					WASM_CHECK_STACK(1);
					auto value = stack.back(); stack.pop_back();
					if (value.i32)
					{
						SOUP_IF_UNLIKELY (!doBranch(r, depth, func_index, ctrlflow))
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
					WASM_READ_OML(num_branches);
					table.reserve(num_branches);
					while (num_branches--)
					{
						uint32_t depth;
						WASM_READ_OML(depth);
						table.emplace_back(depth);
					}
					uint32_t depth;
					WASM_READ_OML(depth);
					WASM_CHECK_STACK(1);
					auto index = static_cast<uint32_t>(stack.back().i32); stack.pop_back();
					if (index < table.size())
					{
						depth = table.at(index);
					}
					SOUP_IF_UNLIKELY (!doBranch(r, depth, func_index, ctrlflow))
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
					WASM_READ_OML(function_index);
					uint32_t type_index = script.getTypeIndexForFunction(function_index);
					SOUP_RETHROW_FALSE(doCall(type_index, function_index, depth));
				}
				break;

			case 0x11: // call_indirect
				{
					uint32_t type_index; WASM_READ_OML(type_index);
					SOUP_IF_UNLIKELY (type_index >= script.types.size())
					{
#if DEBUG_VM
						std::cout << "call: type is out-of-bounds\n";
#endif
						return false;
					}
					uint32_t table_index; WASM_READ_OML(table_index);
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
					auto element_index = static_cast<uint32_t>(stack.back().i32); stack.pop_back();
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
				stack.pop_back();
				break;

			case 0x1c: // select t
				r.skip(2);
				[[fallthrough]];
			case 0x1b: // select
				{
					WASM_CHECK_STACK(3);
					auto cond = stack.back(); stack.pop_back();
					auto fvalue = stack.back(); stack.pop_back();
					auto tvalue = stack.back(); stack.pop_back();
					stack.emplace_back(cond.i32 ? tvalue : fvalue);
				}
				break;

			case 0x20: // local.get
				{
					uint32_t local_index;
					WASM_READ_OML(local_index);
					SOUP_IF_UNLIKELY (local_index >= locals.size())
					{
#if DEBUG_VM
						std::cout << "local.get: index is out-of-bounds: " << local_index << "\n";
#endif
						return false;
					}
					stack.emplace_back(locals.at(local_index));
				}
				break;

			case 0x21: // local.set
				{
					uint32_t local_index;
					WASM_READ_OML(local_index);
					SOUP_IF_UNLIKELY (local_index >= locals.size())
					{
#if DEBUG_VM
						std::cout << "local.set: index is out-of-bounds: " << local_index << "\n";
#endif
						return false;
					}
					WASM_CHECK_STACK(1);
					locals.at(local_index) = stack.back(); stack.pop_back();
				}
				break;

			case 0x22: // local.tee
				{
					uint32_t local_index;
					WASM_READ_OML(local_index);
					SOUP_IF_UNLIKELY (local_index >= locals.size())
					{
#if DEBUG_VM
						std::cout << "local.tee: index is out-of-bounds: " << local_index << "\n";
#endif
						return false;
					}
					WASM_CHECK_STACK(1);
					locals.at(local_index) = stack.back();
				}
				break;

			case 0x23: // global.get
				{
					uint32_t global_index;
					WASM_READ_OML(global_index);
					auto global = script.getGlobalByIndex(global_index);
					SOUP_IF_UNLIKELY (!global)
					{
#if DEBUG_VM
						std::cout << "global.get: invalid global index: " << global_index << "\n";
#endif
						return false;
					}
					stack.emplace_back(*global);
				}
				break;

			case 0x24: // global.set
				{
					uint32_t global_index;
					WASM_READ_OML(global_index);
					auto global = script.getGlobalByIndex(global_index);
					SOUP_IF_UNLIKELY (!global)
					{
#if DEBUG_VM
						std::cout << "global.set: invalid global index: " << global_index << "\n";
#endif
						return false;
					}
					WASM_CHECK_STACK(1);
					*global = stack.back(); stack.pop_back();
				}
				break;

			case 0x25: // table.get
				{
					uint32_t table_index;
					WASM_READ_OML(table_index);
					SOUP_IF_UNLIKELY (table_index >= script.tables.size())
					{
#if DEBUG_VM
						std::cout << "table.get: table index " << table_index << " >= " << script.tables.size() << "\n";
#endif
						return false;
					}
					WASM_CHECK_STACK(1);
					auto elem_index = stack.back().uptr(); stack.pop_back();
					const auto& table = script.tables[table_index];
					SOUP_IF_UNLIKELY (elem_index >= table.values.size())
					{
#if DEBUG_VM
						std::cout << "table.get: element index " << elem_index << " >= " << table.values.size() << "\n";
#endif
						return false;
					}
					stack.emplace_back(table.type).i64 = table.values[elem_index];
				}
				break;

			case 0x26: // table.set
				{
					uint32_t table_index;
					WASM_READ_OML(table_index);
					SOUP_IF_UNLIKELY (table_index >= script.tables.size())
					{
#if DEBUG_VM
						std::cout << "table.set: table index " << table_index << " >= " << script.tables.size() << "\n";
#endif
						return false;
					}
					WASM_CHECK_STACK(2);
					auto value = stack.back(); stack.pop_back();
					auto elem_index = stack.back().uptr(); stack.pop_back();
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
					auto base = stack.back(); stack.pop_back();
					r.skip(1); // memflags
					size_t offset; WASM_READ_OML(offset);
					SOUP_IF_UNLIKELY (!script.memory)
					{
#if DEBUG_VM
						std::cout << "memory access without a memory\n";
#endif
						return false;
					}
					SOUP_IF_LIKELY (auto ptr = script.memory->getPointer<int32_t>(base, offset))
					{
						stack.emplace_back(*ptr);
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
					auto base = stack.back(); stack.pop_back();
					r.skip(1); // memflags
					size_t offset; WASM_READ_OML(offset);
					SOUP_IF_UNLIKELY (!script.memory)
					{
#if DEBUG_VM
						std::cout << "memory access without a memory\n";
#endif
						return false;
					}
					SOUP_IF_LIKELY (auto ptr = script.memory->getPointer<int64_t>(base, offset))
					{
						stack.emplace_back(*ptr);
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
					auto base = stack.back(); stack.pop_back();
					r.skip(1); // memflags
					size_t offset; WASM_READ_OML(offset);
					SOUP_IF_UNLIKELY (!script.memory)
					{
#if DEBUG_VM
						std::cout << "memory access without a memory\n";
#endif
						return false;
					}
					SOUP_IF_LIKELY (auto ptr = script.memory->getPointer<float>(base, offset))
					{
						stack.emplace_back(*ptr);
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
					auto base = stack.back(); stack.pop_back();
					r.skip(1); // memflags
					size_t offset; WASM_READ_OML(offset);
					SOUP_IF_UNLIKELY (!script.memory)
					{
#if DEBUG_VM
						std::cout << "memory access without a memory\n";
#endif
						return false;
					}
					SOUP_IF_LIKELY (auto ptr = script.memory->getPointer<double>(base, offset))
					{
						stack.emplace_back(*ptr);
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
					auto base = stack.back(); stack.pop_back();
					r.skip(1); // memflags
					size_t offset; WASM_READ_OML(offset);
					SOUP_IF_UNLIKELY (!script.memory)
					{
#if DEBUG_VM
						std::cout << "memory access without a memory\n";
#endif
						return false;
					}
					SOUP_IF_LIKELY (auto ptr = script.memory->getPointer<int8_t>(base, offset))
					{
						stack.emplace_back(static_cast<int32_t>(*ptr));
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
					auto base = stack.back(); stack.pop_back();
					r.skip(1); // memflags
					size_t offset; WASM_READ_OML(offset);
					SOUP_IF_UNLIKELY (!script.memory)
					{
#if DEBUG_VM
						std::cout << "memory access without a memory\n";
#endif
						return false;
					}
					SOUP_IF_LIKELY (auto ptr = script.memory->getPointer<uint8_t>(base, offset))
					{
						stack.emplace_back(static_cast<uint32_t>(*ptr));
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
					auto base = stack.back(); stack.pop_back();
					r.skip(1); // memflags
					size_t offset; WASM_READ_OML(offset);
					SOUP_IF_UNLIKELY (!script.memory)
					{
#if DEBUG_VM
						std::cout << "memory access without a memory\n";
#endif
						return false;
					}
					SOUP_IF_LIKELY (auto ptr = script.memory->getPointer<int16_t>(base, offset))
					{
						stack.emplace_back(static_cast<uint32_t>(*ptr));
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
					auto base = stack.back(); stack.pop_back();
					r.skip(1); // memflags
					size_t offset; WASM_READ_OML(offset);
					SOUP_IF_UNLIKELY (!script.memory)
					{
#if DEBUG_VM
						std::cout << "memory access without a memory\n";
#endif
						return false;
					}
					SOUP_IF_LIKELY (auto ptr = script.memory->getPointer<uint16_t>(base, offset))
					{
						stack.emplace_back(static_cast<uint32_t>(*ptr));
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
					auto base = stack.back(); stack.pop_back();
					r.skip(1); // memflags
					size_t offset; WASM_READ_OML(offset);
					SOUP_IF_UNLIKELY (!script.memory)
					{
#if DEBUG_VM
						std::cout << "memory access without a memory\n";
#endif
						return false;
					}
					SOUP_IF_LIKELY (auto ptr = script.memory->getPointer<int8_t>(base, offset))
					{
						stack.emplace_back(static_cast<int64_t>(*ptr));
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
					auto base = stack.back(); stack.pop_back();
					r.skip(1); // memflags
					size_t offset; WASM_READ_OML(offset);
					SOUP_IF_UNLIKELY (!script.memory)
					{
#if DEBUG_VM
						std::cout << "memory access without a memory\n";
#endif
						return false;
					}
					SOUP_IF_LIKELY (auto ptr = script.memory->getPointer<uint8_t>(base, offset))
					{
						stack.emplace_back(static_cast<uint64_t>(*ptr));
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
					auto base = stack.back(); stack.pop_back();
					r.skip(1); // memflags
					size_t offset; WASM_READ_OML(offset);
					SOUP_IF_UNLIKELY (!script.memory)
					{
#if DEBUG_VM
						std::cout << "memory access without a memory\n";
#endif
						return false;
					}
					SOUP_IF_LIKELY (auto ptr = script.memory->getPointer<int16_t>(base, offset))
					{
						stack.emplace_back(static_cast<int64_t>(*ptr));
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
					auto base = stack.back(); stack.pop_back();
					r.skip(1); // memflags
					size_t offset; WASM_READ_OML(offset);
					SOUP_IF_UNLIKELY (!script.memory)
					{
#if DEBUG_VM
						std::cout << "memory access without a memory\n";
#endif
						return false;
					}
					SOUP_IF_LIKELY (auto ptr = script.memory->getPointer<uint16_t>(base, offset))
					{
						stack.emplace_back(static_cast<uint64_t>(*ptr));
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
					auto base = stack.back(); stack.pop_back();
					r.skip(1); // memflags
					size_t offset; WASM_READ_OML(offset);
					SOUP_IF_UNLIKELY (!script.memory)
					{
#if DEBUG_VM
						std::cout << "memory access without a memory\n";
#endif
						return false;
					}
					SOUP_IF_LIKELY (auto ptr = script.memory->getPointer<int32_t>(base, offset))
					{
						stack.emplace_back(static_cast<int64_t>(*ptr));
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
					auto base = stack.back(); stack.pop_back();
					r.skip(1); // memflags
					size_t offset; WASM_READ_OML(offset);
					SOUP_IF_UNLIKELY (!script.memory)
					{
#if DEBUG_VM
						std::cout << "memory access without a memory\n";
#endif
						return false;
					}
					SOUP_IF_LIKELY (auto ptr = script.memory->getPointer<uint32_t>(base, offset))
					{
						stack.emplace_back(static_cast<uint64_t>(*ptr));
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
					auto value = stack.back(); stack.pop_back();
					auto base = stack.back(); stack.pop_back();
					r.skip(1); // memflags
					size_t offset; WASM_READ_OML(offset);
					SOUP_IF_UNLIKELY (!script.memory)
					{
#if DEBUG_VM
						std::cout << "memory access without a memory\n";
#endif
						return false;
					}
					SOUP_IF_LIKELY (auto ptr = script.memory->getPointer<int32_t>(base, offset))
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
					auto value = stack.back(); stack.pop_back();
					auto base = stack.back(); stack.pop_back();
					r.skip(1); // memflags
					size_t offset; WASM_READ_OML(offset);
					SOUP_IF_UNLIKELY (!script.memory)
					{
#if DEBUG_VM
						std::cout << "memory access without a memory\n";
#endif
						return false;
					}
					SOUP_IF_LIKELY (auto ptr = script.memory->getPointer<int64_t>(base, offset))
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
					auto value = stack.back(); stack.pop_back();
					auto base = stack.back(); stack.pop_back();
					r.skip(1); // memflags
					size_t offset; WASM_READ_OML(offset);
					SOUP_IF_UNLIKELY (!script.memory)
					{
#if DEBUG_VM
						std::cout << "memory access without a memory\n";
#endif
						return false;
					}
					SOUP_IF_LIKELY (auto ptr = script.memory->getPointer<float>(base, offset))
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
					auto value = stack.back(); stack.pop_back();
					auto base = stack.back(); stack.pop_back();
					r.skip(1); // memflags
					size_t offset; WASM_READ_OML(offset);
					SOUP_IF_UNLIKELY (!script.memory)
					{
#if DEBUG_VM
						std::cout << "memory access without a memory\n";
#endif
						return false;
					}
					SOUP_IF_LIKELY (auto ptr = script.memory->getPointer<double>(base, offset))
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
					auto value = stack.back(); stack.pop_back();
					auto base = stack.back(); stack.pop_back();
					r.skip(1); // memflags
					size_t offset; WASM_READ_OML(offset);
					SOUP_IF_UNLIKELY (!script.memory)
					{
#if DEBUG_VM
						std::cout << "memory access without a memory\n";
#endif
						return false;
					}
					SOUP_IF_LIKELY (auto ptr = script.memory->getPointer<int8_t>(base, offset))
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
					auto value = stack.back(); stack.pop_back();
					auto base = stack.back(); stack.pop_back();
					r.skip(1); // memflags
					size_t offset; WASM_READ_OML(offset);
					SOUP_IF_UNLIKELY (!script.memory)
					{
#if DEBUG_VM
						std::cout << "memory access without a memory\n";
#endif
						return false;
					}
					SOUP_IF_LIKELY (auto ptr = script.memory->getPointer<int16_t>(base, offset))
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
					auto value = stack.back(); stack.pop_back();
					auto base = stack.back(); stack.pop_back();
					r.skip(1); // memflags
					size_t offset; WASM_READ_OML(offset);
					SOUP_IF_UNLIKELY (!script.memory)
					{
#if DEBUG_VM
						std::cout << "memory access without a memory\n";
#endif
						return false;
					}
					SOUP_IF_LIKELY (auto ptr = script.memory->getPointer<int8_t>(base, offset))
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
					auto value = stack.back(); stack.pop_back();
					auto base = stack.back(); stack.pop_back();
					r.skip(1); // memflags
					size_t offset; WASM_READ_OML(offset);
					SOUP_IF_UNLIKELY (!script.memory)
					{
#if DEBUG_VM
						std::cout << "memory access without a memory\n";
#endif
						return false;
					}
					SOUP_IF_LIKELY (auto ptr = script.memory->getPointer<int16_t>(base, offset))
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
					auto value = stack.back(); stack.pop_back();
					auto base = stack.back(); stack.pop_back();
					r.skip(1); // memflags
					size_t offset; WASM_READ_OML(offset);
					SOUP_IF_UNLIKELY (!script.memory)
					{
#if DEBUG_VM
						std::cout << "memory access without a memory\n";
#endif
						return false;
					}
					SOUP_IF_LIKELY (auto ptr = script.memory->getPointer<int32_t>(base, offset))
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
					SOUP_IF_UNLIKELY (!script.memory)
					{
#if DEBUG_VM
						std::cout << "memory.size: no memory\n";
#endif
						return false;
					}
					script.memory->encodeUPTR(stack.emplace_back(), script.memory->size / 0x10'000);
				}
				break;

			case 0x40: // memory.grow
				{
					r.skip(1); // reserved
					SOUP_IF_UNLIKELY (!script.memory)
					{
#if DEBUG_VM
						std::cout << "memory.grow: no memory\n";
#endif
						return false;
					}
					WASM_CHECK_STACK(1);
					const auto old_size_pages = script.memory->grow(stack.back().uptr());
					script.memory->encodeUPTR(stack.back(), old_size_pages);
				}
				break;

			case 0x41: // i32.const
				{
					int32_t value;
					WASM_READ_SOML(value);
					stack.emplace_back(value);
				}
				break;

			case 0x42: // i64.const
				{
					int64_t value;
					WASM_READ_SOML(value);
					stack.emplace_back(value);
				}
				break;

			case 0x43: // f32.const
				{
					float value;
					r.f32(value);
					stack.emplace_back(value);
				}
				break;

			case 0x44: // f64.const
				{
					double value;
					r.f64(value);
					stack.emplace_back(value);
				}
				break;

			case 0x45: // i32.eqz
				{
					WASM_CHECK_STACK(1);
					auto value = stack.back(); stack.pop_back();
					stack.emplace_back(value.i32 == 0);
				}
				break;

			case 0x46: // i32.eq
				{
					WASM_CHECK_STACK(2);
					auto b = stack.back(); stack.pop_back();
					auto a = stack.back(); stack.pop_back();
					stack.emplace_back(a.i32 == b.i32);
				}
				break;

			case 0x47: // i32.ne
				{
					WASM_CHECK_STACK(2);
					auto b = stack.back(); stack.pop_back();
					auto a = stack.back(); stack.pop_back();
					stack.emplace_back(a.i32 != b.i32);
				}
				break;

			case 0x48: // i32.lt_s
				{
					WASM_CHECK_STACK(2);
					auto b = stack.back(); stack.pop_back();
					auto a = stack.back(); stack.pop_back();
					stack.emplace_back(a.i32 < b.i32);
				}
				break;

			case 0x49: // i32.lt_u
				{
					WASM_CHECK_STACK(2);
					auto b = stack.back(); stack.pop_back();
					auto a = stack.back(); stack.pop_back();
					stack.emplace_back(static_cast<uint32_t>(a.i32) < static_cast<uint32_t>(b.i32));
				}
				break;

			case 0x4a: // i32.gt_s
				{
					WASM_CHECK_STACK(2);
					auto b = stack.back(); stack.pop_back();
					auto a = stack.back(); stack.pop_back();
					stack.emplace_back(a.i32 > b.i32);
				}
				break;

			case 0x4b: // i32.gt_u
				{
					WASM_CHECK_STACK(2);
					auto b = stack.back(); stack.pop_back();
					auto a = stack.back(); stack.pop_back();
					stack.emplace_back(static_cast<uint32_t>(a.i32) > static_cast<uint32_t>(b.i32));
				}
				break;

			case 0x4c: // i32.le_s
				{
					WASM_CHECK_STACK(2);
					auto b = stack.back(); stack.pop_back();
					auto a = stack.back(); stack.pop_back();
					stack.emplace_back(a.i32 <= b.i32);
				}
				break;

			case 0x4d: // i32.le_u
				{
					WASM_CHECK_STACK(2);
					auto b = stack.back(); stack.pop_back();
					auto a = stack.back(); stack.pop_back();
					stack.emplace_back(static_cast<uint32_t>(a.i32) <= static_cast<uint32_t>(b.i32));
				}
				break;

			case 0x4e: // i32.ge_s
				{
					WASM_CHECK_STACK(2);
					auto b = stack.back(); stack.pop_back();
					auto a = stack.back(); stack.pop_back();
					stack.emplace_back(a.i32 >= b.i32);
				}
				break;

			case 0x4f: // i32.ge_u
				{
					WASM_CHECK_STACK(2);
					auto b = stack.back(); stack.pop_back();
					auto a = stack.back(); stack.pop_back();
					stack.emplace_back(static_cast<uint32_t>(a.i32) >= static_cast<uint32_t>(b.i32));
				}
				break;

			case 0x50: // i64.eqz
				{
					WASM_CHECK_STACK(1);
					auto value = stack.back(); stack.pop_back();
					stack.emplace_back(value.i64 == 0);
				}
				break;

			case 0x51: // i64.eq
				{
					WASM_CHECK_STACK(2);
					auto b = stack.back(); stack.pop_back();
					auto a = stack.back(); stack.pop_back();
					stack.emplace_back(a.i64 == b.i64);
				}
				break;

			case 0x52: // i64.ne
				{
					WASM_CHECK_STACK(2);
					auto b = stack.back(); stack.pop_back();
					auto a = stack.back(); stack.pop_back();
					stack.emplace_back(a.i64 != b.i64);
				}
				break;

			case 0x53: // i64.lt_s
				{
					WASM_CHECK_STACK(2);
					auto b = stack.back(); stack.pop_back();
					auto a = stack.back(); stack.pop_back();
					stack.emplace_back(a.i64 < b.i64);
				}
				break;

			case 0x54: // i64.lt_u
				{
					WASM_CHECK_STACK(2);
					auto b = stack.back(); stack.pop_back();
					auto a = stack.back(); stack.pop_back();
					stack.emplace_back(static_cast<uint64_t>(a.i64) < static_cast<uint64_t>(b.i64));
				}
				break;

			case 0x55: // i64.gt_s
				{
					WASM_CHECK_STACK(2);
					auto b = stack.back(); stack.pop_back();
					auto a = stack.back(); stack.pop_back();
					stack.emplace_back(a.i64 > b.i64);
				}
				break;

			case 0x56: // i64.gt_u
				{
					WASM_CHECK_STACK(2);
					auto b = stack.back(); stack.pop_back();
					auto a = stack.back(); stack.pop_back();
					stack.emplace_back(static_cast<uint64_t>(a.i64) > static_cast<uint64_t>(b.i64));
				}
				break;

			case 0x57: // i64.le_s
				{
					WASM_CHECK_STACK(2);
					auto b = stack.back(); stack.pop_back();
					auto a = stack.back(); stack.pop_back();
					stack.emplace_back(a.i64 <= b.i64);
				}
				break;

			case 0x58: // i64.le_u
				{
					WASM_CHECK_STACK(2);
					auto b = stack.back(); stack.pop_back();
					auto a = stack.back(); stack.pop_back();
					stack.emplace_back(static_cast<uint64_t>(a.i64) <= static_cast<uint64_t>(b.i64));
				}
				break;

			case 0x59: // i64.ge_s
				{
					WASM_CHECK_STACK(2);
					auto b = stack.back(); stack.pop_back();
					auto a = stack.back(); stack.pop_back();
					stack.emplace_back(a.i64 >= b.i64);
				}
				break;

			case 0x5a: // i64.ge_u
				{
					WASM_CHECK_STACK(2);
					auto b = stack.back(); stack.pop_back();
					auto a = stack.back(); stack.pop_back();
					stack.emplace_back(static_cast<uint64_t>(a.i64) >= static_cast<uint64_t>(b.i64));
				}
				break;

			case 0x5b: // f32.eq
				{
					WASM_CHECK_STACK(2);
					auto b = stack.back(); stack.pop_back();
					auto a = stack.back(); stack.pop_back();
					stack.emplace_back(a.f32 == b.f32);
				}
				break;

			case 0x5c: // f32.ne
				{
					WASM_CHECK_STACK(2);
					auto b = stack.back(); stack.pop_back();
					auto a = stack.back(); stack.pop_back();
					stack.emplace_back(a.f32 != b.f32);
				}
				break;

			case 0x5d: // f32.lt
				{
					WASM_CHECK_STACK(2);
					auto b = stack.back(); stack.pop_back();
					auto a = stack.back(); stack.pop_back();
					stack.emplace_back(a.f32 < b.f32);
				}
				break;

			case 0x5e: // f32.gt
				{
					WASM_CHECK_STACK(2);
					auto b = stack.back(); stack.pop_back();
					auto a = stack.back(); stack.pop_back();
					stack.emplace_back(a.f32 > b.f32);
				}
				break;

			case 0x5f: // f32.le
				{
					WASM_CHECK_STACK(2);
					auto b = stack.back(); stack.pop_back();
					auto a = stack.back(); stack.pop_back();
					stack.emplace_back(a.f32 <= b.f32);
				}
				break;

			case 0x60: // f32.ge
				{
					WASM_CHECK_STACK(2);
					auto b = stack.back(); stack.pop_back();
					auto a = stack.back(); stack.pop_back();
					stack.emplace_back(a.f32 >= b.f32);
				}
				break;

			case 0x61: // f64.eq
				{
					WASM_CHECK_STACK(2);
					auto b = stack.back(); stack.pop_back();
					auto a = stack.back(); stack.pop_back();
					stack.emplace_back(a.f64 == b.f64);
				}
				break;

			case 0x62: // f64.ne
				{
					WASM_CHECK_STACK(2);
					auto b = stack.back(); stack.pop_back();
					auto a = stack.back(); stack.pop_back();
					stack.emplace_back(a.f64 != b.f64);
				}
				break;

			case 0x63: // f64.lt
				{
					WASM_CHECK_STACK(2);
					auto b = stack.back(); stack.pop_back();
					auto a = stack.back(); stack.pop_back();
					stack.emplace_back(a.f64 < b.f64);
				}
				break;

			case 0x64: // f64.gt
				{
					WASM_CHECK_STACK(2);
					auto b = stack.back(); stack.pop_back();
					auto a = stack.back(); stack.pop_back();
					stack.emplace_back(a.f64 > b.f64);
				}
				break;

			case 0x65: // f64.le
				{
					WASM_CHECK_STACK(2);
					auto b = stack.back(); stack.pop_back();
					auto a = stack.back(); stack.pop_back();
					stack.emplace_back(a.f64 <= b.f64);
				}
				break;

			case 0x66: // f64.ge
				{
					WASM_CHECK_STACK(2);
					auto b = stack.back(); stack.pop_back();
					auto a = stack.back(); stack.pop_back();
					stack.emplace_back(a.f64 >= b.f64);
				}
				break;

			case 0x67: // i32.clz
				WASM_CHECK_STACK(1);
				stack.back().i32 = bitutil::getNumLeadingZeros(static_cast<uint32_t>(stack.back().i32));
				break;

			case 0x68: // i32.ctz
				WASM_CHECK_STACK(1);
				stack.back().i32 = bitutil::getNumTrailingZeros(static_cast<uint32_t>(stack.back().i32));
				break;

			case 0x69: // i32.popcnt
				WASM_CHECK_STACK(1);
				stack.back().i32 = bitutil::getNumSetBits(static_cast<uint32_t>(stack.back().i32));
				break;

			case 0x6a: // i32.add
				{
					WASM_CHECK_STACK(2);
					auto b = stack.back(); stack.pop_back();
					auto a = stack.back(); stack.pop_back();
					stack.emplace_back(a.i32 + b.i32);
				}
				break;

			case 0x6b: // i32.sub
				{
					WASM_CHECK_STACK(2);
					auto b = stack.back(); stack.pop_back();
					auto a = stack.back(); stack.pop_back();
					stack.emplace_back(a.i32 - b.i32);
				}
				break;

			case 0x6c: // i32.mul
				{
					WASM_CHECK_STACK(2);
					auto b = stack.back(); stack.pop_back();
					auto a = stack.back(); stack.pop_back();
					stack.emplace_back(a.i32 * b.i32);
				}
				break;

			case 0x6d: // i32.div_s
				{
					WASM_CHECK_STACK(2);
					auto b = stack.back(); stack.pop_back();
					auto a = stack.back(); stack.pop_back();
					SOUP_IF_UNLIKELY (b.i32 == 0 || (a.i32 == INT32_MIN && b.i32 == -1))
					{
						return false;
					}
					stack.emplace_back(a.i32 / b.i32);
				}
				break;

			case 0x6e: // i32.div_u
				{
					WASM_CHECK_STACK(2);
					auto b = stack.back(); stack.pop_back();
					auto a = stack.back(); stack.pop_back();
					SOUP_IF_UNLIKELY (b.i32 == 0)
					{
						return false;
					}
					stack.emplace_back(static_cast<uint32_t>(a.i32) / static_cast<uint32_t>(b.i32));
				}
				break;

			case 0x6f: // i32.rem_s
				{
					WASM_CHECK_STACK(2);
					auto b = stack.back(); stack.pop_back();
					auto a = stack.back(); stack.pop_back();
					SOUP_IF_UNLIKELY (b.i32 == 0)
					{
						return false;
					}
					if (a.i32 == INT32_MIN && b.i32 == -1)
					{
						stack.emplace_back(0);
					}
					else
					{
						stack.emplace_back(a.i32 % b.i32);
					}
				}
				break;

			case 0x70: // i32.rem_u
				{
					WASM_CHECK_STACK(2);
					auto b = stack.back(); stack.pop_back();
					auto a = stack.back(); stack.pop_back();
					SOUP_IF_UNLIKELY (b.i32 == 0)
					{
						return false;
					}
					stack.emplace_back(static_cast<uint32_t>(a.i32) % static_cast<uint32_t>(b.i32));
				}
				break;

			case 0x71: // i32.and
				{
					WASM_CHECK_STACK(2);
					auto b = stack.back(); stack.pop_back();
					auto a = stack.back(); stack.pop_back();
					stack.emplace_back(a.i32 & b.i32);
				}
				break;

			case 0x72: // i32.or
				{
					WASM_CHECK_STACK(2);
					auto b = stack.back(); stack.pop_back();
					auto a = stack.back(); stack.pop_back();
					stack.emplace_back(a.i32 | b.i32);
				}
				break;

			case 0x73: // i32.xor
				{
					WASM_CHECK_STACK(2);
					auto b = stack.back(); stack.pop_back();
					auto a = stack.back(); stack.pop_back();
					stack.emplace_back(a.i32 ^ b.i32);
				}
				break;

			case 0x74: // i32.shl
				{
					WASM_CHECK_STACK(2);
					auto b = stack.back(); stack.pop_back();
					auto a = stack.back(); stack.pop_back();
					stack.emplace_back(a.i32 << (static_cast<uint32_t>(b.i32) % (sizeof(uint32_t) * 8)));
				}
				break;

			case 0x75: // i32.shr_s ("arithmetic right shift")
				{
					WASM_CHECK_STACK(2);
					auto b = stack.back(); stack.pop_back();
					auto a = stack.back(); stack.pop_back();
					stack.emplace_back(a.i32 >> (static_cast<uint32_t>(b.i32) % (sizeof(uint32_t) * 8)));
				}
				break;

			case 0x76: // i32.shr_u ("logical right shift")
				{
					WASM_CHECK_STACK(2);
					auto b = stack.back(); stack.pop_back();
					auto a = stack.back(); stack.pop_back();
					stack.emplace_back(static_cast<uint32_t>(a.i32) >> (static_cast<uint32_t>(b.i32) % (sizeof(uint32_t) * 8)));
				}
				break;

			case 0x77: // i32.rotl
				{
					WASM_CHECK_STACK(2);
					auto b = stack.back(); stack.pop_back();
					auto a = stack.back(); stack.pop_back();
					stack.emplace_back(soup::rotl<uint32_t>(a.i32, b.i32));
				}
				break;

			case 0x78: // i32.rotr
				{
					WASM_CHECK_STACK(2);
					auto b = stack.back(); stack.pop_back();
					auto a = stack.back(); stack.pop_back();
					stack.emplace_back(soup::rotr<uint32_t>(a.i32, b.i32));
				}
				break;

			case 0x79: // i64.clz
				WASM_CHECK_STACK(1);
				stack.back().i64 = bitutil::getNumLeadingZeros(static_cast<uint64_t>(stack.back().i64));
				break;

			case 0x7a: // i64.ctz
				WASM_CHECK_STACK(1);
				stack.back().i64 = bitutil::getNumTrailingZeros(static_cast<uint64_t>(stack.back().i64));
				break;

			case 0x7b: // i64.popcnt
				WASM_CHECK_STACK(1);
				stack.back().i64 = bitutil::getNumSetBits(static_cast<uint64_t>(stack.back().i64));
				break;

			case 0x7c: // i64.add
				{
					WASM_CHECK_STACK(2);
					auto b = stack.back(); stack.pop_back();
					auto a = stack.back(); stack.pop_back();
					stack.emplace_back(a.i64 + b.i64);
				}
				break;

			case 0x7d: // i64.sub
				{
					WASM_CHECK_STACK(2);
					auto b = stack.back(); stack.pop_back();
					auto a = stack.back(); stack.pop_back();
					stack.emplace_back(a.i64 - b.i64);
				}
				break;

			case 0x7e: // i64.mul
				{
					WASM_CHECK_STACK(2);
					auto b = stack.back(); stack.pop_back();
					auto a = stack.back(); stack.pop_back();
					stack.emplace_back(a.i64 * b.i64);
				}
				break;

			case 0x7f: // i64.div_s
				{
					WASM_CHECK_STACK(2);
					auto b = stack.back(); stack.pop_back();
					auto a = stack.back(); stack.pop_back();
					SOUP_IF_UNLIKELY (b.i64 == 0 || (a.i64 == INT64_MIN && b.i64 == -1))
					{
						return false;
					}
					stack.emplace_back(a.i64 / b.i64);
				}
				break;

			case 0x80: // i64.div_u
				{
					WASM_CHECK_STACK(2);
					auto b = stack.back(); stack.pop_back();
					auto a = stack.back(); stack.pop_back();
					SOUP_IF_UNLIKELY (b.i64 == 0)
					{
						return false;
					}
					stack.emplace_back(static_cast<uint64_t>(a.i64) / static_cast<uint64_t>(b.i64));
				}
				break;

			case 0x81: // i64.rem_s
				{
					WASM_CHECK_STACK(2);
					auto b = stack.back(); stack.pop_back();
					auto a = stack.back(); stack.pop_back();
					SOUP_IF_UNLIKELY (b.i32 == 0)
					{
						return false;
					}
					if (a.i64 == INT64_MIN && b.i64 == -1)
					{
						stack.emplace_back(static_cast<int64_t>(0));
					}
					else
					{
						stack.emplace_back(a.i64 % b.i64);
					}
				}
				break;

			case 0x82: // i64.rem_u
				{
					WASM_CHECK_STACK(2);
					auto b = stack.back(); stack.pop_back();
					auto a = stack.back(); stack.pop_back();
					SOUP_IF_UNLIKELY (b.i32 == 0)
					{
						return false;
					}
					stack.emplace_back(static_cast<uint64_t>(a.i64) % static_cast<uint64_t>(b.i64));
				}
				break;

			case 0x83: // i64.and
				{
					WASM_CHECK_STACK(2);
					auto b = stack.back(); stack.pop_back();
					auto a = stack.back(); stack.pop_back();
					stack.emplace_back(a.i64 & b.i64);
				}
				break;

			case 0x84: // i64.or
				{
					WASM_CHECK_STACK(2);
					auto b = stack.back(); stack.pop_back();
					auto a = stack.back(); stack.pop_back();
					stack.emplace_back(a.i64 | b.i64);
				}
				break;

			case 0x85: // i64.xor
				{
					WASM_CHECK_STACK(2);
					auto b = stack.back(); stack.pop_back();
					auto a = stack.back(); stack.pop_back();
					stack.emplace_back(a.i64 ^ b.i64);
				}
				break;

			case 0x86: // i64.shl
				{
					WASM_CHECK_STACK(2);
					auto b = stack.back(); stack.pop_back();
					auto a = stack.back(); stack.pop_back();
					stack.emplace_back(a.i64 << (static_cast<uint64_t>(b.i64) % (sizeof(uint64_t) * 8)));
				}
				break;

			case 0x87: // i64.shr_s ("arithmetic right shift")
				{
					WASM_CHECK_STACK(2);
					auto b = stack.back(); stack.pop_back();
					auto a = stack.back(); stack.pop_back();
					stack.emplace_back(a.i64 >> (static_cast<uint64_t>(b.i64) % (sizeof(uint64_t) * 8)));
				}
				break;

			case 0x88: // i64.shr_u ("logical right shift")
				{
					WASM_CHECK_STACK(2);
					auto b = stack.back(); stack.pop_back();
					auto a = stack.back(); stack.pop_back();
					stack.emplace_back(static_cast<uint64_t>(a.i64) >> (static_cast<uint64_t>(b.i64) % (sizeof(uint64_t) * 8)));
				}
				break;

			case 0x89: // i64.rotl
				{
					WASM_CHECK_STACK(2);
					auto b = stack.back(); stack.pop_back();
					auto a = stack.back(); stack.pop_back();
					stack.emplace_back(soup::rotl<uint64_t>(a.i64, static_cast<int>(b.i64)));
				}
				break;

			case 0x8a: // i64.rotr
				{
					WASM_CHECK_STACK(2);
					auto b = stack.back(); stack.pop_back();
					auto a = stack.back(); stack.pop_back();
					stack.emplace_back(soup::rotr<uint64_t>(a.i64, static_cast<int>(b.i64)));
				}
				break;

			case 0x8b: // f32.abs
				WASM_CHECK_STACK(1);
				stack.back().f32 = std::abs(stack.back().f32);
				break;

			case 0x8c: // f32.neg
				WASM_CHECK_STACK(1);
				stack.back().f32 = stack.back().f32 * -1.0f;
				break;

			case 0x8d: // f32.ceil
				WASM_CHECK_STACK(1);
				stack.back().f32 = std::ceil(stack.back().f32);
				break;

			case 0x8e: // f32.floor
				WASM_CHECK_STACK(1);
				stack.back().f32 = std::floor(stack.back().f32);
				break;

			case 0x8f: // f32.trunc
				WASM_CHECK_STACK(1);
				stack.back().f32 = std::trunc(stack.back().f32);
				break;

			case 0x90: // f32.nearest
				WASM_CHECK_STACK(1);
				stack.back().f32 = std::nearbyint(stack.back().f32);
				break;

			case 0x91: // f32.sqrt
				WASM_CHECK_STACK(1);
				stack.back().f32 = std::sqrt(stack.back().f32);
				break;

			case 0x92: // f32.add
				WASM_CHECK_STACK(2);
				{
					auto b = stack.back(); stack.pop_back();
					auto a = stack.back(); stack.pop_back();
					stack.emplace_back(a.f32 + b.f32);
				}
				break;

			case 0x93: // f32.sub
				WASM_CHECK_STACK(2);
				{
					auto b = stack.back(); stack.pop_back();
					auto a = stack.back(); stack.pop_back();
					stack.emplace_back(a.f32 - b.f32);
				}
				break;

			case 0x94: // f32.mul
				WASM_CHECK_STACK(2);
				{
					auto b = stack.back(); stack.pop_back();
					auto a = stack.back(); stack.pop_back();
					stack.emplace_back(a.f32 * b.f32);
				}
				break;

			case 0x95: // f32.div
				WASM_CHECK_STACK(2);
				{
					auto b = stack.back(); stack.pop_back();
					auto a = stack.back(); stack.pop_back();
					stack.emplace_back(a.f32 / b.f32);
				}
				break;

			case 0x96: // f32.min
				WASM_CHECK_STACK(2);
				{
					auto b = stack.back(); stack.pop_back();
					auto a = stack.back(); stack.pop_back();
					if (std::isnan(a.f32) || std::isnan(b.f32))
					{
						stack.emplace_back(std::numeric_limits<float>::quiet_NaN());
					}
					else if (a.f32 < b.f32 || (std::signbit(a.f32) && !std::signbit(b.f32)))
					{
						stack.emplace_back(a.f32);
					}
					else
					{
						stack.emplace_back(b.f32);
					}
				}
				break;

			case 0x97: // f32.max
				WASM_CHECK_STACK(2);
				{
					auto b = stack.back(); stack.pop_back();
					auto a = stack.back(); stack.pop_back();
					if (std::isnan(a.f32) || std::isnan(b.f32))
					{
						stack.emplace_back(std::numeric_limits<float>::quiet_NaN());
					}
					else if (a.f32 < b.f32 || (std::signbit(a.f32) && !std::signbit(b.f32)))
					{
						stack.emplace_back(b.f32);
					}
					else
					{
						stack.emplace_back(a.f32);
					}
				}
				break;

			case 0x98: // f32.copysign
				WASM_CHECK_STACK(2);
				{
					auto b = stack.back(); stack.pop_back();
					auto a = stack.back(); stack.pop_back();
					stack.emplace_back(std::copysign(a.f32, b.f32));
				}
				break;

			case 0x99: // f64.abs
				WASM_CHECK_STACK(1);
				stack.back().f64 = std::abs(stack.back().f64);
				break;

			case 0x9a: // f64.neg
				WASM_CHECK_STACK(1);
				stack.back().f64 = stack.back().f64 * -1.0;
				break;

			case 0x9b: // f64.ceil
				WASM_CHECK_STACK(1);
				stack.back().f64 = std::ceil(stack.back().f64);
				break;

			case 0x9c: // f64.floor
				WASM_CHECK_STACK(1);
				stack.back().f64 = std::floor(stack.back().f64);
				break;

			case 0x9d: // f64.trunc
				WASM_CHECK_STACK(1);
				stack.back().f64 = std::trunc(stack.back().f64);
				break;

			case 0x9e: // f64.nearest
				WASM_CHECK_STACK(1);
				stack.back().f64 = std::nearbyint(stack.back().f64);
				break;

			case 0x9f: // f64.sqrt
				WASM_CHECK_STACK(1);
				stack.back().f64 = std::sqrt(stack.back().f64);
				break;

			case 0xa0: // f64.add
				WASM_CHECK_STACK(2);
				{
					auto b = stack.back(); stack.pop_back();
					auto a = stack.back(); stack.pop_back();
					stack.emplace_back(a.f64 + b.f64);
				}
				break;

			case 0xa1: // f64.sub
				WASM_CHECK_STACK(2);
				{
					auto b = stack.back(); stack.pop_back();
					auto a = stack.back(); stack.pop_back();
					stack.emplace_back(a.f64 - b.f64);
				}
				break;

			case 0xa2: // f64.mul
				WASM_CHECK_STACK(2);
				{
					auto b = stack.back(); stack.pop_back();
					auto a = stack.back(); stack.pop_back();
					stack.emplace_back(a.f64 * b.f64);
				}
				break;

			case 0xa3: // f64.div
				WASM_CHECK_STACK(2);
				{
					auto b = stack.back(); stack.pop_back();
					auto a = stack.back(); stack.pop_back();
					stack.emplace_back(a.f64 / b.f64);
				}
				break;

			case 0xa4: // f64.min
				WASM_CHECK_STACK(2);
				{
					auto b = stack.back(); stack.pop_back();
					auto a = stack.back(); stack.pop_back();
					if (std::isnan(a.f64) || std::isnan(b.f64))
					{
						stack.emplace_back(std::numeric_limits<double>::quiet_NaN());
					}
					else if (a.f64 < b.f64 || (std::signbit(a.f64) && !std::signbit(b.f64)))
					{
						stack.emplace_back(a.f64);
					}
					else
					{
						stack.emplace_back(b.f64);
					}
				}
				break;

			case 0xa5: // f64.max
				WASM_CHECK_STACK(2);
				{
					auto b = stack.back(); stack.pop_back();
					auto a = stack.back(); stack.pop_back();
					if (std::isnan(a.f64) || std::isnan(b.f64))
					{
						stack.emplace_back(std::numeric_limits<double>::quiet_NaN());
					}
					else if (a.f64 < b.f64 || (std::signbit(a.f64) && !std::signbit(b.f64)))
					{
						stack.emplace_back(b.f64);
					}
					else
					{
						stack.emplace_back(a.f64);
					}
				}
				break;

			case 0xa6: // f64.copysign
				WASM_CHECK_STACK(2);
				{
					auto b = stack.back(); stack.pop_back();
					auto a = stack.back(); stack.pop_back();
					stack.emplace_back(std::copysign(a.f64, b.f64));
				}
				break;

			case 0xa7: // i32.wrap_i64
				WASM_CHECK_STACK(1);
				stack.back() = static_cast<int32_t>(stack.back().i64);
				break;

			case 0xa8: // i32.trunc_f32_s
				WASM_CHECK_STACK(1);
				SOUP_IF_UNLIKELY (std::isnan(stack.back().f32) || stack.back().f32 < F32_I32_MIN || stack.back().f32 > F32_I32_MAX)
				{
#if DEBUG_VM
					std::cout << "float cannot be represented as int\n";
#endif
					return false;
				}
				stack.back() = static_cast<int32_t>(stack.back().f32);
				break;

			case 0xa9: // i32.trunc_f32_u
				WASM_CHECK_STACK(1);
				SOUP_IF_UNLIKELY (std::isnan(stack.back().f32) || stack.back().f32 < F32_U32_MIN || stack.back().f32 > F32_U32_MAX)
				{
#if DEBUG_VM
					std::cout << "float cannot be represented as int\n";
#endif
					return false;
				}
				stack.back() = static_cast<uint32_t>(stack.back().f32);
				break;

			case 0xaa: // i32.trunc_f64_s
				WASM_CHECK_STACK(1);
				SOUP_IF_UNLIKELY (std::isnan(stack.back().f64) || stack.back().f64 < F64_I32_MIN || stack.back().f64 > F64_I32_MAX)
				{
#if DEBUG_VM
					std::cout << "float cannot be represented as int\n";
#endif
					return false;
				}
				stack.back() = static_cast<int32_t>(stack.back().f64);
				break;

			case 0xab: // i32.trunc_f64_u
				WASM_CHECK_STACK(1);
				SOUP_IF_UNLIKELY (std::isnan(stack.back().f64) || stack.back().f64 < F64_U32_MIN || stack.back().f64 > F64_U32_MAX)
				{
#if DEBUG_VM
					std::cout << "invalid value for i32.trunc_f64_u\n";
#endif
					return false;
				}
				stack.back() = static_cast<uint32_t>(stack.back().f64);
				break;

			case 0xac: // i64.extend_i32_s
				WASM_CHECK_STACK(1);
				stack.back() = static_cast<int64_t>(stack.back().i32);
				break;

			case 0xad: // i64.extend_i32_u
				WASM_CHECK_STACK(1);
				stack.back() = static_cast<int64_t>(static_cast<uint64_t>(static_cast<uint32_t>(stack.back().i32)));
				break;

			case 0xae: // i64.trunc_f32_s
				WASM_CHECK_STACK(1);
				SOUP_IF_UNLIKELY (std::isnan(stack.back().f32) || stack.back().f32 < F32_I64_MIN || stack.back().f32 > F32_I64_MAX)
				{
#if DEBUG_VM
					std::cout << "float cannot be represented as int\n";
#endif
					return false;
				}
				stack.back() = static_cast<int64_t>(stack.back().f32);
				break;

			case 0xaf: // i64.trunc_f32_u
				WASM_CHECK_STACK(1);
				SOUP_IF_UNLIKELY (std::isnan(stack.back().f32) || stack.back().f32 < F32_U64_MIN || stack.back().f32 > F32_U64_MAX)
				{
#if DEBUG_VM
					std::cout << "float cannot be represented as int\n";
#endif
					return false;
				}
				stack.back() = static_cast<uint64_t>(stack.back().f32);
				break;

			case 0xb0: // i64.trunc_f64_s
				WASM_CHECK_STACK(1);
				SOUP_IF_UNLIKELY (std::isnan(stack.back().f64) || stack.back().f64 < F64_I64_MIN || stack.back().f64 > F64_I64_MAX)
				{
#if DEBUG_VM
					std::cout << "float cannot be represented as int\n";
#endif
					return false;
				}
				stack.back() = static_cast<int64_t>(stack.back().f64);
				break;

			case 0xb1: // i64.trunc_f64_u
				WASM_CHECK_STACK(1);
				SOUP_IF_UNLIKELY (std::isnan(stack.back().f64) || stack.back().f64 < F64_U64_MIN || stack.back().f64 > F64_U64_MAX)
				{
#if DEBUG_VM
					std::cout << "float cannot be represented as int\n";
#endif
					return false;
				}
				stack.back() = static_cast<uint64_t>(stack.back().f64);
				break;

			case 0xb2: // f32.convert_i32_s
				WASM_CHECK_STACK(1);
				stack.back() = static_cast<float>(stack.back().i32);
				break;

			case 0xb3: // f32.convert_i32_u
				WASM_CHECK_STACK(1);
				stack.back() = static_cast<float>(static_cast<uint32_t>(stack.back().i32));
				break;

			case 0xb4: // f32.convert_i64_s
				WASM_CHECK_STACK(1);
				stack.back() = static_cast<float>(stack.back().i64);
				break;

			case 0xb5: // f32.convert_i64_u
				WASM_CHECK_STACK(1);
				stack.back() = static_cast<float>(static_cast<uint64_t>(stack.back().i64));
				break;

			case 0xb6: // f32.demote_f64
				WASM_CHECK_STACK(1);
				stack.back() = static_cast<float>(stack.back().f64);
				break;

			case 0xb7: // f64.convert_i32_s
				WASM_CHECK_STACK(1);
				stack.back() = static_cast<double>(stack.back().i32);
				break;

			case 0xb8: // f64.convert_i32_u
				WASM_CHECK_STACK(1);
				stack.back() = static_cast<double>(static_cast<uint32_t>(stack.back().i32));
				break;

			case 0xb9: // f64.convert_i64_s
				WASM_CHECK_STACK(1);
				stack.back() = static_cast<double>(stack.back().i64);
				break;

			case 0xba: // f64.convert_i64_u
				WASM_CHECK_STACK(1);
				stack.back() = static_cast<double>(static_cast<uint64_t>(stack.back().i64));
				break;

			case 0xbb: // f64.promote_f32
				WASM_CHECK_STACK(1);
				stack.back() = static_cast<double>(stack.back().f32);
				break;

			case 0xbc: // i32.reinterpret_f32
				WASM_CHECK_STACK(1);
				stack.back().type = WASM_I32;
				break;

			case 0xbd: // i64.reinterpret_f64
				WASM_CHECK_STACK(1);
				stack.back().type = WASM_I64;
				break;

			case 0xbe: // f32.reinterpret_i32
				WASM_CHECK_STACK(1);
				stack.back().type = WASM_F32;
				break;

			case 0xbf: // f64.reinterpret_i64
				WASM_CHECK_STACK(1);
				stack.back().type = WASM_F64;
				break;

			case 0xc0: // i32.extend8_s
				WASM_CHECK_STACK(1);
				stack.back().i32 = static_cast<int32_t>(static_cast<int8_t>(stack.back().i32));
				break;

			case 0xc1: // i32.extend16_s
				WASM_CHECK_STACK(1);
				stack.back().i32 = static_cast<int32_t>(static_cast<int16_t>(stack.back().i32));
				break;

			case 0xc2: // i64.extend8_s
				WASM_CHECK_STACK(1);
				stack.back().i64 = static_cast<int64_t>(static_cast<int8_t>(stack.back().i64));
				break;

			case 0xc3: // i64.extend16_s
				WASM_CHECK_STACK(1);
				stack.back().i64 = static_cast<int64_t>(static_cast<int16_t>(stack.back().i64));
				break;

			case 0xc4: // i64.extend32_s
				WASM_CHECK_STACK(1);
				stack.back().i64 = static_cast<int64_t>(static_cast<int32_t>(stack.back().i64));
				break;

			case 0xd0: // ref.null
				{
					uint8_t type;
					r.u8(type);
					stack.emplace_back(static_cast<WasmType>(type));
				}
				break;

			case 0xd1: // ref.is_null
				WASM_CHECK_STACK(1);
				stack.emplace_back(static_cast<int32_t>(stack.back().i64 == 0));
				break;

			case 0xd2: // ref.func
				{
					uint32_t idx;
					WASM_READ_OML(idx);
					stack.emplace_back(WASM_FUNCREF);
					stack.back().i64 = 0x1'0000'0000 | idx;
				}
				break;

			case 0xfc:
				r.u8(op);
				switch (op)
				{
				case 0x00: // i32.trunc_sat_f32_s
					WASM_CHECK_STACK(1);
					if (std::isnan(stack.back().f32))
					{
						stack.back() = 0;
					}
					else if (stack.back().f32 < F32_I32_MIN)
					{
						stack.back() = INT32_MIN;
					}
					else if (stack.back().f32 > F32_I32_MAX)
					{
						stack.back() = INT32_MAX;
					}
					else
					{
						stack.back() = static_cast<int32_t>(stack.back().f32);
					}
					break;

				case 0x01: // i32.trunc_sat_f32_u
					WASM_CHECK_STACK(1);
					if (std::isnan(stack.back().f32))
					{
						stack.back() = 0;
					}
					else if (stack.back().f32 < F32_U32_MIN)
					{
						stack.back() = 0;
					}
					else if (stack.back().f32 > F32_U32_MAX)
					{
						stack.back() = UINT32_MAX;
					}
					else
					{
						stack.back() = static_cast<uint32_t>(stack.back().f32);
					}
					break;

				case 0x02: // i32.trunc_sat_f64_s
					WASM_CHECK_STACK(1);
					if (std::isnan(stack.back().f64))
					{
						stack.back() = 0;
					}
					else if (stack.back().f64 < F64_I32_MIN)
					{
						stack.back() = INT32_MIN;
					}
					else if (stack.back().f64 > F64_I32_MAX)
					{
						stack.back() = INT32_MAX;
					}
					else
					{
						stack.back() = static_cast<int32_t>(stack.back().f64);
					}
					break;

				case 0x03: // i32.trunc_sat_f64_u
					WASM_CHECK_STACK(1);
					if (std::isnan(stack.back().f64))
					{
						stack.back() = 0;
					}
					else if (stack.back().f64 < F64_U32_MIN)
					{
						stack.back() = 0;
					}
					else if (stack.back().f64 > F64_U32_MAX)
					{
						stack.back() = UINT32_MAX;
					}
					else
					{
						stack.back() = static_cast<uint32_t>(stack.back().f64);
					}
					break;

				case 0x04: // i64.trunc_sat_f32_s
					WASM_CHECK_STACK(1);
					if (std::isnan(stack.back().f32))
					{
						stack.back() = static_cast<int64_t>(0);
					}
					else if (stack.back().f32 < F32_I64_MIN)
					{
						stack.back() = INT64_MIN;
					}
					else if (stack.back().f32 > F32_I64_MAX)
					{
						stack.back() = INT64_MAX;
					}
					else
					{
						stack.back() = static_cast<int64_t>(stack.back().f32);
					}
					break;

				case 0x05: // i64.trunc_sat_f32_u
					WASM_CHECK_STACK(1);
					if (std::isnan(stack.back().f32))
					{
						stack.back() = static_cast<int64_t>(0);
					}
					else if (stack.back().f32 < F32_U64_MIN)
					{
						stack.back() = static_cast<int64_t>(0);
					}
					else if (stack.back().f32 > F32_U64_MAX)
					{
						stack.back() = UINT64_MAX;
					}
					else
					{
						stack.back() = static_cast<uint64_t>(stack.back().f32);
					}
					break;

				case 0x06: // i64.trunc_sat_f64_s
					WASM_CHECK_STACK(1);
					if (std::isnan(stack.back().f64))
					{
						stack.back() = static_cast<int64_t>(0);
					}
					else if (stack.back().f64 < F64_I64_MIN)
					{
						stack.back() = INT64_MIN;
					}
					else if (stack.back().f64 > F64_I64_MAX)
					{
						stack.back() = INT64_MAX;
					}
					else
					{
						stack.back() = static_cast<int64_t>(stack.back().f64);
					}
					break;

				case 0x07: // i64.trunc_sat_f64_u
					WASM_CHECK_STACK(1);
					if (std::isnan(stack.back().f64))
					{
						stack.back() = static_cast<int64_t>(0);
					}
					else if (stack.back().f64 < F64_U64_MIN)
					{
						stack.back() = static_cast<int64_t>(0);
					}
					else if (stack.back().f64 > F64_U64_MAX)
					{
						stack.back() = UINT64_MAX;
					}
					else
					{
						stack.back() = static_cast<uint64_t>(stack.back().f64);
					}
					break;

				case 0x08: // memory.init
					{
						uint32_t segment_index;
						WASM_READ_OML(segment_index);
						r.skip(1); // memory index
						SOUP_IF_UNLIKELY (!script.memory)
						{
#if DEBUG_VM
							std::cout << "memory.init: no memory\n";
#endif
							return false;
						}
						WASM_CHECK_STACK(3);
						auto size = stack.back().uptr(); stack.pop_back();
						auto src_offset = stack.back().uptr(); stack.pop_back();
						auto dst_offset = stack.back().uptr(); stack.pop_back();
						auto e = script.passive_data_segments.find(segment_index);
						SOUP_IF_UNLIKELY (e == script.passive_data_segments.end() && size != 0)
						{
#if DEBUG_VM
							std::cout << "memory.init: invalid segment index\n";
#endif
							return false;
						}
						std::string scrap;
						const auto& data = e == script.passive_data_segments.end() ? scrap : e->second;
						auto dst_ptr = script.memory->getView(dst_offset, size);
						SOUP_IF_UNLIKELY (!dst_ptr || !can_add_without_overflow(src_offset, size) || src_offset + size > data.size())
						{
#if DEBUG_VM
							std::cout << "out-of-bounds memory.init\n";
#endif
							return false;
						}
						memcpy(dst_ptr, data.data() + src_offset, size);
					}
					break;

				case 0x09: // data.drop
					{
						uint32_t segment_index;
						WASM_READ_OML(segment_index);
						if (auto e = script.passive_data_segments.find(segment_index); e != script.passive_data_segments.end())
						{
							e->second.clear();
							e->second.shrink_to_fit();
						}
					}
					break;

				case 0x0a: // memory.copy
					{
						r.skip(2); // reserved
						SOUP_IF_UNLIKELY (!script.memory)
						{
#if DEBUG_VM
							std::cout << "memory.copy: no memory\n";
#endif
							return false;
						}
						WASM_CHECK_STACK(3);
						auto size = stack.back().uptr(); stack.pop_back();
						auto src = stack.back().uptr(); stack.pop_back();
						auto dst = stack.back().uptr(); stack.pop_back();
						auto dst_ptr = script.memory->getView(dst, size);
						auto src_ptr = script.memory->getView(src, size);
						SOUP_IF_UNLIKELY (!dst_ptr || !src_ptr)
						{
#if DEBUG_VM
							std::cout << "out-of-bounds memory.copy\n";
#endif
							return false;
						}
						memcpy(dst_ptr, src_ptr, size);
					}
					break;

				case 0x0b: // memory.fill
					{
						r.skip(1); // reserved
						SOUP_IF_UNLIKELY (!script.memory)
						{
#if DEBUG_VM
							std::cout << "memory.fill: no memory\n";
#endif
							return false;
						}
						WASM_CHECK_STACK(3);
						auto size = stack.back().uptr(); stack.pop_back();
						auto value = stack.back().i32; stack.pop_back();
						auto addr = stack.back().uptr(); stack.pop_back();
						auto ptr = script.memory->getView(addr, size);
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

				case 0x0c: // table.init
					{
						uint32_t segment_index;
						WASM_READ_OML(segment_index);
						uint32_t table_index;
						WASM_READ_OML(table_index);
						SOUP_IF_UNLIKELY (table_index >= script.tables.size())
						{
#if DEBUG_VM
							std::cout << "table.init: table index " << table_index << " >= " << script.tables.size() << "\n";
#endif
							return false;
						}
						auto& table = script.tables[table_index];
						WASM_CHECK_STACK(3);
						auto size = stack.back().uptr(); stack.pop_back();
						auto src_offset = stack.back().uptr(); stack.pop_back();
						auto dst_offset = stack.back().uptr(); stack.pop_back();
						auto e = script.passive_elem_segments.find(segment_index);
						SOUP_IF_UNLIKELY (e == script.passive_elem_segments.end() && size != 0)
						{
#if DEBUG_VM
							std::cout << "table.init: invalid segment index\n";
#endif
							return false;
						}
						WasmScript::Table scrap(table.type);
						SOUP_RETHROW_FALSE(table.copy(e == script.passive_elem_segments.end() ? scrap : e->second, dst_offset, src_offset, size));
					}
					break;

				case 0x0d: // elem.drop
					{
						uint32_t segment_index;
						WASM_READ_OML(segment_index);
						if (auto e = script.passive_elem_segments.find(segment_index); e != script.passive_elem_segments.end())
						{
							e->second.values.clear();
							e->second.values.shrink_to_fit();
						}
					}
					break;

				case 0x0e: // table.copy
					{
						uint32_t dst_table_index;
						WASM_READ_OML(dst_table_index);
						uint32_t src_table_index;
						WASM_READ_OML(src_table_index);
						SOUP_IF_UNLIKELY (dst_table_index >= script.tables.size() || src_table_index >= script.tables.size())
						{
#if DEBUG_VM
							std::cout << "table.copy: out-of bounds table index\n";
#endif
							return false;
						}
						auto& dst_table = script.tables[dst_table_index];
						const auto& src_table = script.tables[src_table_index];
						WASM_CHECK_STACK(3);
						auto size = stack.back().uptr(); stack.pop_back();
						auto src_offset = stack.back().uptr(); stack.pop_back();
						auto dst_offset = stack.back().uptr(); stack.pop_back();
						SOUP_RETHROW_FALSE(dst_table.copy(src_table, dst_offset, src_offset, size));
					}
					break;

				case 0x0f: // table.grow
					{
						uint32_t table_index;
						WASM_READ_OML(table_index);
						SOUP_IF_UNLIKELY (table_index >= script.tables.size())
						{
#if DEBUG_VM
							std::cout << "table.grow: table index " << table_index << " >= " << script.tables.size() << "\n";
#endif
							return false;
						}
						auto& table = script.tables[table_index];
						WASM_CHECK_STACK(2);
						auto delta = stack.back().i32; stack.pop_back();
						auto& value = stack.back();
						SOUP_IF_UNLIKELY (value.type != table.type)
						{
#if DEBUG_VM
							std::cout << "table.grow: attempt to assign " << wasm_type_to_string(value.type) << " to a table of " << wasm_type_to_string(table.type) << "\n";
#endif
							return false;
						}
						const auto old_size = table.grow(delta, value.i64);
#if SOUP_WASM_MEMORY64
						if (table.table64)
						{
							stack.back() = static_cast<uint64_t>(old_size);
						}
						else
#endif
						{
							stack.back() = static_cast<uint32_t>(old_size);
						}
				}
					break;

				case 0x10: // table.size
					{
						uint32_t table_index;
						WASM_READ_OML(table_index);
						SOUP_IF_UNLIKELY (table_index >= script.tables.size())
						{
#if DEBUG_VM
							std::cout << "table.size: table index " << table_index << " >= " << script.tables.size() << "\n";
#endif
							return false;
						}
						const auto& table = script.tables[table_index];
#if SOUP_WASM_MEMORY64
						if (table.table64)
						{
							stack.emplace_back(static_cast<uint64_t>(table.values.size()));
						}
						else
#endif
						{
							stack.emplace_back(static_cast<uint32_t>(table.values.size()));
						}
					}
					break;

				case 0x11: // table.fill
					{
						uint32_t table_index;
						WASM_READ_OML(table_index);
						SOUP_IF_UNLIKELY (table_index >= script.tables.size())
						{
#if DEBUG_VM
							std::cout << "table.fill: table index " << table_index << " >= " << script.tables.size() << "\n";
#endif
							return false;
						}
						auto& table = script.tables[table_index];
						WASM_CHECK_STACK(3);
						auto& size = stack[stack.size() - 1].i32;
						auto& value = stack[stack.size() - 2];
						auto& offset = stack[stack.size() - 3].i32;
						SOUP_IF_UNLIKELY (value.type != table.type)
						{
#if DEBUG_VM
							std::cout << "table.fill: attempt to assign " << wasm_type_to_string(value.type) << " to a table of " << wasm_type_to_string(table.type) << "\n";
#endif
							return false;
						}
						SOUP_IF_UNLIKELY (offset + size > table.values.size())
						{
#if DEBUG_VM
							std::cout << "out-of-bounds table.fill\n";
#endif
							return false;
						}
						while (size--)
						{
							table.values[offset++] = value.i64;
						}
						stack.erase(stack.end() - 3, stack.end());
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

	bool WasmVm::skipOverBranch(Reader& r, uint32_t target_depth, WasmScript& script, uint32_t func_index) SOUP_EXCAL
	{
		std::vector<uint32_t> scrap;
		std::vector<uint32_t>* hints = &scrap;
		uint64_t hint_key = 0;
		int32_t depth = 0;
		if (func_index != -1)
		{
			hint_key = (static_cast<uint64_t>(func_index) << 32) | (r.getPosition() & 0xffff'ffff);
			if (auto e = script._internal_branch_hints.find(hint_key); e != script._internal_branch_hints.end())
			{
				hints = &e->second;
				if (target_depth < hints->size())
				{
					depth = target_depth;
					r.seek(e->second[target_depth]);
#if DEBUG_BRANCHING
					std::cout << "skipOverBranch: straight hit: " << hint_key << " + depth " << depth << " -> " << e->second[target_depth] << "\n";
#endif
				}
				else
				{
					depth = hints->size() - 1;
					r.seek(hints->back());
#if DEBUG_BRANCHING
					std::cout << "skipOverBranch: partial hit: " << hint_key << " -> " << e->second[target_depth] << " (depth " << depth << "/" << target_depth << ")\n";
#endif
				}
			}
			else
			{
				hints = &script._internal_branch_hints.emplace(hint_key, std::vector<uint32_t>{}).first->second;
			}
		}

		uint8_t op;
		while (r.u8(op))
		{
			switch (op)
			{
			case 0x02: // block
			case 0x03: // loop
			case 0x04: // if
				r.skip(1); // result type
				--depth;
				break;

			case 0x05: // else
				if (depth == hints->size())
				{
					hints->emplace_back(r.getPosition() - 1);
#if DEBUG_BRANCHING
					std::cout << "skipOverBranch: caching " << hint_key << " + depth " << depth << " -> " << hints->back() << "\n";
#endif
				}
				if (depth == target_depth)
				{
					return true;
				}
				break;

			case 0x0b: // end
				if (depth == hints->size())
				{
					hints->emplace_back(r.getPosition() - 1);
#if DEBUG_BRANCHING
					std::cout << "skipOverBranch: caching " << hint_key << " + depth " << depth << " -> " << hints->back() << "\n";
#endif
				}
				if (depth == target_depth)
				{
					return false;
				}
				++depth;
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
			case 0xd2: // ref.func
				{
					uint32_t imm;
					WASM_READ_OML(imm);
				}
				break;

			case 0x0e: // br_table
				{
					uint32_t num_branches;
					WASM_READ_OML(num_branches);
					while (num_branches--)
					{
						uint32_t depth;
						WASM_READ_OML(depth);
					}
					uint32_t default_depth;
					WASM_READ_OML(default_depth);
				}
				break;

			case 0x11: // call_indirect
				{
					uint32_t type_index; WASM_READ_OML(type_index);
					uint32_t table_index; WASM_READ_OML(table_index);
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
#if SOUP_WASM_PEDANTIC
					uint32_t memflags; WASM_READ_OML(memflags);
#else
					r.skip(1); // memflags
#endif
					size_t offset; WASM_READ_OML(offset);
				}
				break;

			case 0x3f: // memory.size
			case 0x40: // memory.grow
				r.skip(1);
				break;

			case 0x41: // i32.const
				{
					int32_t value;
					WASM_READ_SOML(value);
				}
				break;

			case 0x42: // i64.const
				{
					int64_t value;
					WASM_READ_SOML(value);
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
#if SOUP_WASM_PEDANTIC
				{
					uint32_t extended_op;
					WASM_READ_OML(extended_op);
					op = extended_op;
				}
#else
				r.u8(op);
#endif
				switch (op)
				{
				case 0x08: // memory.init
					{
						uint32_t imm;
						WASM_READ_OML(imm);
						r.skip(1);
					}
					break;

				case 0x09: // data.drop
				case 0x0d: // elem.drop
				case 0x0f: // table.grow
				case 0x10: // table.size
				case 0x11: // table.fill
					{
						uint32_t imm;
						WASM_READ_OML(imm);
					}
					break;

				case 0x0a: // memory.copy
					r.skip(2);
					break;

				case 0x0b: // memory.fill
					r.skip(1);
					break;

				case 0x0c: // table.init
					{
						uint32_t segment_index;
						WASM_READ_OML(segment_index);
						uint32_t table_index;
						WASM_READ_OML(table_index);
					}
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

	bool WasmVm::doBranch(Reader& r, uint32_t depth, uint32_t func_index, std::stack<CtrlFlowEntry>& ctrlflow) SOUP_EXCAL
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
			if (skipOverBranch(r, depth, script, func_index))
			{
				// also skip over 'else' branch
				skipOverBranch(r, depth, script, func_index);
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

		if (const auto result_stack_size = ctrlflow.top().stack_size + ctrlflow.top().num_values; stack.size() > result_stack_size)
		{
			stack.erase(stack.begin() + ctrlflow.top().stack_size, stack.end() - ctrlflow.top().num_values);
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

	// function_index will be range-checked. type_index is assumed to be in-bounds if function_index is in-bounds. (as guaranteed by getTypeIndexForFunction)
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

		WasmScript* script = &this->script;
	_doCall_other_script:

#if SOUP_WASM_PEDANTIC
		if (script == &this->script)
		{
			uint32_t func_type_index = script->getTypeIndexForFunction(function_index);
			if (type_index != func_type_index)
			{
				const auto& type = script->types[type_index];
#if false // already checked at load
				SOUP_IF_UNLIKELY (func_type_index >= script->types.size())
				{
#if DEBUG_VM
					std::cout << "call(pedantic): function type is out-of-bounds\n";
#endif
					return false;
				}
#endif
				const auto& func_type = script->types[func_type_index];
#if DEBUG_VM
				std::cout << "call: calling " << func_type.toString() << " with " << type.toString() << "\n";
#endif
				SOUP_IF_UNLIKELY (type != func_type)
				{
#if DEBUG_VM
					std::cout << "call(pedantic): function type is incompatible with call type\n";
#endif
					return false;
				}
			}
		}
#endif

		if (function_index < script->function_imports.size())
		{
			const auto& imp = script->function_imports[function_index];
#if DEBUG_VM || DEBUG_API
			std::cout << "Calling into " << imp.module_name << ":" << imp.function_name << "\n";
#endif
			if (imp.ptr)
			{
				const auto& type = script->types[type_index];
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

			script = imp.source.get();
			type_index = imp.source->getTypeIndexForFunction(imp.func_index);
			function_index = imp.func_index;
			goto _doCall_other_script;
		}
		function_index -= script->function_imports.size();
		SOUP_IF_UNLIKELY (function_index >= script->code.size())
		{
#if DEBUG_VM
			std::cout << "call: function is out-of-bounds\n";
#endif
			return false;
		}
		const auto& type = script->types[type_index];

		WasmVm callvm(*script);
		for (uint32_t i = 0; i != type.parameters.size(); ++i)
		{
			SOUP_IF_UNLIKELY (stack.empty())
			{
#if DEBUG_VM
				std::cout << "call: not enough values on the stack for parameters\n";
#endif
				return false;
			}
			callvm.locals.insert(callvm.locals.begin(), stack.back()); stack.pop_back();
		}
#if DEBUG_VM
		//std::cout << "call: enter " << function_index << "\n";
		//std::cout << string::bin2hex(script->code[function_index]) << "\n";
#endif
		const auto pre_call_stack_size = stack.size();
		callvm.stack = std::move(stack);
		SOUP_RETHROW_FALSE(callvm.run(script->code[function_index], depth, function_index));
		stack = std::move(callvm.stack);
#if DEBUG_VM
		//std::cout << "call: leave " << function_index << "\n";
#endif
		if (const auto result_stack_size = pre_call_stack_size + type.results.size(); stack.size() > result_stack_size)
		{
			stack.erase(stack.begin() + pre_call_stack_size, stack.end() - type.results.size());
		}
		return true;
	}
}
