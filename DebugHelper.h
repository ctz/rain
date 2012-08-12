#ifndef __DEBUGHELPER_H
#define __DEBUGHELPER_H

#include <wx/wx.h>
#include <wx/xml/xml.h>
#include <wx/datetime.h>

namespace RAIN
{
	class Message;

	class DebugHelper
	{
	public:
		DebugHelper();
		~DebugHelper();

		void LogMessage(RAIN::Message *msg);
		void LogState(RAIN::Message *msg);

	private:
		wxString        filename;
		wxXmlDocument	*doc;
		wxXmlNode		*mEle;
	};
}

#endif//__DEBUGHELPER_H
