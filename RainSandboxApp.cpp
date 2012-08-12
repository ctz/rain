#include "RainSandboxApp.h"

#include "DebugHelper.h"
#include "Message.h"
#include "CredentialManager.h"
#include "Connection.h"
#include "ConnectionPool.h"
#include "MessageCore.h"
#include "MeshCore.h"
#include "PeerID.h"
#include "BValue.h"
#include "RoutingManager.h"
#include "ManifestManager.h"
#include "NetworkServices.h"
#include "PieceManager.h"
#include "PieceQueue.h"
#include "GUI-ChatMain.h"
#include "SystemIDs.h"

#include "TestingFramework.h"

#ifdef WIN32
#include <crtdbg.h>
#endif//WIN32

#include <wx/debugrpt.h>

IMPLEMENT_APP(RAIN::SandboxApp)

BEGIN_EVENT_TABLE(RAIN::SandboxApp, wxApp)
	EVT_TIMER(ID_SBA_CORETIMER, RAIN::SandboxApp::PollCore)
END_EVENT_TABLE()

extern RAIN::CredentialManager *globalCredentialManager;
extern RAIN::MessageCore *globalMessageCore;
extern RAIN::DebugHelper *debug;

bool RAIN::SandboxApp::OnInit()
{
	/* run tests 
	RAIN::RunAllTests();
	return false;*/

	/* let us catch and do something with fatals */
	::wxHandleFatalExceptions(true);

	this->SetExitOnFrameDelete(true);

	wxConfig *cfg = new wxConfig(wxT("RAIN"));
	wxConfigBase::Set(cfg);

	FILE *fdebug = fopen("debug-log.txt", "a");

	if (fdebug)
	{
		fwrite("-------\n", 8, 1, fdebug);
		wxLog::SetActiveTarget(new wxLogStderr(fdebug));
		wxLog::SetVerbose();
	}

	wxInitAllImageHandlers();

	debug = new RAIN::DebugHelper();

	globalCredentialManager = new RAIN::CredentialManager(true);
	this->router = new RAIN::RoutingManager();

	this->msgCore = new RAIN::MessageCore(this);
	globalMessageCore = this->msgCore;

	this->manifest = new ManifestManager();
	this->mesh = new RAIN::MeshCore(globalMessageCore);
	this->pieceQueue = new RAIN::PieceQueue(this);
	this->pieces = new RAIN::PieceManager();
	this->networkServices = new RAIN::NetworkServices();

	this->gui = new RAIN::GUI::ChatMain(this);
	this->SetTopWindow(this->gui);

	/* register each component with the message core */
	this->msgCore->RegisterMessageHandler(this->gui);
	this->msgCore->RegisterMessageHandler(this->manifest);
	this->msgCore->RegisterMessageHandler(this->mesh);
	this->msgCore->RegisterMessageHandler(this->pieces);
	this->msgCore->RegisterMessageHandler(this->pieceQueue);
	this->msgCore->RegisterMessageHandler(this->networkServices);

	/* set up the connections pool */
	this->connections = new RAIN::ConnectionPool(this);

	this->coreTimer = new wxTimer(this, ID_SBA_CORETIMER);
	this->coreTimer->Start(250);

	this->stop = new wxSemaphore(0, 1);
	this->Create();
	this->GetThread()->Run();

	this->gui->Show();
			
	return true;
}

void RAIN::SandboxApp::OnFatalException()
{
	wxDebugReport report;
	wxDebugReportPreviewStd preview;

	report.AddAll();

	if (report.IsOk() && preview.Show(report))
		report.Process();
}

int RAIN::SandboxApp::OnExit()
{
	this->coreTimer->Stop();

	this->stop->Post();
	while (this->GetThread()->IsRunning())
	{
		wxThread::Sleep(50);
	}

	if (debug)
		delete debug;

	delete this->coreTimer;
	delete this->mesh;
	delete this->manifest;
	delete this->pieces;
	delete this->pieceQueue;
	delete this->networkServices;
	delete this->connections;
	delete this->msgCore;
	delete globalCredentialManager;

	wxConfig *cfg = (wxConfig*) wxConfigBase::Get();
	delete cfg;

	return 0;
}

void * RAIN::SandboxApp::Entry()
{
	wxThread::Sleep(500);

	while (true)
	{
		if (wxSEMA_TIMEOUT != this->stop->WaitTimeout(50))
			return NULL;

		for (int i = 0; i < this->connections->pool.size(); i++)
		{
			if (this->connections->pool.at(i) && this->connections->pool.at(i)->isConnected)
			{
				this->connections->pool.at(i)->Poll();
			}
		}
	}

	return NULL;
}

void RAIN::SandboxApp::PollCore(wxTimerEvent& event)
{
	if (this->msgCore)
		this->msgCore->Pump();
}

