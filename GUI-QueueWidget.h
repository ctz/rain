#ifndef _RAIN_GUI_QUEUEWIDGET_H
#define _RAIN_GUI_QUEUEWIDGET_H

#include <wx/wx.h>
#include <wx/listctrl.h>
#include <wx/imaglist.h>
#include <wx/bitmap.h>

enum
{
	ID_QW_START, 
	ID_QW_PAUSE,
	ID_QW_STOP,
	
	ID_QW_EMPTY
};

namespace RAIN
{
	class SandboxApp;

	namespace GUI
	{
		class QueueWidget : public wxPanel
		{
		public:
			QueueWidget(wxWindow *parent, wxWindowID id, RAIN::SandboxApp *app);

			void StartQueue(wxCommandEvent &e);
			void PauseQueue(wxCommandEvent &e);
			void StopQueue(wxCommandEvent &e);
			void EmptyQueue(wxCommandEvent &e);

			void Update();
			
			DECLARE_EVENT_TABLE();

		private:
			RAIN::SandboxApp *app;

			wxListView *list;
			wxButton *start, *pause, *stop, *empty;
		};
	}
}

#endif//_RAIN_GUI_PEERSWIDGET_H
