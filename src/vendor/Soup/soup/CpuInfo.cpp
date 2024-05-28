#include "CpuInfo.hpp"
#if !SOUP_WASM

#if SOUP_X86
	#include "string.hpp"

	#if defined(_MSC_VER) && !defined(__clang__)
		#include <intrin.h>
	#else
		#include <cpuid.h>
	#endif
#elif SOUP_ARM
	#if SOUP_WINDOWS
		#include <windows.h>
	#else
		#include <sys/auxv.h>
	#endif
#endif

NAMESPACE_SOUP
{
	CpuInfo::CpuInfo() noexcept
	{
#if SOUP_X86
	#define EAX arr[0]
	#define EBX arr[1]
	#define EDX arr[2]
	#define ECX arr[3]

		char buf[17];
		buf[16] = 0;
		invokeCpuid(buf, 0);
		cpuid_max_eax = *reinterpret_cast<uint32_t*>(&buf[0]);
		vendor_id = &buf[4];

		uint32_t arr[4];

		if (cpuid_max_eax >= 0x01)
		{
			invokeCpuid(arr, 0x01);
			stepping_id = (EAX & 0xF);
			model = ((EAX >> 4) & 0xF);
			family = ((EAX >> 8) & 0xF);
			feature_flags_ecx = ECX;
			feature_flags_edx = EDX;

			if (cpuid_max_eax >= 0x07)
			{
				invokeCpuid(arr, 0x07, 0);
				extended_features_0_ebx = EBX;

				if (cpuid_max_eax >= 0x16)
				{
					invokeCpuid(arr, 0x16);
					base_frequency = EAX;
					max_frequency = EBX;
					bus_frequency = ECX;
				}
			}
		}

		invokeCpuid(arr, 0x80000000);
		cpuid_extended_max_eax = EAX;

		if (cpuid_extended_max_eax >= 0x80000001)
		{
			invokeCpuid(arr, 0x80000001);
			extended_features_1_ecx = ECX;
		}

	#undef EAX
	#undef EBX
	#undef EDX
	#undef ECX
#elif SOUP_ARM
	#if SOUP_WINDOWS
		armv8_aes = IsProcessorFeaturePresent(PF_ARM_V8_CRYPTO_INSTRUCTIONS_AVAILABLE);
		armv8_sha1 = IsProcessorFeaturePresent(PF_ARM_V8_CRYPTO_INSTRUCTIONS_AVAILABLE);
		armv8_sha2 = IsProcessorFeaturePresent(PF_ARM_V8_CRYPTO_INSTRUCTIONS_AVAILABLE);
		armv8_crc32 = IsProcessorFeaturePresent(PF_ARM_V8_CRC32_INSTRUCTIONS_AVAILABLE);
	#elif SOUP_BITS == 32
		armv8_aes = false;
		armv8_sha1 = false;
		armv8_sha2 = false;
		armv8_crc32 = false;
	#else
		// These HWCAP_* are only defined on aarch64.
		armv8_aes = getauxval(AT_HWCAP) & HWCAP_AES;
		armv8_sha1 = getauxval(AT_HWCAP) & HWCAP_SHA1;
		armv8_sha2 = getauxval(AT_HWCAP) & HWCAP_SHA2;
		armv8_crc32 = getauxval(AT_HWCAP) & HWCAP_CRC32;
	#endif
#endif
	}

	std::string CpuInfo::toString() const SOUP_EXCAL
	{
#if SOUP_X86
		std::string str = "CPUID Support Level: ";
		str.append(string::hex(cpuid_max_eax));
		str.append("\nCPUID Extended Support Level: ");
		str.append(string::hex(cpuid_extended_max_eax & ~0x80000000));
		str.append("\nVendor: ");
		str.append(vendor_id.c_str());

		if (cpuid_max_eax >= 0x01)
		{
			str.append("\nStepping ID: ").append(std::to_string(stepping_id));
			str.append("\nModel: ").append(std::to_string(model));
			str.append("\nFamily: ").append(std::to_string(family));
			str.append("\nFeature Flags 1: ").append(string::hex(feature_flags_ecx));
			str.append("\nFeature Flags 2: ").append(string::hex(feature_flags_edx));

			if (cpuid_max_eax >= 0x07)
			{
				str.append("\nFeature Flags 3: ").append(string::hex(extended_features_0_ebx));

				if (cpuid_max_eax >= 0x16)
				{
					str.append("\nBase Frequency: ").append(std::to_string(base_frequency)).append(
						" MHz\n"
						"Max. Frequency: "
					).append(std::to_string(max_frequency)).append(
						" MHz\n"
						"Bus (Reference) Frequency: "
					).append(std::to_string(bus_frequency)).append(" MHz");
				}
			}
		}

		if (cpuid_extended_max_eax >= 0x80000001)
		{
			str.append("\nExtended Feature Flags: ").append(string::hex(extended_features_1_ecx));
		}

		return str;
#elif SOUP_ARM
		std::string str = "ARMv8 AES: ";
		str.append(armv8_aes ? "true" : "false");
		str.append("\nARMv8 SHA1: ").append(armv8_sha1 ? "true" : "false");
		str.append("\nARMv8 SHA2: ").append(armv8_sha2 ? "true" : "false");
		str.append("\nARMv8 CRC32: ").append(armv8_crc32 ? "true" : "false");
		return str;
#endif
	}

#if SOUP_X86
	void CpuInfo::invokeCpuid(void* out, uint32_t eax) noexcept
	{
	#if defined(_MSC_VER) && !defined(__clang__)
		__cpuid(((int*)out), eax);
		std::swap(((int*)out)[2], ((int*)out)[3]);
	#else
		__cpuid(eax, ((int*)out)[0], ((int*)out)[1], ((int*)out)[3], ((int*)out)[2]);
	#endif
	}

	void CpuInfo::invokeCpuid(void* out, uint32_t eax, uint32_t ecx) noexcept
	{
	#if defined(__GNUC__)
		((uint32_t*)out)[3] = ecx;
		__get_cpuid(eax, &((uint32_t*)out)[0], &((uint32_t*)out)[1], &((uint32_t*)out)[3], &((uint32_t*)out)[2]);
	#else
		__cpuidex(((int*)out), eax, ecx);
		std::swap(((int*)out)[2], ((int*)out)[3]);
	#endif
	}
#endif
}

#endif
