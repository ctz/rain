#include "BDecoder.h"
#include "BValue.h"

#include <map>

using namespace RAIN;
using namespace RAIN::BEnc;

/* copy */
Value * RAIN::BEnc::copy(Value *v)
{
	Value *ret = NULL;

	switch (v->type)
	{
		case BENC_VAL_DICT:
			ret = new Dictionary((Dictionary *) v);
			break;
		case BENC_VAL_INT:
			ret = new Integer((Integer *) v);
			break;
		case BENC_VAL_STRING:
			ret = new String((String *) v);
			break;
		case BENC_VAL_LIST:
			ret = new List((List *) v);
			break;
		default:
			ret = NULL;
			break;
	}

	return ret;
}

/* base */
Value::Value(int type)
{
	this->type = type;
}

/* benc string */
String::String(const wxString &str)
: Value(BENC_VAL_STRING)
{
	this->str = str;
}

String::String(String *that)
: Value(BENC_VAL_STRING)
{
	this->str = that->str;
}

String::String()
: Value(BENC_VAL_STRING)
{
}

String::~String()
{
}

int String::Read(wxString &ins)
{
	wxString len_s;
	unsigned long len = 0;
	size_t p = 0;

	while (ins.GetChar(p) >= '0' && ins.GetChar(p) <= '9' && ins.Length() > p)
	{
		len_s.append(1, ins.GetChar(p));
		p++;
	}

	if (p == 0)
		return BENC_BAD_TOKEN;

	if (p >= ins.Length())
		return BENC_TRUNCATED_STREAM;

	if (ins.GetChar(p) != ':')
		return BENC_TRUNCATED_STREAM;

	p++; // : sep

	len_s.ToULong(&len);
		
	if (ins.Length() < p + len)
		return BENC_TRUNCATED_STREAM;

	this->str = ins.Mid(p, len);

	ins = ins.Mid(p + len);

	if (ins.Length() == 0)
		return BENC_NOERROR;
	else
		return BENC_NOERROR_TOOMUCHDATA;
}

int String::Write(wxString *outs)
{
	outs->Append(wxString::Format("%d:", this->str.Length()));
	outs->Append(this->str);
	return 1;
}

void String::Debug()
{
	wxLogVerbose("BENC_VAL_STRING(%d): \"%s\"", this->str.Length(), this->str.c_str());
}

void String::toXML(wxXmlNode *n)
{
	try 
	{
		n->AddChild(new wxXmlNode(wxXML_TEXT_NODE, "", this->str));
	} catch (...) {
		wxLogVerbose("Exception at %s(%d)", __FILE__, __LINE__);
	}
}

wxString String::getString()
{
	return this->str;
}

/* benc list */
List::List()
: Value(BENC_VAL_LIST)
{
}

List::List(List *that)
: Value(BENC_VAL_LIST)
{
	wxString tmp;
	that->Write(&tmp);
	this->Read(tmp);
}

List::~List()
{
	while (this->list.size() > 0)
	{
		delete this->list.at(0);
		this->list.erase(this->list.begin());
	}
}

int List::Read(wxString &ins)
{
	Value *v = NULL;
	int ret = 1;

	ins = ins.Mid(1); // l delim

	while (ins.GetChar(0) != 'e')
	{
		switch (ins.GetChar(0))
		{
		case 'd':
			v = new Dictionary();
			ret = v->Read(ins);
			if (ret < BENC_NOERROR)
				return ret;
			break;
		case 'l':
			v = new List();
			ret = v->Read(ins);
			if (ret < BENC_NOERROR)
				return ret;
			break;
		case '0':	case '1':	case '2':	case '3':	case '4':
		case '5':	case '6':	case '7':	case '8':	case '9':
			v = new String();
			ret = v->Read(ins);
			if (ret < BENC_NOERROR)
				return ret;
			break;
		case 'i':
			v = new Integer();
			ret = v->Read(ins);
			if (ret < BENC_NOERROR)
				return ret;
			break;
		default:
			wxLogVerbose("BENC_BAD_TOKEN in list read: %c", ins.GetChar(0));
			return BENC_BAD_TOKEN;
		}

		this->list.push_back(v);
	}

	if (ins.Length() == 0)
		return BENC_TRUNCATED_STREAM;

	ins = ins.Mid(1);

	if (ins.Length() == 0)
		return BENC_NOERROR;
	else
		return BENC_NOERROR_TOOMUCHDATA;
}

