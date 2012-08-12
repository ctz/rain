#include "GUI-NetworkMonitor.h"

#include "MessageCore.h"
#include "RainSandboxApp.h"
#include "ConnectionPool.h"

#include <wx/wx.h>
#include <wx/dc.h>
#include <wx/dcbuffer.h>

/* this switch controls drawing of black borders around
 * labels - neatness at the expense of 8 extra text renderings */
//#define USE_LABEL_OVERDRAW

using namespace RAIN;
extern RAIN::MessageCore *globalMessageCore;

BEGIN_EVENT_TABLE(RAIN::GUI::NetworkMonitor, wxWindow)
	EVT_PAINT(RAIN::GUI::NetworkMonitor::OnPaint)
	EVT_TIMER(ID_UPDATE, GUI::NetworkMonitor::OnTimer)
END_EVENT_TABLE()

GUI::NetworkMonitor::NetworkMonitor(wxWindow *parent, wxWindowID id)
 : wxWindow(parent, id, wxDefaultPosition, wxDefaultSize, wxSIMPLE_BORDER|wxNO_FULL_REPAINT_ON_RESIZE|wxCLIP_CHILDREN)
{
	refresh.SetOwner(this, ID_UPDATE);

	dataidx = 0;
	for (int i = 0; i < DATA_POINTS; i++)
	{
		in_data[i] = 0;
		out_data[i] = 0;	
	}

	refresh.Start(1000 / SAMPLES_PER_SEC);
	backcolour = wxSystemSettings::GetColour(wxSYS_COLOUR_WINDOW);
}

GUI::NetworkMonitor::~NetworkMonitor()
{

}

void GUI::NetworkMonitor::OnPaint(wxPaintEvent &e)
{
	wxBufferedPaintDC dc(this);
	dc.BeginDrawing();
	wxSize s = this->GetClientSize();

	if (s.y < 3 || s.x < 3)
		return;

	wxRect r(wxPoint(0, 0), s);
	int half = r.y + (r.height / 2);
	wxColour greenl(0x00, 0xAA, 0x00), greend(0x00, 0x88, 0x00);
	wxPen	minorLine(greend, 1, wxDOT),
			majorLine(greenl, 1, wxSOLID),
			incomingData(*wxBLUE, 1, wxSOLID),
			outgoingData(*wxRED, 1, wxSOLID);

	dc.SetPen(wxPen(backcolour, 1));
	dc.SetBrush(wxBrush(backcolour));
	dc.DrawRectangle(r.x, r.y, r.width, r.height);

	dc.SetPen(majorLine);
	dc.DrawLine(r.x, half, r.x + r.width, half);

	int max = 0;
	float scale = 0.0;

	int rx = 0;
	for (int x = dataidx; x > dataidx - DATA_POINTS; x--)
	{
		int idx = (x < 0) ? DATA_POINTS + x : x;
		if (max < in_data[idx])
			max = in_data[idx];

		if (max < out_data[idx])
			max = out_data[idx];

		rx++;

		if (rx > r.width)
			break;
	}

	if (max == 0)
		max = 1;

	scale = (float) half / (float) max;
	int gran = 10;

	if (max < 50)
		gran = 10;
	else if (max < 100)
		gran = 25;
	else if (max < 500)
		gran = 100;
	else if (max < 1000)
		gran = 250;
	else if (max < 5000)
		gran = 1000;
	else if (max < 10000)
		gran = 2500;

	float gran_scaled = gran * scale;
	if (gran_scaled < 5)
		gran_scaled = 20;

	dc.SetPen(minorLine);
	for (int y = half + gran_scaled; y < r.height; y += gran_scaled)
	{
		this->DrawLine(dc, r.x, y, r.x + r.width, y);	
	}

	for (int y = half - gran_scaled; y > r.y; y -= gran_scaled)
	{
		this->DrawLine(dc, r.x, y, r.x + r.width, y);	
	}

	rx = 0;
	for (int x = dataidx; x > dataidx - DATA_POINTS; x--)
	{
		int idx = (x < 0) ? DATA_POINTS + x : x;
		dc.SetPen(incomingData);
		this->DrawLine(dc, r.width - rx, half - 1, r.width - rx, half - 1 - in_data[idx] * scale);
		dc.SetPen(outgoingData);
		this->DrawLine(dc, r.width - rx, half + 1, r.width - rx, half + 1 + out_data[idx] * scale);
		rx++;

		if (rx > r.width)
			break;
	}

	dc.SetPen(wxNullPen);


	wxCoord tx,ty;
	dc.SetFont(wxFont(7, wxSWISS, wxNORMAL, wxNORMAL));
	dc.GetTextExtent("M", &tx, &ty);

	this->DrawText(dc, wxString("Incoming"), wxPoint(r.x + tx, r.y + 3), incomingData.GetColour());
	this->DrawText(dc, wxString("Outgoing"), wxPoint(r.x + tx, r.height - ty - 3), outgoingData.GetColour());
	this->DrawText(dc, wxString::Format("% 4d KB/s in", in_data[dataidx] * 4), wxPoint(r.x + tx * 10, r.height - ty - 3), outgoingData.GetColour());
	this->DrawText(dc, wxString::Format("% 4d KB/s out", out_data[dataidx] * 4), wxPoint(r.x + tx * 20, r.height - ty - 3), outgoingData.GetColour());
	this->DrawText(dc, wxString::Format("% 4d KB/s per grad.", gran), wxPoint(r.x + tx * 30, r.height - ty - 3), outgoingData.GetColour());

	dc.EndDrawing();
}

void GUI::NetworkMonitor::DrawText(wxBufferedPaintDC &dc, wxString text, wxPoint p, wxColour &c)
{
#ifdef USE_LABEL_OVERDRAW
	dc.SetTextForeground(*wxBLACK);
	dc.DrawText(text, p.x - 1, p.y);
	dc.DrawText(text, p.x + 1, p.y);
	dc.DrawText(text, p.x, p.y - 1);
	dc.DrawText(text, p.x, p.y + 1);
#endif //USE_LABEL_OVERDRAW

	dc.SetTextForeground(c);
	dc.DrawText(text, p.x, p.y);
}

void GUI::NetworkMonitor::DrawLine(wxBufferedPaintDC &dc, int x1, int y1, int x2, int y2)
{
	if ((x1 == x2 && y1 == y2) || (x1 == x2 && x1 == 0) || (y1 == y2 && y1 == 0))
		return;
	else 
		dc.DrawLine(x1, y1, x2, y2);
}

void GUI::NetworkMonitor::OnTimer(wxTimerEvent &e)
{
	dataidx++;
	if (dataidx == DATA_POINTS)
		dataidx = 0;
	wxPoint p = globalMessageCore->app->connections->GetBandwidthSample();
	in_data[dataidx] = p.x / (float) SAMPLES_PER_SEC;
	out_data[dataidx] = p.y / (float) SAMPLES_PER_SEC;
	Refresh(false);
}
