#include "HardwareRng.hpp"

#include "CpuInfo.hpp"

#if SOUP_WINDOWS
#include <windows.h>
#include <bcrypt.h>
#pragma comment(lib, "Bcrypt.lib")
#elif SOUP_LINUX
#include <sys/random.h>
#else
#include <fcntl.h> // open
#include <unistd.h> // read, close
#endif

NAMESPACE_SOUP
{
#if SOUP_X86 && defined(SOUP_USE_INTRIN)
	namespace intrin
	{
		extern uint16_t hardware_rng_generate16() noexcept;
		extern uint32_t hardware_rng_generate32() noexcept;
		extern uint64_t hardware_rng_generate64() noexcept;
	}
#endif

	// HardwareRng

	bool HardwareRng::isAvailable() noexcept
	{
#if SOUP_X86 && defined(SOUP_USE_INTRIN)
		return CpuInfo::get().supportsRDSEED();
#else
		return false;
#endif
	}

	uint16_t HardwareRng::generate16() noexcept
	{
#if SOUP_X86 && defined(SOUP_USE_INTRIN)
		return intrin::hardware_rng_generate16();
#else
		SOUP_ASSERT_UNREACHABLE;
#endif
	}

	uint32_t HardwareRng::generate32() noexcept
	{
#if SOUP_X86 && defined(SOUP_USE_INTRIN)
		return intrin::hardware_rng_generate32();
#else
		SOUP_ASSERT_UNREACHABLE;
#endif
	}

	uint64_t HardwareRng::generate64() noexcept
	{
#if SOUP_X86 && defined(SOUP_USE_INTRIN)
	#if SOUP_BITS >= 64
		return intrin::hardware_rng_generate64();
	#else
		return (static_cast<uint64_t>(intrin::hardware_rng_generate32()) << 32) | intrin::hardware_rng_generate32();
	#endif
#else
		SOUP_ASSERT_UNREACHABLE;
#endif
	}

	// FastHardwareRng

	void FastHardwareRng::generate(void* buf, size_t buflen) noexcept
	{
#if SOUP_WINDOWS
		BCryptGenRandom(nullptr, (PUCHAR)buf, (ULONG)buflen, BCRYPT_USE_SYSTEM_PREFERRED_RNG);
#elif SOUP_LINUX
		getrandom(buf, buflen, 0);
#else
		const auto fd = open("/dev/urandom", O_RDONLY);
		read(fd, buf, buflen);
		close(fd);
#endif
	}

	uint16_t FastHardwareRng::generate16() noexcept
	{
		uint16_t res;
		generate(&res, sizeof(res));
		return res;
	}

	uint32_t FastHardwareRng::generate32() noexcept
	{
		uint32_t res;
		generate(&res, sizeof(res));
		return res;
	}

	uint64_t FastHardwareRng::generate64() noexcept
	{
		uint64_t res;
		generate(&res, sizeof(res));
		return res;
	}
}
