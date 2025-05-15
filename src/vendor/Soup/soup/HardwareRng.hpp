#pragma once

#include <cstdint>
#include <cstddef> // size_t

#include "RngInterface.hpp"

NAMESPACE_SOUP
{
	// For the purposes of this class, "hardware RNG" is a true RNG, with each bit coming directly from a hardware entropy source.
	struct HardwareRng
	{
		[[nodiscard]] static bool isAvailable() noexcept;

		[[nodiscard]] static uint16_t generate16() noexcept;
		[[nodiscard]] static uint32_t generate32() noexcept;
		[[nodiscard]] static uint64_t generate64() noexcept;

		// To satisfy the named requirement UniformRandomBitGenerator:
		using result_type = uint64_t;
		[[nodiscard]] static constexpr uint64_t min() noexcept { return 0; }
		[[nodiscard]] static constexpr uint64_t max() noexcept { return -1; }
		[[nodiscard]] uint64_t operator()() const noexcept { return generate64(); }
	};

	// For the purposes of this class, "fast hardware RNG" is backed by a hardware entropy source, but uses processing (a DRBG) to extrapolate more bits.
	struct FastHardwareRng
	{
		static void generate(void* buf, size_t buflen) noexcept;
		[[nodiscard]] static uint16_t generate16() noexcept;
		[[nodiscard]] static uint32_t generate32() noexcept;
		[[nodiscard]] static uint64_t generate64() noexcept;

		// To satisfy the named requirement UniformRandomBitGenerator:
		using result_type = uint64_t;
		[[nodiscard]] static constexpr uint64_t min() noexcept { return 0; }
		[[nodiscard]] static constexpr uint64_t max() noexcept { return -1; }
		[[nodiscard]] uint64_t operator()() const noexcept { return generate64(); }
	};

	template <typename T>
	struct StatelessRngWrapper : public StatelessRngInterface
	{
		uint64_t generate() final
		{
			return T::generate64();
		}
	};
	using HardwareRngInterface = StatelessRngWrapper<HardwareRng>;
	using FastHardwareRngInterface = StatelessRngWrapper<FastHardwareRng>;
}
