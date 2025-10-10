#pragma once

#include "base.hpp"

NAMESPACE_SOUP
{
	template <typename Node, typename Data>
	struct PoppedNode
	{
		Node* node;

		constexpr PoppedNode(Node* node = nullptr) noexcept
			: node(node)
		{
		}

		PoppedNode(PoppedNode&& b) noexcept
			: node(b.node)
		{
			b.node = nullptr;
		}

		~PoppedNode()
		{
			free();
		}

		operator bool() const noexcept
		{
			return node != nullptr;
		}

		void free() noexcept
		{
			if (*this)
			{
				delete node;
			}
		}

		PoppedNode& operator =(PoppedNode&& b) noexcept
		{
			free();
			node = b.node;
			b.node = nullptr;
			return *this;
		}

		Data& operator *()
		{
			return node->data;
		}

		Data* operator ->()
		{
			return &node->data;
		}
	};
}
