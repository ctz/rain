#include "MessageCore.h"
#include "DebugHelper.h"
#include "ConnectionPool.h"
#include "CredentialManager.h"
#include "Message.h"
#include "BValue.h"
#include "PeerID.h"
#include "MeshCore.h"

RAIN::MessageCore *globalMessageCore = NULL;
extern RAIN::CredentialManager *globalCredentialManager;
extern RAIN::DebugHelper *debug;

RAIN::MessageCore::MessageCore(RAIN::SandboxApp *app)
{
	this->app = app;
}

RAIN::MessageCore::~MessageCore()
{
	// empty queues
	wxMutexLocker locker(this->messageQueueMutex);
	while (this->queue.size() > 0)
	{
		delete this->queue.at(0);
		this->queue.erase(this->queue.begin());
	}

	wxMutexLocker lpLocker(this->messagelpQueueMutex);
	while (this->lpQueue.size() > 0)
	{
		delete this->lpQueue.at(0);
		this->lpQueue.erase(this->lpQueue.begin());
	}

	this->handlers.erase(this->handlers.begin(), this->handlers.end());
}

void RAIN::MessageCore::PushMessage(RAIN::Message *msg, MSG_PRIORITY priority)
{
	BENC_SAFE_CAST(msg->headers["Event"], msgevent, String);

	if (priority == MSG_PRIORITY_NORMAL)
	{
		wxMutexLocker locker(this->messageQueueMutex);
		wxLogVerbose("MessageCore::PushMessage(): pushing 0x%08x : %s into normal priority queue", msg, (msgevent) ? msgevent->getString().c_str() : "null event");
		this->queue.push_back(msg);
	} else {
		wxMutexLocker lpLocker(this->messagelpQueueMutex);
		wxLogVerbose("MessageCore::PushMessage(): pushing 0x%08x : %s into low priority queue", msg, (msgevent) ? msgevent->getString().c_str() : "null event");
		this->lpQueue.push_back(msg);
	}
}

void RAIN::MessageCore::Pump()
{
	for (std::map<int, RAIN::MessageHandler*>::iterator i = this->handlers.begin(); i != this->handlers.end(); i++)
	{
		i->second->Tick();
	}

	if (this->queue.size() == 0)
	{
		this->quietness++;

		if (this->quietness > QUIETNESS_THRESHOLD)
		{
			if (this->lpQueue.size() > 0)
			{
				this->messagelpQueueMutex.Lock();
				RAIN::Message *lpmsg = this->lpQueue.at(0);
				this->lpQueue.erase(this->lpQueue.begin());
				this->messagelpQueueMutex.Unlock();

				this->app->mesh->ImportIdentifiers(lpmsg);
				this->HandleMessage(lpmsg);

				delete lpmsg;
			}
			
			this->quietness = 0;
			return;
		}
	} else {
		this->quietness = 0;
	}

	while (this->queue.size() > 0)
	{
		this->messageQueueMutex.Lock();
		RAIN::Message *msg = this->queue.at(0);
		this->queue.erase(this->queue.begin());
		this->messageQueueMutex.Unlock();

		this->app->mesh->ImportIdentifiers(msg);
		this->HandleMessage(msg);

		delete msg;
	}
}

bool RAIN::MessageCore::RegisterMessageHandler(RAIN::MessageHandler *hdlr)
{
	int hdlrid = hdlr->GetMessageHandlerID();
	if (hdlrid > 0)
	{
		this->handlers[hdlrid] = hdlr;
		return true;
	} else {
		return false;
	}
}

void RAIN::MessageCore::HandleMessage(RAIN::Message *msg)
{
	wxLogVerbose("MessageCore::HandleMessage(): msg = %08X", msg);
	int handler = 0;

	if (debug)
		debug->LogMessage(msg);

	BENC_SAFE_CAST(msg->headers["Destination"], msgdest, String);
	BENC_SAFE_CAST(msg->headers["To"], msgto, String);

	switch (msg->status)
	{
		case MSG_STATUS_LOCAL:
			/* meaning it didn't come off the network and can get
			 * broadcast internally */
			if (msgdest && msgdest->getString().Cmp("Broadcast") == 0)
			{
				for (std::map<int, RAIN::MessageHandler*>::iterator i = this->handlers.begin(); i != this->handlers.end(); i++)
				{
					i->second->HandleMessage(msg);
				}
			} else {
				handler = msg->subsystem;
				if (this->handlers.find(handler) != this->handlers.end())
				{
					this->handlers[handler]->HandleMessage(msg);
					// handler take a copy if they want it because it will get deleted in a sec
				} else {
					wxLogError("MessageCore::HandleMessage(): Cannot handle local message with handlerid: %d", handler);
				}
			}
			break;
		case MSG_STATUS_INCOMING:
			/* is it for us or maybe a broadcast */
			if ((msgdest && msgdest->getString() == "Peer" && msgto && msgto->getString() == globalCredentialManager->GetMyPeerID()->hash) || (msgdest && msgdest->getString() == "Broadcast"))
			{
				bool actOnMessage = true;

				if (msgdest && msgdest->getString() == "Broadcast")
				{
					/* check serial and set actOnMessage to false if we
					 * already saw it */
					BENC_SAFE_CAST(msg->headers["Serial"], serialstr, String);
					
					if (serialstr)
					{
						if (this->HasSeenSerial(serialstr->getString()))
						{
							actOnMessage = false;
						} else {
							this->seenSerials.push_back(serialstr->getString());
						}
					}
				}

				if (actOnMessage)
				{
					/* yes - need to dispatch it to a handler here */
					handler = msg->subsystem;

					if (this->handlers.find(handler) != this->handlers.end())
					{
						this->handlers[handler]->HandleMessage(msg);
						// handler take a copy if they want it because it will get deleted in a sec
					} else {
						wxLogError("MessageCore::HandleMessage(): Cannot handle incoming message with handlerid: %d", handler);
					}
				}
			} else {
				/* no, just redispatch it */
				msg->status = MSG_STATUS_OUTGOING;
				this->app->connections->DispatchMessage(msg);
			}

			/* we need to rebroadcast broadcast messages. this
			 * might do entirely nothing if we have nothing to do */
			if (msgdest && msgdest->getString() == "Broadcast")
			{
				msg->status = MSG_STATUS_OUTGOING;
				this->app->connections->DispatchMessage(msg);
			}

			break;
		case MSG_STATUS_OUTGOING:
			if (msgdest && msgdest->getString() == "Peer" && msgto && msgto->getString() == globalCredentialManager->GetMyPeerID()->hash)
			{
				/* outgoing message is for us, happens rarely but is an error to send 
				 * messages out to ourselves - this code effectively implements a loopback
				 * interface */
				msg->status = MSG_STATUS_INCOMING;
				this->HandleMessage(msg);
				return;
			}

			this->app->connections->DispatchMessage(msg);
			wxLogVerbose("MessageCore::HandleMessage(): Dispatched outgoing message 0x%08x", msg);
			break;
		case MSG_STATUS_UNKNOWN:
			/* this is a badly formed message and will be deleted */
			break;
	}
}

bool RAIN::MessageCore::HasSeenSerial(const wxString &ser)
{
	for (size_t i = 0; i < this->seenSerials.size(); i++)
	{
		if (RAIN::SafeStringsEq(ser, this->seenSerials.at(i)))
			return true;
	}

	return false;
}
