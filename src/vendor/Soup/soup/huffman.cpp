#include "huffman.hpp"

#include <cstring> // memset
#include <queue>

#include "bitutil.hpp"

NAMESPACE_SOUP
{
	uint8_t huffman_lengths(uint8_t lengths[], const size_t frequencies[], uint_fast16_t num_symbols) SOUP_EXCAL
	{
		memset(lengths, 0, num_symbols);

		std::vector<size_t> used;
		for (size_t i = 0; i != num_symbols; ++i)
		{
			if (frequencies[i] != 0)
			{
				used.push_back(i);
			}
		}

		const size_t m = used.size();
		if (m == 0)
		{
			return 0;
		}
		if (m == 1)
		{
			lengths[used[0]] = 1;
			return 1;
		}

		// Build the Huffman tree (leaves: 0..m-1, internal: m..2m-2)
		using Item = std::pair<size_t, size_t>; // (weight, node id) -> deterministic ties
		std::priority_queue<Item, std::vector<Item>, std::greater<Item>> pq;
		for (size_t i = 0; i != m; ++i)
		{
			pq.emplace(frequencies[used[i]], i);
		}
		std::vector<size_t> parent(2 * m - 1, 0);
		size_t next = m;
		while (pq.size() > 1)
		{
			Item a = pq.top(); pq.pop();
			Item b = pq.top(); pq.pop();
			parent[a.second] = parent[b.second] = next;
			pq.emplace(a.first + b.first, next);
			++next;
		}

		// Depths: parents always have larger ids than children, so go top-down.
		std::vector<uint8_t> depth(2 * m - 1, 0);
		uint8_t deepest = 0;
		for (size_t i = 2 * m - 2; i-- > 0;)
		{
			depth[i] = depth[parent[i]] + 1;
			if (i < m) deepest = std::max(deepest, depth[i]);
		}

		for (size_t i = 0; i != m; ++i)
		{
			lengths[used[i]] = depth[i];
		}

		return deepest;
	}

	void huffman_codes(uint16_t codes[], const uint8_t lengths[], uint_fast16_t num_symbols) noexcept
	{
		constexpr uint_fast16_t MAX_LENGTH = 15;

		uint_fast16_t bl_count[MAX_LENGTH + 1] = {};
		uint16_t next_code[MAX_LENGTH + 2] = {};

		// Step 1: count the number of codes for each code length.
		for (uint_fast16_t i = 0; i != num_symbols; ++i)
		{
			const uint8_t len = lengths[i];
			if (len <= MAX_LENGTH)
			{
				bl_count[len]++;
			}
		}
		bl_count[0] = 0;

		// Step 2: find the numerical value of the smallest code for each length.
		{
			uint16_t code = 0;
			for (uint_fast16_t length = 1; length <= MAX_LENGTH; length++)
			{
				code = (code + bl_count[length - 1]) << 1;
				next_code[length] = code;
			}
		}

		// Step 3: assign consecutive values to all symbols of the same length, in symbol order.
		for (uint_fast16_t i = 0; i != num_symbols; ++i)
		{
			const uint8_t len = lengths[i];
			if (len != 0 && len <= MAX_LENGTH)
			{
				codes[i] = bitutil::reverse(next_code[len]++, len);
			}
			else
			{
				codes[i] = 0;
			}
		}
	}
}
