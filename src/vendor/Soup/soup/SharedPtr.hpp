#pragma once

#include <atomic>
#include <memory> // destroy_at

#include "Exception.hpp"
#include "memory.hpp" // construct_at
#include "type_traits.hpp"

#ifndef SOUP_DEBUG_SHAREDPTR
#define SOUP_DEBUG_SHAREDPTR false
#endif

#if SOUP_DEBUG_SHAREDPTR
#include <iostream>
#include <unordered_set>
#endif

NAMESPACE_SOUP
{
	template <typename T>
	class SharedPtr
	{
	public:
#if SOUP_DEBUG_SHAREDPTR
		inline static std::unordered_set<T*> managed_instances{};
#endif

		struct Data
		{
			T* inst;
			std::atomic_uint refcount;
			bool was_created_with_make_shared = false;

			Data(T* inst) noexcept
				: inst(inst), refcount(1)
			{
#if SOUP_DEBUG_SHAREDPTR
				SOUP_ASSERT(inst != nullptr);
#endif
#if SOUP_DEBUG_SHAREDPTR
				if (managed_instances.contains(inst))
				{
					__debugbreak(); // Already managed by another SharedPtr instance
				}
				managed_instances.emplace(inst);
				std::cout << (void*)inst << " :: New SharedPtr\n";
#endif
			}

			void incref() noexcept
			{
#if SOUP_DEBUG_SHAREDPTR
				const auto refcount_now = ++refcount;
				if (refcount_now == 1)
				{
					__debugbreak(); // Attempt to revive the dead
				}
				std::cout << (void*)inst << " :: Increment to " << refcount_now << "\n";
#else
				++refcount;
#endif
			}

			void decref() noexcept
			{
#if SOUP_DEBUG_SHAREDPTR
				const auto predec = refcount.load();
#endif
				if (--refcount == 0)
				{
#if SOUP_DEBUG_SHAREDPTR
					std::cout << (void*)inst << " :: Decrement to 0; destroying instance\n";
					managed_instances.erase(inst);
#endif
					if (was_created_with_make_shared)
					{
						const auto inst = this->inst;
						std::destroy_at<>(inst);
						std::destroy_at<>(this);
						::operator delete(reinterpret_cast<void*>(inst));
					}
					else
					{
						delete inst;
						delete this;
					}
				}
#if SOUP_DEBUG_SHAREDPTR
				else
				{
					std::cout << (void*)inst << " :: Decrement to " << refcount.load() << "\n";
				}
#endif
			}
		};

		std::atomic<Data*> data;

		SharedPtr() noexcept
			: data(nullptr)
		{
		}

		SharedPtr(Data* data) noexcept
			: data(data)
		{
		}

		SharedPtr(T* inst) SOUP_EXCAL
			: data(new Data(inst))
		{
		}

		SharedPtr(const SharedPtr<T>& b) noexcept
			: data(b.data.load())
		{
			if (data != nullptr)
			{
				data.load()->incref();
			}
		}

		SharedPtr(SharedPtr<T>&& b) noexcept
			: data(b.data.load())
		{
			b.data = nullptr;
		}

		// reinterpret_cast<Data*> is okay because it contains a pointer to the actual instance, so it's all the same regardless of T.

		template <typename T2, SOUP_RESTRICT(std::is_base_of_v<T, T2> || std::is_base_of_v<T2, T>)>
		SharedPtr(const SharedPtr<T2>& b) noexcept
			: data(reinterpret_cast<Data*>(b.data.load()))
		{
			if (data != nullptr)
			{
				data.load()->incref();
			}
		}

		template <typename T2, SOUP_RESTRICT(std::is_base_of_v<T, T2> || std::is_base_of_v<T2, T>)>
		SharedPtr(SharedPtr<T2>&& b) noexcept
			: data(reinterpret_cast<Data*>(b.data.load()))
		{
			b.data = nullptr;
		}

		void operator=(const SharedPtr<T>& b) noexcept
		{
			Data* const prev_data = this->data;
			Data* const new_data = b.data;
			this->data = new_data;
			if (new_data != nullptr)
			{
				new_data->incref();
			}
			if (prev_data != nullptr)
			{
#if SOUP_DEBUG_SHAREDPTR
				SOUP_ASSERT(prev_data != this->data);
#endif
				prev_data->decref();
			}
		}

		void operator=(SharedPtr<T>&& b) noexcept
		{
			Data* const prev_data = this->data;
			this->data = b.data.load();
			b.data = nullptr;
			if (prev_data != nullptr)
			{
#if SOUP_DEBUG_SHAREDPTR
				SOUP_ASSERT(prev_data != this->data);
#endif
				prev_data->decref();
			}
		}

		~SharedPtr() noexcept
		{
			if (data.load() != nullptr)
			{
				data.load()->decref();
			}
		}

		void reset() noexcept
		{
			Data* const data = this->data;
			if (data != nullptr)
			{
				this->data = nullptr;
				data->decref();
			}
		}

		[[nodiscard]] operator bool() const noexcept
		{
			return data != nullptr;
		}

		[[nodiscard]] operator T* () const noexcept
		{
			return get();
		}

		[[nodiscard]] T* get() const noexcept
		{
			Data* const data = this->data;
			if (data)
			{
				return data->inst;
			}
			return nullptr;
		}

		[[nodiscard]] T& operator*() const noexcept
		{
			return *get();
		}

		[[nodiscard]] T* operator->() const noexcept
		{
			return get();
		}

		[[nodiscard]] size_t getRefCount() const noexcept
		{
			return data.load()->refcount.load();
		}

		[[nodiscard]] T* release()
		{
			Data* const data = this->data;
			this->data = nullptr;
			if (data->refcount.load() != 1)
			{
				this->data = data;
				SOUP_THROW(Exception("Attempt to release SharedPtr with more than 1 reference"));
			}
			T* const inst = data->inst;
			const auto was_created_with_make_shared = data->was_created_with_make_shared;
			std::destroy_at<>(data);
			if (was_created_with_make_shared)
			{
				// data will continue to be allocated behind the instance, but once the instance is free'd, data is also free'd.
			}
			else
			{
				::operator delete(reinterpret_cast<void*>(data));
			}
			return inst;
		}
	};

	template <typename T, typename...Args, SOUP_RESTRICT(!std::is_array_v<T>)>
	[[nodiscard]] SharedPtr<T> make_shared(Args&&...args)
	{
		struct Combined
		{
			alignas(T) char t[sizeof(T)];
			typename SharedPtr<T>::Data data;
		};

		void* const b = ::operator new(sizeof(Combined));
		typename SharedPtr<T>::Data* data;
		SOUP_TRY
		{
			auto inst = soup::construct_at<>(reinterpret_cast<T*>(b), std::forward<Args>(args)...);
			data = soup::construct_at<>(reinterpret_cast<typename SharedPtr<T>::Data*>(reinterpret_cast<uintptr_t>(b) + offsetof(Combined, data)), inst);
		}
		SOUP_CATCH_ANY
		{
			::operator delete(b);
			std::rethrow_exception(std::current_exception());
		}
		data->was_created_with_make_shared = true;
		return SharedPtr<T>(data);
	}
}
