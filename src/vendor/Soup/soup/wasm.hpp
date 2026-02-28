#pragma once

#include "base.hpp"
#include "fwd.hpp"
#include "type_traits.hpp"

#include <stack>
#include <string>
#include <unordered_map>
#include <vector>

#include "math.hpp"
#include "Optional.hpp"
#include "SharedPtr.hpp"
#include "StructMap.hpp"

// Soup fully implements the WebAssembly 2.0 specification.

// By default, Soup uses the scalar profile, meaning SIMD is unavailable.
#ifndef SOUP_WASM_SIMD
#define SOUP_WASM_SIMD false
#endif

// Additionally, the following proposals that landed in WebAssembly 3.0 are implemented:
#ifndef SOUP_WASM_MEMORY64
#define SOUP_WASM_MEMORY64 (SOUP_BITS >= 64)
#endif
#ifndef SOUP_WASM_MULTI_MEMORY
#define SOUP_WASM_MULTI_MEMORY false
#endif
#ifndef SOUP_WASM_EXTENDED_CONST
#define SOUP_WASM_EXTENDED_CONST false
#endif
#ifndef SOUP_WASM_TAIL_CALL
#define SOUP_WASM_TAIL_CALL false
#endif
#ifndef SOUP_WASM_EXCEPTIONS
#define SOUP_WASM_EXCEPTIONS false
#endif
// Unimplemented WASM 3.0 proposals: Typed References, Garbage Collection, Relaxed SIMD + Deterministic Profile, and JS string builtins (dependent on GC proposal)

// And the following phase 3 proposals are implemented:
#ifndef SOUP_WASM_CUSTOM_PAGE_SIZES
#define SOUP_WASM_CUSTOM_PAGE_SIZES false
#endif

// When running Soup against the test suite, you may wish to enable pedantic mode.
// In pedantic mode, Soup spends more time when loading a WASM module to make sure all UTF-8 is valid, LEB128 encodings are not overlong, etc.
#ifndef SOUP_WASM_PEDANTIC
#define SOUP_WASM_PEDANTIC false
#endif

