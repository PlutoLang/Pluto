#pragma once

#include "base.hpp"
#include "fwd.hpp"

#include <string>
#include <unordered_map>

#include "SharedPtr.hpp"

NAMESPACE_SOUP
{
	class Mixed
	{
	public:
		enum Type : uint8_t
		{
			NONE = 0,
			INT,
			UINT,
			STRING,
			FUNC,
			VAR_NAME,
			MIXED_SP_MIXED_MAP,
			AST_BLOCK,
			CANVAS,
		};

		Type type = NONE;
		uint64_t val;

		Mixed() = default;

		Mixed(const Mixed& b);

		Mixed(Mixed&& b)
			: type(b.type), val(b.val)
		{
			b.type = NONE;
		}

		Mixed(int32_t val)
			: type(INT), val((uint64_t)val)
		{
		}
		
		Mixed(int64_t val)
			: type(INT), val((uint64_t)val)
		{
		}

		Mixed(uint32_t val)
			: type(UINT), val((uint64_t)val)
		{
		}
		
		Mixed(uint64_t val)
			: type(UINT), val(val)
		{
		}

		Mixed(const char* val)
			: type(STRING), val((uint64_t)new std::string(val))
		{
		}
		
		Mixed(const std::string& val)
			: type(STRING), val((uint64_t)new std::string(val))
		{
		}

		Mixed(std::string&& val)
			: type(STRING), val((uint64_t)new std::string(std::move(val)))
		{
		}

		Mixed(std::string* val) // takes ownership
			: type(STRING), val((uint64_t)val)
		{
		}

		Mixed(std::unordered_map<Mixed, SharedPtr<Mixed>>&& val);

		Mixed(astBlock* val); // takes ownership

		Mixed(Canvas&& val);

		~Mixed() noexcept
		{
			release();
		}

	private:
		void release();

	public:
		void reset()
		{
			release();
			type = NONE;
		}

		void operator =(const Mixed& b)
		{
			release();
			type = b.type;
			if (type == STRING)
			{
				val = (uint64_t)new std::string(b.getString());
			}
			else
			{
				val = b.val;
			}
		}

		void operator =(Mixed&& b)
		{
			release();
			type = b.type;
			val = b.val;
			b.type = NONE;
		}

		void operator =(int32_t val)
		{
			release();
			this->type = INT;
			this->val = (uint64_t)val;
		}

		void operator =(int64_t val)
		{
			release();
			this->type = INT;
			this->val = (uint64_t)val;
		}
		
		void operator =(uint64_t val)
		{
			release();
			this->type = UINT;
			this->val = val;
		}

		void operator =(const std::string& val)
		{
			release();
			this->type = STRING;
			this->val = (uint64_t)new std::string(val);
		}

		void operator =(std::string&& val)
		{
			release();
			this->type = STRING;
			this->val = (uint64_t)new std::string(std::move(val));
		}

		[[nodiscard]] bool operator==(const Mixed& b) const noexcept
		{
			if (type != b.type)
			{
				return false;
			}
			if (type == STRING)
			{
				return getString() == b.getString();
			}
			return val == b.val;
		}

		[[nodiscard]] constexpr bool empty() const noexcept
		{
			return type == NONE;
		}
		
		[[nodiscard]] constexpr bool isInt() const noexcept
		{
			return type == INT;
		}

		[[nodiscard]] constexpr bool isUInt() const noexcept
		{
			return type == UINT;
		}

		[[nodiscard]] constexpr bool isString() const noexcept
		{
			return type == STRING;
		}

		[[nodiscard]] constexpr bool isFunc() const noexcept
		{
			return type == FUNC;
		}

		[[nodiscard]] constexpr bool isVarName() const noexcept
		{
			return type == VAR_NAME;
		}

		[[nodiscard]] constexpr bool isAstBlock() const noexcept
		{
			return type == AST_BLOCK;
		}

		[[nodiscard]] constexpr bool isCanvas() const noexcept
		{
			return type == CANVAS;
		}

		[[nodiscard]] static const char* getTypeName(Type t) noexcept;
		[[nodiscard]] const char* getTypeName() const noexcept;

		[[nodiscard]] std::string toString(const std::string& ast_block_prefix = {}) const noexcept;
		[[nodiscard]] std::string toStringWithFallback() const noexcept;

		friend std::ostream& operator<<(std::ostream& os, const Mixed& v);

		void assertType(Type e) const;

		[[nodiscard]] int64_t getInt() const;
		[[nodiscard]] uint64_t getUInt() const;
		[[nodiscard]] std::string& getString() const;
		[[nodiscard]] std::string& getFunc() const;
		[[nodiscard]] std::string& getVarName() const;
		[[nodiscard]] std::unordered_map<Mixed, SharedPtr<Mixed>>& getMixedSpMixedMap() const;
		[[nodiscard]] astBlock& getAstBlock() const;
		[[nodiscard]] Canvas& getCanvas() const;
	};
}

namespace std
{
	template<>
	struct hash<::soup::Mixed>
	{
		size_t operator() (const ::soup::Mixed& key) const
		{
			if (key.isString())
			{
				return hash<string>()(key.getString());
			}
			return key.type | static_cast<size_t>(key.val);
		}
	};
}
