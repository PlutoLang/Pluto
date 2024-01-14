#pragma once

#include <cstdint>
#include <unordered_map>

#include "Capture.hpp"
#include "joaat.hpp"

namespace soup
{
	struct StructMap : public std::unordered_map<uint32_t, Capture>
	{
		[[nodiscard]] bool containsImpl(uint32_t id) const noexcept
		{
			return find(id) != end();
		}

		template <typename T, uint32_t id>
		[[nodiscard]] T& getImpl()
		{
			auto e = find(id);
			if (e != end())
			{
				return e->second.get<T>();
			}
			return emplace(id, T{}).first->second.template get<T>();
		}

		void removeImpl(uint32_t id)
		{
			if (auto e = find(id); e != end())
			{
				erase(e);
			}
		}

#define isStructInMap(T) containsImpl(::soup::joaat::compileTimeHash(#T))
#define addStructToMap(T, inst) emplace(::soup::joaat::compileTimeHash(#T), inst); static_assert(std::is_same_v<T, decltype(inst)>)
#define getStructFromMap(T) getImpl<T, ::soup::joaat::compileTimeHash(#T)>()
#define getStructFromMapConst(T) at(::soup::joaat::compileTimeHash(#T)).get<T>()
#define removeStructFromMap(T) removeImpl(::soup::joaat::compileTimeHash(#T))
	};
}
