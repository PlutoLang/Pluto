#pragma once

#include "base.hpp"
#include "fwd.hpp"
#include "type_traits.hpp"

#include <stack>
#include <string>
#include <unordered_map>
#include <vector>

#include "math.hpp"
#include "SharedPtr.hpp"
#include "StructMap.hpp"

#ifndef SOUP_WASM_MEMORY64
#define SOUP_WASM_MEMORY64 SOUP_BITS >= 64
#endif

#ifndef SOUP_WASM_PEDANTIC
// Set to true if you love wasting CPU time just so you can error in edge cases for spec conformity.
#define SOUP_WASM_PEDANTIC false
#endif

NAMESPACE_SOUP
{
	struct WasmVm;

	enum WasmType : uint8_t
	{
		WASM_I32 = 0x7F, // -1
		WASM_I64 = 0x7E, // -2
		WASM_F32 = 0x7D, // -3
		WASM_F64 = 0x7C, // -4
		//WASM_V128 = 0x7B, // -5 (from the simd extension, which is currently not supported by Soup)
		WASM_FUNCREF = 0x70,
		WASM_EXTERNREF = 0x6F,
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

	using wasm_ffi_func_t = void(*)(WasmVm&, uint32_t func_index, const WasmFunctionType&);

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
		};
		WasmType type;

		constexpr WasmValue() noexcept : i64(0), type(static_cast<WasmType>(0)) {}
		constexpr WasmValue(WasmType type) noexcept : i64(0), type(type) {}
		constexpr WasmValue(int32_t i32) noexcept : i32(i32), hi32(0), type(WASM_I32) {}
		constexpr WasmValue(uint32_t u32) noexcept : WasmValue(static_cast<int32_t>(u32)) {}
		constexpr WasmValue(int64_t i64) noexcept : i64(i64), type(WASM_I64) {}
		constexpr WasmValue(uint64_t u64) noexcept : WasmValue(static_cast<int64_t>(u64)) {}
		constexpr WasmValue(float f32) noexcept : f32(f32), hi32(0), type(WASM_F32) {}
		constexpr WasmValue(double f64) noexcept : f64(f64), type(WASM_F64) {}
		WasmValue(void* ptr) noexcept : i64(reinterpret_cast<uintptr_t>(ptr)), type(WASM_EXTERNREF) {}

		[[nodiscard]] bool operator==(const WasmValue& b) const noexcept { return i64 == b.i64 && type == b.type; }
		[[nodiscard]] bool operator!=(const WasmValue& b) const noexcept { return !operator==(b); }

		[[nodiscard]] size_t uptr() const noexcept
		{
#if SOUP_WASM_MEMORY64
			return i64; // i32 values are guaranteed to have 0 in hi32.
#else
			return i32;
#endif
		}
	};

	struct WasmScript
	{
		struct Memory
		{
			uint8_t* data;
			size_t size;
#if SOUP_WASM_MEMORY64
			uint64_t page_limit : 48;
			uint64_t memory64 : 1;
#else
			uint32_t page_limit;
#endif

			Memory() noexcept
				: data(nullptr), size(0), page_limit(0x10'000)
#if SOUP_WASM_MEMORY64
				, memory64(0)
#endif
			{
			}

			~Memory() noexcept;

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
			[[nodiscard]] T* getPointer(const WasmValue& base, size_t offset = 0) noexcept
			{
				return getPointer<T>(base.uptr() + offset);
			}

			[[nodiscard]] std::string readString(size_t addr, size_t size) SOUP_EXCAL;
			[[nodiscard]] std::string readNullTerminatedString(size_t addr) SOUP_EXCAL; // a safe alternative to getPointer<const char>(addr)

			bool write(size_t addr, const void* src, size_t size) noexcept;
			bool write(const WasmValue& addr, const void* src, size_t size) noexcept;

			void encodeUPTR(WasmValue& out, size_t in) noexcept;

			size_t grow(size_t delta_pages) noexcept;
		};

		struct FunctionImport
		{
			std::string module_name;
			std::string function_name;

			wasm_ffi_func_t ptr;
			SharedPtr<WasmScript> source;
			uint32_t type_index; // an index in WasmScript::types of the importing WasmScript
			uint32_t func_index; // to be used with `source` for WASM imports
		};

		struct Import
		{
			std::string module_name;
			std::string field_name;
		};

		struct Export
		{
			enum Kind : uint8_t
			{
				kFunction = 0,
				kTable = 1,
				kMemory = 2,
				kGlobal = 3,
			};

			uint8_t kind;
			uint32_t index;
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

			Table(WasmType type) noexcept
				: type(type), limit(0x10'000)
			{
			}

			size_t grow(size_t delta, int64_t value = 0) SOUP_EXCAL;
			bool copy(const Table& src, size_t dst_offset, size_t src_offset, size_t size) noexcept;
		};

		SharedPtr<Memory> memory;
		std::vector<uint32_t> functions{}; // (function_index - function_imports.size()) -> type_index
		std::vector<WasmFunctionType> types{};
		std::vector<FunctionImport> function_imports{};
		std::vector<Import> global_imports{};
		std::vector<SharedPtr<WasmValue>> globals{};
		std::unordered_map<std::string, Export> export_map{};
		std::vector<std::string> code{};
		std::unordered_map<uint64_t, std::vector<uint32_t>> _internal_branch_hints{};
		std::vector<Table> tables{};
		std::unordered_map<uint32_t, std::string> passive_data_segments{};
		std::unordered_map<uint32_t, Table> passive_elem_segments{};
		StructMap custom_data;
		uint32_t start_func_idx = -1;

		WasmScript() noexcept { /* default */ }
		WasmScript(WasmScript&&) noexcept = default;
		WasmScript(const WasmScript&) = delete;
		WasmScript& operator = (WasmScript&&) noexcept = default;
		WasmScript& operator = (const WasmScript&) = delete;

		bool load(const std::string& data) SOUP_EXCAL;
		bool load(Reader& r) SOUP_EXCAL;
		static bool readConstant(Reader& r, WasmValue& out) noexcept;
		bool validateFunctionBody(Reader& r) noexcept;

		[[nodiscard]] bool hasUnresolvedImports() const noexcept;
		void provideImportedFunction(const std::string& module_name, const std::string& function_name, wasm_ffi_func_t ptr) noexcept;
		void provideImportedFunctions(const std::string& module_name, const std::unordered_map<std::string, wasm_ffi_func_t>& map) noexcept;
		void provideImportedGlobal(const std::string& module_name, const std::string& field_name, SharedPtr<WasmValue> value) noexcept;
		void provideImportedGlobals(const std::string& module_name, const std::unordered_map<std::string, SharedPtr<WasmValue>>& map) noexcept;
		void importFromModule(const std::string& module_name, SharedPtr<WasmScript> other) noexcept;
		void linkWasiPreview1(std::vector<std::string> args = {}) noexcept;

		// Runs the start function of the script, if defined. May throw if an imported C++ function throws.
		bool instantiate();

		[[nodiscard]] const std::string* getExportedFuntion(const std::string& name, const WasmFunctionType** optOutType = nullptr) const noexcept;
		[[nodiscard]] uint32_t getExportedFuntion2(const std::string& name) const noexcept;
		[[nodiscard]] uint32_t getTypeIndexForFunction(uint32_t func_index) const noexcept;
		[[nodiscard]] WasmValue* getGlobalByIndex(uint32_t global_index) noexcept;
		[[nodiscard]] WasmValue* getExportedGlobal(const std::string& name) noexcept;

		// May throw if an imported C++ function throws.
		bool call(uint32_t func_index, std::vector<WasmValue>&& args = {}, std::vector<WasmValue>* out = nullptr);
	};

	struct WasmVm
	{
		std::vector<WasmValue> stack;
		std::vector<WasmValue> locals;
		WasmScript& script;

		WasmVm(WasmScript& script) noexcept
			: script(script)
		{
		}

		// May throw if an imported C++ function throws.
		bool run(const std::string& data, unsigned depth = 0, uint32_t func_index = -1);
		bool run(Reader& r, unsigned depth = 0, uint32_t func_index = -1);

		struct CtrlFlowEntry
		{
			std::streamoff position; // -1 for forward jumps
			size_t stack_size;
			uint32_t num_values; // Number of values to keep on the stack top after branching. num_results for forward jumps; num_params for backward jumps.
		};

		static bool skipOverBranch(Reader& r, uint32_t depth, WasmScript& script, uint32_t func_index) SOUP_EXCAL;
		[[nodiscard]] bool doBranch(Reader& r, uint32_t depth, uint32_t func_index, std::stack<CtrlFlowEntry>& ctrlflow) SOUP_EXCAL;
		[[nodiscard]] bool doCall(uint32_t type_index, uint32_t function_index, unsigned depth = 0);
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
