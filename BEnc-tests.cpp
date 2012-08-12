#include "TestingFramework.h"
#include "BEnc-tests.h"

#include "BValue.h"
#include "BDecoder.h"

using namespace RAIN;

void BEncTests::RunTests()
{
	RunIntegerTests();
	RunStringTests();
	RunListTests();
	RunDictionaryTests();

	RunDecoderTests();
}

void BEncTests::RunIntegerTests()
{
	/* integer storage and retrieval */
	const long test = 10;
	BEnc::Integer *v = new BEnc::Integer(test);

	wxASSERT(v->IsOk());
	wxASSERT(test == v->getNum());

	/* integer copying */
	BEnc::Value *vc = BEnc::copy(v);
	wxASSERT(vc->IsOk());

	/* integer casting */
	BENC_SAFE_CAST(vc, vc_casted, Integer);
	wxASSERT((vc_casted->getNum() == v->getNum()) && (vc_casted->getNum() == test));

	wxString buff;
	v->Write(&buff);
	wxASSERT(buff == "i10e");
}

void BEncTests::RunStringTests()
{
	/* string storage and retrieval */
	const wxString test("hello world");
	BEnc::String *v = new BEnc::String(test);

	wxASSERT(v->IsOk());
	wxASSERT(test == v->getString());

	/* string copying */
	BEnc::Value *vc = BEnc::copy(v);
	wxASSERT(vc->IsOk());

	/* string casting */
	BENC_SAFE_CAST(vc, vc_casted, String);
	wxASSERT(vc_casted->getString() == test && v->getString() == test);

	wxString buff;
	v->Write(&buff);
	wxASSERT(buff == "11:hello world");
}

void BEncTests::RunListTests()
{
	/* list creation */
	BEnc::List *v = new BEnc::List();
	v->pushList(new BEnc::String("hello world"));
	v->pushList(new BEnc::Integer(10));

	/* list order consistency */
	wxASSERT(v->getListSize() == 2);
	BENC_SAFE_CAST(v->getListItem(0), lst_string, String);
	BENC_SAFE_CAST(v->getListItem(1), lst_integer, Integer);
	wxASSERT(lst_string->getString() == "hello world");
	wxASSERT(lst_integer->getNum() == 10);

	/* list encoding */
	wxString buff;
	v->Write(&buff);
	wxASSERT(buff == "l11:hello worldi10ee");
}

void BEncTests::RunDictionaryTests()
{
	BEnc::Dictionary *v = new BEnc::Dictionary();

	/* order is important here because dicts should always be
	 * Written in alphabetical key order */
	v->hash["b"] = new BEnc::Integer(10);
	v->hash["a"] = new BEnc::String("hello world");

	wxASSERT(!v->hasDictValue(""));
	wxASSERT(!v->hasDictValue("c"));
	wxASSERT(v->hasDictValue("b"));
	BENC_SAFE_CAST(v->getDictValue("a"), dct_string, String);
	BENC_SAFE_CAST(v->getDictValue("b"), dct_integer, Integer);
	wxASSERT(dct_string->getString() == "hello world");
	wxASSERT(dct_integer->getNum() == 10);

	/* dict encoding - note ordering here */
	wxString buff;
	v->Write(&buff);
	wxASSERT(buff == "d1:a11:hello world1:bi10ee");
}

void BEncTests::RunDecoderTests()
{
	/* some example encodings */
	wxString
		truncated_test("d5:helloi10e5:wo"),
		too_long_test("d5:helloi10e5:worldi5eeasfioasnfasfonasifn"),
		ok_test("d5:helloi10e5:world2:hie"),
		invalid_test("d5:hellox10e5:worldx5ee");

	/* and their expected decoder outputs */
	BEnc::Decoder dec;
	wxASSERT(BENC_TRUNCATED_STREAM == dec.TryDecode(truncated_test));
	wxASSERT(BENC_NOERROR_TOOMUCHDATA == dec.TryDecode(too_long_test));
	wxASSERT(BENC_NOERROR == dec.TryDecode(ok_test));
	wxASSERT(BENC_BAD_TOKEN == dec.TryDecode(invalid_test));

	/* now try to actually decode the valid encoding and
	 * test the result */
	dec.Decode(ok_test);
	BENC_SAFE_CAST(dec.getRoot(), root, Dictionary);
	wxASSERT(root->hasDictValue("hello"));
	wxASSERT(root->hasDictValue("world"));
	wxASSERT(!root->hasDictValue("test"));
	wxASSERT(root->getDictValue("hello")->type == BEncTypes::Integer);
	wxASSERT(root->getDictValue("world")->type == BEncTypes::String);
}
