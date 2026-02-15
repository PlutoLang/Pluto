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

	struct WasmValue
	{
		union
		{
			int32_t i32;
			int64_t i64;
			float f32;
			double f64;
		};
		WasmType type;

		WasmValue() = default;
		WasmValue(WasmType type) : i64(0), type(type) {}
		WasmValue(int32_t i32) : i32(i32), type(WASM_I32) {}
		WasmValue(uint32_t u32) : WasmValue(static_cast<int32_t>(u32)) {}
		WasmValue(int64_t i64) : i64(i64), type(WASM_I64) {}
		WasmValue(uint64_t u64) : WasmValue(static_cast<int64_t>(u64)) {}
		WasmValue(float f32) : f32(f32), type(WASM_F32) {}
		WasmValue(double f64) : f64(f64), type(WASM_F64) {}
		WasmValue(void* ptr) : i64(reinterpret_cast<uintptr_t>(ptr)), type(WASM_EXTERNREF) {}

		template <typename T, SOUP_RESTRICT(std::is_same_v<T, size_t>)>
		WasmValue(T ptr) : i32(static_cast<int32_t>(ptr)) {}
	};

	struct WasmScript
	{
		struct Memory
		{
			uint8_t* data = nullptr;
			size_t size = 0;
			uint32_t page_limit = 0x10'000;
			bool memory64 = false;

			~Memory() noexcept;

			[[nodiscard]] void* getView(size_t addr, size_t size) noexcept
			{
				SOUP_IF_LIKELY (addr + size <= this->size)
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
				if (base.type == WASM_I64)
				{
					return getPointer<T>(static_cast<uint64_t>(base.i64) + offset);
				}
				return getPointer<T>(static_cast<uint32_t>(base.i32) + offset);
			}

			[[nodiscard]] std::string readString(size_t addr, size_t size) SOUP_EXCAL;
			[[nodiscard]] std::string readNullTerminatedString(size_t addr) SOUP_EXCAL; // a safe alternative to getPointer<const char>(addr)

			bool write(size_t addr, const void* src, size_t size) noexcept;
			bool write(const WasmValue& addr, const void* src, size_t size) noexcept;
		};

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

		Memory memory;
		std::vector<uint32_t> functions{}; // (function_index - function_imports.size()) -> type_index
		std::vector<FunctionType> types{};
		std::vector<FunctionImport> function_imports{};
		std::vector<WasmValue> globals{};
		std::unordered_map<std::string, uint32_t> export_map{};
		std::vector<std::string> code{};
		std::vector<uint32_t> elements{};
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

		// Runs the start function of the script, if defined. Throws if imported functions throw.
		bool instantiate();

		[[nodiscard]] FunctionImport* getImportedFunction(const std::string& module_name, const std::string& function_name) noexcept;
		[[nodiscard]] const std::string* getExportedFuntion(const std::string& name, const FunctionType** optOutType = nullptr) const noexcept;

		void linkWasiPreview1(std::vector<std::string> args = {}) noexcept;
		void linkSpectestShim() noexcept;
	};

	class WasmVm
	{
	public:
		std::stack<WasmValue> stack;
		std::vector<WasmValue> locals;
		WasmScript& script;

		WasmVm(WasmScript& script) noexcept
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
		[[nodiscard]] static size_t readUPTR(Reader& r) noexcept;
	};

	struct WasmScrapAllocator
	{
		WasmScript::Memory& memory;
		size_t last_alloc = -1;

		WasmScrapAllocator(WasmScript::Memory& memory) noexcept
			: memory(memory)
		{
		}

		WasmScrapAllocator(WasmScript& script) noexcept
			: WasmScrapAllocator(script.memory)
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
