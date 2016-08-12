#ifndef WINDOWS_H
#define WINDOWS_H
#include <Windows.h>
#include "Manager.h"

class Window
{
public:
	static int createWindow(Manager* manager, HINSTANCE hInstance, int nCmdShow);
	static HWND getWindow();
protected:
	static LRESULT CALLBACK procWindow(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);
private:
	static HWND m_hwnd;
};

#endif