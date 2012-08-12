#ifndef __PIECEMANAGER_H
#define __PIECEMANAGER_H

#include <wx/wx.h>
#include <wx/file.h>
#include <wx/filename.h>
#include <wx/ffile.h>

#include "_wxconfig.h"

#include <vector>

#include "Message.h"

namespace RAIN
{
	class PieceEntry
	{
	public:
		PieceEntry();
		~PieceEntry();

		void UpdateSize();

		long size;
		wxString hash;
	};

	class PieceManager : public RAIN::MessageHandler
	{
	public:
		PieceManager();
		~PieceManager();
		
		void HandleMessage(RAIN::Message *msg);
		void Tick();
		int GetMessageHandlerID();

		void UpdateConfig();
		void UpdateFromFS();
		unsigned long long GetCurrentUsage();
		void ValidateSizeLimit();

		bool HasGotPiece(const wxString& hash);

		bool ImportPiece(RAIN::Message *pm);
		bool ExportPiece(RAIN::Message *pm);

	private:
		wxString basedir;
		std::vector<PieceEntry*> cache;
		unsigned long long cacheLimit;
		bool full;
	};
}

#endif //__PIECEMANAGER_H
