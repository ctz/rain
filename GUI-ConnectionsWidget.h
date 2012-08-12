#ifndef _RAIN_GUI_CONNECTIONSWIDGET_H
#define _RAIN_GUI_CONNECTIONSWIDGET_H

#include <wx/wx.h>
#include <wx/listctrl.h>

namespace RAIN
{
	class SandboxApp;

	namespace GUI
	{
		class ConnectionsWidget : public wxListView
		{
		public:
			ConnectionsWidget(wxWindow *parent, wxWindowID id, RAIN::SandboxApp *app);

			void Update();

		private:
			RAIN::SandboxApp *app;
		};
	}
}

#endif//_RAIN_GUI_CONNECTIONSWIDGET_H
