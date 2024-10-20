#pragma once

#include <atomic>

#include "base.hpp" // SOUP_EXCAL
#include "PoppedNode.hpp"

NAMESPACE_SOUP
{
	template <typename Data>
	struct AtomicDeque
	{
		struct Node
		{
			std::atomic<Node*> next = nullptr;
			Data data;

			Node(Data&& data)
				: data(std::move(data))
			{
			}
		};

		std::atomic<Node*> head = nullptr;

		~AtomicDeque() noexcept
		{
			for (Node* node = head.load(); node != nullptr; )
			{
				Node* tbd = node;
				node = node->next;
				delete tbd;
			}
		}

		[[nodiscard]] bool empty() const noexcept
		{
			return head.load() == nullptr;
		}

		[[nodiscard]] size_t size() const noexcept
		{
			size_t i = 0;
			for (Node* node = head.load(); node != nullptr; node = node->next)
			{
				++i;
			}
			return i;
		}

		Node* back() noexcept
		{
			return *getTail();
		}

		std::atomic<Node*>* getTail() noexcept
		{
			Node* node = head.load();
			std::atomic<Node*>* next = &head;
			std::atomic<Node*>* ptr = next;
			while (node != nullptr)
			{
				ptr = next;
				next = &node->next;
				node = next->load();
			}
			return ptr;
		}

		void emplace_front(Data&& data) SOUP_EXCAL
		{
			Node* node = new Node(std::move(data));
			Node* next = head.load();
			do
			{
				node->next = next;
			} while (!head.compare_exchange_weak(next, node));
		}

		// This is not safe to be concurrently called with `pop_front`
		[[deprecated]] void emplace_back(Data&& data) SOUP_EXCAL
		{
			Node* node = new Node(std::move(data));
			while (true)
			{
				Node* expected = nullptr;
				if (head.compare_exchange_weak(expected, node))
				{
					break;
				}
				auto tail = getTail();
				Node* prev = tail->load();
				if (prev)
				{
					expected = nullptr;
					if (prev->next.compare_exchange_weak(expected, node))
					{
						break;
					}
				}
			}
		}

		PoppedNode<Node, Data> pop_front() noexcept
		{
			Node* node = head.load();
			while (node && !head.compare_exchange_weak(node, node->next))
			{
			}
			return node;
		}

		PoppedNode<Node, Data> pop_back() noexcept
		{
			std::atomic<Node*>* tail;
			Node* node;
			do
			{
				tail = getTail();
				node = tail->load();
			} while (node && !tail->compare_exchange_weak(node, node->next));
			return node;
		}

		// Iterators for this type are not a good idea because a node could be popped while an interator points to it.

		/*using Iterator = LinkedIterator<Node, Data>;

		[[nodiscard]] Iterator begin() const noexcept
		{
			return head.load();
		}

		[[nodiscard]] static constexpr Iterator end() noexcept
		{
			return nullptr;
		}

		void decouple(Node* node)
		{
			if (!head.compare_exchange_weak(node, node->next))
			{
				Node* v;
				do
				{
					v = head.load();
				} while (v == node && !head.compare_exchange_weak(v, node->next));
			}

			while (true)
			{
				Node* prev = nullptr;
				for (auto i = begin(); i != end(); ++i)
				{
					if (i.node->next == node)
					{
						prev = i.node;
						break;
					}
				}
				if (prev == nullptr)
				{
					break;
				}
				Node* n = node;
				prev->next.compare_exchange_weak(n, nullptr);
			}
		}

		Iterator erase(Iterator it)
		{
			Node* node = it.node;
			decouple(node);
			it = node->next.load();
			delete node;
			return it;
		}*/
	};
}
