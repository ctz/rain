#ifndef _RAIN_GUI_PEERSWIDGET_H
#define _RAIN_GUI_PEERSWIDGET_H

#include <wx/wx.h>
#include <wx/listctrl.h>

namespace RAIN
{
	class SandboxApp;

	namespace GUI
	{
		class PeersWidget : public wxListView
		{
		public:
			PeersWidget(wxWindow *parent, wxWindowID id, RAIN::SandboxApp *app);

			void Update();

		private:
			RAIN::SandboxApp *app;
		};
	}
}

#endif//_RAIN_GUI_PEERSWIDGET_H
