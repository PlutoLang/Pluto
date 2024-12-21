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

#if SOUP_X86
#include <immintrin.h>
#endif

NAMESPACE_SOUP
{
	// HardwareRng

	bool HardwareRng::isAvailable() noexcept
	{
#if SOUP_X86
		return CpuInfo::get().supportsRDSEED();
#else
		return false;
#endif
	}

	#if SOUP_X86 && (defined(__GNUC__) || defined(__clang__))
		__attribute__((target("rdseed")))
	#endif
	uint16_t HardwareRng::generate16() noexcept
	{
#if SOUP_X86
		uint16_t res;
		while (_rdseed16_step(&res) == 0);
		return res;
#else
		SOUP_ASSERT_UNREACHABLE;
#endif
	}

	#if SOUP_X86 && (defined(__GNUC__) || defined(__clang__))
		__attribute__((target("rdseed")))
	#endif
	uint32_t HardwareRng::generate32() noexcept
	{
#if SOUP_X86
		uint32_t res;
		while (_rdseed32_step(&res) == 0);
		return res;
#else
		SOUP_ASSERT_UNREACHABLE;
#endif
	}

	#if SOUP_X86 && SOUP_BITS >= 64 && (defined(__GNUC__) || defined(__clang__))
		__attribute__((target("rdseed")))
	#endif
	uint64_t HardwareRng::generate64() noexcept
	{
#if SOUP_X86
	#if SOUP_BITS >= 64
		unsigned long long res;
		while (_rdseed64_step(&res) == 0);
		return res;
	#else
		return (static_cast<uint64_t>(generate32()) << 32) | generate32();
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
