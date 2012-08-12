#include "BDecoder.h"
#include "BValue.h"

#include <map>

RAIN::BEnc::Decoder::Decoder()
{
	this->root = NULL;
	this->errorState = BENC_NOERROR;
}

RAIN::BEnc::Decoder::~Decoder()
{
	delete this->root;
}

void RAIN::BEnc::Decoder::Verify(const wxString &str)
{
	this->errorState = this->TryDecode(str);
}

void RAIN::BEnc::Decoder::Decode(wxString &str)
{
	this->errorState = this->DoDecode(str);
}

int RAIN::BEnc::Decoder::DoDecode(wxString &str)
{
	this->root = new BEnc::Dictionary();
	return this->root->Read(str);
}

int RAIN::BEnc::Decoder::TryDecode(const wxString &str)
{
	wxString mystr(str);
	BEnc::Dictionary b;
	return b.Read(mystr);
}

void RAIN::BEnc::Decoder::PopulateHash(HeaderHash& hash)
{
	RAIN::BEnc::Value *key = NULL;
	RAIN::BEnc::Value *val = NULL;

	std::map<wxString, BEnc::Value*, BEnc::Cmp>::iterator it;

	it = this->root->hash.begin();

	while (it != this->root->hash.end())
	{
		hash[it->first] = RAIN::BEnc::copy(it->second);
		it++;
	}
}

RAIN::BEnc::Value * RAIN::BEnc::Decoder::getRoot()
{
	return this->root;
}
