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
#include "string.hpp"
#include "StringRefWriter.hpp"
#if SOUP_WASM_PEDANTIC
#include "unicode.hpp"
#endif

#define WASM_CALL_REUSES_STACK true
static_assert(WASM_CALL_REUSES_STACK || !SOUP_WASM_EXCEPTIONS);

#define DEBUG_LOAD false
#define DEBUG_LINK false
#define DEBUG_VM false
#define DEBUG_BRANCHING false
#define DEBUG_API false
#define DEBUG_GC false

#if DEBUG_LOAD || DEBUG_LINK || DEBUG_VM || DEBUG_BRANCHING || DEBUG_API || DEBUG_GC
#include <iostream>
#endif

// Useful resources:
// - https://webassembly.github.io/wabt/demo/wat2wasm/
// - https://github.com/sunfishcode/wasm-reference-manual/blob/master/WebAssembly.md
// - https://pengowray.github.io/wasm-ops/
// - https://webassembly.github.io/spec/versions/core/WebAssembly-2.0.pdf

/*
Spec tests (https://github.com/Sainan/wasm-spec/tree/wast2json/test/core)
> Use wast2json from wabt then run `soup wast [jsonfile]`
> Results:
- address: pass
- align: pedantic_pass
- binary: pedantic_pass
- binary-leb128: pedantic_pass
- block: pass
- br: pass
- br_if: pass
- br_table: pass
- bulk: pass
- call: pass
- call_indirect: pedantic_pass
- comments: pass
- const: pass
- conversions: pass
- custom: pedantic_pass
- data: pass
- elem: pass
- endianness: pass
- exports: pass
- extended-const: pass
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
- global: pass
- i32: pass
- i64: pass
- if: pass
- imports: pass
- inline-module: pass
- int_exprs: pass
- int_literals: pass
- labels: pass
- left-to-right: pass
- linking: pass
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
- return_call: pass
- return_call_indirect: pass
- select: pass
- skip-stack-guard-page: pass
- stack: pass
- start: pass
- store: pass
- switch: pass
- table: pass
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
- utf8-custom-section-id: pedantic_pass
- utf8-import-field: pedantic_pass
- utf8-import-module: pedantic_pass
- utf8-invalid-encoding: pass (due to Soup not parsing .wat files)
- simd/simd_address: pass
- simd/simd_align: pass
- simd/simd_bit_shift: pass
- simd/simd_bitwise: pass
- simd/simd_boolean: pass
- simd/simd_const: pass
- simd/simd_conversions: pass
- simd/simd_f32x4: pass
- simd/simd_f32x4_arith: pass
- simd/simd_f32x4_cmp: pass
- simd/simd_f32x4_pmin_pmax: pass
- simd/simd_f32x4_rounding: pass
- simd/simd_f64x2: pass
- simd/simd_f64x2_arith: pass
- simd/simd_f64x2_cmp: pass
- simd/simd_f64x2_pmin_pmax: apss
- simd/simd_f64x2_rounding: pass
- simd/simd_i16x8_arith: pass
- simd/simd_i16x8_arith2: pass
- simd/simd_i16x8_cmp: pass
- simd/simd_i16x8_extadd_pairwise_i8x16: pass
- simd/simd_i16x8_extmul_i8x16: pass
- simd/simd_i16x8_q15mulr_sat_s: pass
- simd/simd_i16x8_sat_arith: pass
- simd/simd_i32x4_arith: pass
- simd/simd_i32x4_arith2: pass
- simd/simd_i32x4_cmp: pass
- simd/simd_i32x4_dot_i16x8: pass
- simd/simd_i32x4_extadd_pairwise_i16x8: pass
- simd/simd_i32x4_extmul_i16x8: pass
- simd/simd_i32x4_trunc_sat_f32x4: pass
- simd/simd_i32x4_trunc_sat_f64x2: pass
- simd/simd_i64x2_arith: pass
- simd/simd_i64x2_arith2: pass
- simd/simd_i64x2_cmp: pass
- simd/simd_i64x2_extmul_i32x4: pass
- simd/simd_i8x16_arith: pass
- simd/simd_i8x16_arith2: pass
- simd/simd_i8x16_cmp: pass
- simd/simd_i8x16_sat_arith: pass
- simd/simd_int_to_int_extend: pass
- simd/simd_lane: pass
- simd/simd_linking: pass
- simd/simd_load: pass
- simd/simd_load16_lane: pass
- simd/simd_load32_lane: pass
- simd/simd_load64_lane: pass
- simd/simd_load8_lane: pass
- simd/simd_load_extend: pass
- simd/simd_load_splat: pass
- simd/simd_load_zero: pass
- simd/simd_splat: pass
- simd/simd_store: pass
- simd/simd_store16_lane: pass
- simd/simd_store32_lane: pass
- simd/simd_store64_lane: pass
- simd/simd_store8_lane: pass
- memory64/address64: pass
- memory64/align64: pass
- memory64/binary_leb128_64: pedantic_pass
- memory64/bulk64: pass
- memory64/call_indirect64: pass
- memory64/endianness64: pass
- memory64/float_memory64: pass
- memory64/load64: pass
- memory64/memory64: pass
- memory64/memory64-imports: pass
- memory64/memory_copy64: pass
- memory64/memory_fill64: pass
- memory64/memory_grow64: pass
- memory64/memory_init64: pass
- memory64/memory_redundancy64: pass
- memory64/memory_trap64: pass
- memory64/table64: pass
- memory64/table_copy64: pass
- memory64/table_copy_mixed: pass
- memory64/table_fill64: pass
- memory64/table_get64: pass
- memory64/table_grow64: pass
- memory64/table_init64: pass
- memory64/table_set64: pass
- memory64/table_size64: pass
- multi-memory/address0: pass
- multi-memory/address1: pass
- multi-memory/align0: pass
- multi-memory/binary0: pedantic_pass
- multi-memory/data0: pass
- multi-memory/data1: pass
- multi-memory/data_drop0: pass
- multi-memory/exports0: pass
- multi-memory/float_exprs0: pass
- multi-memory/float_exprs1: pass
- multi-memory/float_memory0: pass
- multi-memory/imports0: pass
- multi-memory/imports1: pass
- multi-memory/imports2: pass
- multi-memory/imports3: pass
- multi-memory/imports4: pass
- multi-memory/linking0: pass
- multi-memory/linking1: pass
- multi-memory/linking2: pass
- multi-memory/linking3: pass
- multi-memory/load0: pass
- multi-memory/load1: pass
- multi-memory/load2: pass
- multi-memory/memory-multi: pass
- multi-memory/memory_copy0: pass
- multi-memory/memory_copy1: pass
- multi-memory/memory_fill0: pass
- multi-memory/memory_grow: pass
- multi-memory/memory_init0: pass
- multi-memory/memory_size0: pass
- multi-memory/memory_size1: pass
- multi-memory/memory_size2: pass
- multi-memory/memory_size3: pass
- multi-memory/memory_size_import: pass
- multi-memory/memory_trap0: pass
- multi-memory/memory_trap1: pass
- multi-memory/start0: pass
- multi-memory/store0: pass
- multi-memory/store1: pass
- multi-memory/store2: pass
- multi-memory/traps0: pass
- exceptions/tag: pass
- exceptions/throw: pass
- exceptions/throw_ref: pass
- exceptions/try_table: pass
*/