int List::Write(wxString *outs)
{
	outs->Append('l');
	for (size_t i = 0; i < this->list.size(); i++)
	{
		this->list.at(i)->Write(outs);
	}
	outs->Append('e');
	return BENC_NOERROR;
}

void List::Debug()
{
	wxLogVerbose("BENC_VAL_LIST(%d) begin:", this->list.size());
	for (size_t i = 0; i < this->list.size(); i++)
	{
		wxLogVerbose("%d => \t", i);
		this->list.at(i)->Debug();
	}
	wxLogVerbose("BENC_VAL_LIST end.");
}

void List::toXML(wxXmlNode *n)
{
	try {
		for (size_t i = 0; i < this->list.size(); i++)
		{
			wxXmlNode *k = new wxXmlNode(wxXML_ELEMENT_NODE, "list-entry");
			n->AddChild(k);
			k->AddProperty("index", wxString::Format("%d", i));
			this->list.at(i)->toXML(k);
		}
	} catch (...) {
		wxLogVerbose("Exception at %s(%d)", __FILE__, __LINE__);
	}
}

size_t List::getListSize()
{
	return this->list.size();
}

RAIN::BEnc::Value * List::getListItem(size_t i)
{
	return this->list.at(i);
}

void List::pushList(RAIN::BEnc::Value *v)
{
	this->list.push_back(v);
}

void List::removeByString(const wxString &str)
{
	std::vector<BEnc::Value*>::iterator it = list.begin();

	while (it != list.end())
	{
		if ((*it)->type == BEncTypes::String)
		{
			BENC_SAFE_CAST(*it, string, String);
			if (string->getString() == str)
			{
				list.erase(it);
				return;
			}
		}

		it++;
	}
}

/* benc dict */
Dictionary::Dictionary()
: Value(BENC_VAL_DICT)
{
}

Dictionary::Dictionary(Dictionary *that)
: Value(BENC_VAL_DICT)
{
	wxString tmp;
	that->Write(&tmp);
	this->Read(tmp);
}

Dictionary::~Dictionary()
{
	std::map<wxString, RAIN::BEnc::Value*, BEnc::Cmp>::iterator it = this->hash.begin();

	while (this->hash.size() > 0)
	{
		delete this->hash.begin()->second;
		this->hash.erase(this->hash.begin());
	}
}

int Dictionary::Read(wxString &ins)
{
	Value *v = NULL;
	String *key = NULL;
	int ret = 0;

	ins = ins.Mid(1); // l delim

	while (ins.GetChar(0) != 'e')
	{
		key = new String();
		key->Read(ins);

		switch (ins.GetChar(0))
		{
		case 'd':
			v = new Dictionary();
			ret = v->Read(ins);

			if (ret < BENC_NOERROR)
				return ret;
			break;
		case 'l':
			v = new List();
			ret = v->Read(ins);
			
			if (ret < BENC_NOERROR)
				return ret;
			break;
		case '0':	case '1':	case '2':	case '3':	case '4':
		case '5':	case '6':	case '7':	case '8':	case '9':
			v = new String();
			ret = v->Read(ins);
			
			if (ret < BENC_NOERROR)
				return ret;
			break;
		case 'i':
			v = new Integer();
			ret = v->Read(ins);
			
			if (ret < BENC_NOERROR)
				return ret;
			break;
		default:
			wxLogVerbose("BENC_BAD_TOKEN in dict read: %c", ins.GetChar(0));
			return BENC_BAD_TOKEN;
		}

		this->hash[key->str] = v;
		delete key;
	}

	if (ins.Length() == 0)
		return BENC_TRUNCATED_STREAM;

	ins = ins.Mid(1);

	if (ins.Length() == 0)
		return BENC_NOERROR;
	else
		return BENC_NOERROR_TOOMUCHDATA;
}

