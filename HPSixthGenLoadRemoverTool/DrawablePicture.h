#include <windows.h>
#include <stdio.h>
#include <psapi.h>
#include <tchar.h>
#include <gdiplus.h>
#include <string>
#pragma comment (lib,"Gdiplus.lib")
using namespace Gdiplus;
class DrawablePicture
{
	private:
		Rect selRect;
		Bitmap* baseBmp;

	public:
		DrawablePicture();
};

