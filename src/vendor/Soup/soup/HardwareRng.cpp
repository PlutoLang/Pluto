#include "HardwareRng.hpp"

#include "CpuInfo.hpp"

namespace soup
{
	// HardwareRng

	bool HardwareRng::isAvailable() noexcept
	{
#if SOUP_X86 && defined(SOUP_USE_INTRIN)
		return CpuInfo::get().supportsRDSEED();
#else
		return false;
#endif
	}

#if SOUP_X86 && defined(SOUP_USE_INTRIN)
	extern uint16_t hardware_rng_generate16() noexcept;
	extern uint32_t hardware_rng_generate32() noexcept;
	extern uint64_t hardware_rng_generate64() noexcept;
#endif

	uint16_t HardwareRng::generate16() noexcept
	{
#if SOUP_X86 && defined(SOUP_USE_INTRIN)
		return hardware_rng_generate16();
#else
		SOUP_ASSERT_UNREACHABLE;
#endif
	}

	uint32_t HardwareRng::generate32() noexcept
	{
#if SOUP_X86 && defined(SOUP_USE_INTRIN)
		return hardware_rng_generate32();
#else
		SOUP_ASSERT_UNREACHABLE;
#endif
	}

	uint64_t HardwareRng::generate64() noexcept
	{
#if SOUP_X86 && defined(SOUP_USE_INTRIN)
	#if SOUP_BITS >= 64
		return hardware_rng_generate64();
	#else
		return (static_cast<uint64_t>(hardware_rng_generate32()) << 32) | hardware_rng_generate32();
	#endif
#else
		SOUP_ASSERT_UNREACHABLE;
#endif
	}

	// FastHardwareRng

	bool FastHardwareRng::isAvailable() noexcept
	{
#if SOUP_X86 && defined(SOUP_USE_INTRIN)
		return CpuInfo::get().supportsRDRAND();
#else
		return false;
#endif
	}

#if SOUP_X86 && defined(SOUP_USE_INTRIN)
	extern uint16_t fast_hardware_rng_generate16() noexcept;
	extern uint32_t fast_hardware_rng_generate32() noexcept;
	extern uint64_t fast_hardware_rng_generate64() noexcept;
#endif

	uint16_t FastHardwareRng::generate16() noexcept
	{
#if SOUP_X86 && defined(SOUP_USE_INTRIN)
		return fast_hardware_rng_generate16();
#else
		SOUP_ASSERT_UNREACHABLE;
#endif
	}

	uint32_t FastHardwareRng::generate32() noexcept
	{
#if SOUP_X86 && defined(SOUP_USE_INTRIN)
		return fast_hardware_rng_generate32();
#else
		SOUP_ASSERT_UNREACHABLE;
#endif
	}

	uint64_t FastHardwareRng::generate64() noexcept
	{
#if SOUP_X86 && defined(SOUP_USE_INTRIN)
	#if SOUP_BITS >= 64
		return fast_hardware_rng_generate64();
	#else
		return (static_cast<uint64_t>(fast_hardware_rng_generate32()) << 32) | fast_hardware_rng_generate32();
	#endif
#else
		SOUP_ASSERT_UNREACHABLE;
#endif
	}
}
