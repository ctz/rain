#ifndef __MANIFESTMANAGER_H
#define __MANIFESTMANAGER_H

#include <wx/wx.h>
#include <wx/filename.h>
#include <wx/dir.h>
#include <wx/ffile.h>

#include <vector>

/* the number of ticks to wait between sending synch requests */
#define SYNC_INTERVAL 512

/* the number of manifests to send when an unsyncronised peer requests we check
 * their sync */
#define MANIFEST_CHUNKS 32

#include "Message.h"

namespace RAIN
{
	class ManifestEntry;

	class ManifestManager : public RAIN::MessageHandler
	{
	public:
		ManifestManager();
		~ManifestManager();
		
		void HandleMessage(RAIN::Message *msg);
		void Tick();
		int GetMessageHandlerID();

		size_t GetAllManifests(std::vector<RAIN::ManifestEntry*>& v);
		
	private:
		off_t tickCount;
		
		void MergeManifests(RAIN::Message *m);
		void CheckSynchronise(RAIN::Message *m);
		void Synchronise();

		wxString getManifestDir();
		size_t getManifestsAfter(std::vector<RAIN::ManifestEntry*>& v, time_t after, int n);
		time_t getLatestTimestamp();
	};
}

#endif//__MANIFESTMANAGER_H
