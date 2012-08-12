#ifndef __BVALUE_H
#define __BVALUE_H

#include <wx/wx.h>
#include <wx/xml/xml.h>
#include <map>
#include <vector>

enum
{
	BENC_VAL_STRING,
	BENC_VAL_INT,
	BENC_VAL_LIST,
	BENC_VAL_DICT,
};

namespace BEncTypes
{
	enum
	{
		String,
		Integer,
		List,
		Dictionary
	};
}

#define BENC_SAFE_CAST(ptr, name, btype) \
	RAIN::BEnc::btype *name = NULL;\
	if ((ptr) == NULL)\
	wxLogVerbose("%s(%d): Tried to cast " #btype " from NULL!", __FILE__, __LINE__);\
	else\
		wxASSERT_MSG((ptr)->type == BEncTypes::btype, "Bad cast to " #btype "!");\
	if ((ptr) != NULL && (ptr)->type == BEncTypes::btype)\
		name = static_cast<RAIN::BEnc::btype *> (ptr);

namespace RAIN
{
	namespace BEnc
	{
		class Value;

		class Cmp
		{
		public:
			bool operator()(const wxString& a, const wxString& b) const
			{
				return (a.Cmp(b.c_str()) < 0);
			}
		}; 

		class Value
		{
		public:
			Value(int type);
			virtual ~Value() {};
			
			virtual int Read(wxString &ins) = 0;
			virtual int Write(wxString *outs) = 0;

			virtual void Debug() = 0;
			virtual void toXML(wxXmlNode *n) = 0;

			bool IsOk() {return (type >= BENC_VAL_STRING && type <= BENC_VAL_DICT);}

			int type;

			/* this is hack for route discovery */
			wxString key;
		};

		class String : public Value
		{
		public:
			String(String *that);
			String(const wxString& str);
			String();
			~String();

			int Read(wxString &ins);
			int Write(wxString *outs);

			void Debug();
			void toXML(wxXmlNode *n);

			wxString getString();

			wxString str;
		};

		class List : public Value
		{
		public:
			List(List *that);
			List();
			~List();

			int Read(wxString &ins);
			int Write(wxString *outs);

			void Debug();
			void toXML(wxXmlNode *n);

			size_t getListSize();
			Value * getListItem(size_t i);
			void pushList(Value *v);
			void removeByString(const wxString &str);

			std::vector<BEnc::Value*> list;
		};

		class Dictionary : public Value
		{
		public:
			Dictionary(Dictionary *that);
			Dictionary();
			~Dictionary();

			int Read(wxString &ins);
			int Write(wxString *outs);

			void Debug();
			void toXML(wxXmlNode *n);

			bool hasDictValue(const wxString& key);
			Value * getDictValue(const wxString& key);
			wxArrayString getArrayString();

			std::map <wxString, BEnc::Value*, Cmp> hash;
		};

		class Integer : public Value
		{
		public:
			Integer(Integer *that);
			Integer(long i);
			Integer();
			~Integer();

			int Read(wxString &ins);
			int Write(wxString *outs);

			void Debug();
			void toXML(wxXmlNode *n);

			long getNum();
			
			long num;
		};

		Value * copy(Value *v);
	}
}

#endif//__BVALUE_H
