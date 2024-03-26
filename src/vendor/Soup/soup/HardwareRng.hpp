#pragma once

#include <cstdint>

#include "RngInterface.hpp"

namespace soup
{
	// For the purposes of this class, "hardware RNG" is such that no bit is predictable to an attacker even with unlimited resources.
	// As such, it uses RDSEED on x86, whereas FastHardwareRng uses RDRAND.
	struct HardwareRng
	{
		[[nodiscard]] static bool isAvailable() noexcept;

		[[nodiscard]] static uint16_t generate16() noexcept;
		[[nodiscard]] static uint32_t generate32() noexcept;
		[[nodiscard]] static uint64_t generate64() noexcept;
	};

	// For the purposes of this class, "fast hardware RNG" is such that no bit is predictable to an attacker with limited resources,
	// but some bits may be predictable to an attacker with unlimited resources.
	// As such, it uses RDRAND on x86, whereas HardwareRng uses RDSEED.
	struct FastHardwareRng
	{
		[[nodiscard]] static bool isAvailable() noexcept;

		[[nodiscard]] static uint16_t generate16() noexcept;
		[[nodiscard]] static uint32_t generate32() noexcept;
		[[nodiscard]] static uint64_t generate64() noexcept;
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
