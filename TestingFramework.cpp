#include "BEnc-tests.h"

#include <wx/wx.h>

using namespace RAIN;

void RAIN::RunAllTests()
{
	BEncTests benc;
	benc.RunTests();
}
