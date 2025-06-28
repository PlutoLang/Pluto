#pragma once

#include "base.hpp"
#if !SOUP_WASM

#include <string>
#if SOUP_X86
#include <cstdint>

#include "ShortString.hpp"
#endif

NAMESPACE_SOUP
{
	class CpuInfo
	{
	private:
		CpuInfo() noexcept;

	public:
		[[nodiscard]] static SOUP_PURE const CpuInfo& get() noexcept
		{
			static CpuInfo inst;
			return inst;
		}

		[[nodiscard]] std::string toString() const SOUP_EXCAL;

#if SOUP_X86
		uint32_t cpuid_max_eax;
		uint32_t cpuid_extended_max_eax;
		ShortString<16> vendor_id;

		// EAX=1
		uint8_t stepping_id = 0;
		uint8_t model = 0;
		uint8_t family = 0;
		uint32_t feature_flags_ecx = 0;
		uint32_t feature_flags_edx = 0;

		// EAX=7, ECX=0
		uint32_t extended_features_max_ecx = 0;
		uint32_t extended_features_0_ebx = 0;

		// EAX=7, ECX=1
		uint32_t extended_features_1_eax = 0;

		// EAX=16h
		uint16_t base_frequency = 0;
		uint16_t max_frequency = 0;
		uint16_t bus_frequency = 0;

		// EAX=80000001h
		uint32_t extended_flags_1_ecx = 0;

		[[nodiscard]] bool supportsSSE() const noexcept
		{
			return (feature_flags_edx >> 25) & 1;
		}

		[[nodiscard]] bool supportsSSE2() const noexcept
		{
			return (feature_flags_edx >> 26) & 1;
		}

		[[nodiscard]] bool supportsSSE3() const noexcept
		{
			return (feature_flags_ecx >> 0) & 1;
		}

		[[nodiscard]] bool supportsPCLMULQDQ() const noexcept
		{
			return (feature_flags_ecx >> 1) & 1;
		}

		[[nodiscard]] bool supportsSSSE3() const noexcept
		{
			return (feature_flags_ecx >> 9) & 1;
		}

		[[nodiscard]] bool supportsSSE4_1() const noexcept
		{
			return (feature_flags_ecx >> 19) & 1;
		}

		[[nodiscard]] bool supportsSSE4_2() const noexcept
		{
			return (feature_flags_ecx >> 20) & 1;
		}

		[[nodiscard]] bool supportsAESNI() const noexcept
		{
			return (feature_flags_ecx >> 25) & 1;
		}

		[[nodiscard]] bool supportsAVX() const noexcept
		{
			return (feature_flags_ecx >> 28) & 1;
		}

		[[nodiscard]] bool supportsRDRAND() const noexcept
		{
			return (feature_flags_ecx >> 30) & 1;
		}

		[[nodiscard]] bool supportsAVX2() const noexcept
		{
			return (extended_features_0_ebx >> 5) & 1;
		}

		[[nodiscard]] bool supportsAVX512F() const noexcept
		{
			return (extended_features_0_ebx >> 16) & 1;
		}

		[[nodiscard]] bool supportsRDSEED() const noexcept
		{
			return (extended_features_0_ebx >> 18) & 1;
		}

		[[nodiscard]] bool supportsSHA() const noexcept
		{
			return (extended_features_0_ebx >> 29) & 1;
		}

		[[nodiscard]] bool supportsAVX512BW() const noexcept
		{
			return (extended_features_0_ebx >> 30) & 1;
		}

		[[nodiscard]] bool supportsSHA512() const noexcept
		{
			return (extended_features_1_eax >> 0) & 1;
		}

		[[nodiscard]] bool supportsXOP() const noexcept
		{
			return (extended_flags_1_ecx >> 11) & 1;
		}

		static void invokeCpuid(void* out, uint32_t eax) noexcept;
		static void invokeCpuid(void* out, uint32_t eax, uint32_t ecx) noexcept;
#elif SOUP_ARM
		bool armv8_aes;
		bool armv8_sha1;
		bool armv8_sha2;
		bool armv8_crc32;
#endif
	};
}

#endif