int Dictionary::Write(wxString *outs)
{
	outs->Append('d');

	std::map<wxString, RAIN::BEnc::Value*, BEnc::Cmp>::iterator it = this->hash.begin();

	while (it != this->hash.end())
	{
		String *key = new String(it->first);
		key->Write(outs);
		delete key;

		it->second->Write(outs);

		it++;
	}

	outs->Append('e');
	return 1;
}

void Dictionary::Debug()
{
	std::map<wxString, RAIN::BEnc::Value*, BEnc::Cmp>::iterator it = this->hash.begin();
		
	wxLogVerbose("BENC_VAL_DICT begin:");
	
	while (it != this->hash.end())
	{
		wxLogVerbose("\t '%s' =>\t", it->first.c_str());
		it->second->Debug();
		it++;
	}

	wxLogVerbose("BENC_VAL_DICT end.");
}

void Dictionary::toXML(wxXmlNode *n)
{
	try
	{
		std::map<wxString, RAIN::BEnc::Value*, BEnc::Cmp>::iterator it = this->hash.begin();
	
		while (it != this->hash.end())
		{
			wxXmlNode *k = new wxXmlNode(wxXML_ELEMENT_NODE, "hash-entry");
			n->AddChild(k);
			k->AddProperty("key", it->first);

			BEnc::Value *value = it->second;
			value->toXML(k);
			it++;
		}
	} catch (...) {
		wxLogVerbose("Exception at %s(%d)", __FILE__, __LINE__);
	}
}

bool Dictionary::hasDictValue(const wxString &key)
{
	return (this->getDictValue(key) != NULL);
}

Value * Dictionary::getDictValue(const wxString &key)
{
	std::map<wxString, RAIN::BEnc::Value*, BEnc::Cmp>::iterator it = this->hash.begin();
	
	while (it != this->hash.end())
	{
		if (it->first.CmpNoCase(key) == 0)
			return it->second;
		it++;
	}

	return NULL;
}

wxArrayString Dictionary::getArrayString()
{
	wxArrayString ret;
	std::map<wxString, RAIN::BEnc::Value*, BEnc::Cmp>::iterator it = this->hash.begin();
	
	while (it != this->hash.end())
	{
		ret.Add(it->first);
		it++;
	}

	return ret;
}

/* benc integer */
Integer::Integer(long i)
: Value(BENC_VAL_INT)
{
	this->num = i;
}

Integer::Integer(Integer *that)
: Value(BENC_VAL_INT)
{
	this->num = that->num;
}

Integer::Integer()
: Value(BENC_VAL_INT)
{
	this->num = 0;
}

Integer::~Integer() {}

int Integer::Read(wxString &ins)
{
	wxString int_s;
	size_t p = 0;

	if (ins.GetChar(0) != 'i')
	{
		wxLogVerbose("BENC_BAD_TOKEN in integer read: %c", ins.GetChar(0));
		return BENC_BAD_TOKEN;
	}

	p++; // i delim

	while (p < ins.Length() && (ins.GetChar(p) >= '0' && ins.GetChar(p) <= '9'))
	{
		int_s.append(1, ins.GetChar(p));
		p++;
	}

	p++; // e delim

	if (p >= ins.Length())
		return BENC_TRUNCATED_STREAM;

	int_s.ToLong(&this->num);

	ins = ins.Mid(p);
	
	if (ins.Length() == 0)
		return BENC_NOERROR;
	else
		return BENC_NOERROR_TOOMUCHDATA;
}

int Integer::Write(wxString *outs)
{
	outs->Append(wxString::Format("i%de", this->num));
	return 1;
}

void Integer::Debug()
{
	wxLogVerbose("BENC_VAL_INT: %d", this->num);
}

void Integer::toXML(wxXmlNode *n)
{
	try
	{
		n->AddChild(new wxXmlNode(wxXML_TEXT_NODE, "", wxString::Format("Integer: %d", this->num)));
	} catch (...) {
		wxLogVerbose("Exception at %s(%d)", __FILE__, __LINE__);
	}
}

long Integer::getNum()
{
	return this->num;
}
