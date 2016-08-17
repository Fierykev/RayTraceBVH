#include <Windows.h>
#include "Window.h"
#include <time.h>

HWND Window::m_hwnd = nullptr;

int Window::createWindow(Manager* manager, HINSTANCE hInstance, int nCmdShow)
{
	// Initialize the window class.
	WNDCLASSEX windowClass = { 0 };
	windowClass.cbSize = sizeof(WNDCLASSEX);
	windowClass.style = CS_HREDRAW | CS_VREDRAW;
	windowClass.lpfnWndProc = procWindow;
	windowClass.hInstance = hInstance;
	windowClass.hCursor = LoadCursor(NULL, IDC_ARROW);
	windowClass.lpszClassName = L"DXSampleClass";
	RegisterClassEx(&windowClass);

	RECT windowRect = { 0, 0, static_cast<LONG>(manager->getWidth()), static_cast<LONG>(manager->getHeight()) };
	AdjustWindowRect(&windowRect, WS_OVERLAPPEDWINDOW, FALSE);

	// Create the window and store a handle to it.
	m_hwnd = CreateWindow(
		windowClass.lpszClassName,
		manager->getTitle().c_str(),
		WS_OVERLAPPEDWINDOW,
		CW_USEDEFAULT,
		CW_USEDEFAULT,
		windowRect.right - windowRect.left,
		windowRect.bottom - windowRect.top,
		nullptr,		// We have no parent window.
		nullptr,		// We aren't using menus.
		hInstance,
		manager);
	// init the manager
	manager->onInit();

	// Display the window
	ShowWindow(m_hwnd, nCmdShow);
	SetForegroundWindow(m_hwnd);
	SetFocus(m_hwnd);

	// Main sample loop.
	MSG msg = {};
	while (msg.message != WM_QUIT)
	{
		// Process any messages in the queue.
		if (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
		{
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
	}

	// Return this part of the WM_QUIT message to Windows.
	return static_cast<char>(msg.wParam);
}

HWND Window::getWindow()
{
	return m_hwnd;
}

LRESULT CALLBACK Window::procWindow(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	Manager* manager = reinterpret_cast<Manager*>(GetWindowLongPtr(hWnd, GWLP_USERDATA));

	switch (message)
	{
	case WM_CREATE:
	{
		// Save the DXSample* passed in to CreateWindow.
		LPCREATESTRUCT pCreateStruct = reinterpret_cast<LPCREATESTRUCT>(lParam);
		SetWindowLongPtr(hWnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(pCreateStruct->lpCreateParams));

		return 0;
	}
	case WM_KEYDOWN:
		manager->onKeyDown(static_cast<UINT8>(wParam));
		return 0;

	case WM_KEYUP:
		manager->onKeyUp(static_cast<UINT8>(wParam));
		return 0;
	case WM_PAINT:
		if (manager)
		{
			//clock_t start = clock();

			manager->onUpdate();
			manager->onRender();

			//printf("STOP %f\n", ((clock() - start) / (double)CLOCKS_PER_SEC));
		}
		return 0;
	case WM_CLOSE:
		PostQuitMessage(0);
		return 0;
	case WM_DESTROY:
		PostQuitMessage(0);
		return 0;
	}

	// Handle any message the switch statment did not
	return DefWindowProc(hWnd, message, wParam, lParam);
}