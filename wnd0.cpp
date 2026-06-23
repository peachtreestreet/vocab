
////////////////////////////////////////////////////////////////////////////////////////////

#include "vocab.h"
#include "wnd0.h"

////////////////////////////////////////////////////////////////////////////////////////////

#ifdef _DEBUG
#define new new(_NORMAL_BLOCK, __FILE__, __LINE__)
#endif

////////////////////////////////////////////////////////////////////////////////////////////

using namespace VocabApp;

////////////////////////////////////////////////////////////////////////////////////////////

const WCHAR		CWnd0::WND0_CLASSNAME[] = L"flatWnd";
map<HWND,CWnd0*>		CWnd0::_mapHandleToPtr;

////////////////////////////////////////////////////////////////////////////////////////////

CWnd0::CWnd0()
{
	m_hWndParent = 0;
}

CWnd0::~CWnd0(){}

LRESULT WINAPI CWnd0::_wndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	CWnd0 *pWnd;

	if(WM_NCCREATE == uMsg)
	{
		LPCREATESTRUCT	lpcs = reinterpret_cast<LPCREATESTRUCT>(lParam);
		CWnd0 *pWnd= reinterpret_cast<CWnd0*>(lpcs->lpCreateParams);
		if(0 == pWnd)return 0;
		pWnd->m_hWnd = hWnd;
		_mapHandleToPtr.insert(pair<HWND, CWnd0*>(hWnd, pWnd));
		return pWnd->WndProc(uMsg, wParam, lParam);
	}
	if(WM_NCDESTROY == uMsg)
	{
		pWnd = CWnd0::FromHandle(hWnd);
		if (0 != pWnd)return pWnd->WndProc(uMsg, wParam, lParam);
		_mapHandleToPtr.erase(hWnd);
		return 0;
	}

	pWnd = CWnd0::FromHandle(hWnd);
	if (0 != pWnd)return pWnd->WndProc(uMsg, wParam, lParam);

	// errors occured,should not run to this line
	return ::DefDlgProc(hWnd, uMsg, wParam, lParam);
}

CWnd0* CWnd0::FromHandle(HWND hWnd)
{
	map<HWND,CWnd0*>::const_iterator iterMap;

	iterMap = CWnd0::_mapHandleToPtr.find(hWnd);
	if(CWnd0::_mapHandleToPtr.end() == iterMap)return 0;

	return iterMap->second;
}

HWND CWnd0::Create(LPVOID lpParam, DWORD dwExStyle, LPCWSTR lpClassName, LPCWSTR lpWindowName, DWORD dwStyle, CRect const& rcWnd, HWND hWndParent, HMENU hMenu, HINSTANCE hInstance)
{
	m_hWndParent = hWndParent;
	return ::CreateWindowEx(dwExStyle, (0 == lpClassName) ? CWnd0::WND0_CLASSNAME : lpClassName,
		lpWindowName, dwStyle, rcWnd.left, rcWnd.top, rcWnd.Width(), rcWnd.Height(), hWndParent, hMenu, hInstance, lpParam);
}