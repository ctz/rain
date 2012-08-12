#ifndef __GUI_ABOUTDIALOG_H
#define __GUI_ABOUTDIALOG_H

#include <wx/wx.h>
#include <wx/image.h>
#include <wx/dc.h>
#include <wx/dcbuffer.h>

namespace RAIN
{
	namespace GUI
	{
		class StaticImage : public wxWindow
		{
		public:
			StaticImage(wxWindow *w, const wxImage& img)
				: wxWindow(w, -1), image(img)
			{
				SetSize(wxSize(image.GetWidth(), image.GetHeight()));
			}

			DECLARE_EVENT_TABLE();

			void OnPaint(wxPaintEvent &e)
			{
				wxBufferedPaintDC dc(this);
				
				dc.BeginDrawing();

				unsigned char *alpha = image.GetAlpha();
				unsigned char *data = image.GetData();

				float a, inv_a;
				wxASSERT(alpha);
				wxSize s(image.GetWidth(), image.GetHeight());
				wxPen p(*wxBLACK, 1, wxSOLID);
				wxColour base = wxSystemSettings::GetColour(wxSYS_COLOUR_3DFACE);
				
				unsigned char rr, rg, rb, ar, ag, ab;
				rr = base.Red();
				rg = base.Green();
				rb = base.Blue();
				int offset;

				for (int iy = 0; iy < s.y; iy++)
				{
					for (int ix = 0; ix < s.x; ix++)
					{
						offset = iy * s.x + ix;
						a = (float) alpha[offset] / 255;
						inv_a = 1.0 - a;

						ar = data[offset * 3 + 0];
						ag = data[offset * 3 + 1];
						ab = data[offset * 3 + 2];

						p.SetColour(wxColour(rr * inv_a + ar * a, rg * inv_a + ag * a, rb * inv_a + ab * a));
						dc.SetPen(p);
						dc.DrawPoint(ix, iy);
					}
				}
				dc.EndDrawing();
			}

		private:
			wxImage image;
		};

		class AboutDialog: public wxDialog
		{
		public:
			// begin wxGlade: AboutDialog::ids
			// end wxGlade
			AboutDialog(wxWindow* parent);

			void OnOK(wxCommandEvent &e);

			DECLARE_EVENT_TABLE();

		private:
			// begin wxGlade: AboutDialog::methods
			void set_properties();
			void do_layout();
			// end wxGlade

		protected:
			// begin wxGlade: AboutDialog::attributes
			StaticImage* RAINBitmap;
			wxStaticText* VersionLabel;
			wxStaticText* VersionText;
			wxStaticText* DateLabel;
			wxStaticText* DateText;
			wxStaticText* AuthorLabel;
			wxStaticText* AuthorText;
			wxStaticText* OSLabel;
			wxStaticText* OSText;
			wxStaticBitmap* OpenSSLBitmap;
			wxStaticBitmap* WXBitmap;
			wxButton* OKButton;
			// end wxGlade
		};
	}
}

#endif//__GUI_ABOUTDIALOG_H
