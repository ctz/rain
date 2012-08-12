#include "GUI-ConnectionsWidget.h"
#include "CryptoHelper.h"
#include "Connection.h"
#include "PeerID.h"
#include "ConnectionPool.h"
#include "RainSandboxApp.h"

RAIN::GUI::ConnectionsWidget::ConnectionsWidget(wxWindow *parent, wxWindowID id, RAIN::SandboxApp *app)
	: wxListView(parent, id, wxDefaultPosition, wxDefaultSize, wxLC_REPORT|wxLC_NO_SORT_HEADER)
{
	this->app = app;

	this->ClearAll();
	this->InsertColumn(0, wxT("Connected?"));
	this->InsertColumn(1, wxT("Name"));
	this->InsertColumn(2, wxT("Address"));
	this->InsertColumn(3, wxT("Bytes In"));
	this->InsertColumn(4, wxT("Bytes Out"));
	this->InsertColumn(5, wxT("Hash"));
}

void RAIN::GUI::ConnectionsWidget::Update()
{
	this->ClearAll();
	this->InsertColumn(0, wxT("Connected?"));
	this->InsertColumn(1, wxT("Name"));
	this->InsertColumn(2, wxT("Address"));
	this->InsertColumn(3, wxT("Bytes In"));
	this->InsertColumn(4, wxT("Bytes Out"));
	this->InsertColumn(5, wxT("Hash"));

	size_t added = 0;

	for (size_t i = 0;; i++)
	{
		RAIN::Connection *c = this->app->connections->Get(i);

		if (c)
		{
			added++;
			this->InsertItem(i, (c->isConnected) ? wxT("Yes") : wxT("No"));
			this->SetItem(i, 1, c->pid->GetCertificateField(NID_commonName));
			this->SetItem(i, 2, c->pid->address);
			this->SetItem(i, 3, wxString::Format("%d bytes", c->GetBytesRecv()));
			this->SetItem(i, 4, wxString::Format("%d bytes", c->GetBytesSent()));
			this->SetItem(i, 5, RAIN::CryptoHelper::HashToHex(c->pid->hash, true));
		} else {
			break;
		}
	}

	if (!added)
		return;

	this->SetColumnWidth(0, -1);
	this->SetColumnWidth(1, -1);
	this->SetColumnWidth(2, -1);
	this->SetColumnWidth(3, -1);
	this->SetColumnWidth(4, -1);
	this->SetColumnWidth(5, -1);
}
