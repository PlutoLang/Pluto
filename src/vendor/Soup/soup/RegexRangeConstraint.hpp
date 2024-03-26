#pragma once

#include "RegexConstraint.hpp"

#include "base.hpp"
#include "BigBitset.hpp"
#include "RegexMatcher.hpp"

NAMESPACE_SOUP
{
	struct RegexRangeConstraint : public RegexConstraint
	{
		BigBitset<0x100 / 8> mask{};
		bool inverted = false;

		inline static const char digits[] = { '0', '1', '2', '3', '4', '5', '6', '7', '8', '9' };
		inline static const char whitespace[] = { '\r', '\n', '\t', '\f', '\v', ' ' };

		RegexRangeConstraint(const char*& it, const char* end, bool insensitive)
		{
			if (++it == end)
			{
				return;
			}
			if (*it == '^')
			{
				inverted = true;
			}
			else
			{
				--it;
			}
			char range_begin = 0;
			while (++it != end && *it != ']')
			{
				if (*it == '-')
				{
					SOUP_IF_UNLIKELY (++it == end)
					{
						break;
					}
					if (range_begin <= *it)
					{
						for (char c = range_begin; c != *it; ++c)
						{
							mask.enable(c);
						}
					}
				}
				else if (*it == '\\')
				{
					SOUP_IF_UNLIKELY (++it == end)
					{
						break;
					}
					if (*it == 'd')
					{
						for (auto& c : digits)
						{
							mask.enable(c);
						}
						continue;
					}
					if (*it == 's')
					{
						for (auto& c : whitespace)
						{
							mask.enable(c);
						}
						continue;
					}
				}
				else if (*it == '['
					&& (it + 1) != end && *++it == ':'
					)
				{
					if ((it + 1) != end && *(it + 1) == 'a'
						&& (it + 2) != end && *(it + 2) == 'l'
						&& (it + 3) != end && *(it + 3) == 'n'
						&& (it + 4) != end && *(it + 4) == 'u'
						&& (it + 5) != end && *(it + 5) == 'm'
						)
					{
						it += 5;
						for (uint8_t c = '0'; c != '9' + 1; ++c)
						{
							mask.enable(c);
						}
						for (uint8_t c = 'A'; c != 'Z' + 1; ++c)
						{
							mask.enable(c);
						}
						for (uint8_t c = 'a'; c != 'z' + 1; ++c)
						{
							mask.enable(c);
						}
					}
					else if ((it + 1) != end && *(it + 1) == 'a'
						&& (it + 2) != end && *(it + 2) == 'l'
						&& (it + 3) != end && *(it + 3) == 'p'
						&& (it + 4) != end && *(it + 4) == 'h'
						&& (it + 5) != end && *(it + 5) == 'a'
						)
					{
						it += 5;
						for (uint8_t c = 'A'; c != 'Z' + 1; ++c)
						{
							mask.enable(c);
						}
						for (uint8_t c = 'a'; c != 'z' + 1; ++c)
						{
							mask.enable(c);
						}
					}
					else if ((it + 1) != end && *(it + 1) == 'a'
						&& (it + 2) != end && *(it + 2) == 's'
						&& (it + 3) != end && *(it + 3) == 'c'
						&& (it + 4) != end && *(it + 4) == 'i'
						&& (it + 5) != end && *(it + 5) == 'i'
						)
					{
						it += 5;
						for (uint8_t c = 0x00; c != 0x7F + 1; ++c)
						{
							mask.enable(c);
						}
					}
					else if ((it + 1) != end && *(it + 1) == 'b'
						&& (it + 2) != end && *(it + 2) == 'l'
						&& (it + 3) != end && *(it + 3) == 'a'
						&& (it + 4) != end && *(it + 4) == 'n'
						&& (it + 5) != end && *(it + 5) == 'k'
						)
					{
						it += 5;
						mask.enable(' ');
						mask.enable('\t');
					}
					else if ((it + 1) != end && *(it + 1) == 'c'
						&& (it + 2) != end && *(it + 2) == 'n'
						&& (it + 3) != end && *(it + 3) == 't'
						&& (it + 4) != end && *(it + 4) == 'r'
						&& (it + 5) != end && *(it + 5) == 'l'
						)
					{
						it += 5;
						for (uint8_t c = 0x00; c != 0x1F + 1; ++c)
						{
							mask.enable(c);
						}
						mask.enable(0x7F);
					}
					else if ((it + 1) != end && *(it + 1) == 'd'
						&& (it + 2) != end && *(it + 2) == 'i'
						&& (it + 3) != end && *(it + 3) == 'g'
						&& (it + 4) != end && *(it + 4) == 'i'
						&& (it + 5) != end && *(it + 5) == 't'
						)
					{
						it += 5;
						for (uint8_t c = '0'; c != '9' + 1; ++c)
						{
							mask.enable(c);
						}
					}
					else if ((it + 1) != end && *(it + 1) == 'g'
						&& (it + 2) != end && *(it + 2) == 'r'
						&& (it + 3) != end && *(it + 3) == 'a'
						&& (it + 4) != end && *(it + 4) == 'p'
						&& (it + 5) != end && *(it + 5) == 'h'
						)
					{
						it += 5;
						for (uint8_t c = 0x21; c != 0x7E + 1; ++c)
						{
							mask.enable(c);
						}
					}
					else if ((it + 1) != end && *(it + 1) == 'l'
						&& (it + 2) != end && *(it + 2) == 'o'
						&& (it + 3) != end && *(it + 3) == 'w'
						&& (it + 4) != end && *(it + 4) == 'e'
						&& (it + 5) != end && *(it + 5) == 'r'
						)
					{
						it += 5;
						for (uint8_t c = 'a'; c != 'z' + 1; ++c)
						{
							mask.enable(c);
						}
					}
					else if ((it + 1) != end && *(it + 1) == 'u'
						&& (it + 2) != end && *(it + 2) == 'p'
						&& (it + 3) != end && *(it + 3) == 'p'
						&& (it + 4) != end && *(it + 4) == 'e'
						&& (it + 5) != end && *(it + 5) == 'r'
						)
					{
						it += 5;
						for (uint8_t c = 'A'; c != 'Z' + 1; ++c)
						{
							mask.enable(c);
						}
					}
					else if ((it + 1) != end && *(it + 1) == 'w'
						&& (it + 2) != end && *(it + 2) == 'o'
						&& (it + 3) != end && *(it + 3) == 'r'
						&& (it + 4) != end && *(it + 4) == 'd'
						)
					{
						it += 4;
						for (uint8_t c = '0'; c != '9' + 1; ++c)
						{
							mask.enable(c);
						}
						for (uint8_t c = 'A'; c != 'Z' + 1; ++c)
						{
							mask.enable(c);
						}
						for (uint8_t c = 'a'; c != 'z' + 1; ++c)
						{
							mask.enable(c);
						}
						mask.enable('_');
					}
					else if ((it + 1) != end && *(it + 1) == 'x'
						&& (it + 2) != end && *(it + 2) == 'd'
						&& (it + 3) != end && *(it + 3) == 'i'
						&& (it + 4) != end && *(it + 4) == 'g'
						&& (it + 5) != end && *(it + 5) == 'i'
						&& (it + 6) != end && *(it + 6) == 't'
						)
					{
						it += 6;
						for (uint8_t c = '0'; c != '9' + 1; ++c)
						{
							mask.enable(c);
						}
						for (uint8_t c = 'A'; c != 'F' + 1; ++c)
						{
							mask.enable(c);
						}
						for (uint8_t c = 'a'; c != 'f' + 1; ++c)
						{
							mask.enable(c);
						}
					}
					else
					{
						SOUP_THROW(Exception("Unrecognised class in [[:class:]]"));
					}
					if ((it + 1) != end) { ++it; } // :
					if ((it + 1) != end) { ++it; } // ]
					continue;
				}
				mask.enable(*it);
				range_begin = (*it) + 1;
			}
			if (insensitive)
			{
				for (uint8_t c = 'a'; c != 'z' + 1; ++c)
				{
					if (mask.get(c))
					{
						mask.enable(c - 'a' + 'A');
					}
				}
				for (uint8_t c = 'A'; c != 'Z' + 1; ++c)
				{
					if (mask.get(c))
					{
						mask.enable(c - 'A' + 'a');
					}
				}
			}
		}

		template <size_t S>
		RegexRangeConstraint(const char(&arr)[S])
		{
			for (const auto& c : arr)
			{
				mask.enable(c);
			}
		}

		[[nodiscard]] bool matches(RegexMatcher& m) const noexcept final
		{
			if (m.it == m.end)
			{
				return false;
			}
			if (mask.get(static_cast<unsigned char>(*m.it)) == inverted)
			{
				return false;
			}
			++m.it;
			return true;
		}

		static void appendPresentably(std::string& str, char c) noexcept
		{
			switch (c)
			{
			case '\r': str.append("\\r"); return;
			case '\n': str.append("\\n"); return;
			case '\t': str.append("\\t"); return;
			case '\f': str.append("\\f"); return;
			case '\v': str.append("\\v"); return;
			}
			str.push_back(c);
		}

		[[nodiscard]] std::string toString() const noexcept final
		{
			std::string str(1, '[');
			if (inverted)
			{
				str.push_back('^');
			}
			uint16_t range_begin = 0x100;
			for (uint16_t i = 0; i != 0x100; ++i)
			{
				if (mask.get(i))
				{
					if (range_begin == 0x100)
					{
						range_begin = i;
					}
				}
				else
				{
					if (range_begin != 0x100)
					{
						const uint8_t range_end = static_cast<uint8_t>(i);
						const uint8_t range_len = (range_end - range_begin);
						if (range_len > 3)
						{
							appendPresentably(str, static_cast<uint8_t>(range_begin));
							str.push_back('-');
							appendPresentably(str, range_end - 1);
						}
						else
						{
							for (uint16_t j = range_begin; j != range_end; ++j)
							{
								appendPresentably(str, static_cast<uint8_t>(j));
							}
						}
						range_begin = 0x100;
					}
				}
			}
			if (range_begin != 0x100)
			{
				constexpr uint16_t range_end = 0x100;
				const uint8_t range_len = (range_end - range_begin);
				if (range_len > 3)
				{
					appendPresentably(str, static_cast<uint8_t>(range_begin));
					str.push_back('-');
					appendPresentably(str, (char)(range_end - 1));
				}
				else
				{
					for (uint16_t j = range_begin; j != range_end; ++j)
					{
						appendPresentably(str, static_cast<uint8_t>(j));
					}
				}
			}
			str.push_back(']');
			return str;
		}

		[[nodiscard]] size_t getCursorAdvancement() const final
		{
			return 1;
		}

		[[nodiscard]] UniquePtr<RegexConstraint> clone(RegexTransitionsVector& success_transitions) const final
		{
			auto cc = soup::make_unique<RegexRangeConstraint>(*this);
			success_transitions.setTransitionTo(cc->getEntrypoint());
			success_transitions.emplace(&cc->success_transition);
			return cc;
		}
	};
}
