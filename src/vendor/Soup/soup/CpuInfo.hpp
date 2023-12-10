#pragma once

#include "base.hpp"
#if SOUP_X86

#include <cstdint>
#include <string>

#include "ShortString.hpp"

namespace soup
{
	class CpuInfo
	{
	public:
		uint32_t cpuid_max_eax;
		uint64_t cpuid_extended_max_eax;
		ShortString<16> vendor_id;

		uint8_t stepping_id;
		uint8_t model;
		uint8_t family;
		uint32_t feature_flags_ecx;
		uint32_t feature_flags_edx;

		// EAX=7, ECX=0
		uint32_t extended_features_0_ebx;

		// EAX=80000001h
		uint64_t extended_features_1_ecx;

		uint16_t base_frequency;
		uint16_t max_frequency;
		uint16_t bus_frequency;

	private:
		CpuInfo();

	public:
		[[nodiscard]] static const CpuInfo& get();

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

		[[nodiscard]] bool supportsRDRND() const noexcept
		{
			return (feature_flags_ecx >> 30) & 1;
		}

		[[nodiscard]] bool supportsAVX2() const noexcept
		{
			return (extended_features_0_ebx >> 5) & 1;
		}

		[[nodiscard]] bool supportsSHA() const noexcept
		{
			return (extended_features_0_ebx >> 29) & 1;
		}

		[[nodiscard]] bool supportsXOP() const noexcept
		{
			return (extended_features_1_ecx >> 11) & 1;
		}

		[[nodiscard]] std::string toString() const;

		static void invokeCpuid(void* out, uint32_t eax);
		static void invokeCpuid(void* out, uint32_t eax, uint32_t ecx);
	};
}

#endif
