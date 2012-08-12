#ifndef __GUI_NETWORKMONITOR
#define __GUI_NETWORKMONITOR

#include <wx/wx.h>
#include <wx/dc.h>
#include <wx/dcbuffer.h>

#define DATA_POINTS 2048
#define SAMPLES_PER_SEC 1

enum
{
	ID_UPDATE
};

namespace RAIN
{
	namespace GUI
	{
		class NetworkMonitor : public wxWindow
		{
		public:
			NetworkMonitor(wxWindow *parent, wxWindowID id);
			~NetworkMonitor();

			void OnPaint(wxPaintEvent &e);
			void OnTimer(wxTimerEvent &e);

			DECLARE_EVENT_TABLE();

		private:
			unsigned int in_data[DATA_POINTS];
			unsigned int out_data[DATA_POINTS];
			int dataidx;
			wxTimer refresh;
			wxColour backcolour;

			void DrawText(wxBufferedPaintDC &dc, wxString text, wxPoint p, wxColour &c);
			void DrawLine(wxBufferedPaintDC &dc, int x1, int y1, int x2, int y2);
		};
	}
}

#endif//__GUI_NETWORKMONITOR
