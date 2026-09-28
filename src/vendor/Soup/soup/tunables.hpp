#pragma once

#include <atomic>
#include <optional>
#include <unordered_map>

#include "joaat.hpp"

// Example usage:
// {static|inline} SOUP_TUNABLE(uint32_t, MY_TUNABLE) = 69;
// soup::tunables<uint32_t>::set("MY_TUNABLE", 420);

#define SOUP_TUNABLE(T, name) ::soup::Tunable<T, ::soup::joaat::compileTimeHash(#name)> name

NAMESPACE_SOUP
{
	template <typename T>
	struct tunables
	{
		// No mutexing as we're assuming that tunables are only registered in single-threaded static init, and we only read the map afterwards.
		// Using optional instead of unordered_map directly because static init order is not guaranteed and unordered_map would require init of its own.
		inline static std::optional<std::unordered_map<uint32_t, std::atomic<T>*>> s_tunables_map;

		[[nodiscard]] static std::atomic<T>* find(const char* name) noexcept
		{
			return find(soup::joaat::hash(name));
		}

		[[nodiscard]] static std::atomic<T>* find(uint32_t name_joaat_hash) noexcept
		{
			if (s_tunables_map)
			{
				if (auto e = s_tunables_map->find(name_joaat_hash); e != s_tunables_map->end())
				{
					return e->second;
				}
			}
			return nullptr;
		}

		static void set(const char* name, T value) noexcept
		{
			if (auto pValue = find(name))
			{
				pValue->store(std::move(value));
			}
		}

		static void set(uint32_t name_joaat_hash, T value) noexcept
		{
			if (auto pValue = find(name_joaat_hash))
			{
				pValue->store(std::move(value));
			}
		}

		static std::unordered_map<uint32_t, std::atomic<T>*>& internal_get_map()
		{
			if (!s_tunables_map.has_value())
			{
				s_tunables_map.emplace();
			}
			return *s_tunables_map;
		}

		static void internal_register(uint32_t name_joaat_hash, std::atomic<T>* pValue) noexcept
		{
			internal_get_map().emplace(name_joaat_hash, pValue);
		}
	};
	

	template <typename T, uint32_t name_joaat_hash>
	struct Tunable
	{
		std::atomic<T> value;

		Tunable() noexcept
		{
			tunables<T>::internal_register(name_joaat_hash, &value);
		}

		Tunable(T default_value) noexcept
			: value(std::move(default_value))
		{
			tunables<T>::internal_register(name_joaat_hash, &value);
		}

		void operator=(T value) noexcept
		{
			this->value.store(std::move(value));
		}

		[[nodiscard]] operator T() const noexcept
		{
			return value.load();
		}
	};
}
