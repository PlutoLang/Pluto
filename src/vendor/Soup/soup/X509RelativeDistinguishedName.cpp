#include "X509RelativeDistinguishedName.hpp"

#include "Asn1Sequence.hpp"

NAMESPACE_SOUP
{
	void X509RelativeDistinguishedName::read(const Asn1Sequence& seq)
	{
		for (size_t i = 0; i != seq.countChildren(); ++i)
		{
			auto kv = seq.getSeq(i).getSeq(0);
			emplace_back(kv.getOid(0), kv.getString(1));
		}
	}

	Asn1Sequence X509RelativeDistinguishedName::toSet() const
	{
		Asn1Sequence set;
		for (const auto& e : *this)
		{
			Asn1Sequence seq;
			seq.addOid(e.first);
			seq.addPrintableString(e.second);
			set.addSeq(seq);
		}
		return set;
	}
}
