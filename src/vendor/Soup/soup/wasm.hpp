#pragma once

#include "base.hpp"
#include "fwd.hpp"
#include "type_traits.hpp"

#include <stack>
#include <string>
#include <unordered_map>
#include <vector>

#include "StructMap.hpp"

NAMESPACE_SOUP
{
	class WasmVm;

	using wasm_ffi_func_t = void(*)(WasmVm&, uint32_t func_index);

	union WasmValue
	{
		int32_t i32;
		int64_t i64;
		float f32;
		double f64;

		WasmValue() : i64(0) {}
		WasmValue(int32_t i32) : i32(i32) {}
		WasmValue(uint32_t u32) : WasmValue(static_cast<int32_t>(u32)) {}
		WasmValue(int64_t i64) : i64(i64) {}
		WasmValue(uint64_t u64) : WasmValue(static_cast<int64_t>(u64)) {}
		WasmValue(float f32) : f32(f32) {}
		WasmValue(double f64) : f64(f64) {}

		template <typename T, SOUP_RESTRICT(std::is_same_v<T, size_t>)>
		WasmValue(T ptr) : i32(static_cast<int32_t>(ptr)) {}
	};

	enum WasmType : uint8_t
	{
		WASM_I32 = 0x7F,
		WASM_I64 = 0x7E,
		WASM_F32 = 0x7D,
		WASM_F64 = 0x7C,
	};

	struct WasmScript
	{
		struct FunctionType
		{
			std::vector<WasmType> parameters;
			std::vector<WasmType> results;
		};

		struct FunctionImport
		{
			std::string module_name;
			std::string function_name;
			wasm_ffi_func_t ptr;
			uint32_t type_index;
		};

		uint8_t* memory = nullptr;
		size_t memory_size = 0;
		size_t last_alloc = -1;
		uint32_t memory_page_limit : 31;
		uint32_t memory64 : 1;
		uint32_t start_func_idx = -1;
		std::vector<uint32_t> functions{}; // (function_index - function_imports.size()) -> type_index
		std::vector<FunctionType> types{};
		std::vector<FunctionImport> function_imports{};
		std::vector<int32_t> globals{};
		std::unordered_map<std::string, uint32_t> export_map{};
		std::vector<std::string> code{};
		std::vector<uint32_t> elements{};
		StructMap custom_data;

		WasmScript() noexcept
			: memory_page_limit(0x10'000), memory64(0)
		{
		}

		WasmScript(WasmScript&&) noexcept = default;
		WasmScript(const WasmScript&) = delete;
		WasmScript& operator = (WasmScript&&) noexcept = default;
		WasmScript& operator = (const WasmScript&) = delete;
		~WasmScript() noexcept;

		bool load(const std::string& data) SOUP_EXCAL;
		bool load(Reader& r) SOUP_EXCAL;

		// Runs the start function of the script, if defined. Throws if imported functions throw.
		bool instantiate();

		[[nodiscard]] FunctionImport* getImportedFunction(const std::string& module_name, const std::string& function_name) noexcept;
		[[nodiscard]] const std::string* getExportedFuntion(const std::string& name, const FunctionType** optOutType = nullptr) const noexcept;

		[[nodiscard]] size_t allocateMemory(size_t len) noexcept;

		template <typename T>
		[[nodiscard]] T* getMemory(size_t ptr) noexcept
		{
			SOUP_IF_UNLIKELY (ptr + sizeof(T) > memory_size)
			{
				return nullptr;
			}
			return (T*)&memory[ptr];
		}

		template <typename T>
		[[nodiscard]] T* getMemory(WasmValue base, size_t offset = 0) noexcept
		{
			if (memory64)
			{
				return getMemory<T>(static_cast<uint64_t>(base.i64) + offset);
			}
			return getMemory<T>(static_cast<uint32_t>(base.i32) + offset);
		}

		bool setMemory(size_t ptr, const void* src, size_t len) noexcept;
		bool setMemory(WasmValue ptr, const void* src, size_t len) noexcept;

		void linkWasiPreview1(std::vector<std::string> args = {}) noexcept;
		void linkSpectestShim() noexcept;

		[[nodiscard]] size_t readUPTR(Reader& r) const noexcept;
	};

	class WasmVm
	{
	public:
		std::stack<WasmValue> stack;
		std::vector<WasmValue> locals;
		WasmScript& script;

		WasmVm(WasmScript& script)
			: script(script)
		{
		}

		// Throws if imported functions throw.
		bool run(const std::string& data, unsigned depth = 0);
		bool run(Reader& r, unsigned depth = 0);

	private:
		struct CtrlFlowEntry
		{
			std::streamoff position; // -1 for forward jumps
			size_t stack_size;
			uint32_t num_values; // Number of values to keep on the stack top after branching. num_results for forward jumps; num_params for backward jumps.
		};

		bool skipOverBranch(Reader& r, uint32_t depth = 0) SOUP_EXCAL;
		[[nodiscard]] bool doBranch(Reader& r, uint32_t depth, std::stack<CtrlFlowEntry>& ctrlflow) SOUP_EXCAL;
		[[nodiscard]] bool doCall(uint32_t type_index, uint32_t function_index, unsigned depth);
		void pushIPTR(size_t ptr) SOUP_EXCAL;
		[[nodiscard]] size_t popIPTR();
	};
}
