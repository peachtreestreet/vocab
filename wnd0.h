
#ifndef __WND0__
#define __WND0__

////////////////////////////////////////////////////////////////////////////////////////////
#pragma once
////////////////////////////////////////////////////////////////////////////////////////////

#include <atlbase.h>
#include <atlwin.h>
#include <atltypes.h>
#include <map>

////////////////////////////////////////////////////////////////////////////////////////////

using namespace std;

////////////////////////////////////////////////////////////////////////////////////////////

class CWnd0 : public CWindow
{
public:
	CWnd0();
	virtual ~CWnd0();

	virtual LRESULT WndProc(UINT uMsg, WPARAM wParam, LPARAM lParam) = 0;
	virtual HWND Create(LPVOID lpParam, DWORD dwExStyle = 0,
		LPCWSTR lpClassName = WND0_CLASSNAME,
		LPCWSTR lpWindowName = 0,
		DWORD dwStyle = WS_OVERLAPPEDWINDOW,
		CRect const& rcWnd = CRect(0, 0, 0, 0),
		HWND hWndParent = 0,
		HMENU hMenu = 0,
		HINSTANCE hInstance = 0);

	inline HWND GetParentWnd(){return m_hWndParent;}

	static CWnd0* FromHandle(HWND hWnd);
	static LRESULT WINAPI	_wndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

public :
	static const WCHAR		WND0_CLASSNAME[];

protected:
	HWND	m_hWndParent;

	static map<HWND,CWnd0*>	_mapHandleToPtr;
};

#endif