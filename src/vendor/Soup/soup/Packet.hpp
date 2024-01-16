#pragma once

#define SOUP_PACKET(name) struct name : public ::soup::Packet<name>
#define SOUP_PACKET_IO(s) template <typename T> bool io(T& s)
#define SOUP_IF_ISREAD if constexpr (T::isRead())
#define SOUP_ELSEIF_ISWRITE else

#include <sstream>

#include "BufferWriter.hpp"
#include "IstreamReader.hpp"
#include "OstreamWriter.hpp"
#include "StringReader.hpp"
#include "StringRefReader.hpp"
#include "StringWriter.hpp"

namespace soup
{
	template <typename T>
	class Packet
	{
	protected:
#include "shortint_impl.hpp"
		using u24 = u32;
		using u40 = u64;
		using u48 = u64;
		using u56 = u64;

	public:
		bool fromBinary(std::string&& bin, Endian endian = ENDIAN_BIG) noexcept
		{
			StringReader r(std::move(bin), endian);
			return read(r);
		}

		bool fromBinary(const std::string& bin, Endian endian = ENDIAN_BIG) noexcept
		{
			StringRefReader r(bin, endian);
			return read(r);
		}

		bool fromBinaryLE(std::string&& bin) noexcept
		{
			StringReader r(std::move(bin), ENDIAN_LITTLE);
			return read(r);
		}

		bool fromBinaryLE(const std::string& bin) noexcept
		{
			StringRefReader r(bin, ENDIAN_LITTLE);
			return read(r);
		}

		bool read(std::istream& is, Endian endian = ENDIAN_BIG)
		{
			IstreamReader r(is, endian);
			return read(r);
		}

		bool readLE(std::istream& is)
		{
			return read(is, ENDIAN_LITTLE);
		}

		template <typename Reader = Reader>
		bool read(Reader& r)
		{
			return static_cast<T*>(this)->template io<Reader>(r);
		}

		[[nodiscard]] Buffer toBinary(Endian endian = ENDIAN_BIG) SOUP_EXCAL
		{
			BufferWriter w(endian);
			write(w);
			return w.buf;
		}

		[[nodiscard]] std::string toBinaryString(Endian endian = ENDIAN_BIG) SOUP_EXCAL
		{
			StringWriter w(endian);
			write(w);
			return w.data;
		}

		[[nodiscard]] std::string toBinaryStringLE()
		{
			return toBinaryString(ENDIAN_LITTLE);
		}

		bool write(std::ostream& os, Endian endian = ENDIAN_BIG)
		{
			OstreamWriter w(os, endian);
			return write(w);
		}

		bool writeLE(std::ostream& os)
		{
			return write(os, ENDIAN_LITTLE);
		}

		template <typename Writer = Writer>
		bool write(Writer& w)
		{
			return static_cast<T*>(this)->template io<Writer>(w);
		}
	};
}
