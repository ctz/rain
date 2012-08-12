#include "GUI-PeersWidget.h"
#include "CryptoHelper.h"
#include "MessageCore.h"
#include "MeshCore.h"
#include "PeerID.h"
#include "ConnectionPool.h"
#include "RainSandboxApp.h"

RAIN::GUI::PeersWidget::PeersWidget(wxWindow *parent, wxWindowID id, RAIN::SandboxApp *app)
	: wxListView(parent, id, wxDefaultPosition, wxDefaultSize, wxLC_REPORT|wxLC_NO_SORT_HEADER)
{
	this->app = app;

	this->ClearAll();
	this->InsertColumn(0, wxT("Name"));
	this->InsertColumn(1, wxT("Address"));
	this->InsertColumn(2, wxT("Known?"));
	this->InsertColumn(3, wxT("Hash"));
	this->InsertColumn(4, wxT("Country"));
}

void RAIN::GUI::PeersWidget::Update()
{
	this->ClearAll();
	this->InsertColumn(0, wxT("Name"));
	this->InsertColumn(1, wxT("Address"));
	this->InsertColumn(2, wxT("Known?"));
	this->InsertColumn(3, wxT("Hash"));
	this->InsertColumn(4, wxT("Country"));

	size_t added = 0;

	for (size_t i = 0;; i++)
	{
		RAIN::PeerID *pid = this->app->mesh->GetPeer(i);

		if (pid)
		{
			added++;
			this->InsertItem(i, pid->GetCertificateField(NID_commonName));
			this->SetItem(i, 1, (pid->address.Length()) ? pid->address : wxT("<unknown>"));
			this->SetItem(i, 2, (pid->certificate) ? wxT("Yes") : wxT("No"));
			this->SetItem(i, 3, RAIN::CryptoHelper::HashToHex(pid->hash, true));
			this->SetItem(i, 4, pid->GetCertificateField(NID_countryName));
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
}
