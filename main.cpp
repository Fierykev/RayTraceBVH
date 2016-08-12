#include <Windows.h>
#include "Window.h"
#include "Graphics.h"

int main()
{
	Graphics manager(L"RayTaceBVH", 500, 500);

	Window window;
	window.createWindow((Manager*)&manager, GetModuleHandle(NULL), SW_SHOW);

	return 0;
}