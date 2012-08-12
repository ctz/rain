#ifndef __GUI_PREFERENCES_H
#define __GUI_PREFERENCES_H

#include <wx/wx.h>

enum
{
	ID_PR_DISKSLIDER,

	ID_PR_CREDBROWSE,
	ID_PR_CREDOPEN,

	ID_PR_MANIBROWSE,
	ID_PR_MANIOPEN,

	ID_PR_PIECEBROWSE,
	ID_PR_PIECEOPEN,

	ID_PR_OUTPUTBROWSE,
	ID_PR_OUTPUTOPEN,

	ID_PR_PC_WAIT,
	ID_PR_CONNCACHE,
	ID_PR_SMALLICON,

	ID_PR_OK,
	ID_PR_CANCEL,
};

namespace RAIN
{
	namespace GUI
	{
		class Preferences : public wxDialog
		{
		public:
			Preferences(wxWindow *parent);

			void ImportSettings();
			void ExportSettings();
			void UpdateCacheSlider();
			void UpdateCurrentlyUsed();

			void OnOK(wxCommandEvent &e);
			void OnCancel(wxCommandEvent &e);

			void OnSlide(wxScrollEvent &e);

			void OnBrowse(wxCommandEvent &e);
			void OnOpen(wxCommandEvent &e);

			DECLARE_EVENT_TABLE();

		private:
			void set_properties();
			void do_layout();
			long currVolTotal, currVolFree;

		protected:
			wxStaticBox* OptionsSizer_staticbox;
			wxStaticBox* LocationsSizer_staticbox;
			wxStaticBox* DiskSizer_staticbox;
			wxStaticText* DiskLabel, *DiskLabel2;
			wxSlider* DiskSlider;
			wxStaticText* CredLabel;
			wxTextCtrl* CredText;
			wxButton* CredBrowse;
			wxButton* CredOpen;
			wxStaticText* ManiLabel;
			wxTextCtrl* ManiText;
			wxButton* ManiBrowse;
			wxButton* ManiOpen;
			wxStaticText* PieceLabel;
			wxTextCtrl* PieceText;
			wxButton* PieceBrowse;
			wxButton* PieceOpen;
			wxStaticText* OutputLabel;
			wxTextCtrl* OutputText;
			wxButton* OutputBrowse;
			wxButton* OutputOpen;
			wxCheckBox* PieceQueueCheck;
			wxCheckBox* ConnCacheCheck;
			wxCheckBox* SmallIconsCheck;
			wxPanel* ButtonSpacer;
			wxButton* CancelButton;
			wxButton* OkButton;
		};
	}
}

#endif//__GUI_PREFERENCES_H