NAMESPACE_SOUP
{
	WasmType wasm_type_from_string(const std::string& str) noexcept
	{
		if (str == "i32") { return WASM_I32; }
		if (str == "i64") { return WASM_I64; }
		if (str == "f32") { return WASM_F32; }
		if (str == "f64") { return WASM_F64; }
#if SOUP_WASM_SIMD
		if (str == "v128") { return WASM_V128; }
#endif
		if (str == "funcref") { return WASM_FUNCREF; }
		if (str == "externref") { return WASM_EXTERNREF; }
#if SOUP_WASM_EXCEPTIONS
		if (str == "exnref") { return WASM_EXNREF; }
#endif
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
#if SOUP_WASM_SIMD
		case WASM_V128: return "v128";
#endif
		case WASM_FUNCREF: return "funcref";
		case WASM_EXTERNREF: return "externref";
#if SOUP_WASM_EXCEPTIONS
		case WASM_EXNREF: return "exnref";
#endif
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

	// WasmValue

	std::string WasmValue::toString() const SOUP_EXCAL
	{
		std::string str(1, '<');
		str.append(wasm_type_to_string(type));
		str.append("> ");
		str.append(string::hex((uint64_t)i64));
		if ((type == WASM_EXTERNREF || type == WASM_FUNCREF) && i64 == 0)
		{
			str.append(" (null)");
		}
#if SOUP_WASM_SIMD
		if (type == WASM_V128)
		{
			str.append(", ");
			str.append(string::hex((uint64_t)i64x2[1]));
		}
#endif
		return str;
	}

	// WasmScript::Memory

	WasmScript::Memory::Memory(size_t size, uint64_t limit, bool _64bit, bool one_byte_pages) SOUP_EXCAL
		: data(size ? (uint8_t*)soup::malloc(size) : nullptr), size(size), limit(limit), memory64(_64bit), one_byte_pages(one_byte_pages)
	{
		memset(this->data, 0, this->size);
	}

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
		size_t old_size_pages = -1;
		const auto page_size = getPageSize();
		const auto delta_bytes = delta_pages * page_size;
		if ((delta_pages == 0 || delta_bytes / delta_pages == page_size) // Multiplication didn't overflow?
			&& can_add_without_overflow(this->size, delta_bytes)
			)
		{
			const auto new_size_bytes = this->size + delta_bytes;
			if (new_size_bytes <= this->limit)
			{
				if (auto nmem = (uint8_t*)::realloc(this->data, new_size_bytes))
				{
					memset(&nmem[this->size], 0, delta_bytes);
					old_size_pages = this->size / page_size;
					this->data = nmem;
					this->size = new_size_bytes;
				}
			}
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

	bool WasmScript::Table::init(WasmScript& scr, const ElemSegment& src, size_t dst_offset, size_t src_offset, size_t size) noexcept
	{
		SOUP_IF_UNLIKELY (this->type != src.type)
		{
#if DEBUG_LOAD
			std::cout << "WasmScript::Table::init: different element types\n";
#endif
			return false;
		}
		SOUP_IF_UNLIKELY (!can_add_without_overflow(dst_offset, size) || dst_offset + size > this->values.size() || !can_add_without_overflow(src_offset, size) || src_offset + size > src.values.size())
		{
#if DEBUG_LOAD
			std::cout << "out-of-bounds WasmScript::Table::init: dst_offset=" << dst_offset << ", dst_size=" << this->values.size() << ", src_offset=" << src_offset << ", src_size=" << src.values.size() << ", size=" << size << "\n";
#endif
			return false;
		}
		if (src.flags & 0b100)
		{
			while (size--)
			{
				const uint64_t& buf = src.values[src_offset++];
#if DEBUG_LOAD
				std::cout << "evaluating elem value: " << string::bin2hex((const char*)&buf, sizeof(buf)) << "\n";
#endif
				MemoryRefReader r(&buf, sizeof(buf));
				WasmValue value;
				SOUP_RETHROW_FALSE(scr.evaluateConstantExpression(r, value));
				SOUP_RETHROW_FALSE(value.type == this->type);
				this->values[dst_offset++] = value.i64;
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

	// WasmScript::MemoryImport

	bool WasmScript::MemoryImport::isCompatibleWith(const Memory& mem) const noexcept
	{
#if SOUP_WASM_MEMORY64
		if (memory64 != mem.memory64)
		{
#if DEBUG_LOAD
			std::cout << "type mismatch for memory " << module_name << ":" << field_name << " - export addr type " << wasm_type_to_string(mem.getAddrType()) << "; import addr type " << wasm_type_to_string(memory64 ? WASM_I64 : WASM_I32) << "\n";
#endif
			return false;
		}
#endif
#if SOUP_WASM_CUSTOM_PAGE_SIZES
		if (one_byte_pages != mem.one_byte_pages)
		{
			return false;
		}
#endif
		if (min_bytes > mem.size)
		{
			return false;
		}
		if (max_bytes != 0x1'0000'0000) // Import has a page limit?
		{
			if (mem.limit == 0x1'0000'0000) // Memory has no page limit?
			{
				return false;
			}
			if (max_bytes < mem.limit)
			{
				return false;
			}
		}
		return true;
	}

	// WasmScript::TableImport

	bool WasmScript::TableImport::isCompatibleWith(const Table& tbl) const noexcept
	{
		if (type != tbl.type)
		{
			return false;
		}
#if SOUP_WASM_MEMORY64
		if (table64 != tbl.table64)
		{
			return false;
		}
#endif
		if (min_size > tbl.values.size())
		{
			return false;
		}
		if (max_size != 0x10'000) // Import has a size limit?
		{
			if (tbl.limit == 0x10'000) // Table has no size limit?
			{
				return false;
			}
			if (max_size < tbl.limit)
			{
				return false;
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

#if SOUP_WASM_PEDANTIC
#define WASM_READ_MEMALIGN uint32_t align; WASM_READ_OML(align)
#define WASM_READ_MEMOFFSET uint64_t offset; WASM_READ_OML(offset)
#else
#define WASM_READ_MEMALIGN uint8_t align; r.u8(align)
#define WASM_READ_MEMOFFSET size_t offset; WASM_READ_OML(offset)
#endif

#if SOUP_WASM_MULTI_MEMORY
#define WASM_READ_MEMARG uint32_t memidx = 0; WASM_READ_MEMALIGN; if (align & 0x40) { WASM_READ_OML(memidx); } WASM_READ_MEMOFFSET
#else
#define WASM_READ_MEMARG constexpr uint32_t memidx = 0; WASM_READ_MEMALIGN; WASM_READ_MEMOFFSET
#endif

	using WasmInternalStartCode = std::string;

	bool WasmScript::load(const std::string& data) SOUP_EXCAL
	{
		MemoryRefReader r(data);
		return load(r);
	}

	enum WasmSectionType : uint8_t
	{
		SEC_CUSTOM = 0,
		SEC_TYPE = 1,
		SEC_IMPORT = 2,
		SEC_FUNCTION = 3,
		SEC_TABLE = 4,
		SEC_MEMORY = 5,
#if SOUP_WASM_EXCEPTIONS
		SEC_TAG = 13,
#endif
		SEC_GLOBAL = 6,
		SEC_EXPORT = 7,
		SEC_START = 8,
		SEC_ELEM = 9,
		SEC_DATA_COUNT = 12,
		SEC_CODE = 10,
		SEC_DATA = 11,
	};

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
		uint32_t data_count = -1;
#if SOUP_WASM_PEDANTIC
		uint32_t sections_set = 0;
#endif
		while (r.hasMore())
		{
			uint8_t section_type;
			r.u8(section_type);
#if SOUP_WASM_PEDANTIC
			if (section_type != SEC_CUSTOM && section_type < 32)
			{
				SOUP_RETHROW_FALSE((sections_set & (1 << section_type)) == 0);
				sections_set |= (1 << section_type);
			}
#endif
			uint32_t section_size;
			WASM_READ_OML(section_size);
			const auto section_end = r.getPosition() + section_size;
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
				r.seek(section_end);
				break;

			case SEC_CUSTOM:
				{
					SOUP_RETHROW_FALSE(section_size != 0);
					uint32_t name_len;
					WASM_READ_OML(name_len);
					//std::cout << "custom section: name_len = " << name_len << "\n";
					SOUP_RETHROW_FALSE(name_len <= 0x1000);
					std::string name;
					r.str(name_len, name);
#if SOUP_WASM_PEDANTIC
					{
						auto name_utf32 = unicode::utf8_to_utf32(name);
						SOUP_RETHROW_FALSE(name_utf32.find(unicode::REPLACEMENT_CHAR) == std::string::npos); // UTF-8 must be valid
						SOUP_RETHROW_FALSE(unicode::utf32_to_utf8(name_utf32) == name); // UTF-8 must also be represented canonically (so, no overlong encodings)
					}
					r.seekEnd();
					SOUP_RETHROW_FALSE(section_end <= r.getPosition());
#endif
					if (name == "name")
					{
						auto& nd = custom_data.getStructFromMap(WasmNameData);
						while (r.getPosition() < section_end)
						{
							uint8_t subsection_id;
							r.u8(subsection_id);
							uint32_t subsection_size;
							WASM_READ_OML(subsection_size);
							SOUP_RETHROW_FALSE(subsection_size != 0);
							const auto subsection_end = r.getPosition() + subsection_size;
							switch (subsection_id)
							{
							case 0:
								WASM_READ_OML(name_len);
								r.str(name_len, nd.module_name);
								break;

							case 1:
								{
									uint32_t list_len;
									WASM_READ_OML(list_len);
									while (list_len--)
									{
										uint32_t func_idx;
										WASM_READ_OML(func_idx);
										WASM_READ_OML(name_len);
										std::string func_name;
										r.str(name_len, func_name);
										nd.function_names.emplace(func_idx, std::move(func_name));
									}
								}
								break;
							}
							r.seek(subsection_end);
						}
					}
					r.seek(section_end);
				}
				break;

			case SEC_TYPE:
				{
					uint32_t num_types;
					WASM_READ_OML(num_types);
#if DEBUG_LOAD
					std::cout << num_types << " type(s)\n";
#endif
#if SOUP_WASM_PEDANTIC
					SOUP_RETHROW_FALSE((sections_set & (1 << SEC_IMPORT)) == 0);
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

			case SEC_IMPORT:
				{
					uint32_t num_imports;
					WASM_READ_OML(num_imports);
#if DEBUG_LOAD
					std::cout << num_imports << " import(s)\n";
#endif
#if SOUP_WASM_PEDANTIC
					SOUP_RETHROW_FALSE((sections_set & (1 << SEC_FUNCTION)) == 0);
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
						if (kind == IE_kFunction)
						{
							uint32_t type_index; WASM_READ_OML(type_index);
							SOUP_RETHROW_FALSE(type_index < types.size());
							function_imports.emplace_back(std::move(module_name), std::move(field_name), type_index);
						}
						else if (kind == IE_kTable)
						{
							uint8_t type; r.u8(type);
							uint8_t flags; r.u8(flags);
#if SOUP_WASM_MEMORY64
							SOUP_RETHROW_FALSE((flags & 0xfa) == 0);
#else
							SOUP_RETHROW_FALSE((flags & 0xfe) == 0);
#endif
							wasm_uptr_t min_size;
							WASM_READ_OML(min_size);
							wasm_uptr_t max_size = 0x10'000;
							if (flags & 1)
							{
								WASM_READ_OML(max_size);
							}
							tables.emplace_back();
							table_imports.emplace_back(TableImport{ { std::move(module_name), std::move(field_name) }, static_cast<WasmType>(type), (bool)(flags & 4), min_size, max_size });
						}
						else if (kind == IE_kMemory)
						{
							uint8_t flags; r.u8(flags);
							uint8_t known_flags = 1;
#if SOUP_WASM_MEMORY64
							known_flags |= 4;
#endif
#if SOUP_WASM_CUSTOM_PAGE_SIZES
							known_flags |= 8;
#endif
#if SOUP_WASM_MEMORY64
							SOUP_RETHROW_FALSE((flags & ~known_flags) == 0);
#endif
							uint64_t min_pages;
							WASM_READ_OML(min_pages);
							uint64_t max_pages = 0x10'000;
							if (flags & 1)
							{
								WASM_READ_OML(max_pages);
							}
							uint32_t page_size = 0x10'000;
#if SOUP_WASM_CUSTOM_PAGE_SIZES
							if (flags & 8)
							{
								WASM_READ_OML(page_size);
								page_size = (1 << page_size);
								SOUP_RETHROW_FALSE(page_size == 1 || page_size == 0x10'000);
								if (page_size == 1 && !(flags & 1))
								{
									max_pages = 0x1'0000'0000;
								}
							}
#endif
#if SOUP_WASM_MULTI_MEMORY
							memories.emplace_back();
							memory_imports.emplace_back(std::move(module_name), std::move(field_name), min_pages * page_size, max_pages * page_size, flags & 4, page_size == 1);
#else
							SOUP_IF_UNLIKELY (memory || memory_import)
							{
#if DEBUG_LOAD
								std::cout << "Too many memories\n";
#endif
								return false;
							}
							memory_import.emplace(std::move(module_name), std::move(field_name), min_pages * page_size, max_pages * page_size, flags & 4, page_size == 1);
#endif
						}
						else if (kind == IE_kGlobal)
						{
							uint8_t type; r.u8(type);
							uint8_t flags; r.u8(flags);
							SOUP_RETHROW_FALSE((flags & 0xfe) == 0);
							global_imports.emplace_back(GlobalImport{ { std::move(module_name), std::move(field_name) }, static_cast<WasmType>(type), (bool)(flags & 1)});
							globals.emplace_back();
						}
#if SOUP_WASM_EXCEPTIONS
						else if (kind == IE_kTag)
						{
							uint8_t type; r.u8(type);
							SOUP_RETHROW_FALSE(type == 0);
							uint32_t typeidx; WASM_READ_OML(typeidx);
							SOUP_RETHROW_FALSE(typeidx < types.size());
							tag_imports.emplace_back(TagImport{ { std::move(module_name), std::move(field_name) }, types[typeidx].parameters });
							tags.emplace_back();
						}
#endif
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

			case SEC_FUNCTION:
				{
					uint32_t num_functions;
					WASM_READ_OML(num_functions);
#if DEBUG_LOAD
					std::cout << num_functions << " function type(s)\n";
#endif
#if SOUP_WASM_PEDANTIC
					SOUP_RETHROW_FALSE((sections_set & (1 << SEC_TABLE)) == 0);
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

			case SEC_TABLE:
				{
					uint32_t num_tables; WASM_READ_OML(num_tables);
#if DEBUG_LOAD
					std::cout << num_tables << " table(s)\n";
#endif
#if SOUP_WASM_PEDANTIC
					SOUP_RETHROW_FALSE((sections_set & (1 << SEC_MEMORY)) == 0);
#endif
					tables.reserve(num_tables);
					while (num_tables--)
					{
						uint8_t type;
						r.u8(type);
						uint8_t flags;
						r.u8(flags);
#if SOUP_WASM_MEMORY64
						SOUP_RETHROW_FALSE((flags & 0xfa) == 0);
#else
						SOUP_RETHROW_FALSE((flags & 0xfe) == 0);
#endif
						wasm_uptr_t initial;
						WASM_READ_OML(initial);
						wasm_uptr_t limit = 0x10'000;
						if (flags & 1)
						{
							WASM_READ_OML(limit);
						}
						tables.emplace_back(soup::make_shared<Table>(static_cast<WasmType>(type), initial, limit, flags & 4));
					}
				}
				break;

			case SEC_MEMORY:
				{
					uint32_t num_memories; WASM_READ_OML(num_memories);
#if DEBUG_LOAD
					std::cout << num_memories << " memory/memories\n";
#endif
#if SOUP_WASM_MULTI_MEMORY
					memories.reserve(memories.size() + num_memories);
#endif
#if SOUP_WASM_PEDANTIC
					SOUP_RETHROW_FALSE((sections_set & (1 << SEC_GLOBAL)) == 0);
#endif
					while (num_memories--)
					{
#if !SOUP_WASM_MULTI_MEMORY
						SOUP_IF_UNLIKELY (memory || memory_import)
						{
#if DEBUG_LOAD
							std::cout << "Too many memories\n";
#endif
							return false;
						}
#endif
						uint8_t flags; r.u8(flags);
						uint8_t known_flags = 1;
#if SOUP_WASM_MEMORY64
						known_flags |= 4;
#endif
#if SOUP_WASM_CUSTOM_PAGE_SIZES
						known_flags |= 8;
#endif
#if SOUP_WASM_MEMORY64
						SOUP_RETHROW_FALSE((flags & ~known_flags) == 0);
#endif
						uint64_t pages;
						uint64_t page_limit = 0x10'000;
						uint32_t page_size = 0x10'000;
#if SOUP_WASM_MEMORY64
						if (flags & 4)
						{
							WASM_READ_OML(pages);
						}
						else
#endif
						{
							uint32_t pages_u32;
							WASM_READ_OML(pages_u32);
							pages = pages_u32;
						}
						if (flags & 1)
						{
#if SOUP_WASM_MEMORY64
							if (flags & 4)
							{
								WASM_READ_OML(page_limit);
							}
							else
#endif
							{
								uint32_t page_limit_u32;
								WASM_READ_OML(page_limit_u32);
								page_limit = page_limit_u32;
							}
						}
#if SOUP_WASM_CUSTOM_PAGE_SIZES
						if (flags & 8)
						{
							WASM_READ_OML(page_size);
							page_size = (1 << page_size);
							SOUP_RETHROW_FALSE(page_size == 1 || page_size == 0x10'000);
							if (page_size == 1 && !(flags & 1))
							{
								page_limit = 0x1'0000'0000;
							}
						}
#endif
						SOUP_RETHROW_FALSE(pages <= page_limit);
#if DEBUG_LOAD
						std::cout << "- " << pages << " pages (" << (pages * page_size) << " bytes total), max. " << page_limit <<"\n";
#endif
#if SOUP_WASM_MULTI_MEMORY
						this->memories.emplace_back(soup::make_shared<Memory>(pages * page_size, page_limit * page_size, flags & 4, page_size == 1));
#else
						this->memory = soup::make_shared<Memory>(pages * page_size, page_limit * page_size, flags & 4, page_size == 1);
#endif
					}
				}
				break;

#if SOUP_WASM_EXCEPTIONS
			case SEC_TAG:
				{
					uint32_t num_tags;
					WASM_READ_OML(num_tags);
#if DEBUG_LOAD
					std::cout << num_tags << " tag(s)\n";
#endif
					tags.reserve(tags.size() + num_tags);
					while (num_tags--)
					{
						uint8_t type; r.u8(type);
						SOUP_RETHROW_FALSE(type == 0);
						uint32_t typeidx; WASM_READ_OML(typeidx);
						SOUP_RETHROW_FALSE(typeidx < types.size());
						tags.emplace_back(soup::make_shared<Tag>(types[typeidx].parameters));
					}
				}
				break;
#endif

			case SEC_GLOBAL:
				{
					uint32_t num_globals;
					WASM_READ_OML(num_globals);
#if DEBUG_LOAD
					std::cout << num_globals << " global(s)\n";
#endif
#if SOUP_WASM_PEDANTIC
					SOUP_RETHROW_FALSE((sections_set & (1 << SEC_EXPORT)) == 0);
#endif
					globals.reserve(globals.size() + num_globals);
					while (num_globals--)
					{
						const uint32_t global_index = static_cast<uint32_t>(globals.size());
						uint8_t type; r.u8(type);
						uint8_t flags; r.u8(flags);
						SOUP_RETHROW_FALSE((flags & 0xfe) == 0);
						WasmValue& value = *globals.emplace_back(soup::make_shared<WasmValue>(static_cast<WasmType>(type)));
						value.mut = (flags & 1);

						std::string initexpr;
						SOUP_RETHROW_FALSE(readConstantExpression(r, initexpr));
						StringRefWriter w(custom_data.getStructFromMap(WasmInternalStartCode));
						w.raw(initexpr.data(), initexpr.size() - 1);
						{ uint8_t global_set_op = 0x24; w.u8(global_set_op); }
						w.oml(global_index);
					}
				}
				break;

			case SEC_EXPORT:
				{
					uint32_t num_exports;
					WASM_READ_OML(num_exports);
#if DEBUG_LOAD
					std::cout << num_exports << " export(s)\n";
#endif
#if SOUP_WASM_PEDANTIC
					SOUP_RETHROW_FALSE((sections_set & (1 << SEC_START)) == 0);
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

			case SEC_START:
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

			case SEC_ELEM:
				{
					uint32_t num_segments;
					WASM_READ_OML(num_segments);
#if DEBUG_LOAD
					std::cout << num_segments << " element segment(s)\n";
#endif
#if SOUP_WASM_PEDANTIC
					SOUP_RETHROW_FALSE((sections_set & (1 << SEC_DATA_COUNT)) == 0);
#endif
					for (uint32_t i = 0; i != num_segments; ++i)
					{
						uint32_t flags;
						WASM_READ_OML(flags);
#if DEBUG_LOAD
						std::cout << "elem flags: " << flags << "\n";
#endif
						uint32_t tblidx;
						uint8_t type;
						std::string base;
						if (flags & 1)
						{
							tblidx = -1;

							if (flags & 0b100)
							{
								r.u8(type);
							}
							else
							{
								type = WASM_FUNCREF;
								r.skip(1);
							}
						}
						else
						{
							if (flags & 0b10)
							{
								WASM_READ_OML(tblidx);
							}
							else
							{
								tblidx = 0;
							}

							type = getTableElemType(tblidx);

							SOUP_RETHROW_FALSE(readConstantExpression(r, base));
#if !SOUP_WASM_EXTENDED_CONST
							SOUP_RETHROW_FALSE(base.size() <= sizeof(ElemSegment::base));
#endif

							if (flags & 0b10)
							{
								r.skip(1); // reserved
							}
						}

						if (type == WASM_FUNCREF)
						{
							SOUP_RETHROW_FALSE(shared_env);
						}
						else
						{
#if SOUP_WASM_PEDANTIC
							SOUP_RETHROW_FALSE(type == WASM_EXTERNREF);
#endif
						}

						ElemSegment& es = elem_segments.emplace_back(ElemSegment{ static_cast<WasmType>(type), static_cast<uint8_t>(flags) });
#if SOUP_WASM_EXTENDED_CONST
						es.base = std::move(base);
#else
						memcpy(es.base, base.data(), base.size());
#endif
						es.tblidx = tblidx;
						uint32_t num_elements;
						WASM_READ_OML(num_elements);
						es.values.reserve(num_elements);
						while (num_elements--)
						{
							if (flags & 0b100)
							{
								std::string code;
								SOUP_RETHROW_FALSE(readConstantExpression(r, code));
								SOUP_RETHROW_FALSE(code.size() <= 8);
								uint64_t u = 0;
								memcpy(&u, code.data(), code.size());
								es.values.emplace_back(u);
							}
							else
							{
								uint32_t function_index;
								WASM_READ_OML(function_index);
								es.values.emplace_back(
									es.type == WASM_FUNCREF && function_index < function_imports.size() + functions.size()
									? shared_env->createFuncRef(*this, function_index)
									: 0
								);
							}
						}
					}
				}
				break;

			case SEC_DATA_COUNT:
				WASM_READ_OML(data_count);
				has_data_count_section = true;
				break;

			case SEC_CODE:
				{
					uint32_t num_functions;
					WASM_READ_OML(num_functions);
#if DEBUG_LOAD
					std::cout << num_functions << " function(s)\n";
#endif
#if SOUP_WASM_PEDANTIC
					SOUP_RETHROW_FALSE((sections_set & (1 << SEC_DATA)) == 0);
#endif
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
						//std::cout << "- " << string::bin2hex(body) << "\n";
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

			case SEC_DATA:
				{
					uint32_t num_segments;
					WASM_READ_OML(num_segments);
#if DEBUG_LOAD
					std::cout << num_segments << " data segment(s)\n";
#endif
					data_segments.reserve(num_segments);
					for (uint32_t i = 0; i != num_segments; ++i)
					{
						uint32_t flags;
						WASM_READ_OML(flags);
#if DEBUG_LOAD
						std::cout << "data flags: " << flags << "\n";
#endif
						auto& data_segment = data_segments.emplace_back();
						if (flags & 1)
						{
							data_segment.memidx = -1;
						}
						else
						{
							if (flags & 0b10)
							{
								WASM_READ_OML(data_segment.memidx);
							}
							else
							{
								data_segment.memidx = 0;
							}

#if SOUP_WASM_EXTENDED_CONST
							SOUP_RETHROW_FALSE(readConstantExpression(r, data_segment.base));
#else
							std::string base;
							SOUP_RETHROW_FALSE(readConstantExpression(r, base));
							SOUP_RETHROW_FALSE(base.size() <= sizeof(data_segment.base));
							memcpy(data_segment.base, base.data(), base.size());
#endif
						}

						uint32_t size;
						WASM_READ_OML(size);
						SOUP_RETHROW_FALSE(r.str(size, data_segment.data));
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
			else
			{
				SOUP_IF_UNLIKELY (r.getPosition() != section_end)
				{
#if DEBUG_LOAD
					std::cout << "section did not end where it was supposed to\n";
#endif
					return false;
				}
			}
		}
		SOUP_RETHROW_FALSE(functions.size() == code.size());
		if (data_count != -1)
		{
			SOUP_RETHROW_FALSE(data_count == data_segments.size());
		}
		return true;
	}

	bool WasmScript::readConstantExpression(Reader& r, std::string& out) SOUP_EXCAL
	{
		const auto constexpr_start_pos = r.getPosition();
		SOUP_RETHROW_FALSE(WasmVm::skipOverBranch(r, 0, *this, -1) == WasmVm::END_REACHED); // guarantees constexpr_size != 0
		const size_t constexpr_size = r.getPosition() - constexpr_start_pos;
		r.seek(constexpr_start_pos);
		r.str(constexpr_size, out);
		switch (static_cast<uint8_t>(out.front()))
		{
		case 0x23: // global.get
		case 0x41: // i32.const
		case 0x42: // i64.const
		case 0x43: // f32.const
		case 0x44: // f64.const
		case 0xd0: // ref.null
		case 0xd2: // ref.func
#if SOUP_WASM_EXTENDED_CONST
		case 0x6a: // i32.add
		case 0x6b: // i32.sub
		case 0x6c: // i32.mul
		case 0x7c: // i64.add
		case 0x7d: // i64.sub
		case 0x7e: // i64.mul
#endif
			break;

#if SOUP_WASM_SIMD
		case 0xfd:
			if (out.c_str()[1] == 0x0c) // v128.const
			{
				return true;
			}
			[[fallthrough]];
#endif
		default:
			return false;
		}
		return true;
	}

	bool WasmScript::evaluateConstantExpression(Reader& r, WasmValue& out) noexcept
	{
		uint8_t op;
		r.u8(op);
		switch (op)
		{
		case 0x23: // global.get
			{
				uint32_t global_index;
				WASM_READ_SOML(global_index);
				SOUP_IF_UNLIKELY (global_index >= global_imports.size())
				{
#if DEBUG_LOAD
					std::cout << "evaluteConstantExpression: global.get: not an imported global index\n";
#endif
					return false;
				}
				SOUP_IF_UNLIKELY (!globals[global_index])
				{
#if DEBUG_LOAD
					std::cout << "evaluteConstantExpression: global.get: unresolved import\n";
#endif
					return false;
				}
				out = *globals[global_index];
			}
			break;

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
			{
				uint32_t func_index;
				WASM_READ_OML(func_index);
				SOUP_RETHROW_FALSE(shared_env && func_index < function_imports.size() + functions.size());
				out.i64 = shared_env->createFuncRef(*this, func_index);
				out.type = WASM_FUNCREF;
			}
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

#if SOUP_WASM_EXTENDED_CONST
	bool WasmScript::evaluateExtendedConstantExpression(std::string&& code, WasmValue& out) noexcept
	{
		MemoryRefReader r(code);
		WasmVm vm(*this);
		SOUP_RETHROW_FALSE(vm.runCode(r, 0, -1) == WasmVm::CODE_RETURN);
		SOUP_RETHROW_FALSE(vm.stack.size() == 1);
		out = vm.stack.back();
		code.clear();
		code.shrink_to_fit();
		return true;
	}
#endif

	bool WasmScript::validateFunctionBody(Reader& r) noexcept
	{
		uint32_t local_decl_count;
		WASM_READ_OML(local_decl_count);
		uint32_t total_locals = 0;
		while (local_decl_count--)
		{
			uint32_t type_count;
			WASM_READ_OML(type_count);
			SOUP_RETHROW_FALSE(can_add_without_overflow(total_locals, type_count));
			total_locals += type_count;
			r.skip(1); // type
		}

		SOUP_RETHROW_FALSE(WasmVm::skipOverBranch(r, 0, *this, -1) == WasmVm::END_REACHED);
		const auto pos_after_branching = r.getPosition();
		r.seekEnd();
		SOUP_IF_UNLIKELY (r.getPosition() != pos_after_branching)
		{
#if DEBUG_LOAD
			std::cout << "load(pedantic): skipOverBranch bailed early at position " << pos_after_branching << "\n";
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
		for (uint32_t i = 0; i != table_imports.size(); ++i)
		{
			if (!tables[i])
			{
				return true;
			}
		}
#if SOUP_WASM_MULTI_MEMORY
		for (uint32_t i = 0; i != memory_imports.size(); ++i)
		{
			if (!memories[i])
			{
				return true;
			}
		}
#else
		if (memory_import && !memory)
		{
			return true;
		}
#endif
#if SOUP_WASM_EXCEPTIONS
		for (uint32_t i = 0; i != tag_imports.size(); ++i)
		{
			if (!tags[i])
			{
				return true;
			}
		}
#endif
		return false;
	}

	void WasmScript::provideImportedFunction(const std::string& module_name, const std::string& function_name, wasm_ffi_func_t ptr, uint32_t user_data) noexcept
	{
		for (auto& fi : function_imports)
		{
			if (fi.field_name == function_name
				&& fi.module_name == module_name
				)
			{
				fi.ptr = ptr;
				fi.user_data = user_data;
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
				if (auto e = map.find(fi.field_name); e != map.end())
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

	void WasmScript::provideImportedTables(const std::string& module_name, const std::unordered_map<std::string, SharedPtr<Table>>& map) noexcept
	{
		for (size_t i = 0; i != table_imports.size(); ++i)
		{
			const auto& ti = table_imports[i];
			if (ti.module_name == module_name)
			{
				if (auto e = map.find(ti.field_name); e != map.end())
				{
					if (e->second && ti.isCompatibleWith(*e->second))
					{
						tables[i] = e->second;
					}
				}
			}
		}
	}

	void WasmScript::provideImportedMemory(const std::string& module_name, const std::string& field_name, SharedPtr<Memory> value) noexcept
	{
		SOUP_IF_UNLIKELY (!value)
		{
			return;
		}

#if SOUP_WASM_MULTI_MEMORY
		for (size_t i = 0; i != memory_imports.size(); ++i)
		{
			const auto& mi = memory_imports[i];
			if (mi.field_name == field_name
				&& mi.module_name == module_name
				&& mi.isCompatibleWith(*value)
				)
			{
				memories[i] = value;
			}
		}
#else
		if (memory_import
			&& memory_import->module_name == module_name
			&& memory_import->field_name == field_name
			&& memory_import->isCompatibleWith(*value)
			)
		{
			memory = value;
		}
#endif
	}

	void WasmScript::importFromModule(const std::string& module_name, WasmScript& other) noexcept
	{
		for (auto& fi : function_imports)
		{
			if (fi.module_name == module_name)
			{
				if (auto e = other.export_map.find(fi.field_name); e != other.export_map.end())
				{
					if (e->second.kind == IE_kFunction
						&& e->second.index < other.function_imports.size() + other.functions.size()
						)
					{
						auto& import_type = types[fi.type_index];
						auto& export_type = other.types[other.getTypeIndexForFunction(e->second.index)];
#if DEBUG_LINK
						std::cout << "importing " << fi.module_name << ":" << fi.field_name << " as " << import_type.toString() << " from " << export_type.toString() << "\n";
#endif
						if (import_type == export_type)
						{
							fi.source = &other;
							fi.func_index = e->second.index;
						}
					}
				}
			}
		}
		for (size_t i = 0; i != global_imports.size(); ++i)
		{
			const auto& gi = global_imports[i];
			if (gi.module_name == module_name)
			{
				if (auto e = other.export_map.find(gi.field_name); e != other.export_map.end())
				{
					if (e->second.kind == IE_kGlobal
						&& e->second.index < other.globals.size()
						&& other.globals[e->second.index]
						)
					{
						if (other.globals[e->second.index]->type == gi.type && other.globals[e->second.index]->mut == gi.mut)
						{
							globals[i] = other.globals[e->second.index];
						}
#if DEBUG_LINK
						else
						{
							std::cout << "type mismatch for global " << gi.module_name << ":" << gi.field_name << ": export is " << (other.globals[e->second.index]->mut ? "mut " : "") << wasm_type_to_string(other.globals[e->second.index]->type) << "; import is " << (gi.mut ? "mut " : "") << wasm_type_to_string(gi.type) << "\n";
						}
#endif
					}
				}
			}
		}
		for (size_t i = 0; i != table_imports.size(); ++i)
		{
			const auto& ti = table_imports[i];
			if (ti.module_name == module_name)
			{
				if (auto e = other.export_map.find(ti.field_name); e != other.export_map.end())
				{
					if (e->second.kind == IE_kTable
						&& e->second.index < other.tables.size()
						&& other.tables[e->second.index]
						&& ti.isCompatibleWith(*other.tables[e->second.index])
						)
					{
						tables[i] = other.tables[e->second.index];
					}
				}
			}
		}
#if SOUP_WASM_MULTI_MEMORY
		for (size_t i = 0; i != memory_imports.size(); ++i)
		{
			const auto& mi = memory_imports[i];
			if (mi.module_name == module_name)
			{
				if (auto e = other.export_map.find(mi.field_name); e != other.export_map.end())
				{
					if (e->second.kind == IE_kMemory
						&& e->second.index < other.memories.size()
						&& other.memories[e->second.index]
						&& mi.isCompatibleWith(*other.memories[e->second.index])
						)
					{
						memories[i] = other.memories[e->second.index];
					}
				}
			}
		}
#else
		if (memory_import && memory_import->module_name == module_name)
		{
			if (auto e = other.export_map.find(memory_import->field_name); e != other.export_map.end())
			{
				if (e->second.kind == IE_kMemory
					&& e->second.index == 0
					&& other.memory
					&& memory_import->isCompatibleWith(*other.memory)
					)
				{
					this->memory = other.memory;
				}
			}
		}
#endif
#if SOUP_WASM_EXCEPTIONS
		for (size_t i = 0; i != tag_imports.size(); ++i)
		{
			const auto& ti = tag_imports[i];
			if (ti.module_name == module_name)
			{
				if (auto e = other.export_map.find(ti.field_name); e != other.export_map.end())
				{
					if (e->second.kind == IE_kTag
						&& e->second.index < other.tags.size()
						&& other.tags[e->second.index]
						&& ti.parameters == other.tags[e->second.index]->parameters
						)
					{
						tags[i] = other.tags[e->second.index];
					}
				}
			}
		}
#endif
	}

	bool WasmScript::instantiate()
	{
		for (auto& es : elem_segments)
		{
			if (es.isPassive())
			{
				continue;
			}
			if (auto table = getTableByIndex(es.tblidx))
			{
#if DEBUG_LOAD
				std::cout << "instantiate: processing elem segment\n";
#endif
				WasmValue base;
				{
#if SOUP_WASM_EXTENDED_CONST
					SOUP_RETHROW_FALSE(evaluateExtendedConstantExpression(std::move(es.base), base));
#else
					MemoryRefReader r(es.base, sizeof(es.base));
					SOUP_RETHROW_FALSE(evaluateConstantExpression(r, base));
#endif
				}
				SOUP_IF_UNLIKELY (base.type != table->getAddrType())
				{
#if DEBUG_LOAD
					std::cout << "elem segment base offset type doesn't match table's address type\n";
#endif
					return false;
				}
				SOUP_RETHROW_FALSE(table->init(*this, es, base.uptr(), 0, es.values.size()));
			}
			es.values.clear();
			es.values.shrink_to_fit();
		}

		for (auto& ds : data_segments)
		{
			if (auto memory = getMemoryByIndex(ds.memidx))
			{
#if DEBUG_LOAD
				std::cout << "instantiate: processing data segment\n";
#endif
				WasmValue base;
				{
#if SOUP_WASM_EXTENDED_CONST
#if DEBUG_LOAD
					std::cout << "instantiate: ds.base = " << string::bin2hex(ds.base) << "\n";
#endif
					SOUP_RETHROW_FALSE(evaluateExtendedConstantExpression(std::move(ds.base), base));
#else
					MemoryRefReader r(ds.base, sizeof(ds.base));
					SOUP_RETHROW_FALSE(evaluateConstantExpression(r, base));
#endif
				}
				SOUP_RETHROW_FALSE(base.type == memory->getAddrType());
				auto view = memory->getView(base.uptr(), ds.data.size());
				SOUP_RETHROW_FALSE(view || (base.uptr() == 0 && ds.data.size() == 0));
				memcpy(view, ds.data.data(), ds.data.size());
				ds.data.clear();
				ds.data.shrink_to_fit();
			}
		}

		if (custom_data.isStructInMap(WasmInternalStartCode))
		{
#if DEBUG_LOAD
			std::cout << "instantiate: initialising globals by running " << string::bin2hex(custom_data.getStructFromMapConst(WasmInternalStartCode)) << "\n";
#endif
			WasmVm vm(*this);
			MemoryRefReader r(custom_data.getStructFromMapConst(WasmInternalStartCode));
			SOUP_RETHROW_FALSE(vm.runCode(r) == WasmVm::CODE_RETURN);
			custom_data.removeStructFromMap(WasmInternalStartCode);
		}

		if (start_func_idx != -1)
		{
#if DEBUG_LOAD
			std::cout << "instantiate: running start function\n";
#endif
			SOUP_RETHROW_FALSE(this->call(start_func_idx));
		}

		return true;
	}

	const std::string* WasmScript::getExportedFuntion(const std::string& name, const WasmFunctionType** optOutType) const noexcept
	{
		if (auto e = export_map.find(name); e != export_map.end() && e->second.kind == IE_kFunction)
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
		if (auto e = export_map.find(name); e != export_map.end() && e->second.kind == IE_kFunction)
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

	WasmValue* WasmScript::getExportedGlobal(const std::string& name) noexcept
	{
		if (auto e = export_map.find(name); e != export_map.end() && e->second.kind == IE_kGlobal)
		{
			return getGlobalByIndex(e->second.index);
		}
		return nullptr;
	}

	WasmType WasmScript::getTableElemType(uint32_t idx) noexcept
	{
		if (auto tbl = getTableByIndex(idx))
		{
			return tbl->type;
		}
		if (idx < table_imports.size())
		{
			return table_imports[idx].type;
		}
		return static_cast<WasmType>(0);
	}

	WasmScript::ElemSegment* WasmScript::getPassiveElemSegmentByIndex(uint32_t i) noexcept
	{
		if (i < elem_segments.size())
		{
			if (elem_segments[i].isPassive())
			{
				return &elem_segments[i];
			}
		}
		return nullptr;
	}

	std::string* WasmScript::getPassiveDataSegmentByIndex(uint32_t i) noexcept
	{
		if (i < data_segments.size())
		{
			if (data_segments[i].isPassive())
			{
				return &data_segments[i].data;
			}
		}
		return nullptr;
	}

#define API_CHECK_STACK(x) SOUP_IF_UNLIKELY (vm.stack.size() < x) { SOUP_THROW(Exception("Insufficient values on stack for function call")); }

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
		// Note: Technically, WASI should use the memory exported under the name "memory".
		SOUP_IF_UNLIKELY (!getMemoryByIndex(0))
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

		provideImportedFunction("wasi_snapshot_preview1", "args_sizes_get", [](WasmVm& vm, uint32_t user_data, const WasmFunctionType&)
		{
			API_CHECK_STACK(2);
			auto plen = vm.stack.back().i32; vm.stack.pop_back();
			auto pargc = vm.stack.back().i32; vm.stack.pop_back();
			WasiData& wd = vm.script.custom_data.getStructFromMapConst(WasiData);
			if (auto pLen = vm.script.getMemoryByIndex(0)->getPointer<int32_t>(plen))
			{
				*pLen = 0;
				for (const auto& arg : wd.args)
				{
					*pLen += arg.size() + 1;
				}
			}
			if (auto pArgc = vm.script.getMemoryByIndex(0)->getPointer<int32_t>(pargc))
			{
				*pArgc = wd.args.size();
			}
			vm.stack.emplace_back(WASI_ERRNO_SUCCESS);
		});
		provideImportedFunction("wasi_snapshot_preview1", "args_get", [](WasmVm& vm, uint32_t user_data, const WasmFunctionType&)
		{
			API_CHECK_STACK(2);
			auto pstr = vm.stack.back().i32; vm.stack.pop_back();
			auto pargv = vm.stack.back().i32; vm.stack.pop_back();
			WasiData& wd = vm.script.custom_data.getStructFromMapConst(WasiData);
			std::string argstr;
			for (uint32_t i = 0; i != wd.args.size(); ++i)
			{
				if (auto pArg = vm.script.getMemoryByIndex(0)->getPointer<int32_t>(pargv + i * 4))
				{
					*pArg = pstr + argstr.size();
				}
				argstr.append(wd.args[i].data(), wd.args[i].size() + 1);
			}
			vm.script.getMemoryByIndex(0)->write(pstr, argstr.data(), argstr.size());
			vm.stack.emplace_back(WASI_ERRNO_SUCCESS);
		});
		provideImportedFunction("wasi_snapshot_preview1", "environ_sizes_get", [](WasmVm& vm, uint32_t user_data, const WasmFunctionType&)
		{
			API_CHECK_STACK(2);
			auto out_environ_buf_size = vm.stack.back().i32; vm.stack.pop_back();
			auto out_environ_count = vm.stack.back().i32; vm.stack.pop_back();
			if (auto ptr = vm.script.getMemoryByIndex(0)->getPointer<int32_t>(out_environ_count))
			{
				*ptr = 0;
			}
			if (auto ptr = vm.script.getMemoryByIndex(0)->getPointer<int32_t>(out_environ_buf_size))
			{
				*ptr = 0;
			}
			vm.stack.emplace_back(WASI_ERRNO_SUCCESS);
		});
		provideImportedFunction("wasi_snapshot_preview1", "proc_exit", [](WasmVm& vm, uint32_t user_data, const WasmFunctionType&)
		{
			API_CHECK_STACK(1);
			auto code = vm.stack.back().i32; vm.stack.pop_back();
			exit(code);
		});
		provideImportedFunction("wasi_snapshot_preview1", "fd_prestat_get", [](WasmVm& vm, uint32_t user_data, const WasmFunctionType&)
		{
			API_CHECK_STACK(2);
			auto prestat = vm.stack.back().i32; vm.stack.pop_back();
			auto fd = vm.stack.back().i32; vm.stack.pop_back();
#if DEBUG_API
			std::cout << "prestat on fd " << fd << "\n";
#endif
			if (fd == 3)
			{
				if (auto pTag = vm.script.getMemoryByIndex(0)->getPointer<uint32_t>(prestat + 0))
				{
					*pTag = 0; // __WASI_PREOPENTYPE_DIR
				}
				if (auto pDirNameLen = vm.script.getMemoryByIndex(0)->getPointer<uint32_t>(prestat + 4))
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
		provideImportedFunction("wasi_snapshot_preview1", "fd_prestat_dir_name", [](WasmVm& vm, uint32_t user_data, const WasmFunctionType&)
		{
			API_CHECK_STACK(3);
			auto path_len = vm.stack.back().i32; vm.stack.pop_back();
			auto path = vm.stack.back().i32; vm.stack.pop_back();
			auto fd = vm.stack.back().i32; vm.stack.pop_back();
			if (fd == 3)
			{
				if (path_len >= 1)
				{
					vm.script.getMemoryByIndex(0)->write(path, ".", 1);
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
		provideImportedFunction("wasi_snapshot_preview1", "fd_filestat_get", [](WasmVm& vm, uint32_t user_data, const WasmFunctionType&)
		{
			API_CHECK_STACK(2);
			auto out = vm.stack.back().i32; vm.stack.pop_back();
			auto fd = vm.stack.back().i32; vm.stack.pop_back();
			SOUP_UNUSED(out);
			vm.stack.emplace_back(fd < 3 ? WASI_ERRNO_SUCCESS : WASI_ERRNO_BADF);
		});
		provideImportedFunction("wasi_snapshot_preview1", "fd_fdstat_get", [](WasmVm& vm, uint32_t user_data, const WasmFunctionType&)
		{
			API_CHECK_STACK(2);
			auto out = vm.stack.back().i32; vm.stack.pop_back(); // https://github.com/WebAssembly/wasi-libc/blob/d02bdc21afc4d835383b006c11e285c4a7c78439/libc-bottom-half/headers/public/wasi/wasip1.h#L945
			auto fd = vm.stack.back().i32; vm.stack.pop_back();
#if DEBUG_API
			std::cout << "fdstat on fd " << fd << "\n";
#endif
			if (fd == 3)
			{
				if (auto pFiletype = vm.script.getMemoryByIndex(0)->getPointer<uint8_t>(out + 0))
				{
					*pFiletype = 3; // directory, as per https://github.com/WebAssembly/wasi-libc/blob/d02bdc21afc4d835383b006c11e285c4a7c78439/libc-bottom-half/headers/public/wasi/wasip1.h#L785
				}
				if (auto pFlags = vm.script.getMemoryByIndex(0)->getPointer<uint16_t>(out + 2))
				{
					*pFlags = 0;
				}
				if (auto pRightsBase = vm.script.getMemoryByIndex(0)->getPointer<uint64_t>(out + 8))
				{
					*pRightsBase = -1;
				}
				if (auto pRightsInheriting = vm.script.getMemoryByIndex(0)->getPointer<uint64_t>(out + 16))
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
		provideImportedFunction("wasi_snapshot_preview1", "path_filestat_get", [](WasmVm& vm, uint32_t user_data, const WasmFunctionType&)
		{
			API_CHECK_STACK(5);
			auto buf = vm.stack.back().i32; vm.stack.pop_back(); // https://github.com/WebAssembly/wasi-libc/blob/d02bdc21afc4d835383b006c11e285c4a7c78439/libc-bottom-half/headers/public/wasi/wasip1.h#L1064
			auto path_len = vm.stack.back().i32; vm.stack.pop_back();
			auto path = vm.stack.back().i32; vm.stack.pop_back();
			auto flags = vm.stack.back().i32; vm.stack.pop_back();
			auto fd = vm.stack.back().i32; vm.stack.pop_back();
			if (fd == 3)
			{
				auto path_str = vm.script.getMemoryByIndex(0)->readString(path, path_len);
#if DEBUG_API
				std::cout << "path_filestat_get: " << path_str << " (relative to .)\n";
#endif
				SOUP_UNUSED(flags);
				if (auto pFiletype = vm.script.getMemoryByIndex(0)->getPointer<uint8_t>(buf + 16))
				{
					*pFiletype = 4; // regular file, as per https://github.com/WebAssembly/wasi-libc/blob/d02bdc21afc4d835383b006c11e285c4a7c78439/libc-bottom-half/headers/public/wasi/wasip1.h#L785
				}
				if (auto pSize = vm.script.getMemoryByIndex(0)->getPointer<uint64_t>(buf + 32))
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
		provideImportedFunction("wasi_snapshot_preview1", "path_open", [](WasmVm& vm, uint32_t user_data, const WasmFunctionType&)
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
				auto path_str = vm.script.getMemoryByIndex(0)->readString(path, path_len);
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
					if (auto pOutFd = vm.script.getMemoryByIndex(0)->getPointer<uint32_t>(out_fd))
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
		provideImportedFunction("wasi_snapshot_preview1", "fd_seek", [](WasmVm& vm, uint32_t user_data, const WasmFunctionType&)
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
				if (auto pOutOff = vm.script.getMemoryByIndex(0)->getPointer<uint64_t>(out_off))
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
		provideImportedFunction("wasi_snapshot_preview1", "fd_read", [](WasmVm& vm, uint32_t user_data, const WasmFunctionType&)
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
					if (auto ptr = vm.script.getMemoryByIndex(0)->getPointer<int32_t>(iovs))
					{
						iov_base = *ptr;
					}
					iovs += 4;
					int32_t iov_len = 0;
					if (auto ptr = vm.script.getMemoryByIndex(0)->getPointer<int32_t>(iovs))
					{
						iov_len = *ptr;
					}
					iovs += 4;
					if (auto ptr = vm.script.getMemoryByIndex(0)->getView(iov_base, iov_len))
					{
						const auto ret = fread(ptr, 1, iov_len, f);
						if (ret >= 0)
						{
							nread += ret;
						}
					}
				}
				if (auto pOut = vm.script.getMemoryByIndex(0)->getPointer<int32_t>(out_nread))
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
		provideImportedFunction("wasi_snapshot_preview1", "fd_write", [](WasmVm& vm, uint32_t user_data, const WasmFunctionType&)
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
					if (auto ptr = vm.script.getMemoryByIndex(0)->getPointer<int32_t>(iovs))
					{
						iov_base = *ptr;
					}
					iovs += 4;
					int32_t iov_len = 0;
					if (auto ptr = vm.script.getMemoryByIndex(0)->getPointer<int32_t>(iovs))
					{
						iov_len = *ptr;
					}
					iovs += 4;
					if (auto ptr = vm.script.getMemoryByIndex(0)->getView(iov_base, iov_len))
					{
						const auto ret = fwrite(ptr, 1, iov_len, f);
						if (ret >= 0)
						{
							nwritten += ret;
						}
					}
				}
				if (auto pOut = vm.script.getMemoryByIndex(0)->getPointer<int32_t>(out_nwritten))
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
		provideImportedFunction("wasi_snapshot_preview1", "fd_close", [](WasmVm& vm, uint32_t user_data, const WasmFunctionType&)
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
			std::cout << "Calling into " << imp.module_name << ":" << imp.field_name << "\n";
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
				imp.ptr(vm, imp.user_data, script->types[imp.type_index]);
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
			script = imp.source;
			func_index = imp.func_index;
			goto _call_other_script;
		}
		const auto code_index = func_index - script->function_imports.size();
		SOUP_IF_UNLIKELY (code_index >= script->code.size())
		{
#if DEBUG_LOAD || DEBUG_API
			std::cout << "call: function is out-of-bounds\n";
#endif
			return false;
		}
		WasmVm vm(*script);
		vm.locals = std::move(args);
		//std::cout << "code: " << string::bin2hex(script->code[func_index]) << "\n";
		SOUP_IF_UNLIKELY (vm.run(script->code[code_index], 0, func_index) != WasmVm::CODE_RETURN)
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

	// WasmSharedEnvironment

	WasmScript& WasmSharedEnvironment::createScript() SOUP_EXCAL
	{
		WasmScript* scr = new WasmScript(this);
#if DEBUG_GC
		std::cout << "createScript: " << (void*)scr << "\n";
#endif
		return *scripts.emplace_back(scr);
	}

	void WasmSharedEnvironment::onRefCountHitZero(WasmScript& scr) noexcept
	{
		if (!scr.has_created_funcrefs)
		{
			// We can bypass the GC because no funcrefs means gcMark will not reach this script, and gcSweep will not have to free any funcref slots.
#if DEBUG_GC
			std::cout << "onRefCountHitZero: sweeping " << (void*)&scr << "\n";
#endif
			for (auto i = scripts.begin(); i != scripts.end(); ++i)
			{
				if (*i == &scr)
				{
					if (on_pre_free_script)
					{
						on_pre_free_script(**i);
					}
					delete *i;
					scripts.erase(i);
					break;
				}
			}
		}
	}

	/*uint64_t WasmSharedEnvironment::createFuncRef(wasm_ffi_func_t ptr, uint32_t user_data)
	{
		funcrefs.emplace_back(ptr, user_data);
		return funcrefs.size();
	}*/

	uint64_t WasmSharedEnvironment::createFuncRef(WasmScript& scr, uint32_t func_index) SOUP_EXCAL
	{
		scr.has_created_funcrefs = true;

		// Check if there's a free slot to reuse
		for (size_t i = 0; i != funcrefs.size(); ++i)
		{
			if (funcrefs[i].source == nullptr)
			{
				funcrefs[i].source = &scr;
				funcrefs[i].index = func_index;
				return i + 1;
			}
		}
		
		// Allocate a new slot
		funcrefs.emplace_back(&scr, func_index);
		return funcrefs.size();
	}

	const WasmSharedEnvironment::FuncRef& WasmSharedEnvironment::getFuncRef(uint64_t value) const noexcept
	{
		return funcrefs[value - 1];
	}

	void WasmSharedEnvironment::trackExternRef(uint64_t value) SOUP_EXCAL
	{
		tracked_externrefs.emplace(value, false);
	}

	void WasmSharedEnvironment::gcMark() noexcept
	{
		for (auto& scr : scripts)
		{
			for (const auto& g : scr->globals)
			{
				if (g->i64)
				{
					if (g->type == WASM_FUNCREF)
					{
						if (scripts.size() > 1)
						{
							const auto& fr = getFuncRef(g->i64);
							if (fr.source != scr)
							{
#if DEBUG_GC
								std::cout << "gcMark: " << (void*)fr.source << " is reachable via a global in " << scr << "\n";
#endif
								fr.source->_gc_reachable = true;
							}
						}
					}
					else if (g->type == WASM_EXTERNREF)
					{
						if (auto e = tracked_externrefs.find(g->i64); e != tracked_externrefs.end())
						{
							e->second = true;
						}
					}
				}
			}
			for (const auto& t : scr->tables)
			{
				if (t->type == WASM_FUNCREF)
				{
					if (scripts.size() > 1)
					{
						for (const auto& fri : t->values)
						{
							if (fri)
							{
								const auto& fr = getFuncRef(fri);
								if (fr.source != scr)
								{
#if DEBUG_GC
									std::cout << "gcMark: " << (void*)fr.source << " is reachable via a table in " << scr << "\n";
#endif
									fr.source->_gc_reachable = true;
								}
							}
						}
					}
				}
				else if (t->type == WASM_EXTERNREF)
				{
					if (!tracked_externrefs.empty())
					{
						for (const auto& value : t->values)
						{
							if (value)
							{
								if (auto e = tracked_externrefs.find(value); e != tracked_externrefs.end())
								{
									e->second = true;
								}
							}
						}
					}
				}
			}
			if (scr->ref_count > 0)
			{
#if DEBUG_GC
				std::cout << "gcMark: " << (void*)scr << " is reachable via ScriptRaii\n";
#endif
				scr->_gc_reachable = true;
			}
		}
	}

	void WasmSharedEnvironment::gcSweep()
	{
#if DEBUG_GC
		std::cout << "gcSweep\n";
#endif
		for (auto it = scripts.begin(); it != scripts.end(); )
		{
			WasmScript* scr = *it;
			if (scr->_gc_reachable)
			{
#if DEBUG_GC
				std::cout << "- " << (void*)scr << " is reachable\n";
#endif
				// Reset for next mark phase
				scr->_gc_reachable = false;
				++it;
			}
			else
			{
#if DEBUG_GC
				std::cout << "- " << (void*)scr << " is unreachable; sweeping\n";
#endif
				if (on_pre_free_script)
				{
					on_pre_free_script(*scr);
				}
				for (auto& fr : funcrefs)
				{
					if (fr.source == scr)
					{
						fr.source = nullptr;
					}
				}
				delete scr;
				it = scripts.erase(it);
			}
		}
		for (auto it = tracked_externrefs.begin(); it != tracked_externrefs.end(); )
		{
			if (it->second)
			{
				// Reset for next mark phase
				it->second = false;
				++it;
			}
			else
			{
				free_externref(it->first);
				it = tracked_externrefs.erase(it);
			}
		}
	}

	WasmSharedEnvironment::~WasmSharedEnvironment() noexcept
	{
		for (WasmScript* scr : scripts)
		{
#if DEBUG_GC
			std::cout << "dtor: sweeping " << (void*)scr << "\n";
#endif
			if (on_pre_free_script)
			{
				on_pre_free_script(*scr);
			}
			delete scr;
		}
		for (const auto& er : tracked_externrefs)
		{
			free_externref(er.first);
		}
	}

	// WasmVm

	WasmVm::RunCodeResult WasmVm::run(const std::string& data, unsigned depth, uint32_t func_index)
	{
		MemoryRefReader r(data);
		return run(r, depth, func_index);
	}

#if DEBUG_VM
#define WASM_CHECK_STACK(x) SOUP_IF_UNLIKELY (stack.size() < x) { /*__debugbreak();*/ std::cout << "Insufficient values on stack\n"; return {}; }
#else
#define WASM_CHECK_STACK(x) SOUP_IF_UNLIKELY (stack.size() < x) { return {}; }
#endif

	WasmVm::RunCodeResult WasmVm::run(Reader& _r, unsigned depth, uint32_t func_index)
	{
#if SOUP_WASM_TAIL_CALL
		Reader* pr = &_r;
		Optional<MemoryRefReader> rr;
		RunCodeResult result;
		while (true)
		{
			Reader& r = *pr;
			SOUP_RETHROW_FALSE(processLocalDecls(r));
			result = runCode(r, depth, func_index);
			SOUP_IF_LIKELY (result < CODE_RETURN_CALL)
			{
				break;
			}
			if (result == CODE_RETURN_CALL)
			{
				WASM_READ_OML(func_index);
			}
			else //if (result == CODE_RETURN_CALL_INDIRECT)
			{
				uint32_t type_index; WASM_READ_OML(type_index);
				uint32_t table_index; WASM_READ_OML(table_index);
				auto table = script.getTableByIndex(table_index);
				SOUP_RETHROW_FALSE(table && table->type == WASM_FUNCREF);
				WASM_CHECK_STACK(1);
				auto elem_index = static_cast<uint32_t>(stack.back().i32); stack.pop_back();
				SOUP_RETHROW_FALSE(elem_index < table->values.size());
				SOUP_RETHROW_FALSE(table->values[elem_index] != 0);
				const auto& funcref = script.shared_env->getFuncRef(table->values[elem_index]);
				SOUP_IF_UNLIKELY (funcref.source != &script)
				{
					return doCall(funcref.source, type_index, funcref.index, depth);
				}
				func_index = funcref.index;
#if SOUP_WASM_PEDANTIC
				SOUP_RETHROW_FALSE(this->script.types[type_index] == script.types[script.getTypeIndexForFunction(func_index)]);
#endif
			}
			SOUP_IF_UNLIKELY (func_index < script.function_imports.size())
			{
				return doCall(&script, script.function_imports[func_index].type_index, func_index, depth);
			}
#if DEBUG_VM
			std::cout << "tail-call, weee! going to " << func_index << "\n";
#endif
			const auto code_index = func_index - script.function_imports.size();
			SOUP_RETHROW_FALSE(code_index < script.code.size());
			locals.clear();
			SOUP_RETHROW_FALSE(moveArguments(*this, script.types[script.functions[code_index]]));
			stack.clear();
			rr.emplace(script.code[code_index]);
			pr = &*rr;
		}
		return result;
#else
		SOUP_RETHROW_FALSE(processLocalDecls(_r));
		return runCode(_r, depth, func_index);
#endif
	}

	bool WasmVm::processLocalDecls(Reader& r) SOUP_EXCAL
	{
		uint32_t local_decl_count;
		WASM_READ_OML(local_decl_count);
		while (local_decl_count--)
		{
			uint32_t type_count;
			WASM_READ_OML(type_count);
			uint8_t type;
			r.u8(type);
			while (type_count--)
			{
				locals.emplace_back(static_cast<WasmType>(type));
			}
		}
		return true;
	}

	template <typename T>
	[[nodiscard]] static T wasm_fmin(T a, T b)
	{
		if (std::isnan(a) || std::isnan(b))
		{
			return std::numeric_limits<T>::quiet_NaN();
		}
		else if (a < b || (std::signbit(a) && !std::signbit(b)))
		{
			return a;
		}
		else
		{
			return b;
		}
	}

	template <typename T>
	[[nodiscard]] static T wasm_fmax(T a, T b)
	{
		if (std::isnan(a) || std::isnan(b))
		{
			return std::numeric_limits<T>::quiet_NaN();
		}
		else if (a < b || (std::signbit(a) && !std::signbit(b)))
		{
			return b;
		}
		else
		{
			return a;
		}
	}

	template <typename Float, typename Int>
	struct float_to_int_traits;

	template <>
	struct float_to_int_traits<float, int32_t>
	{
		static constexpr float min = -2147483600.0f;
		static constexpr float max = 2147483500.0f;
	};

	template <>
	struct float_to_int_traits<float, uint32_t>
	{
		static constexpr float min = -0.99999994f;
		static constexpr float max = 4294967000.0f;
	};

	template <>
	struct float_to_int_traits<double, int32_t>
	{
		static constexpr double min = -2147483648.9;
		static constexpr double max = 2147483647.9;
	};

	template <>
	struct float_to_int_traits<double, uint32_t>
	{
		static constexpr double min = -0.9999999999999999;
		static constexpr double max = 4294967295.9;
	};

	template <>
	struct float_to_int_traits<float, int64_t>
	{
		static constexpr float min = -9223372000000000000.0f;
		static constexpr float max = 9223371500000000000.0f;
	};

	template <>
	struct float_to_int_traits<float, uint64_t>
	{
		static constexpr float min = -0.99999994f;
		static constexpr float max = 18446743000000000000.0f;
	};

	template <>
	struct float_to_int_traits<double, int64_t>
	{
		static constexpr double min = -9223372036854776000.0;
		static constexpr double max = 9223372036854775000.0;
	};

	template <>
	struct float_to_int_traits<double, uint64_t>
	{
		static constexpr double min = -0.9999999999999999;
		static constexpr double max = 18446744073709550000.0;
	};

	template <typename Float, typename Int>
	static bool float_to_int(Float in, Int& out)
	{
		static_assert(std::is_floating_point_v<Float>);
		static_assert(std::is_integral_v<Int>);
		SOUP_IF_UNLIKELY (std::isnan(in))
		{
			out = 0;
			return false;
		}
		SOUP_IF_UNLIKELY (in < float_to_int_traits<Float, Int>::min)
		{
			out = std::numeric_limits<Int>::min();
			return false;
		}
		SOUP_IF_UNLIKELY (in > float_to_int_traits<Float, Int>::max)
		{
			out = std::numeric_limits<Int>::max();
			return false;
		}
		out = static_cast<Int>(in);
		return true;
	}

#if SOUP_WASM_SIMD
	template <typename T, typename OutT, size_t S>
	[[nodiscard]] static bool simd_loadx(std::vector<WasmValue>& stack, Reader& r, WasmScript& script, OutT(WasmValue::* field)[S]) noexcept
	{
		WASM_CHECK_STACK(1);
		auto base = stack.back().uptr(); stack.pop_back();
		WASM_READ_MEMARG;
		auto memory = script.getMemoryByIndex(memidx);
		SOUP_RETHROW_FALSE(memory);
		auto ptr = (const T*)memory->getView(base + offset, sizeof(T) * S);
		SOUP_RETHROW_FALSE(ptr);
		auto& v = stack.emplace_back(WASM_V128);
		for (size_t i = 0; i != S; ++i)
		{
			(v.*field)[i] = static_cast<OutT>(ptr[i]);
		}
		return true;
	}

	template <typename T, size_t S>
	[[nodiscard]] static bool simd_load_splat(std::vector<WasmValue>& stack, Reader& r, WasmScript& script, T(WasmValue::* field)[S]) noexcept
	{
		WASM_CHECK_STACK(1);
		auto base = stack.back().uptr(); stack.pop_back();
		WASM_READ_MEMARG;
		auto memory = script.getMemoryByIndex(memidx);
		SOUP_RETHROW_FALSE(memory);
		auto ptr = memory->getPointer<T>(base, offset);
		SOUP_RETHROW_FALSE(ptr);
		auto& v = stack.emplace_back(WASM_V128);
		for (size_t i = 0; i != S; ++i)
		{
			(v.*field)[i] = *ptr;
		}
		return true;
	}

	template <typename T, size_t S>
	[[nodiscard]] static bool simd_splat(std::vector<WasmValue>& stack, T(WasmValue::* ptr)[S]) noexcept
	{
		WASM_CHECK_STACK(1);
		const auto value = stack.back().get<T>();
		for (auto& lane : stack.back().*ptr)
		{
			lane = value;
		}
		stack.back().type = WASM_V128;
		return true;
	}

	template <typename T, size_t S>
	[[nodiscard]] static bool extract_lane_s(std::vector<WasmValue>& stack, Reader& r, T(WasmValue::* ptr)[S]) noexcept
	{
		uint8_t idx; r.u8(idx);
		SOUP_RETHROW_FALSE(idx < S);
		WASM_CHECK_STACK(1);
		stack.back().i32 = static_cast<int32_t>((stack.back().*ptr)[idx]);
		stack.back().hi32 = 0;
		stack.back().type = WASM_I32;
		return true;
	}

	template <typename T, size_t S>
	[[nodiscard]] static bool extract_lane_u(std::vector<WasmValue>& stack, Reader& r, T(WasmValue::* ptr)[S]) noexcept
	{
		uint8_t idx; r.u8(idx);
		SOUP_RETHROW_FALSE(idx < S);
		WASM_CHECK_STACK(1);
		stack.back().i32 = static_cast<int32_t>(static_cast<uint32_t>(static_cast<std::make_unsigned_t<T>>((stack.back().*ptr)[idx])));
		stack.back().hi32 = 0;
		stack.back().type = WASM_I32;
		return true;
	}

	template <typename T, size_t S>
	[[nodiscard]] static bool extract_lane(std::vector<WasmValue>& stack, Reader& r, T(WasmValue::* ptr)[S]) noexcept
	{
		uint8_t idx; r.u8(idx);
		SOUP_RETHROW_FALSE(idx < S);
		WASM_CHECK_STACK(1);
		stack.back() = (stack.back().*ptr)[idx];
		return true;
	}

	template <typename T, size_t S>
	[[nodiscard]] static bool replace_lane(std::vector<WasmValue>& stack, Reader& r, T(WasmValue::* ptr)[S]) noexcept
	{
		uint8_t idx; r.u8(idx);
		SOUP_RETHROW_FALSE(idx < S);
		WASM_CHECK_STACK(1);
		const T value = stack.back().get<T>(); stack.pop_back();
		(stack.back().*ptr)[idx] = value;
		return true;
	}

	template <typename T, typename OutT, size_t S>
	[[nodiscard]] static bool simd_eq(std::vector<WasmValue>& stack, T(WasmValue::* ptr)[S], OutT(WasmValue::* outptr)[S]) noexcept
	{
		WASM_CHECK_STACK(2);
		T b[S]; memcpy(b, stack.back().*ptr, 16); stack.pop_back();
		auto& a = stack.back().*ptr;
		auto& out = stack.back().*outptr;
		for (size_t i = 0; i != S; ++i)
		{
			out[i] = (a[i] == b[i]) ? -1 : 0;
		}
		return true;
	}

	template <typename T, typename OutT, size_t S>
	[[nodiscard]] static bool simd_ne(std::vector<WasmValue>& stack, T(WasmValue::* ptr)[S], OutT(WasmValue::* outptr)[S]) noexcept
	{
		WASM_CHECK_STACK(2);
		T b[S]; memcpy(b, stack.back().*ptr, 16); stack.pop_back();
		auto& a = stack.back().*ptr;
		auto& out = stack.back().*outptr;
		for (size_t i = 0; i != S; ++i)
		{
			out[i] = (a[i] != b[i]) ? -1 : 0;
		}
		return true;
	}

	template <typename T, typename OutT, size_t S>
	[[nodiscard]] static bool simd_lt(std::vector<WasmValue>& stack, T(WasmValue::* ptr)[S], OutT(WasmValue::* outptr)[S]) noexcept
	{
		WASM_CHECK_STACK(2);
		T b[S]; memcpy(b, stack.back().*ptr, 16); stack.pop_back();
		auto& a = stack.back().*ptr;
		auto& out = stack.back().*outptr;
		for (size_t i = 0; i != S; ++i)
		{
			out[i] = (a[i] < b[i]) ? -1 : 0;
		}
		return true;
	}

	template <typename T, size_t S>
	[[nodiscard]] static bool simd_lt_u(std::vector<WasmValue>& stack, T(WasmValue::* ptr)[S]) noexcept
	{
		WASM_CHECK_STACK(2);
		T b[S]; memcpy(b, stack.back().*ptr, 16); stack.pop_back();
		auto& a = stack.back().*ptr;
		for (size_t i = 0; i != S; ++i)
		{
			a[i] = (static_cast<std::make_unsigned_t<T>>(a[i]) < static_cast<std::make_unsigned_t<T>>(b[i])) ? -1 : 0;
		}
		return true;
	}

	template <typename T, typename OutT, size_t S>
	[[nodiscard]] static bool simd_gt(std::vector<WasmValue>& stack, T(WasmValue::* ptr)[S], OutT(WasmValue::* outptr)[S]) noexcept
	{
		WASM_CHECK_STACK(2);
		T b[S]; memcpy(b, stack.back().*ptr, 16); stack.pop_back();
		auto& a = stack.back().*ptr;
		auto& out = stack.back().*outptr;
		for (size_t i = 0; i != S; ++i)
		{
			out[i] = (a[i] > b[i]) ? -1 : 0;
		}
		return true;
	}

	template <typename T, size_t S>
	[[nodiscard]] static bool simd_gt_u(std::vector<WasmValue>& stack, T(WasmValue::* ptr)[S]) noexcept
	{
		WASM_CHECK_STACK(2);
		T b[S]; memcpy(b, stack.back().*ptr, 16); stack.pop_back();
		auto& a = stack.back().*ptr;
		for (size_t i = 0; i != S; ++i)
		{
			a[i] = (static_cast<std::make_unsigned_t<T>>(a[i]) > static_cast<std::make_unsigned_t<T>>(b[i])) ? -1 : 0;
		}
		return true;
	}

	template <typename T, typename OutT, size_t S>
	[[nodiscard]] static bool simd_le(std::vector<WasmValue>& stack, T(WasmValue::* ptr)[S], OutT(WasmValue::* outptr)[S]) noexcept
	{
		WASM_CHECK_STACK(2);
		T b[S]; memcpy(b, stack.back().*ptr, 16); stack.pop_back();
		auto& a = stack.back().*ptr;
		auto& out = stack.back().*outptr;
		for (size_t i = 0; i != S; ++i)
		{
			out[i] = (a[i] <= b[i]) ? -1 : 0;
		}
		return true;
	}

	template <typename T, size_t S>
	[[nodiscard]] static bool simd_le_u(std::vector<WasmValue>& stack, T(WasmValue::* ptr)[S]) noexcept
	{
		WASM_CHECK_STACK(2);
		T b[S]; memcpy(b, stack.back().*ptr, 16); stack.pop_back();
		auto& a = stack.back().*ptr;
		for (size_t i = 0; i != S; ++i)
		{
			a[i] = (static_cast<std::make_unsigned_t<T>>(a[i]) <= static_cast<std::make_unsigned_t<T>>(b[i])) ? -1 : 0;
		}
		return true;
	}

	template <typename T, typename OutT, size_t S>
	[[nodiscard]] static bool simd_ge(std::vector<WasmValue>& stack, T(WasmValue::* ptr)[S], OutT(WasmValue::* outptr)[S]) noexcept
	{
		WASM_CHECK_STACK(2);
		T b[S]; memcpy(b, stack.back().*ptr, 16); stack.pop_back();
		auto& a = stack.back().*ptr;
		auto& out = stack.back().*outptr;
		for (size_t i = 0; i != S; ++i)
		{
			out[i] = (a[i] >= b[i]) ? -1 : 0;
		}
		return true;
	}

	template <typename T, size_t S>
	[[nodiscard]] static bool simd_ge_u(std::vector<WasmValue>& stack, T(WasmValue::* ptr)[S]) noexcept
	{
		WASM_CHECK_STACK(2);
		T b[S]; memcpy(b, stack.back().*ptr, 16); stack.pop_back();
		auto& a = stack.back().*ptr;
		for (size_t i = 0; i != S; ++i)
		{
			a[i] = (static_cast<std::make_unsigned_t<T>>(a[i]) >= static_cast<std::make_unsigned_t<T>>(b[i])) ? -1 : 0;
		}
		return true;
	}

	template <typename T, size_t S>
	[[nodiscard]] static bool load_lane(std::vector<WasmValue>& stack, Reader& r, WasmScript& script, T(WasmValue::* field)[S]) noexcept
	{
		WASM_CHECK_STACK(2);
		auto v = stack.back(); stack.pop_back();
		auto base = stack.back().uptr(); stack.pop_back();
		WASM_READ_MEMARG;
		auto memory = script.getMemoryByIndex(memidx);
		SOUP_RETHROW_FALSE(memory);
		auto ptr = memory->getPointer<T>(base, offset);
		SOUP_RETHROW_FALSE(ptr);
		uint8_t laneidx; r.u8(laneidx);
		SOUP_RETHROW_FALSE(laneidx < S);
		(v.*field)[laneidx] = *ptr;
		stack.emplace_back(v);
		return true;
	}

	template <typename T, size_t S>
	[[nodiscard]] static bool store_lane(std::vector<WasmValue>& stack, Reader& r, WasmScript& script, T(WasmValue::* field)[S]) noexcept
	{
		WASM_READ_MEMARG;
		uint8_t laneidx; r.u8(laneidx);
		SOUP_RETHROW_FALSE(laneidx < S);
		WASM_CHECK_STACK(2);
		auto value = (stack.back().*field)[laneidx]; stack.pop_back();
		auto base = stack.back().uptr(); stack.pop_back();
		auto memory = script.getMemoryByIndex(memidx);
		SOUP_RETHROW_FALSE(memory);
		auto ptr = memory->getPointer<T>(base, offset);
		SOUP_RETHROW_FALSE(ptr);
		*ptr = value;
		return true;
	}

	template <typename T, size_t S>
	[[nodiscard]] static bool load_zero(std::vector<WasmValue>& stack, Reader& r, WasmScript& script, T(WasmValue::* field)[S]) noexcept
	{
		WASM_CHECK_STACK(1);
		auto base = stack.back().uptr(); stack.pop_back();
		WASM_READ_MEMARG;
		auto memory = script.getMemoryByIndex(memidx);
		SOUP_RETHROW_FALSE(memory);
		auto ptr = memory->getPointer<T>(base, offset);
		SOUP_RETHROW_FALSE(ptr);
		memset(stack.emplace_back(WASM_V128).*field, 0, 16);
		(stack.back().*field)[0] = *ptr;
		return true;
	}

	template <typename T, size_t S>
	[[nodiscard]] static bool simd_abs(std::vector<WasmValue>& stack, T(WasmValue::* ptr)[S]) noexcept
	{
		WASM_CHECK_STACK(1);
		for (auto& lane : stack.back().*ptr)
		{
			lane = std::abs(lane);
		}
		return true;
	}

	template <typename T, size_t S>
	[[nodiscard]] static bool simd_neg(std::vector<WasmValue>& stack, T(WasmValue::* ptr)[S]) noexcept
	{
		WASM_CHECK_STACK(1);
		for (auto& lane : stack.back().*ptr)
		{
			lane = -lane;
		}
		return true;
	}

	template <typename T, size_t S>
	[[nodiscard]] static bool all_true(std::vector<WasmValue>& stack, T(WasmValue::* ptr)[S]) noexcept
	{
		WASM_CHECK_STACK(1);
		stack.back().type = WASM_I32;
		for (const auto& lane : stack.back().*ptr)
		{
			if (!lane)
			{
				stack.back().i64 = 0;
				return true;
			}
		}
		stack.back().i64 = 1;
		return true;
	}

	template <typename T, size_t S>
	[[nodiscard]] static bool simd_bitmask(std::vector<WasmValue>& stack, T(WasmValue::* ptr)[S]) noexcept
	{
		WASM_CHECK_STACK(1);
		uint32_t mask = 0;
		for (size_t i = 0; i != S; ++i)
		{
			if ((stack.back().*ptr)[i] < 0)
			{
				mask |= (1 << i);
			}
		}
		stack.back().i32 = mask;
		stack.back().hi32 = 0;
		stack.back().type = WASM_I32;
		return true;
	}

	template <typename OutT, typename T>
	[[nodiscard]] static OutT saturate(T in)
	{
		if (in < std::numeric_limits<OutT>::min())
		{
			return std::numeric_limits<OutT>::min();
		}
		if (in > std::numeric_limits<OutT>::max())
		{
			return std::numeric_limits<OutT>::max();
		}
		return static_cast<OutT>(in);
	}

	template <typename T, typename OutT, size_t S, size_t OutS>
	[[nodiscard]] static bool simd_narrow_s(std::vector<WasmValue>& stack, T(WasmValue::* ptr)[S], OutT(WasmValue::* outptr)[OutS]) noexcept
	{
		static_assert(S * 2 == OutS);
		WASM_CHECK_STACK(2);
		T b[S]; memcpy(b, stack.back().*ptr, 16); stack.pop_back();
		const auto a = stack.back().*ptr;
		OutT* out = stack.back().*outptr;
		for (size_t i = 0; i != S; ++i)
		{
			out[i] = saturate<OutT>(a[i]);
		}
		for (size_t i = 0; i != S; ++i)
		{
			out[S + i] = saturate<OutT>(b[i]);
		}
		return true;
	}

	template <typename T, typename OutT, size_t S, size_t OutS>
	[[nodiscard]] static bool simd_narrow_u(std::vector<WasmValue>& stack, T(WasmValue::* ptr)[S], OutT(WasmValue::* outptr)[OutS]) noexcept
	{
		static_assert(S * 2 == OutS);
		WASM_CHECK_STACK(2);
		T b[S]; memcpy(b, stack.back().*ptr, 16); stack.pop_back();
		const auto a = stack.back().*ptr;
		OutT* out = stack.back().*outptr;
		for (size_t i = 0; i != S; ++i)
		{
			out[i] = saturate<std::make_unsigned_t<OutT>>(a[i]);
		}
		for (size_t i = 0; i != S; ++i)
		{
			out[S + i] = saturate<std::make_unsigned_t<OutT>>(b[i]);
		}
		return true;
	}

	template <typename T, typename OutT, size_t S, size_t OutS>
	[[nodiscard]] static bool simd_extend_low_s(std::vector<WasmValue>& stack, T(WasmValue::* ptr)[S], OutT(WasmValue::* outptr)[OutS]) noexcept
	{
		static_assert(OutS * 2 == S);
		WASM_CHECK_STACK(1);
		T in[S]; memcpy(in, stack.back().*ptr, 16);
		OutT* out = stack.back().*outptr;
		for (size_t i = 0; i != OutS; ++i)
		{
			out[i] = static_cast<OutT>(in[i]);
		}
		return true;
	}

	template <typename T, typename OutT, size_t S, size_t OutS>
	[[nodiscard]] static bool simd_extend_high_s(std::vector<WasmValue>& stack, T(WasmValue::* ptr)[S], OutT(WasmValue::* outptr)[OutS]) noexcept
	{
		static_assert(OutS * 2 == S);
		WASM_CHECK_STACK(1);
		T in[S]; memcpy(in, stack.back().*ptr, 16);
		OutT* out = stack.back().*outptr;
		for (size_t i = 0; i != OutS; ++i)
		{
			out[i] = static_cast<OutT>(in[OutS + i]);
		}
		return true;
	}

	template <typename T, typename OutT, size_t S, size_t OutS>
	[[nodiscard]] static bool simd_extend_low_u(std::vector<WasmValue>& stack, T(WasmValue::* ptr)[S], OutT(WasmValue::* outptr)[OutS]) noexcept
	{
		static_assert(OutS * 2 == S);
		WASM_CHECK_STACK(1);
		T in[S]; memcpy(in, stack.back().*ptr, 16);
		OutT* out = stack.back().*outptr;
		for (size_t i = 0; i != OutS; ++i)
		{
			out[i] = static_cast<std::make_unsigned_t<OutT>>(static_cast<std::make_unsigned_t<T>>(in[i]));
		}
		return true;
	}

	template <typename T, typename OutT, size_t S, size_t OutS>
	[[nodiscard]] static bool simd_extend_high_u(std::vector<WasmValue>& stack, T(WasmValue::* ptr)[S], OutT(WasmValue::* outptr)[OutS]) noexcept
	{
		static_assert(OutS * 2 == S);
		WASM_CHECK_STACK(1);
		T in[S]; memcpy(in, stack.back().*ptr, 16);
		OutT* out = stack.back().*outptr;
		for (size_t i = 0; i != OutS; ++i)
		{
			out[i] = static_cast<std::make_unsigned_t<OutT>>(static_cast<std::make_unsigned_t<T>>(in[OutS + i]));
		}
		return true;
	}

	template <typename T, size_t S>
	[[nodiscard]] static bool simd_shl(std::vector<WasmValue>& stack, T(WasmValue::* ptr)[S]) noexcept
	{
		WASM_CHECK_STACK(2);
		const auto b = static_cast<uint32_t>(stack.back().i32) % (sizeof(T) * 8); stack.pop_back();
		for (auto& lane : stack.back().*ptr)
		{
			lane <<= b;
		}
		return true;
	}

	template <typename T, size_t S>
	[[nodiscard]] static bool simd_shr_s(std::vector<WasmValue>& stack, T(WasmValue::* ptr)[S]) noexcept
	{
		WASM_CHECK_STACK(2);
		const auto b = static_cast<uint32_t>(stack.back().i32) % (sizeof(T) * 8); stack.pop_back();
		for (auto& lane : stack.back().*ptr)
		{
			lane >>= b;
		}
		return true;
	}

	template <typename T, size_t S>
	[[nodiscard]] static bool simd_shr_u(std::vector<WasmValue>& stack, T(WasmValue::* ptr)[S]) noexcept
	{
		WASM_CHECK_STACK(2);
		const auto b = static_cast<uint32_t>(stack.back().i32) % (sizeof(T) * 8); stack.pop_back();
		for (auto& lane : stack.back().*ptr)
		{
			lane = static_cast<std::make_unsigned_t<T>>(lane) >> b;
		}
		return true;
	}

	template <typename T, size_t S>
	[[nodiscard]] static bool simd_add(std::vector<WasmValue>& stack, T(WasmValue::* ptr)[S]) noexcept
	{
		WASM_CHECK_STACK(2);
		T b[S]; memcpy(b, stack.back().*ptr, 16); stack.pop_back();
		T* a = stack.back().*ptr;
		for (size_t i = 0; i != S; ++i)
		{
			a[i] += b[i];
		}
		return true;
	}

	template <typename T, size_t S>
	[[nodiscard]] static bool simd_add_sat_s(std::vector<WasmValue>& stack, T(WasmValue::* ptr)[S]) noexcept
	{
		WASM_CHECK_STACK(2);
		T b[S]; memcpy(b, stack.back().*ptr, 16); stack.pop_back();
		T* a = stack.back().*ptr;
		for (size_t i = 0; i != S; ++i)
		{
			a[i] = saturate<T>(static_cast<int32_t>(a[i]) + static_cast<int32_t>(b[i]));
		}
		return true;
	}

	template <typename T, size_t S>
	[[nodiscard]] static bool simd_add_sat_u(std::vector<WasmValue>& stack, T(WasmValue::* ptr)[S]) noexcept
	{
		WASM_CHECK_STACK(2);
		T b[S]; memcpy(b, stack.back().*ptr, 16); stack.pop_back();
		T* a = stack.back().*ptr;
		for (size_t i = 0; i != S; ++i)
		{
			a[i] = saturate<std::make_unsigned_t<T>>(static_cast<int32_t>(static_cast<std::make_unsigned_t<T>>(a[i])) + static_cast<int32_t>(static_cast<std::make_unsigned_t<T>>(b[i])));
		}
		return true;
	}

	template <typename T, size_t S>
	[[nodiscard]] static bool simd_sub(std::vector<WasmValue>& stack, T(WasmValue::* ptr)[S]) noexcept
	{
		WASM_CHECK_STACK(2);
		T b[S]; memcpy(b, stack.back().*ptr, 16); stack.pop_back();
		T* a = stack.back().*ptr;
		for (size_t i = 0; i != S; ++i)
		{
			a[i] -= b[i];
		}
		return true;
	}

	template <typename T, size_t S>
	[[nodiscard]] static bool simd_sub_sat_s(std::vector<WasmValue>& stack, T(WasmValue::* ptr)[S]) noexcept
	{
		WASM_CHECK_STACK(2);
		T b[S]; memcpy(b, stack.back().*ptr, 16); stack.pop_back();
		T* a = stack.back().*ptr;
		for (size_t i = 0; i != S; ++i)
		{
			a[i] = saturate<T>(static_cast<int32_t>(a[i]) - static_cast<int32_t>(b[i]));
		}
		return true;
	}

	template <typename T, size_t S>
	[[nodiscard]] static bool simd_sub_sat_u(std::vector<WasmValue>& stack, T(WasmValue::* ptr)[S]) noexcept
	{
		WASM_CHECK_STACK(2);
		T b[S]; memcpy(b, stack.back().*ptr, 16); stack.pop_back();
		T* a = stack.back().*ptr;
		for (size_t i = 0; i != S; ++i)
		{
			a[i] = saturate<std::make_unsigned_t<T>>(static_cast<int32_t>(static_cast<std::make_unsigned_t<T>>(a[i])) - static_cast<int32_t>(static_cast<std::make_unsigned_t<T>>(b[i])));
		}
		return true;
	}

	template <typename T, size_t S>
	[[nodiscard]] static bool simd_pmin(std::vector<WasmValue>& stack, T(WasmValue::* ptr)[S]) noexcept
	{
		WASM_CHECK_STACK(2);
		T b[S]; memcpy(b, stack.back().*ptr, 16); stack.pop_back();
		T* a = stack.back().*ptr;
		for (size_t i = 0; i != S; ++i)
		{
			a[i] = std::min(a[i], b[i]);
		}
		return true;
	}

	template <typename T, size_t S>
	[[nodiscard]] static bool simd_min_u(std::vector<WasmValue>& stack, T(WasmValue::* ptr)[S]) noexcept
	{
		WASM_CHECK_STACK(2);
		T b[S]; memcpy(b, stack.back().*ptr, 16); stack.pop_back();
		T* a = stack.back().*ptr;
		for (size_t i = 0; i != S; ++i)
		{
			a[i] = std::min(static_cast<std::make_unsigned_t<T>>(a[i]), static_cast<std::make_unsigned_t<T>>(b[i]));
		}
		return true;
	}

	template <typename T, size_t S>
	[[nodiscard]] static bool simd_pmax(std::vector<WasmValue>& stack, T(WasmValue::* ptr)[S]) noexcept
	{
		WASM_CHECK_STACK(2);
		T b[S]; memcpy(b, stack.back().*ptr, 16); stack.pop_back();
		T* a = stack.back().*ptr;
		for (size_t i = 0; i != S; ++i)
		{
			a[i] = std::max(a[i], b[i]);
		}
		return true;
	}

	template <typename T, size_t S>
	[[nodiscard]] static bool simd_max_u(std::vector<WasmValue>& stack, T(WasmValue::* ptr)[S]) noexcept
	{
		WASM_CHECK_STACK(2);
		T b[S]; memcpy(b, stack.back().*ptr, 16); stack.pop_back();
		T* a = stack.back().*ptr;
		for (size_t i = 0; i != S; ++i)
		{
			a[i] = std::max(static_cast<std::make_unsigned_t<T>>(a[i]), static_cast<std::make_unsigned_t<T>>(b[i]));
		}
		return true;
	}

	template <typename T, size_t S>
	[[nodiscard]] static bool simd_avgr_u(std::vector<WasmValue>& stack, T(WasmValue::* ptr)[S]) noexcept
	{
		WASM_CHECK_STACK(2);
		T b[S]; memcpy(b, stack.back().*ptr, 16); stack.pop_back();
		T* a = stack.back().*ptr;
		for (size_t i = 0; i != S; ++i)
		{
			a[i] = (static_cast<std::make_unsigned_t<T>>(a[i]) + static_cast<std::make_unsigned_t<T>>(b[i]) + 1) >> 1;
		}
		return true;
	}

	template <typename T, size_t S>
	[[nodiscard]] static bool simd_mul(std::vector<WasmValue>& stack, T(WasmValue::* ptr)[S]) noexcept
	{
		WASM_CHECK_STACK(2);
		T b[S]; memcpy(b, stack.back().*ptr, 16); stack.pop_back();
		T* a = stack.back().*ptr;
		for (size_t i = 0; i != S; ++i)
		{
			a[i] *= b[i];
		}
		return true;
	}

	template <typename T, size_t S>
	[[nodiscard]] static bool simd_div(std::vector<WasmValue>& stack, T(WasmValue::* ptr)[S]) noexcept
	{
		WASM_CHECK_STACK(2);
		T b[S]; memcpy(b, stack.back().*ptr, 16); stack.pop_back();
		T* a = stack.back().*ptr;
		for (size_t i = 0; i != S; ++i)
		{
			a[i] /= b[i]; static_assert(std::is_floating_point_v<T>);
		}
		return true;
	}

	template <typename T, size_t S>
	[[nodiscard]] static bool simd_ceil(std::vector<WasmValue>& stack, T(WasmValue::* ptr)[S]) noexcept
	{
		WASM_CHECK_STACK(1);
		for (auto& lane : stack.back().*ptr)
		{
			lane = std::ceil(lane);
		}
		return true;
	}

	template <typename T, size_t S>
	[[nodiscard]] static bool simd_floor(std::vector<WasmValue>& stack, T(WasmValue::* ptr)[S]) noexcept
	{
		WASM_CHECK_STACK(1);
		for (auto& lane : stack.back().*ptr)
		{
			lane = std::floor(lane);
		}
		return true;
	}

	template <typename T, size_t S>
	[[nodiscard]] static bool simd_trunc(std::vector<WasmValue>& stack, T(WasmValue::* ptr)[S]) noexcept
	{
		WASM_CHECK_STACK(1);
		for (auto& lane : stack.back().*ptr)
		{
			lane = std::trunc(lane);
		}
		return true;
	}

	template <typename T, size_t S>
	[[nodiscard]] static bool simd_nearest(std::vector<WasmValue>& stack, T(WasmValue::* ptr)[S]) noexcept
	{
		WASM_CHECK_STACK(1);
		for (auto& lane : stack.back().*ptr)
		{
			lane = std::nearbyint(lane);
		}
		return true;
	}

	template <typename T, size_t S>
	[[nodiscard]] static bool simd_sqrt(std::vector<WasmValue>& stack, T(WasmValue::* ptr)[S]) noexcept
	{
		WASM_CHECK_STACK(1);
		for (auto& lane : stack.back().*ptr)
		{
			lane = std::sqrt(lane);
		}
		return true;
	}

	template <typename T, size_t S>
	[[nodiscard]] static bool simd_min(std::vector<WasmValue>& stack, T(WasmValue::* ptr)[S]) noexcept
	{
		static_assert(std::is_floating_point_v<T>);
		WASM_CHECK_STACK(2);
		T b[S]; memcpy(b, stack.back().*ptr, 16); stack.pop_back();
		T* a = stack.back().*ptr;
		for (size_t i = 0; i != S; ++i)
		{
			a[i] = wasm_fmin(a[i], b[i]);
		}
		return true;
	}

	template <typename T, size_t S>
	[[nodiscard]] static bool simd_max(std::vector<WasmValue>& stack, T(WasmValue::* ptr)[S]) noexcept
	{
		static_assert(std::is_floating_point_v<T>);
		WASM_CHECK_STACK(2);
		T b[S]; memcpy(b, stack.back().*ptr, 16); stack.pop_back();
		T* a = stack.back().*ptr;
		for (size_t i = 0; i != S; ++i)
		{
			a[i] = wasm_fmax(a[i], b[i]);
		}
		return true;
	}

	template <typename T, typename OutT, size_t S, size_t OutS>
	[[nodiscard]] static bool extmul_low_s(std::vector<WasmValue>& stack, T(WasmValue::* ptr)[S], OutT(WasmValue::* outptr)[OutS]) noexcept
	{
		static_assert(OutS * 2 == S);
		WASM_CHECK_STACK(2);
		T b[S]; memcpy(b, stack.back().*ptr, 16); stack.pop_back();
		T a[S]; memcpy(a, stack.back().*ptr, 16);
		OutT* out = stack.back().*outptr;
		for (size_t i = 0; i != OutS; ++i)
		{
			auto av = static_cast<OutT>(a[i]);
			auto bv = static_cast<OutT>(b[i]);
			out[i] = av * bv;
		}
		return true;
	}

	template <typename T, typename OutT, size_t S, size_t OutS>
	[[nodiscard]] static bool extmul_low_u(std::vector<WasmValue>& stack, T(WasmValue::* ptr)[S], OutT(WasmValue::* outptr)[OutS]) noexcept
	{
		static_assert(OutS * 2 == S);
		WASM_CHECK_STACK(2);
		T b[S]; memcpy(b, stack.back().*ptr, 16); stack.pop_back();
		T a[S]; memcpy(a, stack.back().*ptr, 16);
		OutT* out = stack.back().*outptr;
		for (size_t i = 0; i != OutS; ++i)
		{
			auto av = static_cast<std::make_unsigned_t<OutT>>(static_cast<std::make_unsigned_t<T>>(a[i]));
			auto bv = static_cast<std::make_unsigned_t<OutT>>(static_cast<std::make_unsigned_t<T>>(b[i]));
			out[i] = av * bv;
		}
		return true;
	}

	template <typename T, typename OutT, size_t S, size_t OutS>
	[[nodiscard]] static bool extmul_high_s(std::vector<WasmValue>& stack, T(WasmValue::* ptr)[S], OutT(WasmValue::* outptr)[OutS]) noexcept
	{
		static_assert(OutS * 2 == S);
		WASM_CHECK_STACK(2);
		T b[S]; memcpy(b, stack.back().*ptr, 16); stack.pop_back();
		T a[S]; memcpy(a, stack.back().*ptr, 16);
		OutT* out = stack.back().*outptr;
		for (size_t i = 0; i != OutS; ++i)
		{
			auto av = static_cast<OutT>(a[OutS + i]);
			auto bv = static_cast<OutT>(b[OutS + i]);
			out[i] = av * bv;
		}
		return true;
	}
	
	template <typename T, typename OutT, size_t S, size_t OutS>
	[[nodiscard]] static bool extmul_high_u(std::vector<WasmValue>& stack, T(WasmValue::* ptr)[S], OutT(WasmValue::* outptr)[OutS]) noexcept
	{
		static_assert(OutS * 2 == S);
		WASM_CHECK_STACK(2);
		T b[S]; memcpy(b, stack.back().*ptr, 16); stack.pop_back();
		T a[S]; memcpy(a, stack.back().*ptr, 16);
		auto& out = stack.back().*outptr;
		for (size_t i = 0; i != OutS; ++i)
		{
			auto av = static_cast<std::make_unsigned_t<OutT>>(static_cast<std::make_unsigned_t<T>>(a[OutS + i]));
			auto bv = static_cast<std::make_unsigned_t<OutT>>(static_cast<std::make_unsigned_t<T>>(b[OutS + i]));
			out[i] = av * bv;
		}
		return true;
	}

	template <typename T, size_t S>
	[[nodiscard]] static bool simd_popcnt(std::vector<WasmValue>& stack, T(WasmValue::* ptr)[S]) noexcept
	{
		WASM_CHECK_STACK(1);
		for (auto& lane : stack.back().*ptr)
		{
			lane = bitutil::getNumSetBits(lane);
		}
		return true;
	}

	template <typename T, typename OutT, size_t S, size_t OutS>
	[[nodiscard]] static bool extadd_pairwise_s(std::vector<WasmValue>& stack, T(WasmValue::* ptr)[S], OutT(WasmValue::* outptr)[OutS]) noexcept
	{
		static_assert(OutS * 2 == S);
		WASM_CHECK_STACK(1);
		auto& in = stack.back().*ptr;
		auto& out = stack.back().*outptr;
		for (size_t i = 0; i != OutS; ++i)
		{
			out[i] = static_cast<OutT>(in[i * 2]) + static_cast<OutT>(in[i * 2 + 1]);
		}
		return true;
	}

	template <typename T, typename OutT, size_t S, size_t OutS>
	[[nodiscard]] static bool extadd_pairwise_u(std::vector<WasmValue>& stack, T(WasmValue::* ptr)[S], OutT(WasmValue::* outptr)[OutS]) noexcept
	{
		static_assert(OutS * 2 == S);
		WASM_CHECK_STACK(1);
		auto& in = stack.back().*ptr;
		auto& out = stack.back().*outptr;
		for (size_t i = 0; i != OutS; ++i)
		{
			out[i] = static_cast<std::make_unsigned_t<OutT>>(static_cast<std::make_unsigned_t<T>>(in[i * 2])) + static_cast<std::make_unsigned_t<OutT>>(static_cast<std::make_unsigned_t<T>>(in[i * 2 + 1]));
		}
		return true;
	}
#endif

	WasmVm::RunCodeResult WasmVm::runCode(Reader& r, unsigned depth, uint32_t func_index)
	{
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
				return CODE_ERROR;

			case 0x00: // unreachable
#if DEBUG_VM
				std::cout << "unreachable\n";
#endif
				return CODE_ERROR;

			case 0x01: // nop
				break;

			case 0x02: // block
				{
					int32_t result_type; WASM_READ_SOML(result_type);
					uint32_t stack_size = stack.size();
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
					ctrlflow.emplace(CtrlFlowEntry{ (uint32_t)-1, stack_size, num_values });
#if DEBUG_VM
					std::cout << "block at position " << r.getPosition() << " with stack size " << stack.size() << " + " << num_values << "\n";
#endif
				}
				break;

			case 0x03: // loop
				{
					int32_t result_type; WASM_READ_SOML(result_type);
					uint32_t stack_size = stack.size();
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
					ctrlflow.emplace(CtrlFlowEntry{ (uint32_t)r.getPosition(), stack_size, num_values });
#if DEBUG_VM
					std::cout << "loop at position " << r.getPosition() << " with stack size " << stack.size() << " + " << num_values << "\n";
#endif
				}
				break;

			case 0x04: // if
				{
					int32_t result_type; WASM_READ_SOML(result_type);
					uint32_t stack_size = stack.size();
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
					auto value = stack.back().i32; stack.pop_back();
					//std::cout << "if: condition is " << (value.i32 ? "true" : "false") << "\n";
					if (value)
					{
						ctrlflow.emplace(CtrlFlowEntry{ (uint32_t)-1, stack_size, num_values });
					}
					else
					{
						if (skipOverBranch(r, 0, script, func_index) == ELSE_REACHED)
						{
							// we're in the 'else' branch
							ctrlflow.emplace(CtrlFlowEntry{ (uint32_t)-1, stack_size, num_values });
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
					return CODE_RETURN;
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
						return CODE_ERROR;
					}
				}
				break;

			case 0x0d: // br_if
				{
					uint32_t depth;
					WASM_READ_OML(depth);
					WASM_CHECK_STACK(1);
					auto value = stack.back().i32; stack.pop_back();
					if (value)
					{
						SOUP_IF_UNLIKELY (!doBranch(r, depth, func_index, ctrlflow))
						{
							return CODE_ERROR;
						}
					}
				}
				break;

			case 0x0e: // br_table
				{
					uint32_t num_branches;
					WASM_READ_OML(num_branches);
					WASM_CHECK_STACK(1);
					auto index = static_cast<uint32_t>(stack.back().i32); stack.pop_back();
					if (index >= num_branches)
					{
						index = num_branches;
					}
					uint32_t depth;
					for (uint32_t i = 0; i != index; ++i)
					{
						WASM_READ_OML(depth);
					}
					WASM_READ_OML(depth);
					uint32_t unused_depth;
					for (uint32_t i = index; i != num_branches; ++i)
					{
						WASM_READ_OML(unused_depth);
					}
					SOUP_IF_UNLIKELY (!doBranch(r, depth, func_index, ctrlflow))
					{
						return CODE_ERROR;
					}
				}
				break;

			case 0x0f: // return
				return CODE_RETURN;

			case 0x10: // call
				{
					uint32_t function_index;
					WASM_READ_OML(function_index);
					uint32_t type_index = script.getTypeIndexForFunction(function_index);
					SOUP_RETHROW_FALSE(type_index != -1);
					const auto result = doCall(&this->script, type_index, function_index, depth);
					SOUP_IF_UNLIKELY (result != CODE_RETURN)
					{
#if SOUP_WASM_EXCEPTIONS
						if (result != CODE_THROW || !doThrow(r, func_index, ctrlflow))
#endif
						{
							return result;
						}
					}
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
						return CODE_ERROR;
					}
					uint32_t table_index; WASM_READ_OML(table_index);
					auto table = script.getTableByIndex(table_index);
					SOUP_IF_UNLIKELY (!table)
					{
#if DEBUG_VM
						std::cout << "call: invalid table index\n";
#endif
						return CODE_ERROR;
					}
					SOUP_IF_UNLIKELY (table->type != WASM_FUNCREF)
					{
#if DEBUG_VM
						std::cout << "call: indexing non-funcref table\n";
#endif
						return CODE_ERROR;
					}
					WASM_CHECK_STACK(1);
					auto element_index = static_cast<uint32_t>(stack.back().i32); stack.pop_back();
					SOUP_IF_UNLIKELY (element_index >= table->values.size())
					{
#if DEBUG_VM
						std::cout << "call: element is out-of-bounds\n";
#endif
						return CODE_ERROR;
					}
					SOUP_IF_UNLIKELY (table->values[element_index] == 0)
					{
#if DEBUG_VM
						std::cout << "indirect call to null\n";
#endif
						return CODE_ERROR;
					}
					const auto& funcref = script.shared_env->getFuncRef(table->values[element_index]);
					/*SOUP_IF_UNLIKELY (funcref.isC())
					{
						funcref.ptr(*this, funcref.user_data, script.types[type_index]);
					}
					else*/
					{
						const auto result = doCall(funcref.source, type_index, funcref.index, depth);
						SOUP_IF_UNLIKELY (result != CODE_RETURN)
						{
#if SOUP_WASM_EXCEPTIONS
							if (result != CODE_THROW || !doThrow(r, func_index, ctrlflow))
#endif
							{
								return result;
							}
						}
					}
				}
				break;

#if SOUP_WASM_TAIL_CALL
			case 0x12: // return_call
				return CODE_RETURN_CALL;

			case 0x13: // return_call_indirect
				return CODE_RETURN_CALL_INDIRECT;
#endif

			case 0x1a: // drop
				WASM_CHECK_STACK(1);
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

#if SOUP_WASM_EXCEPTIONS
			case 0x1f: // try_table
				{
					int32_t result_type; WASM_READ_SOML(result_type);
					uint32_t stack_size = stack.size();
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
					ctrlflow.emplace(CtrlFlowEntry{ (uint32_t)r.getPosition(), stack_size, num_values, true });
					uint32_t num_catches; WASM_READ_OML(num_catches);
#if DEBUG_VM
					std::cout << "try_table: result_type=" << result_type << ", num_catches=" << num_catches << "\n";
#endif
					while (num_catches--)
					{
						uint8_t type; r.u8(type);
#if DEBUG_VM
						std::cout << "try_table: catch type: " << (int)type << "\n";
#endif
						if (type < 0x02) { uint32_t tagidx; WASM_READ_OML(tagidx); }
						uint32_t labelidx; WASM_READ_OML(labelidx);
					}
				}
				break;

			case 0x08: // throw
				{
					uint32_t tagidx;
					WASM_READ_OML(tagidx);
#if DEBUG_VM
					std::cout << "throw: tagidx=" << tagidx << "\n";
#endif
					SOUP_RETHROW_FALSE(tagidx < script.tags.size());
					current_throw_tag = script.tags[tagidx].get();
					if (!doThrow(r, func_index, ctrlflow))
					{
						return CODE_THROW;
					}
				}
				break;

			case 0x0a: // throw_ref
				WASM_CHECK_STACK(1);
				current_throw_tag = reinterpret_cast<WasmScript::Tag*>(stack.back().i64);
				stack.pop_back();
				if (!doThrow(r, func_index, ctrlflow))
				{
					return CODE_THROW;
				}
				break;
#endif

			case 0x20: // local.get
				{
					uint32_t local_index;
					WASM_READ_OML(local_index);
					SOUP_IF_UNLIKELY (local_index >= locals.size())
					{
#if DEBUG_VM
						std::cout << "local.get: index is out-of-bounds: " << local_index << "\n";
#endif
						return CODE_ERROR;
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
						return CODE_ERROR;
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
						return CODE_ERROR;
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
						return CODE_ERROR;
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
						return CODE_ERROR;
					}
					WASM_CHECK_STACK(1);
					SOUP_IF_UNLIKELY (stack.back().type != global->type)
					{
#if DEBUG_VM
						std::cout << "global.set: type mismatch\n";
#endif
						return CODE_ERROR;
					}
#if SOUP_WASM_SIMD
					memcpy(global->i8x16, stack.back().i8x16, 16);
#else
					global->i64 = stack.back().i64;
#endif
					stack.pop_back();
				}
				break;

			case 0x25: // table.get
				{
					uint32_t table_index;
					WASM_READ_OML(table_index);
					auto table = script.getTableByIndex(table_index);
					SOUP_IF_UNLIKELY (!table)
					{
#if DEBUG_VM
						std::cout << "table.get: invalid table index (" << table_index << ")\n";
#endif
						return CODE_ERROR;
					}
					WASM_CHECK_STACK(1);
					auto elem_index = stack.back().uptr(); stack.pop_back();
					SOUP_IF_UNLIKELY (elem_index >= table->values.size())
					{
#if DEBUG_VM
						std::cout << "table.get: element index " << elem_index << " >= " << table->values.size() << "\n";
#endif
						return CODE_ERROR;
					}
					stack.emplace_back(table->type).i64 = table->values[elem_index];
				}
				break;

			case 0x26: // table.set
				{
					uint32_t table_index;
					WASM_READ_OML(table_index);
					auto table = script.getTableByIndex(table_index);
					SOUP_IF_UNLIKELY (!table)
					{
#if DEBUG_VM
						std::cout << "table.set: invalid table index (" << table_index << ")\n";
#endif
						return CODE_ERROR;
					}
					WASM_CHECK_STACK(2);
					auto value = stack.back(); stack.pop_back();
					auto elem_index = stack.back().uptr(); stack.pop_back();
					SOUP_IF_UNLIKELY (elem_index >= table->values.size())
					{
#if DEBUG_VM
						std::cout << "table.set: element index " << elem_index << " >= " << table->values.size() << "\n";
#endif
						return CODE_ERROR;
					}
					SOUP_IF_UNLIKELY (value.type != table->type)
					{
#if DEBUG_VM
						std::cout << "table.set: value type doesn't match table's element type\n";
#endif
						return CODE_ERROR;
					}
					table->values[elem_index] = value.i64;
				}
				break;

			case 0x28: // i32.load
				{
					WASM_CHECK_STACK(1);
					auto base = stack.back().uptr();
					WASM_READ_MEMARG;
					auto memory = script.getMemoryByIndex(memidx);
					SOUP_IF_UNLIKELY (!memory)
					{
#if DEBUG_VM
						std::cout << "memory access without a memory\n";
#endif
						return CODE_ERROR;
					}
					SOUP_IF_LIKELY (auto ptr = memory->getPointer<int32_t>(base, offset))
					{
						stack.back() = *ptr;
					}
					else
					{
#if DEBUG_VM
						std::cout << "memory access out of bounds: " << base << " + " << offset << "\n";
#endif
						return CODE_ERROR;
					}
				}
				break;

			case 0x29: // i64.load
				{
					WASM_CHECK_STACK(1);
					auto base = stack.back().uptr();
					WASM_READ_MEMARG;
					auto memory = script.getMemoryByIndex(memidx);
					SOUP_IF_UNLIKELY (!memory)
					{
#if DEBUG_VM
						std::cout << "memory access without a memory\n";
#endif
						return CODE_ERROR;
					}
					SOUP_IF_LIKELY (auto ptr = memory->getPointer<int64_t>(base, offset))
					{
						stack.back() = *ptr;
					}
					else
					{
#if DEBUG_VM
						std::cout << "memory access out of bounds: " << base << " + " << offset << "\n";
#endif
						return CODE_ERROR;
					}
				}
				break;

			case 0x2a: // f32.load
				{
					WASM_CHECK_STACK(1);
					auto base = stack.back().uptr();
					WASM_READ_MEMARG;
					auto memory = script.getMemoryByIndex(memidx);
					SOUP_IF_UNLIKELY (!memory)
					{
#if DEBUG_VM
						std::cout << "memory access without a memory\n";
#endif
						return CODE_ERROR;
					}
					SOUP_IF_LIKELY (auto ptr = memory->getPointer<float>(base, offset))
					{
						stack.back() = *ptr;
					}
					else
					{
#if DEBUG_VM
						std::cout << "memory access out of bounds: " << base << " + " << offset << "\n";
#endif
						return CODE_ERROR;
					}
				}
				break;

			case 0x2b: // f64.load
				{
					WASM_CHECK_STACK(1);
					auto base = stack.back().uptr();
					WASM_READ_MEMARG;
					auto memory = script.getMemoryByIndex(memidx);
					SOUP_IF_UNLIKELY (!memory)
					{
#if DEBUG_VM
						std::cout << "memory access without a memory\n";
#endif
						return CODE_ERROR;
					}
					SOUP_IF_LIKELY (auto ptr = memory->getPointer<double>(base, offset))
					{
						stack.back() = *ptr;
					}
					else
					{
#if DEBUG_VM
						std::cout << "memory access out of bounds: " << base << " + " << offset << "\n";
#endif
						return CODE_ERROR;
					}
				}
				break;

			case 0x2c: // i32.load8_s
				{
					WASM_CHECK_STACK(1);
					auto base = stack.back().uptr();
					WASM_READ_MEMARG;
					auto memory = script.getMemoryByIndex(memidx);
					SOUP_IF_UNLIKELY (!memory)
					{
#if DEBUG_VM
						std::cout << "memory access without a memory\n";
#endif
						return CODE_ERROR;
					}
					SOUP_IF_LIKELY (auto ptr = memory->getPointer<int8_t>(base, offset))
					{
						stack.back() = static_cast<int32_t>(*ptr);
					}
					else
					{
#if DEBUG_VM
						std::cout << "memory access out of bounds: " << base << " + " << offset << "\n";
#endif
						return CODE_ERROR;
					}
				}
				break;

			case 0x2d: // i32.load8_u
				{
					WASM_CHECK_STACK(1);
					auto base = stack.back().uptr();
					WASM_READ_MEMARG;
					auto memory = script.getMemoryByIndex(memidx);
					SOUP_IF_UNLIKELY (!memory)
					{
#if DEBUG_VM
						std::cout << "memory access without a memory\n";
#endif
						return CODE_ERROR;
					}
					SOUP_IF_LIKELY (auto ptr = memory->getPointer<uint8_t>(base, offset))
					{
						stack.back() = static_cast<uint32_t>(*ptr);
					}
					else
					{
#if DEBUG_VM
						std::cout << "memory access out of bounds: " << base << " + " << offset << "\n";
#endif
						return CODE_ERROR;
					}
				}
				break;

			case 0x2e: // i32.load16_s
				{
					WASM_CHECK_STACK(1);
					auto base = stack.back().uptr();
					WASM_READ_MEMARG;
					auto memory = script.getMemoryByIndex(memidx);
					SOUP_IF_UNLIKELY (!memory)
					{
#if DEBUG_VM
						std::cout << "memory access without a memory\n";
#endif
						return CODE_ERROR;
					}
					SOUP_IF_LIKELY (auto ptr = memory->getPointer<int16_t>(base, offset))
					{
						stack.back() = static_cast<uint32_t>(*ptr);
					}
					else
					{
#if DEBUG_VM
						std::cout << "memory access out of bounds: " << base << " + " << offset << "\n";
#endif
						return CODE_ERROR;
					}
				}
				break;

			case 0x2f: // i32.load16_u
				{
					WASM_CHECK_STACK(1);
					auto base = stack.back().uptr();
					WASM_READ_MEMARG;
					auto memory = script.getMemoryByIndex(memidx);
					SOUP_IF_UNLIKELY (!memory)
					{
#if DEBUG_VM
						std::cout << "memory access without a memory\n";
#endif
						return CODE_ERROR;
					}
					SOUP_IF_LIKELY (auto ptr = memory->getPointer<uint16_t>(base, offset))
					{
						stack.back() = static_cast<uint32_t>(*ptr);
					}
					else
					{
#if DEBUG_VM
						std::cout << "memory access out of bounds: " << base << " + " << offset << "\n";
#endif
						return CODE_ERROR;
					}
				}
				break;

			case 0x30: // i64.load8_s
				{
					WASM_CHECK_STACK(1);
					auto base = stack.back().uptr();
					WASM_READ_MEMARG;
					auto memory = script.getMemoryByIndex(memidx);
					SOUP_IF_UNLIKELY (!memory)
					{
#if DEBUG_VM
						std::cout << "memory access without a memory\n";
#endif
						return CODE_ERROR;
					}
					SOUP_IF_LIKELY (auto ptr = memory->getPointer<int8_t>(base, offset))
					{
						stack.back() = static_cast<int64_t>(*ptr);
					}
					else
					{
#if DEBUG_VM
						std::cout << "memory access out of bounds: " << base << " + " << offset << "\n";
#endif
						return CODE_ERROR;
					}
				}
				break;

			case 0x31: // i64.load8_u
				{
					WASM_CHECK_STACK(1);
					auto base = stack.back().uptr();
					WASM_READ_MEMARG;
					auto memory = script.getMemoryByIndex(memidx);
					SOUP_IF_UNLIKELY (!memory)
					{
#if DEBUG_VM
						std::cout << "memory access without a memory\n";
#endif
						return CODE_ERROR;
					}
					SOUP_IF_LIKELY (auto ptr = memory->getPointer<uint8_t>(base, offset))
					{
						stack.back() = static_cast<uint64_t>(*ptr);
					}
					else
					{
#if DEBUG_VM
						std::cout << "memory access out of bounds: " << base << " + " << offset << "\n";
#endif
						return CODE_ERROR;
					}
				}
				break;

			case 0x32: // i64.load16_s
				{
					WASM_CHECK_STACK(1);
					auto base = stack.back().uptr();
					WASM_READ_MEMARG;
					auto memory = script.getMemoryByIndex(memidx);
					SOUP_IF_UNLIKELY (!memory)
					{
#if DEBUG_VM
						std::cout << "memory access without a memory\n";
#endif
						return CODE_ERROR;
					}
					SOUP_IF_LIKELY (auto ptr = memory->getPointer<int16_t>(base, offset))
					{
						stack.back() = static_cast<int64_t>(*ptr);
					}
					else
					{
#if DEBUG_VM
						std::cout << "memory access out of bounds: " << base << " + " << offset << "\n";
#endif
						return CODE_ERROR;
					}
				}
				break;

			case 0x33: // i64.load16_u
				{
					WASM_CHECK_STACK(1);
					auto base = stack.back().uptr();
					WASM_READ_MEMARG;
					auto memory = script.getMemoryByIndex(memidx);
					SOUP_IF_UNLIKELY (!memory)
					{
#if DEBUG_VM
						std::cout << "memory access without a memory\n";
#endif
						return CODE_ERROR;
					}
					SOUP_IF_LIKELY (auto ptr = memory->getPointer<uint16_t>(base, offset))
					{
						stack.back() = static_cast<uint64_t>(*ptr);
					}
					else
					{
#if DEBUG_VM
						std::cout << "memory access out of bounds: " << base << " + " << offset << "\n";
#endif
						return CODE_ERROR;
					}
				}
				break;

			case 0x34: // i64.load32_s
				{
					WASM_CHECK_STACK(1);
					auto base = stack.back().uptr();
					WASM_READ_MEMARG;
					auto memory = script.getMemoryByIndex(memidx);
					SOUP_IF_UNLIKELY (!memory)
					{
#if DEBUG_VM
						std::cout << "memory access without a memory\n";
#endif
						return CODE_ERROR;
					}
					SOUP_IF_LIKELY (auto ptr = memory->getPointer<int32_t>(base, offset))
					{
						stack.back() = static_cast<int64_t>(*ptr);
					}
					else
					{
#if DEBUG_VM
						std::cout << "memory access out of bounds: " << base << " + " << offset << "\n";
#endif
						return CODE_ERROR;
					}
				}
				break;

			case 0x35: // i64.load32_u
				{
					WASM_CHECK_STACK(1);
					auto base = stack.back().uptr();
					WASM_READ_MEMARG;
					auto memory = script.getMemoryByIndex(memidx);
					SOUP_IF_UNLIKELY (!memory)
					{
#if DEBUG_VM
						std::cout << "memory access without a memory\n";
#endif
						return CODE_ERROR;
					}
					SOUP_IF_LIKELY (auto ptr = memory->getPointer<uint32_t>(base, offset))
					{
						stack.back() = static_cast<uint64_t>(*ptr);
					}
					else
					{
#if DEBUG_VM
						std::cout << "memory access out of bounds: " << base << " + " << offset << "\n";
#endif
						return CODE_ERROR;
					}
				}
				break;


			case 0x36: // i32.store
				{
					WASM_CHECK_STACK(2);
					auto value = stack.back().i32; stack.pop_back();
					auto base = stack.back().uptr(); stack.pop_back();
					WASM_READ_MEMARG;
					auto memory = script.getMemoryByIndex(memidx);
					SOUP_IF_UNLIKELY (!memory)
					{
#if DEBUG_VM
						std::cout << "memory access without a memory\n";
#endif
						return CODE_ERROR;
					}
					SOUP_IF_LIKELY (auto ptr = memory->getPointer<int32_t>(base, offset))
					{
						*ptr = value;
					}
					else
					{
#if DEBUG_VM
						std::cout << "memory access out of bounds: " << base << " + " << offset << "\n";
#endif
						return CODE_ERROR;
					}
				}
				break;

			case 0x37: // i64.store
				{
					WASM_CHECK_STACK(2);
					auto value = stack.back().i64; stack.pop_back();
					auto base = stack.back().uptr(); stack.pop_back();
					WASM_READ_MEMARG;
					auto memory = script.getMemoryByIndex(memidx);
					SOUP_IF_UNLIKELY (!memory)
					{
#if DEBUG_VM
						std::cout << "memory access without a memory\n";
#endif
						return CODE_ERROR;
					}
					SOUP_IF_LIKELY (auto ptr = memory->getPointer<int64_t>(base, offset))
					{
						*ptr = value;
					}
					else
					{
#if DEBUG_VM
						std::cout << "memory access out of bounds: " << base << " + " << offset << "\n";
#endif
						return CODE_ERROR;
					}
				}
				break;

			case 0x38: // f32.store
				{
					WASM_CHECK_STACK(2);
					auto value = stack.back().f32; stack.pop_back();
					auto base = stack.back().uptr(); stack.pop_back();
					WASM_READ_MEMARG;
					auto memory = script.getMemoryByIndex(memidx);
					SOUP_IF_UNLIKELY (!memory)
					{
#if DEBUG_VM
						std::cout << "memory access without a memory\n";
#endif
						return CODE_ERROR;
					}
					SOUP_IF_LIKELY (auto ptr = memory->getPointer<float>(base, offset))
					{
						*ptr = value;
					}
					else
					{
#if DEBUG_VM
						std::cout << "memory access out of bounds: " << base << " + " << offset << "\n";
#endif
						return CODE_ERROR;
					}
				}
				break;

			case 0x39: // f64.store
				{
					WASM_CHECK_STACK(2);
					auto value = stack.back().f64; stack.pop_back();
					auto base = stack.back().uptr(); stack.pop_back();
					WASM_READ_MEMARG;
					auto memory = script.getMemoryByIndex(memidx);
					SOUP_IF_UNLIKELY (!memory)
					{
#if DEBUG_VM
						std::cout << "memory access without a memory\n";
#endif
						return CODE_ERROR;
					}
					SOUP_IF_LIKELY (auto ptr = memory->getPointer<double>(base, offset))
					{
						*ptr = value;
					}
					else
					{
#if DEBUG_VM
						std::cout << "memory access out of bounds: " << base << " + " << offset << "\n";
#endif
						return CODE_ERROR;
					}
				}
				break;

			case 0x3a: // i32.store8
				{
					WASM_CHECK_STACK(2);
					auto value = static_cast<int8_t>(stack.back().i32); stack.pop_back();
					auto base = stack.back().uptr(); stack.pop_back();
					WASM_READ_MEMARG;
					auto memory = script.getMemoryByIndex(memidx);
					SOUP_IF_UNLIKELY (!memory)
					{
#if DEBUG_VM
						std::cout << "memory access without a memory\n";
#endif
						return CODE_ERROR;
					}
					SOUP_IF_LIKELY (auto ptr = memory->getPointer<int8_t>(base, offset))
					{
						*ptr = value;
					}
					else
					{
#if DEBUG_VM
						std::cout << "memory access out of bounds: " << base << " + " << offset << "\n";
#endif
						return CODE_ERROR;
					}
				}
				break;

			case 0x3b: // i32.store16
				{
					WASM_CHECK_STACK(2);
					auto value = static_cast<int16_t>(stack.back().i32); stack.pop_back();
					auto base = stack.back().uptr(); stack.pop_back();
					WASM_READ_MEMARG;
					auto memory = script.getMemoryByIndex(memidx);
					SOUP_IF_UNLIKELY (!memory)
					{
#if DEBUG_VM
						std::cout << "memory access without a memory\n";
#endif
						return CODE_ERROR;
					}
					SOUP_IF_LIKELY (auto ptr = memory->getPointer<int16_t>(base, offset))
					{
						*ptr = value;
					}
					else
					{
#if DEBUG_VM
						std::cout << "memory access out of bounds: " << base << " + " << offset << "\n";
#endif
						return CODE_ERROR;
					}
				}
				break;

			case 0x3c: // i64.store8
				{
					WASM_CHECK_STACK(2);
					auto value = static_cast<int8_t>(stack.back().i64); stack.pop_back();
					auto base = stack.back().uptr(); stack.pop_back();
					WASM_READ_MEMARG;
					auto memory = script.getMemoryByIndex(memidx);
					SOUP_IF_UNLIKELY (!memory)
					{
#if DEBUG_VM
						std::cout << "memory access without a memory\n";
#endif
						return CODE_ERROR;
					}
					SOUP_IF_LIKELY (auto ptr = memory->getPointer<int8_t>(base, offset))
					{
						*ptr = value;
					}
					else
					{
#if DEBUG_VM
						std::cout << "memory access out of bounds: " << base << " + " << offset << "\n";
#endif
						return CODE_ERROR;
					}
				}
				break;

			case 0x3d: // i64.store16
				{
					WASM_CHECK_STACK(2);
					auto value = static_cast<int16_t>(stack.back().i64); stack.pop_back();
					auto base = stack.back().uptr(); stack.pop_back();
					WASM_READ_MEMARG;
					auto memory = script.getMemoryByIndex(memidx);
					SOUP_IF_UNLIKELY (!memory)
					{
#if DEBUG_VM
						std::cout << "memory access without a memory\n";
#endif
						return CODE_ERROR;
					}
					SOUP_IF_LIKELY (auto ptr = memory->getPointer<int16_t>(base, offset))
					{
						*ptr = value;
					}
					else
					{
#if DEBUG_VM
						std::cout << "memory access out of bounds: " << base << " + " << offset << "\n";
#endif
						return CODE_ERROR;
					}
				}
				break;

			case 0x3e: // i64.store32
				{
					WASM_CHECK_STACK(2);
					auto value = static_cast<int32_t>(stack.back().i64); stack.pop_back();
					auto base = stack.back().uptr(); stack.pop_back();
					WASM_READ_MEMARG;
					auto memory = script.getMemoryByIndex(memidx);
					SOUP_IF_UNLIKELY (!memory)
					{
#if DEBUG_VM
						std::cout << "memory access without a memory\n";
#endif
						return CODE_ERROR;
					}
					SOUP_IF_LIKELY (auto ptr = memory->getPointer<int32_t>(base, offset))
					{
						*ptr = value;
					}
					else
					{
#if DEBUG_VM
						std::cout << "memory access out of bounds: " << base << " + " << offset << "\n";
#endif
						return CODE_ERROR;
					}
				}
				break;

			case 0x3f: // memory.size
				{
					uint32_t memidx;
					WASM_READ_OML(memidx);
					auto memory = script.getMemoryByIndex(memidx);
					SOUP_IF_UNLIKELY (!memory)
					{
#if DEBUG_VM
						std::cout << "memory.size: no memory\n";
#endif
						return CODE_ERROR;
					}
					memory->encodeUPTR(stack.emplace_back(), memory->size / memory->getPageSize());
				}
				break;

			case 0x40: // memory.grow
				{
					uint32_t memidx;
					WASM_READ_OML(memidx);
					auto memory = script.getMemoryByIndex(memidx);
					SOUP_IF_UNLIKELY (!memory)
					{
#if DEBUG_VM
						std::cout << "memory.grow: no memory\n";
#endif
						return CODE_ERROR;
					}
					WASM_CHECK_STACK(1);
					const auto old_size_pages = memory->grow(stack.back().uptr());
					memory->encodeUPTR(stack.back(), old_size_pages);
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
				WASM_CHECK_STACK(1);
				stack.back() = (stack.back().i32 == 0);
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
				WASM_CHECK_STACK(1);
				stack.back() = (stack.back().i64 == 0);
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
					auto b = stack.back().i32; stack.pop_back();
					stack.back().i32 += b;
				}
				break;

			case 0x6b: // i32.sub
				{
					WASM_CHECK_STACK(2);
					auto b = stack.back().i32; stack.pop_back();
					stack.back().i32 -= b;
				}
				break;

			case 0x6c: // i32.mul
				{
					WASM_CHECK_STACK(2);
					auto b = stack.back().i32; stack.pop_back();
					stack.back().i32 *= b;
				}
				break;

			case 0x6d: // i32.div_s
				{
					WASM_CHECK_STACK(2);
					auto b = stack.back().i32; stack.pop_back();
					auto a = stack.back().i32;
					SOUP_IF_UNLIKELY (b == 0 || (a == INT32_MIN && b == -1))
					{
						return CODE_ERROR;
					}
					stack.back().i32 = a / b;
				}
				break;

			case 0x6e: // i32.div_u
				{
					WASM_CHECK_STACK(2);
					auto b = stack.back().i32; stack.pop_back();
					SOUP_IF_UNLIKELY (b == 0)
					{
						return CODE_ERROR;
					}
					stack.back().i32 = static_cast<uint32_t>(stack.back().i32) / static_cast<uint32_t>(b);
				}
				break;

			case 0x6f: // i32.rem_s
				{
					WASM_CHECK_STACK(2);
					auto b = stack.back().i32; stack.pop_back();
					auto a = stack.back().i32;
					SOUP_IF_UNLIKELY (b == 0)
					{
						return CODE_ERROR;
					}
					if (a == INT32_MIN && b == -1)
					{
						stack.back().i32 = 0;
					}
					else
					{
						stack.back().i32 = a % b;
					}
				}
				break;

			case 0x70: // i32.rem_u
				{
					WASM_CHECK_STACK(2);
					auto b = stack.back().i32; stack.pop_back();
					SOUP_IF_UNLIKELY (b == 0)
					{
						return CODE_ERROR;
					}
					stack.back().i32 = static_cast<uint32_t>(stack.back().i32) % static_cast<uint32_t>(b);
				}
				break;

			case 0x71: // i32.and
				{
					WASM_CHECK_STACK(2);
					auto b = stack.back().i32; stack.pop_back();
					stack.back().i32 &= b;
				}
				break;

			case 0x72: // i32.or
				{
					WASM_CHECK_STACK(2);
					auto b = stack.back().i32; stack.pop_back();
					stack.back().i32 |= b;
				}
				break;

			case 0x73: // i32.xor
				{
					WASM_CHECK_STACK(2);
					auto b = stack.back().i32; stack.pop_back();
					stack.back().i32 ^= b;
				}
				break;

			case 0x74: // i32.shl
				{
					WASM_CHECK_STACK(2);
					auto b = stack.back().i32; stack.pop_back();
					stack.back().i32 <<= (static_cast<uint32_t>(b) % (sizeof(uint32_t) * 8));
				}
				break;

			case 0x75: // i32.shr_s ("arithmetic right shift")
				{
					WASM_CHECK_STACK(2);
					auto b = stack.back().i32; stack.pop_back();
					stack.back().i32 >>= (static_cast<uint32_t>(b) % (sizeof(uint32_t) * 8));
				}
				break;

			case 0x76: // i32.shr_u ("logical right shift")
				{
					WASM_CHECK_STACK(2);
					auto b = stack.back().i32; stack.pop_back();
					stack.back().i32 = static_cast<uint32_t>(stack.back().i32) >> (static_cast<uint32_t>(b) % (sizeof(uint32_t) * 8));
				}
				break;

			case 0x77: // i32.rotl
				{
					WASM_CHECK_STACK(2);
					auto b = stack.back().i32; stack.pop_back();
					stack.back() = soup::rotl<uint32_t>(stack.back().i32, b);
				}
				break;

			case 0x78: // i32.rotr
				{
					WASM_CHECK_STACK(2);
					auto b = stack.back().i32; stack.pop_back();
					stack.back() = soup::rotr<uint32_t>(stack.back().i32, b);
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
					auto b = stack.back().i64; stack.pop_back();
					stack.back().i64 += b;
				}
				break;

			case 0x7d: // i64.sub
				{
					WASM_CHECK_STACK(2);
					auto b = stack.back().i64; stack.pop_back();
					stack.back().i64 -= b;
				}
				break;

			case 0x7e: // i64.mul
				{
					WASM_CHECK_STACK(2);
					auto b = stack.back().i64; stack.pop_back();
					stack.back().i64 *= b;
				}
				break;

			case 0x7f: // i64.div_s
				{
					WASM_CHECK_STACK(2);
					auto b = stack.back().i64; stack.pop_back();
					auto a = stack.back().i64;
					SOUP_IF_UNLIKELY (b == 0 || (a == INT64_MIN && b == -1))
					{
						return CODE_ERROR;
					}
					stack.back().i64 = a / b;
				}
				break;

			case 0x80: // i64.div_u
				{
					WASM_CHECK_STACK(2);
					auto b = stack.back().i64; stack.pop_back();
					SOUP_IF_UNLIKELY (b == 0)
					{
						return CODE_ERROR;
					}
					stack.back().i64 = static_cast<uint64_t>(stack.back().i64) / static_cast<uint64_t>(b);
				}
				break;

			case 0x81: // i64.rem_s
				{
					WASM_CHECK_STACK(2);
					auto b = stack.back().i64; stack.pop_back();
					auto a = stack.back().i64;
					SOUP_IF_UNLIKELY (b == 0)
					{
						return CODE_ERROR;
					}
					if (a == INT64_MIN && b == -1)
					{
						stack.back().i64 = 0;
					}
					else
					{
						stack.back().i64 = a % b;
					}
				}
				break;

			case 0x82: // i64.rem_u
				{
					WASM_CHECK_STACK(2);
					auto b = stack.back().i64; stack.pop_back();
					SOUP_IF_UNLIKELY (b == 0)
					{
						return CODE_ERROR;
					}
					stack.back().i64 = static_cast<uint64_t>(stack.back().i64) % static_cast<uint64_t>(b);
				}
				break;

			case 0x83: // i64.and
				{
					WASM_CHECK_STACK(2);
					auto b = stack.back().i64; stack.pop_back();
					stack.back().i64 &= b;
				}
				break;

			case 0x84: // i64.or
				{
					WASM_CHECK_STACK(2);
					auto b = stack.back().i64; stack.pop_back();
					stack.back().i64 |= b;
				}
				break;

			case 0x85: // i64.xor
				{
					WASM_CHECK_STACK(2);
					auto b = stack.back().i64; stack.pop_back();
					stack.back().i64 ^= b;
				}
				break;

			case 0x86: // i64.shl
				{
					WASM_CHECK_STACK(2);
					auto b = stack.back().i64; stack.pop_back();
					stack.back().i64 <<= (static_cast<uint64_t>(b) % (sizeof(uint64_t) * 8));
				}
				break;

			case 0x87: // i64.shr_s ("arithmetic right shift")
				{
					WASM_CHECK_STACK(2);
					auto b = stack.back().i64; stack.pop_back();
					stack.back().i64 >>= (static_cast<uint64_t>(b) % (sizeof(uint64_t) * 8));
				}
				break;

			case 0x88: // i64.shr_u ("logical right shift")
				{
					WASM_CHECK_STACK(2);
					auto b = stack.back().i64; stack.pop_back();
					stack.back().i64 = static_cast<uint64_t>(stack.back().i64) >> (static_cast<uint64_t>(b) % (sizeof(uint64_t) * 8));
				}
				break;

			case 0x89: // i64.rotl
				{
					WASM_CHECK_STACK(2);
					auto b = stack.back().i64; stack.pop_back();
					stack.back().i64 = soup::rotl<uint64_t>(stack.back().i64, static_cast<int>(b));
				}
				break;

			case 0x8a: // i64.rotr
				{
					WASM_CHECK_STACK(2);
					auto b = stack.back().i64; stack.pop_back();
					stack.back().i64 = soup::rotr<uint64_t>(stack.back().i64, static_cast<int>(b));
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
					auto b = stack.back().f32; stack.pop_back();
					stack.back().f32 += b;
				}
				break;

			case 0x93: // f32.sub
				WASM_CHECK_STACK(2);
				{
					auto b = stack.back().f32; stack.pop_back();
					stack.back().f32 -= b;
				}
				break;

			case 0x94: // f32.mul
				WASM_CHECK_STACK(2);
				{
					auto b = stack.back().f32; stack.pop_back();
					stack.back().f32 *= b;
				}
				break;

			case 0x95: // f32.div
				WASM_CHECK_STACK(2);
				{
					auto b = stack.back().f32; stack.pop_back();
					stack.back().f32 /= b;
				}
				break;

			case 0x96: // f32.min
				WASM_CHECK_STACK(2);
				{
					auto b = stack.back().f32; stack.pop_back();
					auto a = stack.back().f32;
					stack.back().f32 = wasm_fmin(a, b);
				}
				break;

			case 0x97: // f32.max
				WASM_CHECK_STACK(2);
				{
					auto b = stack.back().f32; stack.pop_back();
					auto a = stack.back().f32;
					stack.back().f32 = wasm_fmax(a, b);
				}
				break;

			case 0x98: // f32.copysign
				WASM_CHECK_STACK(2);
				{
					auto b = stack.back().f32; stack.pop_back();
					stack.back().f32 = std::copysign(stack.back().f32, b);
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
					auto b = stack.back().f64; stack.pop_back();
					stack.back().f64 += b;
				}
				break;

			case 0xa1: // f64.sub
				WASM_CHECK_STACK(2);
				{
					auto b = stack.back().f64; stack.pop_back();
					stack.back().f64 -= b;
				}
				break;

			case 0xa2: // f64.mul
				WASM_CHECK_STACK(2);
				{
					auto b = stack.back().f64; stack.pop_back();
					stack.back().f64 *= b;
				}
				break;

			case 0xa3: // f64.div
				WASM_CHECK_STACK(2);
				{
					auto b = stack.back().f64; stack.pop_back();
					stack.back().f64 /= b;
				}
				break;

			case 0xa4: // f64.min
				WASM_CHECK_STACK(2);
				{
					auto b = stack.back().f64; stack.pop_back();
					auto a = stack.back().f64;
					stack.back().f64 = wasm_fmin(a, b);
				}
				break;

			case 0xa5: // f64.max
				WASM_CHECK_STACK(2);
				{
					auto b = stack.back().f64; stack.pop_back();
					auto a = stack.back().f64;
					stack.back().f64 = wasm_fmax(a, b);
				}
				break;

			case 0xa6: // f64.copysign
				WASM_CHECK_STACK(2);
				{
					auto b = stack.back().f64; stack.pop_back();
					stack.back().f64 = std::copysign(stack.back().f64, b);
				}
				break;

			case 0xa7: // i32.wrap_i64
				WASM_CHECK_STACK(1);
				stack.back() = static_cast<int32_t>(stack.back().i64);
				break;

			case 0xa8: // i32.trunc_f32_s
				WASM_CHECK_STACK(1);
				{
					int32_t i;
					SOUP_IF_UNLIKELY (!float_to_int(stack.back().f32, i))
					{
#if DEBUG_VM
						std::cout << "float cannot be represented as int\n";
#endif
						return CODE_ERROR;
					}
					stack.back() = i;
				}
				break;

			case 0xa9: // i32.trunc_f32_u
				WASM_CHECK_STACK(1);
				{
					uint32_t i;
					SOUP_IF_UNLIKELY (!float_to_int(stack.back().f32, i))
					{
#if DEBUG_VM
						std::cout << "float cannot be represented as int\n";
#endif
						return CODE_ERROR;
					}
					stack.back() = i;
				}
				break;

			case 0xaa: // i32.trunc_f64_s
				WASM_CHECK_STACK(1);
				{
					int32_t i;
					SOUP_IF_UNLIKELY (!float_to_int(stack.back().f64, i))
					{
#if DEBUG_VM
						std::cout << "float cannot be represented as int\n";
#endif
						return CODE_ERROR;
					}
					stack.back() = i;
				}
				break;

			case 0xab: // i32.trunc_f64_u
				WASM_CHECK_STACK(1);
				{
					uint32_t i;
					SOUP_IF_UNLIKELY (!float_to_int(stack.back().f64, i))
					{
#if DEBUG_VM
						std::cout << "float cannot be represented as int\n";
#endif
						return CODE_ERROR;
					}
					stack.back() = i;
				}
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
				{
					int64_t i;
					SOUP_IF_UNLIKELY (!float_to_int(stack.back().f32, i))
					{
#if DEBUG_VM
						std::cout << "float cannot be represented as int\n";
#endif
						return CODE_ERROR;
					}
					stack.back() = i;
				}
				break;

			case 0xaf: // i64.trunc_f32_u
				WASM_CHECK_STACK(1);
				{
					uint64_t i;
					SOUP_IF_UNLIKELY (!float_to_int(stack.back().f32, i))
					{
#if DEBUG_VM
						std::cout << "float cannot be represented as int\n";
#endif
						return CODE_ERROR;
					}
					stack.back() = i;
				}
				break;

			case 0xb0: // i64.trunc_f64_s
				WASM_CHECK_STACK(1);
				{
					int64_t i;
					SOUP_IF_UNLIKELY (!float_to_int(stack.back().f64, i))
					{
#if DEBUG_VM
						std::cout << "float cannot be represented as int\n";
#endif
						return CODE_ERROR;
					}
					stack.back() = i;
				}
				break;

			case 0xb1: // i64.trunc_f64_u
				WASM_CHECK_STACK(1);
				{
					uint64_t i;
					SOUP_IF_UNLIKELY (!float_to_int(stack.back().f64, i))
					{
#if DEBUG_VM
						std::cout << "float cannot be represented as int\n";
#endif
						return CODE_ERROR;
					}
					stack.back() = i;
				}
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
					SOUP_RETHROW_FALSE(script.shared_env && idx < script.function_imports.size() + script.functions.size());
					stack.emplace_back(WASM_FUNCREF).i64 = script.shared_env->createFuncRef(script, idx);
				}
				break;

			case 0xfc:
				r.u8(op);
				switch (op)
				{
				case 0x00: // i32.trunc_sat_f32_s
					WASM_CHECK_STACK(1);
					{
						int32_t i;
						float_to_int(stack.back().f32, i);
						stack.back() = i;
					}
					break;

				case 0x01: // i32.trunc_sat_f32_u
					WASM_CHECK_STACK(1);
					{
						uint32_t i;
						float_to_int(stack.back().f32, i);
						stack.back() = i;
					}
					break;

				case 0x02: // i32.trunc_sat_f64_s
					WASM_CHECK_STACK(1);
					{
						int32_t i;
						float_to_int(stack.back().f64, i);
						stack.back() = i;
					}
					break;

				case 0x03: // i32.trunc_sat_f64_u
					WASM_CHECK_STACK(1);
					{
						uint32_t i;
						float_to_int(stack.back().f64, i);
						stack.back() = i;
					}
					break;

				case 0x04: // i64.trunc_sat_f32_s
					WASM_CHECK_STACK(1);
					{
						int64_t i;
						float_to_int(stack.back().f32, i);
						stack.back() = i;
					}
					break;

				case 0x05: // i64.trunc_sat_f32_u
					WASM_CHECK_STACK(1);
					{
						uint64_t i;
						float_to_int(stack.back().f32, i);
						stack.back() = i;
					}
					break;

				case 0x06: // i64.trunc_sat_f64_s
					WASM_CHECK_STACK(1);
					{
						int64_t i;
						float_to_int(stack.back().f64, i);
						stack.back() = i;
					}
					break;

				case 0x07: // i64.trunc_sat_f64_u
					WASM_CHECK_STACK(1);
					{
						uint64_t i;
						float_to_int(stack.back().f64, i);
						stack.back() = i;
					}
					break;

				case 0x08: // memory.init
					{
						uint32_t segment_index;
						WASM_READ_OML(segment_index);
						uint32_t memidx;
						WASM_READ_OML(memidx);
						auto memory = script.getMemoryByIndex(memidx);
						SOUP_IF_UNLIKELY (!memory)
						{
#if DEBUG_VM
							std::cout << "memory.init: no memory\n";
#endif
							return CODE_ERROR;
						}
						WASM_CHECK_STACK(3);
						auto size = stack.back().uptr(); stack.pop_back();
						auto src_offset = stack.back().uptr(); stack.pop_back();
						auto dst_offset = stack.back().uptr(); stack.pop_back();
						auto data = script.getPassiveDataSegmentByIndex(segment_index);
						SOUP_IF_UNLIKELY (!data && size != 0)
						{
#if DEBUG_VM
							std::cout << "memory.init: invalid segment index\n";
#endif
							return CODE_ERROR;
						}
						std::string scrap;
						if (!data)
						{
							data = &scrap;
						}
						auto dst_ptr = memory->getView(dst_offset, size);
						SOUP_IF_UNLIKELY (!dst_ptr || !can_add_without_overflow(src_offset, size) || src_offset + size > data->size())
						{
#if DEBUG_VM
							std::cout << "out-of-bounds memory.init\n";
#endif
							return CODE_ERROR;
						}
						memcpy(dst_ptr, data->data() + src_offset, size);
					}
					break;

				case 0x09: // data.drop
					{
						uint32_t segment_index;
						WASM_READ_OML(segment_index);
						if (auto data = script.getPassiveDataSegmentByIndex(segment_index))
						{
							data->clear();
							data->shrink_to_fit();
						}
					}
					break;

				case 0x0a: // memory.copy
					{
						uint32_t dst_memidx, src_memidx;
						WASM_READ_OML(dst_memidx);
						WASM_READ_OML(src_memidx);
						auto dst_memory = script.getMemoryByIndex(dst_memidx);
						auto src_memory = script.getMemoryByIndex(src_memidx);
						SOUP_IF_UNLIKELY (!dst_memory || !src_memory)
						{
#if DEBUG_VM
							std::cout << "memory.copy: invalid memory index\n";
#endif
							return CODE_ERROR;
						}
						WASM_CHECK_STACK(3);
						auto size = stack.back().uptr(); stack.pop_back();
						auto src = stack.back().uptr(); stack.pop_back();
						auto dst = stack.back().uptr(); stack.pop_back();
						auto dst_ptr = dst_memory->getView(dst, size);
						auto src_ptr = src_memory->getView(src, size);
						SOUP_IF_UNLIKELY (!dst_ptr || !src_ptr)
						{
#if DEBUG_VM
							std::cout << "out-of-bounds memory.copy\n";
#endif
							return CODE_ERROR;
						}
						memcpy(dst_ptr, src_ptr, size);
					}
					break;

				case 0x0b: // memory.fill
					{
						uint32_t memidx;
						WASM_READ_OML(memidx);
						auto memory = script.getMemoryByIndex(memidx);
						SOUP_IF_UNLIKELY (!memory)
						{
#if DEBUG_VM
							std::cout << "memory.fill: no memory\n";
#endif
							return CODE_ERROR;
						}
						WASM_CHECK_STACK(3);
						auto size = stack.back().uptr(); stack.pop_back();
						auto value = stack.back().i32; stack.pop_back();
						auto addr = stack.back().uptr(); stack.pop_back();
						auto ptr = memory->getView(addr, size);
						SOUP_IF_UNLIKELY (!ptr)
						{
#if DEBUG_VM
							std::cout << "out-of-bounds memory.fill\n";
#endif
							return CODE_ERROR;
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
						auto table = script.getTableByIndex(table_index);
						SOUP_IF_UNLIKELY (!table)
						{
#if DEBUG_VM
							std::cout << "table.init: invalid table index (" << table_index << ")\n";
#endif
							return CODE_ERROR;
						}
						WASM_CHECK_STACK(3);
						auto size = stack.back().uptr(); stack.pop_back();
						auto src_offset = stack.back().uptr(); stack.pop_back();
						auto dst_offset = stack.back().uptr(); stack.pop_back();
						auto vtbl = script.getPassiveElemSegmentByIndex(segment_index);
						SOUP_IF_UNLIKELY (!vtbl && size != 0)
						{
#if DEBUG_VM
							std::cout << "table.init: invalid segment index\n";
#endif
							return CODE_ERROR;
						}
						SOUP_RETHROW_FALSE(table->init(script, vtbl ? *vtbl : WasmScript::ElemSegment{ table->type }, dst_offset, src_offset, size));
					}
					break;

				case 0x0d: // elem.drop
					{
						uint32_t segment_index;
						WASM_READ_OML(segment_index);
						if (auto vtbl = script.getPassiveElemSegmentByIndex(segment_index))
						{
							vtbl->values.clear();
							vtbl->values.shrink_to_fit();
						}
					}
					break;

				case 0x0e: // table.copy
					{
						uint32_t dst_table_index;
						WASM_READ_OML(dst_table_index);
						uint32_t src_table_index;
						WASM_READ_OML(src_table_index);
						auto* const dst_table = script.getTableByIndex(dst_table_index);
						const auto* const src_table = script.getTableByIndex(src_table_index);
						SOUP_IF_UNLIKELY (!dst_table || !src_table)
						{
#if DEBUG_VM
							std::cout << "table.copy: invalid table index\n";
#endif
							return CODE_ERROR;
						}
						WASM_CHECK_STACK(3);
						auto size = stack.back().uptr(); stack.pop_back();
						auto src_offset = stack.back().uptr(); stack.pop_back();
						auto dst_offset = stack.back().uptr(); stack.pop_back();
						SOUP_RETHROW_FALSE(dst_table->copy(*src_table, dst_offset, src_offset, size));
					}
					break;

				case 0x0f: // table.grow
					{
						uint32_t table_index;
						WASM_READ_OML(table_index);
						auto table = script.getTableByIndex(table_index);
						SOUP_IF_UNLIKELY (!table)
						{
#if DEBUG_VM
							std::cout << "table.grow: invalid table index (" << table_index << ")\n";
#endif
							return CODE_ERROR;
						}
						WASM_CHECK_STACK(2);
						auto delta = stack.back().uptr(); stack.pop_back();
						auto& value = stack.back();
						SOUP_IF_UNLIKELY (value.type != table->type)
						{
#if DEBUG_VM
							std::cout << "table.grow: attempt to assign " << wasm_type_to_string(value.type) << " to a table of " << wasm_type_to_string(table->type) << "\n";
#endif
							return CODE_ERROR;
						}
						const auto old_size = table->grow(delta, value.i64);
#if SOUP_WASM_MEMORY64
						if (table->table64)
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
						auto table = script.getTableByIndex(table_index);
						SOUP_IF_UNLIKELY (!table)
						{
#if DEBUG_VM
							std::cout << "table.size: invalid table index (" << table_index << ")\n";
#endif
							return CODE_ERROR;
						}
#if SOUP_WASM_MEMORY64
						if (table->table64)
						{
							stack.emplace_back(static_cast<uint64_t>(table->values.size()));
						}
						else
#endif
						{
							stack.emplace_back(static_cast<uint32_t>(table->values.size()));
						}
					}
					break;

				case 0x11: // table.fill
					{
						uint32_t table_index;
						WASM_READ_OML(table_index);
						auto table = script.getTableByIndex(table_index);
						SOUP_IF_UNLIKELY (!table)
						{
#if DEBUG_VM
							std::cout << "table.fill: invalid table index (" << table_index << ")\n";
#endif
							return CODE_ERROR;
						}
						WASM_CHECK_STACK(3);
						auto& size = stack[stack.size() - 1].i32;
						auto& value = stack[stack.size() - 2];
						auto& offset = stack[stack.size() - 3].i32;
						SOUP_IF_UNLIKELY (value.type != table->type)
						{
#if DEBUG_VM
							std::cout << "table.fill: attempt to assign " << wasm_type_to_string(value.type) << " to a table of " << wasm_type_to_string(table->type) << "\n";
#endif
							return CODE_ERROR;
						}
						SOUP_IF_UNLIKELY (offset + size > table->values.size())
						{
#if DEBUG_VM
							std::cout << "out-of-bounds table.fill\n";
#endif
							return CODE_ERROR;
						}
						while (size--)
						{
							table->values[offset++] = value.i64;
						}
						stack.erase(stack.end() - 3, stack.end());
					}
					break;

				default:
#if DEBUG_VM
					std::cout << "Unsupported opcode: " << string::hex(0xFC00 | op) << "\n";
#endif
					return CODE_ERROR;
				}
				break;

#if SOUP_WASM_SIMD
			case 0xfd: {
				uint32_t simdop;
				WASM_READ_OML(simdop);
				switch (simdop)
				{
				case 0x00: // v128.load
					{
						WASM_CHECK_STACK(1);
						auto base = stack.back().uptr(); stack.pop_back();
						WASM_READ_MEMARG;
						auto memory = script.getMemoryByIndex(memidx);
						SOUP_RETHROW_FALSE(memory);
						auto ptr = memory->getView(base + offset, 16);
						SOUP_RETHROW_FALSE(ptr);
						memcpy(stack.emplace_back(WASM_V128).i8x16, ptr, 16);
					}
					break;

				case 0x01: // v128.load8x8_s
					SOUP_RETHROW_FALSE(simd_loadx<int8_t>(stack, r, script, &WasmValue::i16x8));
					break;

				case 0x02: // v128.load8x8_u
					SOUP_RETHROW_FALSE(simd_loadx<uint8_t>(stack, r, script, &WasmValue::i16x8));
					break;

				case 0x03: // v128.load16x4_s
					SOUP_RETHROW_FALSE(simd_loadx<int16_t>(stack, r, script, &WasmValue::i32x4));
					break;

				case 0x04: // v128.load16x4_u
					SOUP_RETHROW_FALSE(simd_loadx<uint16_t>(stack, r, script, &WasmValue::i32x4));
					break;

				case 0x05: // v128.load32x2_s
					SOUP_RETHROW_FALSE(simd_loadx<int32_t>(stack, r, script, &WasmValue::i64x2));
					break;

				case 0x06: // v128.load32x2_u
					SOUP_RETHROW_FALSE(simd_loadx<uint32_t>(stack, r, script, &WasmValue::i64x2));
					break;

				case 0x07: // v128.load8_splat
					SOUP_RETHROW_FALSE(simd_load_splat(stack, r, script, &WasmValue::i8x16));
					break;

				case 0x08: // v128.load16_splat
					SOUP_RETHROW_FALSE(simd_load_splat(stack, r, script, &WasmValue::i16x8));
					break;

				case 0x09: // v128.load32_splat
					SOUP_RETHROW_FALSE(simd_load_splat(stack, r, script, &WasmValue::i32x4));
					break;

				case 0x0a: // v128.load64_splat
					SOUP_RETHROW_FALSE(simd_load_splat(stack, r, script, &WasmValue::i64x2));
					break;

				case 0x0b: // v128.store
					{
						WASM_CHECK_STACK(2);
						int8_t value[16]; memcpy(value, stack.back().i8x16, 16); stack.pop_back();
						auto base = stack.back().uptr(); stack.pop_back();
						WASM_READ_MEMARG;
						auto memory = script.getMemoryByIndex(memidx);
						SOUP_RETHROW_FALSE(memory);
						auto ptr = memory->getView(base + offset, 16);
						SOUP_RETHROW_FALSE(ptr);
						memcpy(ptr, value, 16);
					}
					break;

				case 0x0c: // v128.const
					r.raw(stack.emplace_back(WASM_V128).i8x16, 16);
					break;

				case 0x0d: // i8x16.shuffle
					{
						WASM_CHECK_STACK(2);
						int8_t b[16]; memcpy(b, stack.back().i8x16, 16); stack.pop_back();
						int8_t a[16]; memcpy(a, stack.back().i8x16, 16);
						WasmValue& result = stack.back();
						for (int i = 0; i != 16; ++i)
						{
							uint8_t idx;
							r.u8(idx);
							SOUP_RETHROW_FALSE(idx < 32);
							if (idx < 16)
							{
								result.i8x16[i] = a[idx];
							}
							else
							{
								result.i8x16[i] = b[idx - 16];
							}
						}
					}
					break;

				case 0x0e: // i8x16.swizzle
					{
						WASM_CHECK_STACK(2);
						int8_t s[16]; memcpy(s, stack.back().i8x16, 16); stack.pop_back();
						int8_t a[16]; memcpy(a, stack.back().i8x16, 16);
						WasmValue& result = stack.back();
						for (int i = 0; i != 16; ++i)
						{
							result.i8x16[i] = static_cast<uint8_t>(s[i]) < 16 ? a[s[i]] : 0;
						}
					}
					break;

				case 0x0f: // i8x16.splat
					SOUP_RETHROW_FALSE(simd_splat(stack, &WasmValue::i8x16));
					break;

				case 0x10: // i16x8.splat
					SOUP_RETHROW_FALSE(simd_splat(stack, &WasmValue::i16x8));
					break;

				case 0x11: // i32x4.splat
					SOUP_RETHROW_FALSE(simd_splat(stack, &WasmValue::i32x4));
					break;

				case 0x12: // i64x2.splat
					SOUP_RETHROW_FALSE(simd_splat(stack, &WasmValue::i64x2));
					break;

				case 0x13: // f32x4.splat
					SOUP_RETHROW_FALSE(simd_splat(stack, &WasmValue::f32x4));
					break;

				case 0x14: // f64x2.splat
					SOUP_RETHROW_FALSE(simd_splat(stack, &WasmValue::f64x2));
					break;

				case 0x15: // i8x16.extract_lane_s
					SOUP_RETHROW_FALSE(extract_lane_s(stack, r, &WasmValue::i8x16));
					break;

				case 0x16: // i8x16.extract_lane_u
					SOUP_RETHROW_FALSE(extract_lane_u(stack, r, &WasmValue::i8x16));
					break;

				case 0x17: // i8x16.replace_lane
					SOUP_RETHROW_FALSE(replace_lane(stack, r, &WasmValue::i8x16));
					break;

				case 0x18: // i16x8.extract_lane_s
					SOUP_RETHROW_FALSE(extract_lane_s(stack, r, &WasmValue::i16x8));
					break;

				case 0x19: // i16x8.extract_lane_u
					SOUP_RETHROW_FALSE(extract_lane_u(stack, r, &WasmValue::i16x8));
					break;

				case 0x1a: // i16x8.replace_lane
					SOUP_RETHROW_FALSE(replace_lane(stack, r, &WasmValue::i16x8));
					break;

				case 0x1b: // i32x4.extract_lane
					SOUP_RETHROW_FALSE(extract_lane(stack, r, &WasmValue::i32x4));
					break;

				case 0x1c: // i32x4.replace_lane
					SOUP_RETHROW_FALSE(replace_lane(stack, r, &WasmValue::i32x4));
					break;

				case 0x1d: // i64x2.extract_lane
					SOUP_RETHROW_FALSE(extract_lane(stack, r, &WasmValue::i64x2));
					break;

				case 0x1e: // i64x2.replace_lane
					SOUP_RETHROW_FALSE(replace_lane(stack, r, &WasmValue::i64x2));
					break;

				case 0x1f: // f32x4.extract_lane
					SOUP_RETHROW_FALSE(extract_lane(stack, r, &WasmValue::f32x4));
					break;

				case 0x20: // f32x4.replace_lane
					SOUP_RETHROW_FALSE(replace_lane(stack, r, &WasmValue::f32x4));
					break;

				case 0x21: // f64x2.extract_lane
					SOUP_RETHROW_FALSE(extract_lane(stack, r, &WasmValue::f64x2));
					break;

				case 0x22: // f64x2.replace_lane
					SOUP_RETHROW_FALSE(replace_lane(stack, r, &WasmValue::f64x2));
					break;

				case 0x23: // i8x16.eq
					SOUP_RETHROW_FALSE(simd_eq(stack, &WasmValue::i8x16, &WasmValue::i8x16));
					break;

				case 0x24: // i8x16.ne
					SOUP_RETHROW_FALSE(simd_ne(stack, &WasmValue::i8x16, &WasmValue::i8x16));
					break;

				case 0x25: // i8x16.lt_s
					SOUP_RETHROW_FALSE(simd_lt(stack, &WasmValue::i8x16, &WasmValue::i8x16));
					break;

				case 0x26: // i8x16.lt_u
					SOUP_RETHROW_FALSE(simd_lt_u(stack, &WasmValue::i8x16));
					break;

				case 0x27: // i8x16.gt_s
					SOUP_RETHROW_FALSE(simd_gt(stack, &WasmValue::i8x16, &WasmValue::i8x16));
					break;

				case 0x28: // i8x16.gt_u
					SOUP_RETHROW_FALSE(simd_gt_u(stack, &WasmValue::i8x16));
					break;

				case 0x29: // i8x16.le_s
					SOUP_RETHROW_FALSE(simd_le(stack, &WasmValue::i8x16, &WasmValue::i8x16));
					break;

				case 0x2a: // i8x16.le_u
					SOUP_RETHROW_FALSE(simd_le_u(stack, &WasmValue::i8x16));
					break;

				case 0x2b: // i8x16.ge_s
					SOUP_RETHROW_FALSE(simd_ge(stack, &WasmValue::i8x16, &WasmValue::i8x16));
					break;

				case 0x2c: // i8x16.ge_u
					SOUP_RETHROW_FALSE(simd_ge_u(stack, &WasmValue::i8x16));
					break;

				case 0x2d: // i16x8.eq
					SOUP_RETHROW_FALSE(simd_eq(stack, &WasmValue::i16x8, &WasmValue::i16x8));
					break;

				case 0x2e: // i16x8.ne
					SOUP_RETHROW_FALSE(simd_ne(stack, &WasmValue::i16x8, &WasmValue::i16x8));
					break;

				case 0x2f: // i16x8.lt_s
					SOUP_RETHROW_FALSE(simd_lt(stack, &WasmValue::i16x8, &WasmValue::i16x8));
					break;

				case 0x30: // i16x8.lt_u
					SOUP_RETHROW_FALSE(simd_lt_u(stack, &WasmValue::i16x8));
					break;

				case 0x31: // i16x8.gt_s
					SOUP_RETHROW_FALSE(simd_gt(stack, &WasmValue::i16x8, &WasmValue::i16x8));
					break;

				case 0x32: // i16x8.gt_u
					SOUP_RETHROW_FALSE(simd_gt_u(stack, &WasmValue::i16x8));
					break;

				case 0x33: // i16x8.le_s
					SOUP_RETHROW_FALSE(simd_le(stack, &WasmValue::i16x8, &WasmValue::i16x8));
					break;

				case 0x34: // i16x8.le_u
					SOUP_RETHROW_FALSE(simd_le_u(stack, &WasmValue::i16x8));
					break;

				case 0x35: // i16x8.ge_s
					SOUP_RETHROW_FALSE(simd_ge(stack, &WasmValue::i16x8, &WasmValue::i16x8));
					break;

				case 0x36: // i16x8.ge_u
					SOUP_RETHROW_FALSE(simd_ge_u(stack, &WasmValue::i16x8));
					break;

				case 0x37: // i32x4.eq
					SOUP_RETHROW_FALSE(simd_eq(stack, &WasmValue::i32x4, &WasmValue::i32x4));
					break;

				case 0x38: // i32x4.ne
					SOUP_RETHROW_FALSE(simd_ne(stack, &WasmValue::i32x4, &WasmValue::i32x4));
					break;

				case 0x39: // i32x4.lt_s
					SOUP_RETHROW_FALSE(simd_lt(stack, &WasmValue::i32x4, &WasmValue::i32x4));
					break;

				case 0x3a: // i32x4.lt_u
					SOUP_RETHROW_FALSE(simd_lt_u(stack, &WasmValue::i32x4));
					break;

				case 0x3b: // i32x4.gt_s
					SOUP_RETHROW_FALSE(simd_gt(stack, &WasmValue::i32x4, &WasmValue::i32x4));
					break;

				case 0x3c: // i32x4.gt_u
					SOUP_RETHROW_FALSE(simd_gt_u(stack, &WasmValue::i32x4));
					break;

				case 0x3d: // i32x4.le_s
					SOUP_RETHROW_FALSE(simd_le(stack, &WasmValue::i32x4, &WasmValue::i32x4));
					break;

				case 0x3e: // i32x4.le_u
					SOUP_RETHROW_FALSE(simd_le_u(stack, &WasmValue::i32x4));
					break;

				case 0x3f: // i32x4.ge_s
					SOUP_RETHROW_FALSE(simd_ge(stack, &WasmValue::i32x4, &WasmValue::i32x4));
					break;

				case 0x40: // i32x4.ge_u
					SOUP_RETHROW_FALSE(simd_ge_u(stack, &WasmValue::i32x4));
					break;

				case 0x41: // f32x4.eq
					SOUP_RETHROW_FALSE(simd_eq(stack, &WasmValue::f32x4, &WasmValue::i32x4));
					break;

				case 0x42: // f32x4.ne
					SOUP_RETHROW_FALSE(simd_ne(stack, &WasmValue::f32x4, &WasmValue::i32x4));
					break;

				case 0x43: // f32x4.lt
					SOUP_RETHROW_FALSE(simd_lt(stack, &WasmValue::f32x4, &WasmValue::i32x4));
					break;

				case 0x44: // f32x4.gt
					SOUP_RETHROW_FALSE(simd_gt(stack, &WasmValue::f32x4, &WasmValue::i32x4));
					break;

				case 0x45: // f32x4.le
					SOUP_RETHROW_FALSE(simd_le(stack, &WasmValue::f32x4, &WasmValue::i32x4));
					break;

				case 0x46: // f32x4.ge
					SOUP_RETHROW_FALSE(simd_ge(stack, &WasmValue::f32x4, &WasmValue::i32x4));
					break;

				case 0x47: // f64x2.eq
					SOUP_RETHROW_FALSE(simd_eq(stack, &WasmValue::f64x2, &WasmValue::i64x2));
					break;

				case 0x48: // f64x2.ne
					SOUP_RETHROW_FALSE(simd_ne(stack, &WasmValue::f64x2, &WasmValue::i64x2));
					break;

				case 0x49: // f64x2.lt
					SOUP_RETHROW_FALSE(simd_lt(stack, &WasmValue::f64x2, &WasmValue::i64x2));
					break;

				case 0x4a: // f64x2.gt
					SOUP_RETHROW_FALSE(simd_gt(stack, &WasmValue::f64x2, &WasmValue::i64x2));
					break;

				case 0x4b: // f64x2.le
					SOUP_RETHROW_FALSE(simd_le(stack, &WasmValue::f64x2, &WasmValue::i64x2));
					break;

				case 0x4c: // f64x2.ge
					SOUP_RETHROW_FALSE(simd_ge(stack, &WasmValue::f64x2, &WasmValue::i64x2));
					break;

				case 0x4d: // v128.not
					WASM_CHECK_STACK(1);
					stack.back().i64x2[0] = ~stack.back().i64x2[0];
					stack.back().i64x2[1] = ~stack.back().i64x2[1];
					break;

				case 0x4e: // v128.and
					{
						WASM_CHECK_STACK(2);
						int64_t b[2]; memcpy(b, stack.back().i64x2, 16); stack.pop_back();
						stack.back().i64x2[0] &= b[0];
						stack.back().i64x2[1] &= b[1];
					}
					break;

				case 0x4f: // v128.andnot
					{
						WASM_CHECK_STACK(2);
						int64_t b[2]; memcpy(b, stack.back().i64x2, 16); stack.pop_back();
						stack.back().i64x2[0] &= ~b[0];
						stack.back().i64x2[1] &= ~b[1];
					}
					break;

				case 0x50: // v128.or
					{
						WASM_CHECK_STACK(2);
						int64_t b[2]; memcpy(b, stack.back().i64x2, 16); stack.pop_back();
						stack.back().i64x2[0] |= b[0];
						stack.back().i64x2[1] |= b[1];
					}
					break;

				case 0x51: // v128.xor
					{
						WASM_CHECK_STACK(2);
						int64_t b[2]; memcpy(b, stack.back().i64x2, 16); stack.pop_back();
						stack.back().i64x2[0] ^= b[0];
						stack.back().i64x2[1] ^= b[1];
					}
					break;

				case 0x52: // v128.bitselect
					{
						WASM_CHECK_STACK(3);
						int64_t c[2]; memcpy(c, stack.back().i64x2, 16); stack.pop_back();
						int64_t b[2]; memcpy(b, stack.back().i64x2, 16); stack.pop_back();
						stack.back().i64x2[0] = (stack.back().i64x2[0] & c[0]) | (b[0] & ~c[0]);
						stack.back().i64x2[1] = (stack.back().i64x2[1] & c[1]) | (b[1] & ~c[1]);
					}
					break;

				case 0x60: // i8x16.abs
					SOUP_RETHROW_FALSE(simd_abs(stack, &WasmValue::i8x16));
					break;

				case 0x61: // i8x16.neg
					SOUP_RETHROW_FALSE(simd_neg(stack, &WasmValue::i8x16));
					break;

				case 0x63: // i8x16.all_true
					SOUP_RETHROW_FALSE(all_true(stack, &WasmValue::i8x16));
					break;

				case 0x64: // i8x16.bitmask
					SOUP_RETHROW_FALSE(simd_bitmask(stack, &WasmValue::i8x16));
					break;

				case 0x65: // i8x16.narrow_i16x8_s
					SOUP_RETHROW_FALSE(simd_narrow_s(stack, &WasmValue::i16x8, &WasmValue::i8x16));
					break;

				case 0x66: // i8x16.narrow_i16x8_u
					SOUP_RETHROW_FALSE(simd_narrow_u(stack, &WasmValue::i16x8, &WasmValue::i8x16));
					break;

				case 0x6b: // i8x16.shl
					SOUP_RETHROW_FALSE(simd_shl(stack, &WasmValue::i8x16));
					break;

				case 0x6c: // i8x16.shr_s
					SOUP_RETHROW_FALSE(simd_shr_s(stack, &WasmValue::i8x16));
					break;

				case 0x6d: // i8x16.shr_u
					SOUP_RETHROW_FALSE(simd_shr_u(stack, &WasmValue::i8x16));
					break;

				case 0x6e: // i8x16.add
					SOUP_RETHROW_FALSE(simd_add(stack, &WasmValue::i8x16));
					break;

				case 0x6f: // i8x16.add_sat_s
					SOUP_RETHROW_FALSE(simd_add_sat_s(stack, &WasmValue::i8x16));
					break;

				case 0x70: // i8x16.add_sat_u
					SOUP_RETHROW_FALSE(simd_add_sat_u(stack, &WasmValue::i8x16));
					break;

				case 0x71: // i8x16.sub
					SOUP_RETHROW_FALSE(simd_sub(stack, &WasmValue::i8x16));
					break;

				case 0x72: // i8x16.sub_sat_s
					SOUP_RETHROW_FALSE(simd_sub_sat_s(stack, &WasmValue::i8x16));
					break;

				case 0x73: // i8x16.sub_sat_u
					SOUP_RETHROW_FALSE(simd_sub_sat_u(stack, &WasmValue::i8x16));
					break;

				case 0x76: // i8x16.min_s
					SOUP_RETHROW_FALSE(simd_pmin(stack, &WasmValue::i8x16));
					break;

				case 0x77: // i8x16.min_u
					SOUP_RETHROW_FALSE(simd_min_u(stack, &WasmValue::i8x16));
					break;

				case 0x78: // i8x16.max_s
					SOUP_RETHROW_FALSE(simd_pmax(stack, &WasmValue::i8x16));
					break;

				case 0x79: // i8x16.max_u
					SOUP_RETHROW_FALSE(simd_max_u(stack, &WasmValue::i8x16));
					break;

				case 0x7b: // i8x16.avgr_u
					SOUP_RETHROW_FALSE(simd_avgr_u(stack, &WasmValue::i8x16));
					break;

				case 0x80: // i16x8.abs
					SOUP_RETHROW_FALSE(simd_abs(stack, &WasmValue::i16x8));
					break;

				case 0x81: // i16x8.neg
					SOUP_RETHROW_FALSE(simd_neg(stack, &WasmValue::i16x8));
					break;

				case 0x83: // i16x8.all_true
					SOUP_RETHROW_FALSE(all_true(stack, &WasmValue::i16x8));
					break;

				case 0x84: // i16x8.bitmask
					SOUP_RETHROW_FALSE(simd_bitmask(stack, &WasmValue::i16x8));
					break;

				case 0x85: // i16x8.narrow_i32x4_s
					SOUP_RETHROW_FALSE(simd_narrow_s(stack, &WasmValue::i32x4, &WasmValue::i16x8));
					break;

				case 0x86: // i16x8.narrow_i32x4_u
					SOUP_RETHROW_FALSE(simd_narrow_u(stack, &WasmValue::i32x4, &WasmValue::i16x8));
					break;
					
				case 0x87: // i16x8.extend_low_i8x16_s
					SOUP_RETHROW_FALSE(simd_extend_low_s(stack, &WasmValue::i8x16, &WasmValue::i16x8));
					break;

				case 0x88: // i16x8.extend_high_i8x16_s
					SOUP_RETHROW_FALSE(simd_extend_high_s(stack, &WasmValue::i8x16, &WasmValue::i16x8));
					break;

				case 0x89: // i16x8.extend_low_i8x16_u
					SOUP_RETHROW_FALSE(simd_extend_low_u(stack, &WasmValue::i8x16, &WasmValue::i16x8));
					break;

				case 0x8a: // i16x8.extend_high_i8x16_u
					SOUP_RETHROW_FALSE(simd_extend_high_u(stack, &WasmValue::i8x16, &WasmValue::i16x8));
					break;

				case 0x8b: // i16x8.shl
					SOUP_RETHROW_FALSE(simd_shl(stack, &WasmValue::i16x8));
					break;

				case 0x8c: // i16x8.shr_s
					SOUP_RETHROW_FALSE(simd_shr_s(stack, &WasmValue::i16x8));
					break;

				case 0x8d: // i16x8.shr_u
					SOUP_RETHROW_FALSE(simd_shr_u(stack, &WasmValue::i16x8));
					break;

				case 0x8e: // i16x8.add
					SOUP_RETHROW_FALSE(simd_add(stack, &WasmValue::i16x8));
					break;

				case 0x8f: // i16x8.add_sat_s
					SOUP_RETHROW_FALSE(simd_add_sat_s(stack, &WasmValue::i16x8));
					break;

				case 0x90: // i16x8.add_sat_u
					SOUP_RETHROW_FALSE(simd_add_sat_u(stack, &WasmValue::i16x8));
					break;

				case 0x91: // i16x8.sub
					SOUP_RETHROW_FALSE(simd_sub(stack, &WasmValue::i16x8));
					break;

				case 0x92: // i16x8.sub_sat_s
					SOUP_RETHROW_FALSE(simd_sub_sat_s (stack, &WasmValue::i16x8));
					break;

				case 0x93: // i16x8.sub_sat_u
					SOUP_RETHROW_FALSE(simd_sub_sat_u(stack, &WasmValue::i16x8));
					break;

				case 0x95: // i16x8.mul
					SOUP_RETHROW_FALSE(simd_mul(stack, &WasmValue::i16x8));
					break;

				case 0x96: // i16x8.min_s
					SOUP_RETHROW_FALSE(simd_pmin(stack, &WasmValue::i16x8));
					break;

				case 0x97: // i16x8.min_u
					SOUP_RETHROW_FALSE(simd_min_u(stack, &WasmValue::i16x8));
					break;

				case 0x98: // i16x8.max_s
					SOUP_RETHROW_FALSE(simd_pmax(stack, &WasmValue::i16x8));
					break;

				case 0x99: // i16x8.max_u
					SOUP_RETHROW_FALSE(simd_max_u(stack, &WasmValue::i16x8));
					break;

				case 0x9b: // i16x8.avgr_u
					SOUP_RETHROW_FALSE(simd_avgr_u(stack, &WasmValue::i16x8));
					break;

				case 0xa0: // i32x4.abs
					SOUP_RETHROW_FALSE(simd_abs(stack, &WasmValue::i32x4));
					break;

				case 0xa1: // i32x4.neg
					SOUP_RETHROW_FALSE(simd_neg(stack, &WasmValue::i32x4));
					break;

				case 0xa3: // i32x4.all_true
					SOUP_RETHROW_FALSE(all_true(stack, &WasmValue::i32x4));
					break;

				case 0xa4: // i32x4.bitmask
					SOUP_RETHROW_FALSE(simd_bitmask(stack, &WasmValue::i32x4));
					break;

				case 0xa7: // i32x4.extend_low_i16x8_s
					SOUP_RETHROW_FALSE(simd_extend_low_s(stack, &WasmValue::i16x8, &WasmValue::i32x4));
					break;

				case 0xa8: // i32x4.extend_high_i16x8_s
					SOUP_RETHROW_FALSE(simd_extend_high_s(stack, &WasmValue::i16x8, &WasmValue::i32x4));
					break;

				case 0xa9: // i32x4.extend_low_i16x8_u
					SOUP_RETHROW_FALSE(simd_extend_low_u(stack, &WasmValue::i16x8, &WasmValue::i32x4));
					break;

				case 0xaa: // i32x4.extend_high_i16x8_u
					SOUP_RETHROW_FALSE(simd_extend_high_u(stack, &WasmValue::i16x8, &WasmValue::i32x4));
					break;

				case 0xab: // i32x4.shl
					SOUP_RETHROW_FALSE(simd_shl(stack, &WasmValue::i32x4));
					break;

				case 0xac: // i32x4.shr_s
					SOUP_RETHROW_FALSE(simd_shr_s(stack, &WasmValue::i32x4));
					break;

				case 0xad: // i32x4.shr_u
					SOUP_RETHROW_FALSE(simd_shr_u(stack, &WasmValue::i32x4));
					break;

				case 0xae: // i32x4.add
					SOUP_RETHROW_FALSE(simd_add(stack, &WasmValue::i32x4));
					break;

				case 0xb1: // i32x4.sub
					SOUP_RETHROW_FALSE(simd_sub(stack, &WasmValue::i32x4));
					break;

				case 0xb5: // i32x4.mul
					SOUP_RETHROW_FALSE(simd_mul(stack, &WasmValue::i32x4));
					break;

				case 0xb6: // i32x4.min_s
					SOUP_RETHROW_FALSE(simd_pmin(stack, &WasmValue::i32x4));
					break;

				case 0xb7: // i32x4.min_u
					SOUP_RETHROW_FALSE(simd_min_u(stack, &WasmValue::i32x4));
					break;

				case 0xb8: // i32x4.max_s
					SOUP_RETHROW_FALSE(simd_pmax(stack, &WasmValue::i32x4));
					break;

				case 0xb9: // i32x4.max_u
					SOUP_RETHROW_FALSE(simd_max_u(stack, &WasmValue::i32x4));
					break;

				case 0xba: // i32x4.dot_i16x8_s
					{
						WASM_CHECK_STACK(1);
						int16_t b[8]; memcpy(b, stack.back().i16x8, 16); stack.pop_back();
						int16_t a[8]; memcpy(a, stack.back().i16x8, 16);
						int32_t* res = stack.back().i32x4;
						for (int i = 0; i != 4; ++i)
						{
							res[i] = static_cast<int32_t>(a[i * 2]) * static_cast<int32_t>(b[i * 2])
								+ static_cast<int32_t>(a[i * 2 + 1]) * static_cast<int32_t>(b[i * 2 + 1])
								;
						}
					}
					break;

				case 0xc0: // i64x2.abs
					SOUP_RETHROW_FALSE(simd_abs(stack, &WasmValue::i64x2));
					break;

				case 0xc1: // i64x2.neg
					SOUP_RETHROW_FALSE(simd_neg(stack, &WasmValue::i64x2));
					break;

				case 0xc4: // i64x2.bitmask
					SOUP_RETHROW_FALSE(simd_bitmask(stack, &WasmValue::i64x2));
					break;

				case 0xc7: // i64x2.extend_low_i32x4_s
					SOUP_RETHROW_FALSE(simd_extend_low_s(stack, &WasmValue::i32x4, &WasmValue::i64x2));
					break;

				case 0xc8: // i64x2.extend_high_i32x4_s
					SOUP_RETHROW_FALSE(simd_extend_high_s(stack, &WasmValue::i32x4, &WasmValue::i64x2));
					break;

				case 0xc9: // i64x2.extend_low_i32x4_u
					SOUP_RETHROW_FALSE(simd_extend_low_u(stack, &WasmValue::i32x4, &WasmValue::i64x2));
					break;

				case 0xca: // i64x2.extend_high_i32x4_u
					SOUP_RETHROW_FALSE(simd_extend_high_u(stack, &WasmValue::i32x4, &WasmValue::i64x2));
					break;

				case 0xcb: // i64x2.shl
					SOUP_RETHROW_FALSE(simd_shl(stack, &WasmValue::i64x2));
					break;

				case 0xcc: // i64x2.shr_s
					SOUP_RETHROW_FALSE(simd_shr_s(stack, &WasmValue::i64x2));
					break;

				case 0xcd: // i64x2.shr_u
					SOUP_RETHROW_FALSE(simd_shr_u(stack, &WasmValue::i64x2));
					break;

				case 0xce: // i64x2.add
					SOUP_RETHROW_FALSE(simd_add(stack, &WasmValue::i64x2));
					break;

				case 0xd1: // i64x2.sub
					SOUP_RETHROW_FALSE(simd_sub(stack, &WasmValue::i64x2));
					break;

				case 0xd5: // i64x2.mul
					SOUP_RETHROW_FALSE(simd_mul(stack, &WasmValue::i64x2));
					break;

				case 0x67: // f32x4.ceil
					SOUP_RETHROW_FALSE(simd_ceil(stack, &WasmValue::f32x4));
					break;

				case 0x68: // f32x4.floor
					SOUP_RETHROW_FALSE(simd_floor(stack, &WasmValue::f32x4));
					break;

				case 0x69: // f32x4.trunc
					SOUP_RETHROW_FALSE(simd_trunc(stack, &WasmValue::f32x4));
					break;

				case 0x6a: // f32x4.nearest
					SOUP_RETHROW_FALSE(simd_nearest(stack, &WasmValue::f32x4));
					break;

				case 0x74: // f64x2.ceil
					SOUP_RETHROW_FALSE(simd_ceil(stack, &WasmValue::f64x2));
					break;

				case 0x75: // f64x2.floor
					SOUP_RETHROW_FALSE(simd_floor(stack, &WasmValue::f64x2));
					break;

				case 0x7a: // f64x2.trunc
					SOUP_RETHROW_FALSE(simd_trunc(stack, &WasmValue::f64x2));
					break;

				case 0x94: // f64x2.nearest
					SOUP_RETHROW_FALSE(simd_nearest(stack, &WasmValue::f64x2));
					break;

				case 0xe0: // f32x4.abs
					SOUP_RETHROW_FALSE(simd_abs(stack, &WasmValue::f32x4));
					break;

				case 0xe1: // f32x4.neg
					SOUP_RETHROW_FALSE(simd_neg(stack, &WasmValue::f32x4));
					break;

				case 0xe3: // f32x4.sqrt
					SOUP_RETHROW_FALSE(simd_sqrt(stack, &WasmValue::f32x4));
					break;

				case 0xe4: // f32x4.add
					SOUP_RETHROW_FALSE(simd_add(stack, &WasmValue::f32x4));
					break;

				case 0xe5: // f32x4.sub
					SOUP_RETHROW_FALSE(simd_sub(stack, &WasmValue::f32x4));
					break;

				case 0xe6: // f32x4.mul
					SOUP_RETHROW_FALSE(simd_mul(stack, &WasmValue::f32x4));
					break;

				case 0xe7: // f32x4.div
					SOUP_RETHROW_FALSE(simd_div(stack, &WasmValue::f32x4));
					break;

				case 0xe8: // f32x4.min
					SOUP_RETHROW_FALSE(simd_min(stack, &WasmValue::f32x4));
					break;

				case 0xe9: // f32x4.max
					SOUP_RETHROW_FALSE(simd_max(stack, &WasmValue::f32x4));
					break;

				case 0xea: // f32x4.pmin
					SOUP_RETHROW_FALSE(simd_pmin(stack, &WasmValue::f32x4));
					break;

				case 0xeb: // f32x4.pmax
					SOUP_RETHROW_FALSE(simd_pmax(stack, &WasmValue::f32x4));
					break;

				case 0xec: // f64x2.abs
					SOUP_RETHROW_FALSE(simd_abs(stack, &WasmValue::f64x2));
					break;

				case 0xed: // f64x2.neg
					SOUP_RETHROW_FALSE(simd_neg(stack, &WasmValue::f64x2));
					break;

				case 0xef: // f64x2.sqrt
					SOUP_RETHROW_FALSE(simd_sqrt(stack, &WasmValue::f64x2));
					break;

				case 0xf0: // f64x2.add
					SOUP_RETHROW_FALSE(simd_add(stack, &WasmValue::f64x2));
					break;

				case 0xf1: // f64x2.sub
					SOUP_RETHROW_FALSE(simd_sub(stack, &WasmValue::f64x2));
					break;

				case 0xf2: // f64x2.mul
					SOUP_RETHROW_FALSE(simd_mul(stack, &WasmValue::f64x2));
					break;

				case 0xf3: // f64x2.div
					SOUP_RETHROW_FALSE(simd_div(stack, &WasmValue::f64x2));
					break;

				case 0xf4: // f64x2.min
					SOUP_RETHROW_FALSE(simd_min(stack, &WasmValue::f64x2));
					break;

				case 0xf5: // f64x2.max
					SOUP_RETHROW_FALSE(simd_max(stack, &WasmValue::f64x2));
					break;

				case 0xf6: // f64x2.pmin
					SOUP_RETHROW_FALSE(simd_pmin(stack, &WasmValue::f64x2));
					break;

				case 0xf7: // f64x2.pmax
					SOUP_RETHROW_FALSE(simd_pmax(stack, &WasmValue::f64x2));
					break;

				case 0xf8: // i32x4.trunc_sat_f32x4_s
					WASM_CHECK_STACK(1);
					for (int i = 0; i != 4; ++i)
					{
						float_to_int(stack.back().f32x4[i], stack.back().i32x4[i]);
					}
					break;

				case 0xf9: // i32x4.trunc_sat_f32x4_u
					WASM_CHECK_STACK(1);
					for (int i = 0; i != 4; ++i)
					{
						float_to_int(stack.back().f32x4[i], reinterpret_cast<uint32_t&>(stack.back().i32x4[i]));
					}
					break;

				case 0xfa: // f32x4.convert_i32x4_s
					WASM_CHECK_STACK(1);
					for (int i = 0; i != 4; ++i)
					{
						stack.back().f32x4[i] = static_cast<float>(stack.back().i32x4[i]);
					}
					break;

				case 0xfb: // f32x4.convert_i32x4_u
					WASM_CHECK_STACK(1);
					for (int i = 0; i != 4; ++i)
					{
						stack.back().f32x4[i] = static_cast<float>(static_cast<uint32_t>(stack.back().i32x4[i]));
					}
					break;

				case 0x5c: // v128.load32_zero
					SOUP_RETHROW_FALSE(load_zero(stack, r, script, &WasmValue::i32x4));
					break;

				case 0x5d: // v128.load64_zero
					SOUP_RETHROW_FALSE(load_zero(stack, r, script, &WasmValue::i64x2));
					break;

				case 0x9c: // i16x8.extmul_low_i8x16_s
					SOUP_RETHROW_FALSE(extmul_low_s(stack, &WasmValue::i8x16, &WasmValue::i16x8));
					break;

				case 0x9d: // i16x8.extmul_high_i8x16_s
					SOUP_RETHROW_FALSE(extmul_high_s(stack, &WasmValue::i8x16, &WasmValue::i16x8));
					break;

				case 0x9e: // i16x8.extmul_low_i8x16_u
					SOUP_RETHROW_FALSE(extmul_low_u(stack, &WasmValue::i8x16, &WasmValue::i16x8));
					break;

				case 0x9f: // i16x8.extmul_high_i8x16_u
					SOUP_RETHROW_FALSE(extmul_high_u(stack, &WasmValue::i8x16, &WasmValue::i16x8));
					break;

				case 0xbc: // i32x4.extmul_low_i16x8_s
					SOUP_RETHROW_FALSE(extmul_low_s(stack, &WasmValue::i16x8, &WasmValue::i32x4));
					break;

				case 0xbd: // i32x4.extmul_high_i16x8_s
					SOUP_RETHROW_FALSE(extmul_high_s(stack, &WasmValue::i16x8, &WasmValue::i32x4));
					break;

				case 0xbe: // i32x4.extmul_low_i16x8_u
					SOUP_RETHROW_FALSE(extmul_low_u(stack, &WasmValue::i16x8, &WasmValue::i32x4));
					break;

				case 0xbf: // i32x4.extmul_high_i16x8_u
					SOUP_RETHROW_FALSE(extmul_high_u(stack, &WasmValue::i16x8, &WasmValue::i32x4));
					break;

				case 0xdc: // i64x2.extmul_low_i32x4_s
					SOUP_RETHROW_FALSE(extmul_low_s(stack, &WasmValue::i32x4, &WasmValue::i64x2));
					break;

				case 0xdd: // i64x2.extmul_high_i32x4_s
					SOUP_RETHROW_FALSE(extmul_high_s(stack, &WasmValue::i32x4, &WasmValue::i64x2));
					break;

				case 0xde: // i64x2.extmul_low_i32x4_u
					SOUP_RETHROW_FALSE(extmul_low_u(stack, &WasmValue::i32x4, &WasmValue::i64x2));
					break;

				case 0xdf: // i64x2.extmul_high_i32x4_u
					SOUP_RETHROW_FALSE(extmul_high_u(stack, &WasmValue::i32x4, &WasmValue::i64x2));
					break;

				case 0x82: // i16x8.q15mulr_sat_s
					WASM_CHECK_STACK(2);
					{
						int16_t b[8]; memcpy(b, stack.back().i16x8, 16); stack.pop_back();
						auto a = stack.back().i16x8;
						for (int i = 0; i != 8; ++i)
						{
							a[i] = saturate<int16_t>((static_cast<int32_t>(a[i]) * static_cast<int32_t>(b[i]) + 0x4000) >> 15);
						}
					}
					break;

				case 0x53: // v128.any_true
					WASM_CHECK_STACK(1);
					if (stack.back().i64x2[0] || stack.back().i64x2[1])
					{
						stack.back().i64 = 1;
					}
					else
					{
						stack.back().i64 = 0;
					}
					stack.back().type = WASM_I32;
					break;

				case 0x54: // v128.load8_lane
					SOUP_RETHROW_FALSE(load_lane(stack, r, script, &WasmValue::i8x16));
					break;

				case 0x55: // v128.load16_lane
					SOUP_RETHROW_FALSE(load_lane(stack, r, script, &WasmValue::i16x8));
					break;

				case 0x56: // v128.load32_lane
					SOUP_RETHROW_FALSE(load_lane(stack, r, script, &WasmValue::i32x4));
					break;

				case 0x57: // v128.load64_lane
					SOUP_RETHROW_FALSE(load_lane(stack, r, script, &WasmValue::i64x2));
					break;

				case 0x58: // v128.store8_lane
					SOUP_RETHROW_FALSE(store_lane(stack, r, script, &WasmValue::i8x16));
					break;

				case 0x59: // v128.store16_lane
					SOUP_RETHROW_FALSE(store_lane(stack, r, script, &WasmValue::i16x8));
					break;

				case 0x5a: // v128.store32_lane
					SOUP_RETHROW_FALSE(store_lane(stack, r, script, &WasmValue::i32x4));
					break;

				case 0x5b: // v128.store64_lane
					SOUP_RETHROW_FALSE(store_lane(stack, r, script, &WasmValue::i64x2));
					break;

				case 0xd6: // i64x2.eq
					SOUP_RETHROW_FALSE(simd_eq(stack, &WasmValue::i64x2, &WasmValue::i64x2));
					break;

				case 0xd7: // i64x2.ne
					SOUP_RETHROW_FALSE(simd_ne(stack, &WasmValue::i64x2, &WasmValue::i64x2));
					break;

				case 0xd8: // i64x2.lt_s
					SOUP_RETHROW_FALSE(simd_lt(stack, &WasmValue::i64x2, &WasmValue::i64x2));
					break;

				case 0xd9: // i64x2.gt_s
					SOUP_RETHROW_FALSE(simd_gt(stack, &WasmValue::i64x2, &WasmValue::i64x2));
					break;

				case 0xda: // i64x2.le_s
					SOUP_RETHROW_FALSE(simd_le(stack, &WasmValue::i64x2, &WasmValue::i64x2));
					break;

				case 0xdb: // i64x2.ge_s
					SOUP_RETHROW_FALSE(simd_ge(stack, &WasmValue::i64x2, &WasmValue::i64x2));
					break;

				case 0xc3: // i64x2.all_true
					SOUP_RETHROW_FALSE(all_true(stack, &WasmValue::i64x2));
					break;

				case 0xfe: // f64x2.convert_low_i32x4_s
					{
						WASM_CHECK_STACK(1);
						const auto in0 = stack.back().i32x4[0];
						const auto in1 = stack.back().i32x4[1];
						stack.back().f64x2[0] = static_cast<double>(in0);
						stack.back().f64x2[1] = static_cast<double>(in1);
					}
					break;

				case 0xff: // f64x2.convert_low_i32x4_u
					{
						WASM_CHECK_STACK(1);
						const auto in0 = stack.back().i32x4[0];
						const auto in1 = stack.back().i32x4[1];
						stack.back().f64x2[0] = static_cast<double>(static_cast<uint32_t>(in0));
						stack.back().f64x2[1] = static_cast<double>(static_cast<uint32_t>(in1));
					}
					break;

				case 0xfc: // i32x4.trunc_sat_f64x2_s_zero
					WASM_CHECK_STACK(1);
					for (int i = 0; i != 2; ++i)
					{
						float_to_int(stack.back().f64x2[i], stack.back().i32x4[i]);
					}
					for (int i = 2; i != 4; ++i)
					{
						stack.back().i32x4[i] = 0;
					}
					break;

				case 0xfd: // i32x4.trunc_sat_f64x2_u_zero
					WASM_CHECK_STACK(1);
					for (int i = 0; i != 2; ++i)
					{
						float_to_int(stack.back().f64x2[i], reinterpret_cast<uint32_t&>(stack.back().i32x4[i]));
					}
					for (int i = 2; i != 4; ++i)
					{
						stack.back().i32x4[i] = 0;
					}
					break;

				case 0x5e: // f32x4.demote_f64x2_zero
					{
						WASM_CHECK_STACK(1);
						const auto in0 = stack.back().f64x2[0];
						const auto in1 = stack.back().f64x2[1];
						stack.back().f32x4[0] = static_cast<float>(in0);
						stack.back().f32x4[1] = static_cast<float>(in1);
						stack.back().f32x4[2] = 0.0f;
						stack.back().f32x4[3] = 0.0f;
					}
					break;

				case 0x5f: // f64x2.promote_low_f32x4
					{
						WASM_CHECK_STACK(1);
						const auto in0 = stack.back().f32x4[0];
						const auto in1 = stack.back().f32x4[1];
						stack.back().f64x2[0] = static_cast<double>(in0);
						stack.back().f64x2[1] = static_cast<double>(in1);
					}
					break;

				case 0x62: // i8x16.popcnt
					SOUP_RETHROW_FALSE(simd_popcnt(stack, &WasmValue::i8x16));
					break;

				case 0x7c: // i16x8.extadd_pairwise_i8x16_s
					SOUP_RETHROW_FALSE(extadd_pairwise_s(stack, &WasmValue::i8x16, &WasmValue::i16x8));
					break;

				case 0x7d: // i16x8.extadd_pairwise_i8x16_u
					SOUP_RETHROW_FALSE(extadd_pairwise_u(stack, &WasmValue::i8x16, &WasmValue::i16x8));
					break;

				case 0x7e: // i32x4.extadd_pairwise_i16x8_s
					SOUP_RETHROW_FALSE(extadd_pairwise_s(stack, &WasmValue::i16x8, &WasmValue::i32x4));
					break;

				case 0x7f: // i32x4.extadd_pairwise_i16x8_u
					SOUP_RETHROW_FALSE(extadd_pairwise_u(stack, &WasmValue::i16x8, &WasmValue::i32x4));
					break;

				default:
#if DEBUG_VM
					std::cout << "Unsupported opcode: FD " << string::hex(simdop) << "\n";
#endif
					return CODE_ERROR;
				}
			} break;
#endif
			}
		}
		return CODE_RETURN;
	}

	WasmVm::SkipOverBranchResult WasmVm::skipOverBranch(Reader& r, uint32_t target_depth, WasmScript& script, uint32_t func_index) SOUP_EXCAL
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
					return ELSE_REACHED;
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
					return END_REACHED;
				}
				++depth;
				break;

#if SOUP_WASM_EXCEPTIONS
			case 0x08: // throw
#endif
			case 0x0c: // br
			case 0x0d: // br_if
			case 0x10: // call
#if SOUP_WASM_TAIL_CALL
			case 0x12: // return_call
#endif
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
#if SOUP_WASM_TAIL_CALL
			case 0x13: // return_call_indirect
#endif
				{
					uint32_t type_index; WASM_READ_OML(type_index);
					uint32_t table_index; WASM_READ_OML(table_index);
				}
				break;

			case 0x1c: // select t
				r.skip(2);
				break;

#if SOUP_WASM_EXCEPTIONS
			case 0x1f: // try_table
				{
					--depth;
					int32_t result_type; WASM_READ_SOML(result_type);
					uint32_t num_catches; WASM_READ_OML(num_catches);
					while (num_catches--)
					{
						uint8_t type; r.u8(type);
						if (type < 0x02) { uint32_t tagidx; WASM_READ_OML(tagidx); }
						uint32_t labelidx; WASM_READ_OML(labelidx);
					}
				}
				break;
#endif

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
					WASM_READ_MEMARG;
					SOUP_UNUSED(memidx);
#if SOUP_WASM_PEDANTIC
	#if SOUP_WASM_MULTI_MEMORY
					SOUP_RETHROW_FALSE(align < 0x80);
	#else
					SOUP_RETHROW_FALSE(align < 0x20);
	#endif
#endif
				}
				break;

			case 0x3f: // memory.size
			case 0x40: // memory.grow
				{
#if SOUP_WASM_PEDANTIC && !SOUP_WASM_MULTI_MEMORY
					uint8_t memidx;
					r.u8(memidx);
					SOUP_RETHROW_FALSE(memidx == 0);
#else
					uint32_t memidx;
					WASM_READ_OML(memidx);
#endif
				}
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
			case 0x0a: // throw_ref
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
#if SOUP_WASM_PEDANTIC
					SOUP_RETHROW_FALSE(script.has_data_count_section);
					[[fallthrough]];
#endif
				case 0x0a: // memory.copy
				case 0x0c: // table.init
				case 0x0e: // table.copy
					{
						uint32_t imm1;
						WASM_READ_OML(imm1);
						uint32_t imm2;
						WASM_READ_OML(imm2);
					}
					break;

				case 0x09: // data.drop
#if SOUP_WASM_PEDANTIC
					SOUP_RETHROW_FALSE(script.has_data_count_section);
					[[fallthrough]];
#endif
				case 0x0b: // memory.fill
				case 0x0d: // elem.drop
				case 0x0f: // table.grow
				case 0x10: // table.size
				case 0x11: // table.fill
					{
						uint32_t imm;
						WASM_READ_OML(imm);
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

#if SOUP_WASM_SIMD
			case 0xfd: {
				uint32_t simdop;
				WASM_READ_OML(simdop);
				switch (simdop)
				{
				case 0x00: // v128.load
				case 0x01: // v128.load8x8_s
				case 0x02: // v128.load8x8_u
				case 0x03: // v128.load16x4_s
				case 0x04: // v128.load16x4_u
				case 0x05: // v128.load32x2_s
				case 0x06: // v128.load32x2_u
				case 0x07: // v128.load8_splat
				case 0x08: // v128.load16_splat
				case 0x09: // v128.load32_splat
				case 0x0a: // v128.load64_splat
				case 0x0b: // v128.store
				case 0x5c: // v128.load32_zero
				case 0x5d: // v128.load64_zero
					{
						WASM_READ_MEMARG;
						SOUP_UNUSED(memidx);
					}
					break;

				case 0x0c: // v128.const
				case 0x0d: // i8x16.shuffle
					r.skip(16);
					break;

				case 0x15: // i8x16.extract_lane_s
				case 0x16: // i8x16.extract_lane_u
				case 0x17: // i8x16.replace_lane
				case 0x18: // i16x8.extract_lane_s
				case 0x19: // i16x8.extract_lane_u
				case 0x1a: // i16x8.replace_lane
				case 0x1b: // i32x4.extract_lane
				case 0x1c: // i32x4.replace_lane
				case 0x1d: // i64x2.extract_lane
				case 0x1e: // i64x2.replace_lane
				case 0x1f: // f32x4.extract_lane
				case 0x20: // f32x4.replace_lane
				case 0x21: // f64x2.extract_lane
				case 0x22: // f64x2.replace_lane
					r.skip(1); // lane idx
					break;

				case 0x54: // v128.load8_lane
				case 0x55: // v128.load16_lane
				case 0x56: // v128.load32_lane
				case 0x57: // v128.load64_lane
				case 0x58: // v128.store8_lane
				case 0x59: // v128.store16_lane
				case 0x5a: // v128.store32_lane
				case 0x5b: // v128.store64_lane
					{
						WASM_READ_MEMARG;
						SOUP_UNUSED(memidx);
					}
					r.skip(1); // lane idx
					break;

#if DEBUG_VM
				case 0x0e: // i8x16.swizzle
				case 0x0f: // i8x16.splat
				case 0x10: // i16x8.splat
				case 0x11: // i32x4.splat
				case 0x12: // i64x2.splat
				case 0x13: // f32x4.splat
				case 0x14: // f64x2.splat
				case 0x23: // i8x16.eq
				case 0x24: // i8x16.ne
				case 0x25: // i8x16.lt_s
				case 0x26: // i8x16.lt_u
				case 0x27: // i8x16.gt_s
				case 0x28: // i8x16.gt_u
				case 0x29: // i8x16.le_s
				case 0x2a: // i8x16.le_u
				case 0x2b: // i8x16.ge_s
				case 0x2c: // i8x16.ge_u
				case 0x2d: // i16x8.eq
				case 0x2e: // i16x8.ne
				case 0x2f: // i16x8.lt_s
				case 0x30: // i16x8.lt_u
				case 0x31: // i16x8.gt_s
				case 0x32: // i16x8.gt_u
				case 0x33: // i16x8.le_s
				case 0x34: // i16x8.le_u
				case 0x35: // i16x8.ge_s
				case 0x36: // i16x8.ge_u
				case 0x37: // i32x4.eq
				case 0x38: // i32x4.ne
				case 0x39: // i32x4.lt_s
				case 0x3a: // i32x4.lt_u
				case 0x3b: // i32x4.gt_s
				case 0x3c: // i32x4.gt_u
				case 0x3d: // i32x4.le_s
				case 0x3e: // i32x4.le_u
				case 0x3f: // i32x4.ge_s
				case 0x40: // i32x4.ge_u
				case 0x41: // f32x4.eq
				case 0x42: // f32x4.ne
				case 0x43: // f32x4.lt
				case 0x44: // f32x4.gt
				case 0x45: // f32x4.le
				case 0x46: // f32x4.ge
				case 0x47: // f64x2.eq
				case 0x48: // f64x2.ne
				case 0x49: // f64x2.lt
				case 0x4a: // f64x2.gt
				case 0x4b: // f64x2.le
				case 0x4c: // f64x2.ge
				case 0x4d: // v128.not
				case 0x4e: // v128.and
				case 0x4f: // v128.andnot
				case 0x50: // v128.or
				case 0x51: // v128.xor
				case 0x52: // v128.bitselect
				case 0x60: // i8x16.abs
				case 0x61: // i8x16.neg
				case 0x63: // i8x16.all_true
				case 0x64: // i8x16.bitmask
				case 0x65: // i8x16.narrow_i16x8_s
				case 0x66: // i8x16.narrow_i16x8_u
				case 0x6b: // i8x16.shl
				case 0x6c: // i8x16.shr_s
				case 0x6d: // i8x16.shr_u
				case 0x6e: // i8x16.add
				case 0x6f: // i8x16.add_sat_s
				case 0x70: // i8x16.add_sat_u
				case 0x71: // i8x16.sub
				case 0x72: // i8x16.sub_sat_s
				case 0x73: // i8x16.sub_sat_u
				case 0x76: // i8x16.min_s
				case 0x77: // i8x16.min_u
				case 0x78: // i8x16.max_s
				case 0x79: // i8x16.max_u
				case 0x7b: // i8x16.avgr_u
				case 0x80: // i16x8.abs
				case 0x81: // i16x8.neg
				case 0x83: // i16x8.all_true
				case 0x84: // i16x8.bitmask
				case 0x85: // i16x8.narrow_i32x4_s
				case 0x86: // i16x8.narrow_i32x4_u
				case 0x87: // i16x8.extend_low_i8x16_s
				case 0x88: // i16x8.extend_high_i8x16_s
				case 0x89: // i16x8.extend_low_i8x16_u
				case 0x8a: // i16x8.extend_high_i8x16_u
				case 0x8b: // i16x8.shl
				case 0x8c: // i16x8.shr_s
				case 0x8d: // i16x8.shr_u
				case 0x8e: // i16x8.add
				case 0x8f: // i16x8.add_sat_s
				case 0x90: // i16x8.add_sat_u
				case 0x91: // i16x8.sub
				case 0x92: // i16x8.sub_sat_s
				case 0x93: // i16x8.sub_sat_u
				case 0x95: // i16x8.mul
				case 0x96: // i16x8.min_s
				case 0x97: // i16x8.min_u
				case 0x98: // i16x8.max_s
				case 0x99: // i16x8.max_u
				case 0x9b: // i16x8.avgr_u
				case 0xa0: // i32x4.abs
				case 0xa1: // i32x4.neg
				case 0xa3: // i32x4.all_true
				case 0xa4: // i32x4.bitmask
				case 0xa7: // i32x4.extend_low_i16x8_s
				case 0xa8: // i32x4.extend_high_i16x8_s
				case 0xa9: // i32x4.extend_low_i16x8_u
				case 0xaa: // i32x4.extend_high_i16x8_u
				case 0xab: // i32x4.shl
				case 0xac: // i32x4.shr_s
				case 0xad: // i32x4.shr_u
				case 0xae: // i32x4.add
				case 0xb1: // i32x4.sub
				case 0xb5: // i32x4.mul
				case 0xb6: // i32x4.min_s
				case 0xb7: // i32x4.min_u
				case 0xb8: // i32x4.max_s
				case 0xb9: // i32x4.max_u
				case 0xba: // i32x4.dot_i16x8_s
				case 0xc0: // i64x2.abs
				case 0xc1: // i64x2.neg
				case 0xc4: // i64x2.bitmask
				case 0xc7: // i64x2.extend_low_i32x4_s
				case 0xc8: // i64x2.extend_high_i32x4_s
				case 0xc9: // i64x2.extend_low_i32x4_u
				case 0xca: // i64x2.extend_high_i32x4_u
				case 0xcb: // i64x2.shl
				case 0xcc: // i64x2.shr_s
				case 0xcd: // i64x2.shr_u
				case 0xce: // i64x2.add
				case 0xd1: // i64x2.sub
				case 0xd5: // i64x2.mul
				case 0x67: // f32x4.ceil
				case 0x68: // f32x4.floor
				case 0x69: // f32x4.trunc
				case 0x6a: // f32x4.nearest
				case 0x74: // f64x2.ceil
				case 0x75: // f64x2.floor
				case 0x7a: // f64x2.trunc
				case 0x94: // f64x2.nearest
				case 0xe0: // f32x4.abs
				case 0xe1: // f32x4.neg
				case 0xe3: // f32x4.sqrt
				case 0xe4: // f32x4.add
				case 0xe5: // f32x4.sub
				case 0xe6: // f32x4.mul
				case 0xe7: // f32x4.div
				case 0xe8: // f32x4.min
				case 0xe9: // f32x4.max
				case 0xea: // f32x4.pmin
				case 0xeb: // f32x4.pmax
				case 0xec: // f64x2.abs
				case 0xed: // f64x2.neg
				case 0xef: // f64x2.sqrt
				case 0xf0: // f64x2.add
				case 0xf1: // f64x2.sub
				case 0xf2: // f64x2.mul
				case 0xf3: // f64x2.div
				case 0xf4: // f64x2.min
				case 0xf5: // f64x2.max
				case 0xf6: // f64x2.pmin
				case 0xf7: // f64x2.pmax
				case 0xf8: // i32x4.trunc_sat_f32x4_s
				case 0xf9: // i32x4.trunc_sat_f32x4_u
				case 0xfa: // f32x4.convert_i32x4_s
				case 0xfb: // f32x4.convert_i32x4_u
				case 0x9c: // i16x8.extmul_low_i8x16_s
				case 0x9d: // i16x8.extmul_high_i8x16_s
				case 0x9e: // i16x8.extmul_low_i8x16_u
				case 0x9f: // i16x8.extmul_high_i8x16_u
				case 0xbc: // i32x4.extmul_low_i16x8_s
				case 0xbd: // i32x4.extmul_high_i16x8_s
				case 0xbe: // i32x4.extmul_low_i16x8_u
				case 0xbf: // i32x4.extmul_high_i16x8_u
				case 0xdc: // i64x2.extmul_low_i32x4_s
				case 0xdd: // i64x2.extmul_high_i32x4_s
				case 0xde: // i64x2.extmul_low_i32x4_u
				case 0xdf: // i64x2.extmul_high_i32x4_u
				case 0x82: // i16x8.q15mulr_sat_s
				case 0x53: // v128.any_true
				case 0xd6: // i64x2.eq
				case 0xd7: // i64x2.ne
				case 0xd8: // i64x2.lt_s
				case 0xd9: // i64x2.gt_s
				case 0xda: // i64x2.le_s
				case 0xdb: // i64x2.ge_s
				case 0xc3: // i64x2.all_true
				case 0xfe: // f64x2.convert_low_i32x4_s
				case 0xff: // f64x2.convert_low_i32x4_u
				case 0xfc: // i32x4.trunc_sat_f64x2_s_zero
				case 0xfd: // i32x4.trunc_sat_f64x2_u_zero
				case 0x5e: // f32x4.demote_f64x2_zero
				case 0x5f: // f64x2.promote_low_f32x4
				case 0x62: // i8x16.popcnt
				case 0x7c: // i16x8.extadd_pairwise_i8x16_s
				case 0x7d: // i16x8.extadd_pairwise_i8x16_u
				case 0x7e: // i32x4.extadd_pairwise_i16x8_s
				case 0x7f: // i32x4.extadd_pairwise_i16x8_u
					break;

				default:
					std::cout << "skipOverBranch: unknown instruction FD " << string::hex(simdop) << ", might cause problems\n";
					break;
#endif
				}
			} break;
#endif

#if SOUP_WASM_PEDANTIC
			case 0xff:
				return PEDANTIC_ERROR;
#endif
			}
		}
#if DEBUG_VM
		std::cout << "skipOverBranch: end of stream reached\n";
#endif
		return INSUFFICIENT_DATA;
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

		const bool is_forward = ctrlflow.top().isForwardJump();
		if (is_forward)
		{
			// branch forwards
			if (skipOverBranch(r, depth, script, func_index) == ELSE_REACHED)
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

		if (is_forward)
		{
			ctrlflow.pop(); // we passed 'end', so need to pop here.
		}

		return true;
	}

	// function_index and type_index must be in-bounds.
	WasmVm::RunCodeResult WasmVm::doCall(WasmScript* script, uint32_t type_index, uint32_t function_index, unsigned depth)
	{
		SOUP_IF_UNLIKELY (depth >= 200)
		{
#if DEBUG_VM
			std::cout << "call: c stack overflow\n";
#endif
			return CODE_ERROR;
		}
		++depth;

	_doCall_other_script:

#if SOUP_WASM_PEDANTIC
		if (script == &this->script)
		{
			uint32_t func_type_index = script->getTypeIndexForFunction(function_index);
			if (type_index != func_type_index)
			{
				const auto& type = this->script.types[type_index];
#if false // already checked at load
				SOUP_IF_UNLIKELY (func_type_index >= script->types.size())
				{
#if DEBUG_VM
					std::cout << "call(pedantic): function type is out-of-bounds\n";
#endif
					return CODE_ERROR;
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
					return CODE_ERROR;
				}
			}
		}
#endif

		if (function_index < script->function_imports.size())
		{
			const auto& imp = script->function_imports[function_index];
#if DEBUG_VM || DEBUG_API
			std::cout << "Calling into " << imp.module_name << ":" << imp.field_name << "\n";
#endif
			if (imp.ptr)
			{
				const auto& type = this->script.types[type_index];
				imp.ptr(*this, imp.user_data, type);
				return CODE_RETURN;
			}
			SOUP_IF_UNLIKELY (!imp.source)
			{
#if DEBUG_VM
				std::cout << "call: unresolved function import\n";
#endif
				return CODE_ERROR;
			}

			script = imp.source;
			function_index = imp.func_index;
			goto _doCall_other_script;
		}
		const auto code_index = function_index - script->function_imports.size();
#if false // already checked by caller
		SOUP_IF_UNLIKELY (code_index >= script->code.size())
		{
#if DEBUG_VM
			std::cout << "call: function is out-of-bounds\n";
#endif
			return CODE_ERROR;
		}
#endif
		const auto& type = this->script.types[type_index];

		WasmVm callvm(*script);
		SOUP_RETHROW_FALSE(moveArguments(callvm, type));
#if DEBUG_VM
		//std::cout << "call: enter " << function_index << "\n";
		//std::cout << string::bin2hex(script->code[code_index]) << "\n";
#endif
#if WASM_CALL_REUSES_STACK
		const auto pre_call_stack_size = stack.size();
		callvm.stack = std::move(stack);
#endif
		const auto result = callvm.run(script->code[code_index], depth, function_index);
#if WASM_CALL_REUSES_STACK
		stack = std::move(callvm.stack);
#endif
#if SOUP_WASM_EXCEPTIONS
		current_throw_tag = callvm.current_throw_tag;
#endif
#if DEBUG_VM
		//std::cout << "call: leave " << function_index << "\n";
#endif
#if WASM_CALL_REUSES_STACK
		SOUP_IF_LIKELY (result == WasmVm::CODE_RETURN)
		{
			if (const auto result_stack_size = pre_call_stack_size + type.results.size(); stack.size() > result_stack_size)
			{
				stack.erase(stack.begin() + pre_call_stack_size, stack.end() - type.results.size());
			}
		}
#else
		SOUP_IF_UNLIKELY (callvm.stack.size() < type.results.size())
		{
#if DEBUG_VM
			std::cout << "call: not enough values on the stack after return\n";
#endif
			return CODE_ERROR;
		}
		this->stack.insert(this->stack.end(), callvm.stack.end() - type.results.size(), callvm.stack.end());
#endif
		return result;
	}

	bool WasmVm::moveArguments(WasmVm& callvm, const WasmFunctionType& type)
	{
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
		return true;
	}

#if SOUP_WASM_EXCEPTIONS
	bool WasmVm::doThrow(Reader& r, uint32_t func_index, std::stack<CtrlFlowEntry>& ctrlflow) noexcept
	{
		while (!ctrlflow.empty())
		{
			if (ctrlflow.top().is_try_table)
			{
				r.seek(ctrlflow.top().position);
				uint32_t num_catches; WASM_READ_OML(num_catches);
				uint32_t handler_depth = 0;
				bool push_ref = false;
				while (num_catches--)
				{
					uint8_t type; r.u8(type);
					uint32_t tagidx = -1;
					if (type < 0x02) { WASM_READ_OML(tagidx); }
					uint32_t labelidx; WASM_READ_OML(labelidx);
					if (handler_depth == 0
						&& (tagidx >= script.tags.size() || script.tags[tagidx].get() == current_throw_tag)
						)
					{
						handler_depth = labelidx + 1; // try_table will 'end' but apparently is not counted in the labelidx, so adding 1.
						push_ref = (type & 1);
					}
				}
				if (handler_depth != 0)
				{
#if DEBUG_VM
					std::cout << "unwind: destination labelidx " << handler_depth << "\n";
#endif
					skipOverBranch(r, handler_depth, script, func_index);
					ctrlflow.pop();
					if (push_ref)
					{
						stack.emplace_back(WASM_EXNREF).i64 = reinterpret_cast<uintptr_t>(current_throw_tag);
					}
					return true;
				}
			}
			ctrlflow.pop();
		}
		return false;
	}
#endif
}
