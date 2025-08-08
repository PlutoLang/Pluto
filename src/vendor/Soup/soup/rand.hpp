#pragma once

#include <random>
#include <string>

#include "base.hpp"
#include "macros.hpp"

#define ARRAY_RAND(arr) arr[::soup::rand(0, COUNT(arr) - 1)]

NAMESPACE_SOUP
{
	class rand_impl
	{
	public:
		[[nodiscard]] static uint64_t getSeed() noexcept;

//#define getConstexprSeedCounted() getConstexprSeed(__COUNTER__ + 1)

		[[nodiscard]] static constexpr uint32_t getConstexprSeed(unsigned int lcg_iterations = 1) noexcept
		{
			uint32_t seed = (((((((((((__TIME__[7] - '0') * 10) + (__TIME__[6] - '0')) * 10) + (__TIME__[4] - '0')) * 10) + (__TIME__[3] - '0')) * 10) + (__TIME__[1] - '0')) * 10) + (__TIME__[0] - '0'));

			// Performing at least 1 iteration of LCG for "better-looking" numbers since `seed` is basically just a timestamp at this point
			while (lcg_iterations--)
			{
				seed *= 22695477u;
				seed += 1;
			}

			return seed;
		}

	private:
		[[nodiscard]] static std::mt19937_64 getMersenneTwisterImpl() noexcept
		{
			return std::mt19937_64{ getSeed() };
		}

	public:
		[[nodiscard]] static SOUP_PURE std::mt19937_64& getMersenneTwister() noexcept
		{
			static std::mt19937_64 mt = getMersenneTwisterImpl();
			return mt;
		}

		template <typename T>
		[[nodiscard]] static T t(T min, T max) noexcept
		{
			std::uniform_int_distribution<T> distr{ min, max };
			return distr(getMersenneTwister());
		}

		[[nodiscard]] size_t operator()(size_t min, size_t max) const noexcept
		{
			return t<size_t>(min, max);
		}

		template <typename T>
		[[nodiscard]] const T& operator()(const std::vector<T>& vec) const
		{
			return vec.at(t<size_t>(0, vec.size() - 1));
		}

		template <typename T>
		[[nodiscard]] T operator()(std::vector<T>&& vec) const
		{
			return std::move(vec.at(t<size_t>(0, vec.size() - 1)));
		}

		[[nodiscard]] static uint8_t byte(uint8_t min = 0) noexcept;

		[[nodiscard]] static char ch(char min = 0) noexcept
		{
			return (char)byte(min);
		}

		[[nodiscard]] static bool coinflip() noexcept
		{
			return t(0, 1) != 0;
		}

		template <typename T>
		[[nodiscard]] static bool one_in(T odds) noexcept
		{
			return t<T>(0, odds - 1) == 0;
		}

		template <typename StrT, typename IntT>
		[[nodiscard]] static StrT str(const IntT len)
		{
			StrT str{};
			static const char chars[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789";
			for (IntT i = 0; i < len; i++)
			{
				str.push_back(chars[t<IntT>(0, COUNT(chars) - 2)]);
			}
			return str;
		}

		[[nodiscard]] static std::string binstr(size_t len);

		template <size_t S>
		static void fill(uint8_t(&arr)[S]) noexcept
		{
			for (size_t i = 0; i != S; ++i)
			{
				arr[i] = byte();
			}
		}

		template <size_t S>
		static void fill(uint8_t arr[S]) noexcept
		{
			for (size_t i = 0; i != S; ++i)
			{
				arr[i] = byte();
			}
		}

		static void fill(std::vector<uint8_t>& vec) noexcept
		{
			for (auto& b : vec)
			{
				b = byte();
			}
		}

		[[nodiscard]] static std::vector<uint8_t> vec_u8(size_t len) noexcept
		{
			std::vector<uint8_t> vec(len, 0);
			fill(vec);
			return vec;
		}
	};

	template <>
	inline int8_t rand_impl::t<int8_t>(int8_t min, int8_t max) noexcept
	{
		return static_cast<int8_t>(t<int16_t>(min, max));
	}

	template <>
	inline uint8_t rand_impl::t<uint8_t>(uint8_t min, uint8_t max) noexcept
	{
		return static_cast<uint8_t>(t<uint16_t>(min, max));
	}

	inline rand_impl rand;
}
