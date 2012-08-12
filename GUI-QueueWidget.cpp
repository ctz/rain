#include "GUI-QueueWidget.h"

#include "CryptoHelper.h"
#include "PieceQueue.h"
#include "MessageCore.h"
#include "RainSandboxApp.h"

BEGIN_EVENT_TABLE(RAIN::GUI::QueueWidget, wxPanel)
	EVT_BUTTON(ID_QW_START, RAIN::GUI::QueueWidget::StartQueue)
	EVT_BUTTON(ID_QW_PAUSE, RAIN::GUI::QueueWidget::PauseQueue)
	EVT_BUTTON(ID_QW_STOP, RAIN::GUI::QueueWidget::StopQueue)
	EVT_BUTTON(ID_QW_EMPTY, RAIN::GUI::QueueWidget::EmptyQueue)
END_EVENT_TABLE()

RAIN::GUI::QueueWidget::QueueWidget(wxWindow *parent, wxWindowID id, RAIN::SandboxApp *app)
	: wxPanel(parent, id)
{
	this->app = app;

	this->start = new wxButton(this, ID_QW_START, wxT("&Start"));
	this->pause = new wxButton(this, ID_QW_PAUSE, wxT("&Pause"));
	this->stop = new wxButton(this, ID_QW_STOP, wxT("S&top"));
	this->empty = new wxButton(this, ID_QW_EMPTY, wxT("&Empty"));

	this->list = new wxListView(this, -1, wxDefaultPosition, wxDefaultSize, wxLC_REPORT|wxLC_NO_SORT_HEADER);

	wxImageList *im = new wxImageList(16, 16);
	
#ifdef __WXMSW__
	im->Add(wxBitmap("IDB_PIECE_DOWNLOAD", wxBITMAP_TYPE_BMP_RESOURCE), wxColour(0xFF, 0x00, 0xFF));
	im->Add(wxBitmap("IDB_PIECE_UPLOAD", wxBITMAP_TYPE_BMP_RESOURCE), wxColour(0xFF, 0x00, 0xFF));
#endif
	
	this->list->AssignImageList(im, wxIMAGE_LIST_SMALL);

	this->list->ClearAll();
	this->list->InsertColumn(0, wxT(""));
	this->list->InsertColumn(1, wxT("Piece Hash"));
	this->list->InsertColumn(2, wxT("Size"));
	this->list->InsertColumn(3, wxT("Status"));

	wxBoxSizer* buttonSizer = new wxBoxSizer(wxHORIZONTAL);
	buttonSizer->Add(this->start, 0, 0, 0);
	buttonSizer->Add(this->pause, 0, 0, 0);
	buttonSizer->Add(this->stop, 0, 0, 0);
	buttonSizer->Add(new wxPanel(this, -1), 1, wxEXPAND, 0);
	buttonSizer->Add(this->empty, 0, 0, 0);

	wxBoxSizer* sizer = new wxBoxSizer(wxVERTICAL);
	sizer->Add(this->list, 1, wxEXPAND, 5);
	sizer->Add(buttonSizer, 0, wxTOP|wxEXPAND, 5);
	this->SetAutoLayout(true);
	this->SetSizer(sizer);
	sizer->Fit(this);
	sizer->SetSizeHints(this);
}

void RAIN::GUI::QueueWidget::Update()
{
	this->list->Freeze();
	this->start->Enable(true);
	this->stop->Enable(true);
	this->pause->Enable(true);
	this->empty->Enable(false);

	if (this->app->pieceQueue->queue.size() > 0)
	{
		switch (this->app->pieceQueue->state)
		{
		case RAIN::QUEUE_PAUSED:
			this->pause->Enable(false);
			break;
		case RAIN::QUEUE_STOPPED:
			this->stop->Enable(false);
			this->pause->Enable(false);
			break;
		case RAIN::QUEUE_RUNNING:
			this->start->Enable(false);
			break;
		}
	} else {
		this->start->Enable(false);
		this->stop->Enable(false);
		this->pause->Enable(false);
	}

	bool insertMode = false;

	if (this->app->pieceQueue->queue.size() != this->list->GetItemCount())
	{
		this->list->ClearAll();
		this->list->InsertColumn(0, wxT(""));
		this->list->InsertColumn(1, wxT("Piece Hash"));
		this->list->InsertColumn(2, wxT("Size"));
		this->list->InsertColumn(3, wxT("Status"));
		insertMode = true;
	}

	if (this->app->pieceQueue->queue.size() == 0)
		return;
	this->empty->Enable(true);

	for (size_t i = 0; i < this->app->pieceQueue->queue.size(); i++)
	{
		if (this->app->pieceQueue->queue.at(i))
		{
			if (insertMode)
				this->list->InsertItem(i, this->app->pieceQueue->queue.at(i)->direction);
			else
				this->list->SetItem(i, 0, "", this->app->pieceQueue->queue.at(i)->direction);

			this->list->SetItem(i, 1, RAIN::CryptoHelper::HashToHex(this->app->pieceQueue->queue.at(i)->hash, true));
			this->list->SetItem(i, 2, wxString::Format("%ld", this->app->pieceQueue->queue.at(i)->pieceSize));
			this->list->SetItem(i, 3, this->app->pieceQueue->queue.at(i)->status);

			if (this->app->pieceQueue->InCurrentQueue(this->app->pieceQueue->queue.at(i)))
				this->list->SetItemBackgroundColour(i, wxColour(0x88, 0xFF, 0x88));
			else
				this->list->SetItemBackgroundColour(i, wxSystemSettings::GetColour(wxSYS_COLOUR_WINDOW));
		}
	}

	this->list->SetColumnWidth(0, -1);
	this->list->SetColumnWidth(1, -1);
	this->list->SetColumnWidth(2, -1);
	this->list->SetColumnWidth(3, -1);
	this->list->Thaw();
}

void RAIN::GUI::QueueWidget::StartQueue(wxCommandEvent &e)
{
	this->app->pieceQueue->state = RAIN::QUEUE_RUNNING;
	this->Update();
}

void RAIN::GUI::QueueWidget::PauseQueue(wxCommandEvent &e)
{
	this->app->pieceQueue->state = RAIN::QUEUE_PAUSED;
	this->Update();
}

void RAIN::GUI::QueueWidget::StopQueue(wxCommandEvent &e)
{
	this->app->pieceQueue->state = RAIN::QUEUE_STOPPED;
	this->Update();
}

void RAIN::GUI::QueueWidget::EmptyQueue(wxCommandEvent &e)
{
	this->app->pieceQueue->EmptyQueue();
	this->Update();
}
