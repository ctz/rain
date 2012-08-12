#ifndef __GUI_HELPERS_H
#define __GUI_HELPERS_H

#include <wx/wx.h>

namespace RAIN
{
	namespace GUI
	{
		void OpenInShell(const wxString& dir);
		wxString FormatFileSize(size_t i);
	}
}

#endif//__GUI__HELPERS_H
