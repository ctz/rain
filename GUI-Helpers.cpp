#include "GUI-Helpers.h"

void RAIN::GUI::OpenInShell(const wxString &dir)
{
#ifdef __WINDOWS__
	ShellExecute(NULL, NULL, dir.c_str(), NULL, NULL, 1);
#else
#error You must implement RAIN::GUI::OpenInShell for this platform
#endif
}

wxString RAIN::GUI::FormatFileSize(size_t i)
{
	wxString bytesize;

	bytesize = wxString::Format("%d", i);
	int n = bytesize.Length();
	while (n - 3 > 0)
	{
		n -= 3;
		bytesize = bytesize.Mid(0, n) + "," + bytesize.Mid(n);
	}

	const char *suffixes[] = {"KB", "MB", "GB", "TB"};
	int used_suffix = -1;
	float fsize = i;

	while (fsize > 1024.0 && used_suffix < 4)
	{
		used_suffix++;
		fsize /= 1024.0;
	}

	if (used_suffix > -1)
        return wxString::Format("%.2f%s (%s bytes)", fsize, suffixes[used_suffix], bytesize.c_str());
	else
		return wxString::Format("%s bytes", bytesize.c_str());
}