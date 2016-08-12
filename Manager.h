#ifndef MANAGER_H
#define MANAGER_H

#include <string>
#include <dxgi1_2.h>
#include <dxgi.h>

class Manager
{
public:
	Manager(std::wstring title, unsigned int width, unsigned int height);
	
	virtual void onInit() = 0;
	virtual void onUpdate() = 0;
	virtual void onRender() = 0;
	virtual void onDestroy() = 0;

	// overide the below methods to allow for keys to be processed
	virtual void onKeyDown(UINT8) {}
	virtual void onKeyUp(UINT8) {}

	// accessor
	unsigned int Manager::getWidth();
	unsigned int Manager::getHeight();
	std::wstring Manager::getTitle();
protected:
	void GetHardwareAdapter(_In_ IDXGIFactory2* pFactory, _Outptr_result_maybenull_ IDXGIAdapter1** ppAdapter);

	std::wstring title;
	unsigned int width, height;

	// adapter information
	bool m_useWarpDevice;
};

#endif