NAMESPACE_SOUP
{
	struct WasmSharedEnvironment;
	struct WasmVm;

	enum WasmType : uint8_t
	{
		WASM_I32 = 0x7F, // -1
		WASM_I64 = 0x7E, // -2
		WASM_F32 = 0x7D, // -3
		WASM_F64 = 0x7C, // -4
#if SOUP_WASM_SIMD
		WASM_V128 = 0x7B, // -5
#endif
		WASM_FUNCREF = 0x70,
		WASM_EXTERNREF = 0x6F,
#if SOUP_WASM_EXCEPTIONS
		WASM_EXNREF = 0x69, // exception ref
#endif
	};
	[[nodiscard]] WasmType wasm_type_from_string(const std::string& str) noexcept;
	[[nodiscard]] std::string wasm_type_to_string(WasmType type) SOUP_EXCAL;

	struct WasmFunctionType
	{
		std::vector<WasmType> parameters;
		std::vector<WasmType> results;

		[[nodiscard]] bool operator==(const WasmFunctionType& b) const noexcept { return parameters == b.parameters && results == b.results; }
		[[nodiscard]] bool operator!=(const WasmFunctionType& b) const noexcept { return !operator==(b); }

		[[nodiscard]] std::string toString() const SOUP_EXCAL;
	};

	using wasm_ffi_func_t = void(*)(WasmVm&, uint32_t user_data, const WasmFunctionType&);

	struct WasmValue
	{
		union
		{
			struct
			{
				union
				{
					int32_t i32;
					float f32;
				};
				int32_t hi32;
			};
			int64_t i64;
			double f64;
#if SOUP_WASM_SIMD
			int8_t i8x16[16];
			int16_t i16x8[8];
			int32_t i32x4[4];
			int64_t i64x2[2];
			float f32x4[4];
			double f64x2[2];
#endif
		};
		WasmType type;
		bool mut; // only used for globals

		SOUP_CONSTEXPR20 WasmValue() noexcept : i64(0), type(static_cast<WasmType>(0)) {}
		SOUP_CONSTEXPR20 WasmValue(WasmType type) noexcept : i64(0), type(type) {}
		SOUP_CONSTEXPR20 WasmValue(int32_t i32) noexcept : i32(i32), hi32(0), type(WASM_I32) {}
		SOUP_CONSTEXPR20 WasmValue(uint32_t u32) noexcept : WasmValue(static_cast<int32_t>(u32)) {}
		SOUP_CONSTEXPR20 WasmValue(int64_t i64) noexcept : i64(i64), type(WASM_I64) {}
		SOUP_CONSTEXPR20 WasmValue(uint64_t u64) noexcept : WasmValue(static_cast<int64_t>(u64)) {}
		SOUP_CONSTEXPR20 WasmValue(float f32) noexcept : f32(f32), hi32(0), type(WASM_F32) {}
		SOUP_CONSTEXPR20 WasmValue(double f64) noexcept : f64(f64), type(WASM_F64) {}
		WasmValue(void* ptr) noexcept : i64(reinterpret_cast<uintptr_t>(ptr)), type(WASM_EXTERNREF) {}

		[[nodiscard]] bool operator==(const WasmValue& b) const noexcept
		{
#if SOUP_WASM_SIMD
			if (type != b.type)
			{
				return false;
			}
			if (type == WASM_V128)
			{
				return memcmp(i8x16, b.i8x16, 16) == 0;
			}
			return i64 == b.i64;
#else
			return i64 == b.i64 && type == b.type;
#endif
		}

		[[nodiscard]] bool operator!=(const WasmValue& b) const noexcept { return !operator==(b); }

		[[nodiscard]] size_t uptr() const noexcept
		{
#if SOUP_WASM_MEMORY64
			return i64; // i32 values are guaranteed to have 0 in hi32.
#else
			return i32;
#endif
		}

		[[nodiscard]] std::string toString() const SOUP_EXCAL;

		template <typename T>
		T get() const noexcept;
	};
	template<> inline int8_t WasmValue::get<int8_t>() const noexcept { return i32; }
	template<> inline int16_t WasmValue::get<int16_t>() const noexcept { return i32; }
	template<> inline int32_t WasmValue::get<int32_t>() const noexcept { return i32; }
	template<> inline int64_t WasmValue::get<int64_t>() const noexcept { return i64; }
	template<> inline float WasmValue::get<float>() const noexcept { return f32; }
	template<> inline double WasmValue::get<double>() const noexcept { return f64; }

#if SOUP_WASM_MEMORY64
	using wasm_uptr_t = uint64_t;
#else
	using wasm_uptr_t = uint32_t;
#endif

	class WasmScript
	{
	public:
		struct DataSegment
		{
			uint32_t memidx;
#if SOUP_WASM_EXTENDED_CONST
			std::string base;
#else
			uint8_t base[12];
#endif
			std::string data;

			[[nodiscard]] bool isPassive() const noexcept { return memidx == -1; }
		};

		struct ElemSegment
		{
			const WasmType type;
			uint8_t flags;
#if !SOUP_WASM_EXTENDED_CONST
			uint8_t base[10];
#endif
			uint32_t tblidx;
#if SOUP_WASM_EXTENDED_CONST
			std::string base;
#endif
			std::vector<uint64_t> values;

			[[nodiscard]] bool isActive() const noexcept { return (flags & 1) == 0; }
			[[nodiscard]] bool isPassive() const noexcept { return !isActive() && (flags & 2) == 0; }
		};

		struct Memory
		{
			uint8_t* data;
			size_t size;
			uint64_t limit : 62;
			uint64_t memory64 : 1;
			uint64_t one_byte_pages : 1;

			Memory(wasm_uptr_t pages = 0, wasm_uptr_t max_pages = 0x10'000, bool _64bit = false) SOUP_EXCAL
				: Memory(pages * 0x10'000, static_cast<uint64_t>(max_pages) * 0x10'000, _64bit, false)
			{
			}

			Memory(size_t size, uint64_t limit, bool _64bit, bool one_byte_pages) SOUP_EXCAL;

			~Memory() noexcept;

			[[nodiscard]] unsigned getPageSize() const noexcept
			{
#if SOUP_WASM_CUSTOM_PAGE_SIZES
				if (one_byte_pages)
				{
					return 1;
				}
#endif
				return 0x10'000;
			}

			[[nodiscard]] void* getView(size_t addr, size_t size) noexcept
			{
				SOUP_IF_LIKELY (can_add_without_overflow(addr, size) && addr + size <= this->size)
				{
					return &this->data[addr];
				}
				return nullptr;
			}

			template <typename T>
			[[nodiscard]] T* getPointer(size_t addr) noexcept
			{
				SOUP_IF_LIKELY (auto ptr = getView(addr, sizeof(T)))
				{
					return (T*)ptr;
				}
				return nullptr;
			}

			template <typename T>
			[[nodiscard]] T* getPointer(size_t base, size_t offset) noexcept
			{
				return can_add_without_overflow(base, offset) ? getPointer<T>(base + offset) : nullptr;
			}

			[[nodiscard]] std::string readString(size_t addr, size_t size) SOUP_EXCAL;
			[[nodiscard]] std::string readNullTerminatedString(size_t addr) SOUP_EXCAL; // a safe alternative to getPointer<const char>(addr)

			bool write(size_t addr, const void* src, size_t size) noexcept;
			bool write(const WasmValue& addr, const void* src, size_t size) noexcept;

			void encodeUPTR(WasmValue& out, size_t in) noexcept;

			[[nodiscard]] WasmType getAddrType() const noexcept
			{
#if SOUP_WASM_MEMORY64
				if (memory64)
				{
					return WASM_I64;
				}
#endif
				return WASM_I32;
			}

			size_t grow(size_t delta_pages) noexcept;
		};

		struct Table
		{
			const WasmType type;
#if SOUP_WASM_MEMORY64
			bool table64 = false;
#else
			uint32_t limit;
#endif
			std::vector<uint64_t> values;
#if SOUP_WASM_MEMORY64
			uint64_t limit;
#endif

			Table(WasmType type, wasm_uptr_t init_size = 0, wasm_uptr_t max_size = 0x10'000, bool _64bit = false) noexcept
				: type(type)
#if SOUP_WASM_MEMORY64
				, table64(_64bit)
#endif
				, limit(max_size)
			{
				while (init_size != values.size())
				{
					values.emplace_back();
				}
			}

			[[nodiscard]] WasmType getAddrType() const noexcept
			{
#if SOUP_WASM_MEMORY64
				if (table64)
				{
					return WASM_I64;
				}
#endif
				return WASM_I32;
			}

			size_t grow(size_t delta, int64_t value = 0) SOUP_EXCAL;
			bool init(WasmScript& scr, const ElemSegment& src, size_t dst_offset, size_t src_offset, size_t size) noexcept;
			bool copy(const Table& src, size_t dst_offset, size_t src_offset, size_t size) noexcept;
		};

		struct Import
		{
			std::string module_name;
			std::string field_name;
		};

		struct FunctionImport : public Import
		{
			wasm_ffi_func_t ptr;
			WasmScript* source;
			uint32_t type_index; // an index in WasmScript::types of the importing WasmScript
			union
			{
				uint32_t func_index; // to be used with `source` for WASM imports
				uint32_t user_data;
			};

			FunctionImport(std::string&& module_name, std::string&& function_name, uint32_t type_index)
				: Import{ std::move(module_name), std::move(function_name) }, ptr(nullptr), source(nullptr), type_index(type_index), func_index(-1)
			{
			}
		};

		struct MemoryImport : public Import
		{
			uint64_t min_bytes;
			uint64_t max_bytes : 62;
			uint64_t memory64 : 1;
			uint64_t one_byte_pages : 1;

			MemoryImport(std::string&& module_name, std::string&& field_name, uint64_t min_bytes, uint64_t max_bytes, bool _64bit, bool one_byte_pages) noexcept
				: Import{ std::move(module_name), std::move(field_name) }, min_bytes(min_bytes), max_bytes(max_bytes), memory64(_64bit), one_byte_pages(one_byte_pages)
			{
			}

			[[nodiscard]] bool isCompatibleWith(const Memory& mem) const noexcept;
		};

		struct GlobalImport : public Import
		{
			WasmType type;
			bool mut;
		};

		struct TableImport : public Import
		{
			WasmType type;
			bool table64;
			wasm_uptr_t min_size;
			wasm_uptr_t max_size;

			[[nodiscard]] bool isCompatibleWith(const Table& tbl) const noexcept;
		};

		enum ImportExportKind : uint8_t
		{
			IE_kFunction = 0,
			IE_kTable = 1,
			IE_kMemory = 2,
			IE_kGlobal = 3,
#if SOUP_WASM_EXCEPTIONS
			IE_kTag = 4,
#endif
		};

		struct Export
		{
			uint8_t kind;
			uint32_t index;
		};

#if SOUP_WASM_EXCEPTIONS
		struct Tag
		{
			//uint32_t type_index; // would also need a WasmScript* for reference at which point the GC would need to get involved and meh, we really don't need to know this information
		};
#endif

		std::vector<FunctionImport> function_imports{};
		std::vector<GlobalImport> global_imports{};
		std::vector<TableImport> table_imports{};
#if SOUP_WASM_MULTI_MEMORY
		std::vector<MemoryImport> memory_imports;
#else
		Optional<MemoryImport> memory_import;
#endif
#if SOUP_WASM_EXCEPTIONS
		std::vector<Import> tag_imports{};
#endif
		std::unordered_map<std::string, Export> export_map{};
#if SOUP_WASM_MULTI_MEMORY
		std::vector<SharedPtr<Memory>> memories;
#else
		SharedPtr<Memory> memory;
#endif
		std::vector<uint32_t> functions{}; // (function_index - function_imports.size()) -> type_index
		std::vector<WasmFunctionType> types{};
		std::vector<SharedPtr<WasmValue>> globals{};
		std::vector<std::string> code{};
		std::unordered_map<uint64_t, std::vector<uint32_t>> _internal_branch_hints{};
		std::vector<SharedPtr<Table>> tables{};
		std::vector<DataSegment> data_segments{};
		std::vector<ElemSegment> elem_segments{};
#if SOUP_WASM_EXCEPTIONS
		std::vector<SharedPtr<Tag>> tags{};
#endif
		StructMap custom_data;
		WasmSharedEnvironment* shared_env = nullptr;
		uint32_t start_func_idx = -1;
		uint32_t ref_count : 29; // how many ScriptRaii are pointing at this script
		uint32_t has_data_count_section : 1;
		uint32_t has_created_funcrefs : 1;
		uint32_t _gc_reachable : 1;

		WasmScript() noexcept : ref_count(0), has_data_count_section(0), has_created_funcrefs(0), _gc_reachable(0) {}
	protected:
		WasmScript(WasmSharedEnvironment* shared_env) : shared_env(shared_env), ref_count(0), has_data_count_section(0), has_created_funcrefs(0), _gc_reachable(0) {}
		friend WasmSharedEnvironment;
	public:
		WasmScript(WasmScript&&) noexcept = default;
		WasmScript(const WasmScript&) = delete;
		WasmScript& operator = (WasmScript&&) noexcept = default;
		WasmScript& operator = (const WasmScript&) = delete;

		bool load(const std::string& data) SOUP_EXCAL;
		bool load(Reader& r) SOUP_EXCAL;
		bool readConstantExpression(Reader& r, std::string& out) SOUP_EXCAL;
		bool evaluateConstantExpression(Reader& r, WasmValue& out) noexcept;
#if SOUP_WASM_EXTENDED_CONST
		bool evaluateExtendedConstantExpression(std::string&& code, WasmValue& out) noexcept;
#endif
		bool validateFunctionBody(Reader& r) noexcept;

		[[nodiscard]] bool hasUnresolvedImports() const noexcept;
		void provideImportedFunction(const std::string& module_name, const std::string& function_name, wasm_ffi_func_t ptr, uint32_t user_data = 0) noexcept;
		void provideImportedFunctions(const std::string& module_name, const std::unordered_map<std::string, wasm_ffi_func_t>& map) noexcept;
		void provideImportedGlobal(const std::string& module_name, const std::string& field_name, SharedPtr<WasmValue> value) noexcept;
		void provideImportedGlobals(const std::string& module_name, const std::unordered_map<std::string, SharedPtr<WasmValue>>& map) noexcept;
		void provideImportedTables(const std::string& module_name, const std::unordered_map<std::string, SharedPtr<Table>>& map) noexcept;
		void provideImportedMemory(const std::string& module_name, const std::string& field_name, SharedPtr<Memory> value) noexcept;
		void importFromModule(const std::string& module_name, WasmScript& other) noexcept; // both scripts must be part of the same WasmSharedEnvironment
		void linkWasiPreview1(std::vector<std::string> args = {}) noexcept;

		// Runs data, elem, and global intialisers, as well as the start function of the script, if defined, which may throw if an imported C++ function throws.
		bool instantiate();

		[[nodiscard]] const std::string* getExportedFuntion(const std::string& name, const WasmFunctionType** optOutType = nullptr) const noexcept;
		[[nodiscard]] uint32_t getExportedFuntion2(const std::string& name) const noexcept;
		[[nodiscard]] uint32_t getTypeIndexForFunction(uint32_t func_index) const noexcept;
		[[nodiscard]] WasmValue* getGlobalByIndex(uint32_t global_index) noexcept { return global_index < globals.size() ? globals[global_index].get()  : nullptr; }
		[[nodiscard]] WasmValue* getExportedGlobal(const std::string& name) noexcept;
		[[nodiscard]] Table* getTableByIndex(uint32_t idx) noexcept { return idx < tables.size() ? tables[idx].get() : nullptr; }
		[[nodiscard]] WasmType getTableElemType(uint32_t idx) noexcept;
		[[nodiscard]] ElemSegment* getPassiveElemSegmentByIndex(uint32_t i) noexcept;
#if SOUP_WASM_MULTI_MEMORY
		[[nodiscard]] Memory* getMemoryByIndex(uint32_t memidx) noexcept { return memidx < memories.size() ? memories[memidx].get() : nullptr; }
#else
		[[nodiscard]] Memory* getMemoryByIndex(uint32_t memidx) noexcept { return memidx == 0 ? memory.get() : nullptr; }
#endif
		[[nodiscard]] std::string* getPassiveDataSegmentByIndex(uint32_t i) noexcept;

		// May throw if an imported C++ function throws.
		bool call(uint32_t func_index, std::vector<WasmValue>&& args = {}, std::vector<WasmValue>* out = nullptr);
	};

	struct WasmSharedEnvironment
	{
		struct FuncRef
		{
			union
			{
				WasmScript* source;
				//wasm_ffi_func_t ptr;
			};
			uint32_t index;
			//uint32_t user_data;

			FuncRef(WasmScript* source, uint32_t index)
				: source(source), index(index)//, user_data(0)
			{
			}

			/*FuncRef(wasm_ffi_func_t ptr, uint32_t user_data)
				: ptr(ptr), index(-1), user_data(user_data)
			{
			}

			[[nodiscard]] bool isC() const noexcept
			{
				return index == -1;
			}*/
		};

		using on_pre_free_script_t = void(*)(WasmScript&);
		using free_externref_t = void(*)(uint64_t);

		std::vector<WasmScript*> scripts;
		on_pre_free_script_t on_pre_free_script = nullptr;
		std::vector<FuncRef> funcrefs;
		std::unordered_map<uint64_t, bool> tracked_externrefs;
		free_externref_t free_externref = nullptr;

		WasmScript& createScript() SOUP_EXCAL;
		void onRefCountHitZero(WasmScript& scr) noexcept;

		//[[nodiscard]] uint64_t createFuncRef(wasm_ffi_func_t ptr, uint32_t user_data = 0) SOUP_EXCAL;
		[[nodiscard]] uint64_t createFuncRef(WasmScript& scr, uint32_t func_index) SOUP_EXCAL;
		[[nodiscard]] const FuncRef& getFuncRef(uint64_t value) const noexcept;

		// Requests that `this->free_externref(value);` be called at some point when no more scripts are referencing it.
		void trackExternRef(uint64_t value) SOUP_EXCAL;

		void collectGarbage() { gcMark(); gcSweep(); }
	private:
		void gcMark() noexcept;
		void gcSweep();

	public:
		struct ScriptRaii
		{
			WasmScript& script;

			ScriptRaii(WasmScript& script) noexcept
				: script(script)
			{
				++script.ref_count;
			}

			~ScriptRaii() noexcept
			{
				if (--script.ref_count == 0)
				{
					script.shared_env->onRefCountHitZero(script);
				}
			}

			[[nodiscard]] WasmScript& operator*() const noexcept
			{
				return script;
			}

			[[nodiscard]] WasmScript* operator->() const noexcept
			{
				return &script;
			}
		};

		~WasmSharedEnvironment() noexcept;
	};

	struct WasmVm
	{
		std::vector<WasmValue> stack;
		std::vector<WasmValue> locals;
		WasmScript& script;
#if SOUP_WASM_EXCEPTIONS
		WasmScript::Tag* current_throw_tag;
#endif

		WasmVm(WasmScript& script) noexcept
			: script(script)
		{
		}

		enum RunCodeResult
		{
			CODE_ERROR = 0,
			CODE_RETURN,
#if SOUP_WASM_EXCEPTIONS
			CODE_THROW,
#endif
#if SOUP_WASM_TAIL_CALL
			CODE_RETURN_CALL, // will not be returned by `run`
			CODE_RETURN_CALL_INDIRECT, // will not be returned by `run`
#endif
		};

		// May throw if an imported C++ function throws.
		RunCodeResult run(const std::string& data, unsigned depth = 0, uint32_t func_index = -1);
		RunCodeResult run(Reader& r, unsigned depth = 0, uint32_t func_index = -1);

		bool processLocalDecls(Reader& r) SOUP_EXCAL;
		RunCodeResult runCode(Reader& r, unsigned depth = 0, uint32_t func_index = -1);

		struct CtrlFlowEntry
		{
			uint32_t position;
			uint32_t stack_size;
			uint32_t num_values; // Number of values to keep on the stack top after branching. num_results for forward jumps; num_params for backward jumps.
#if SOUP_WASM_EXCEPTIONS
			bool is_try_table = false;
#endif

			[[nodiscard]] bool isForwardJump() const noexcept
			{
				return position == -1
#if SOUP_WASM_EXCEPTIONS
					|| is_try_table
#endif
					;
			}
		};

		enum SkipOverBranchResult
		{
#if SOUP_WASM_PEDANTIC
			PEDANTIC_ERROR = 0,
#endif
			ELSE_REACHED,
			END_REACHED,
			INSUFFICIENT_DATA,
		};

		static SkipOverBranchResult skipOverBranch(Reader& r, uint32_t depth, WasmScript& script, uint32_t func_index) SOUP_EXCAL;
		[[nodiscard]] bool doBranch(Reader& r, uint32_t depth, uint32_t func_index, std::stack<CtrlFlowEntry>& ctrlflow) SOUP_EXCAL;
		[[nodiscard]] RunCodeResult doCall(WasmScript* script, uint32_t type_index, uint32_t function_index, unsigned depth = 0);
		bool moveArguments(WasmVm& callvm, const WasmFunctionType& type) SOUP_EXCAL;
#if SOUP_WASM_EXCEPTIONS
		[[nodiscard]] bool doThrow(Reader& r, uint32_t func_index, std::stack<CtrlFlowEntry>& ctrlflow) noexcept;
#endif
	};

	struct WasmScrapAllocator
	{
		WasmScript::Memory& memory;
		size_t last_alloc = -1;

		WasmScrapAllocator(WasmScript::Memory& memory) noexcept
			: memory(memory)
		{
		}

		[[nodiscard]] size_t allocate(size_t len) noexcept
		{
			if (last_alloc >= memory.size)
			{
				last_alloc = memory.size - 1;
			}
			last_alloc -= len;
			return last_alloc;
		}
	};
}
