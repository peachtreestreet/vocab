
#ifndef __PROGRESSWND__
#define __PROGRESSWND__

////////////////////////////////////////////////////////////////////////////////////////////

#pragma once

////////////////////////////////////////////////////////////////////////////////////////////

#include "wnd0.h"

////////////////////////////////////////////////////////////////////////////////////////////

class CProgressWnd : public CWnd0
{
public:
	CProgressWnd();
	virtual ~CProgressWnd();
	virtual LRESULT WndProc(UINT uMsg,WPARAM wParam,LPARAM lParam);
	virtual HWND Create(LPVOID lpParam,DWORD dwExStyle = 0,
		LPCWSTR lpClassName = WND0_CLASSNAME,
		LPCWSTR lpWindowName = 0,
		DWORD dwStyle = WS_OVERLAPPEDWINDOW,
		CRect const& rcWnd = CRect(0,0,0,0),
		HWND hWndParent = 0,
		HMENU hMenu = 0,
		HINSTANCE hInstance = 0);

public:
	inline int SetRoundRadius(int nRadius){m_nCornerRadius = nRadius;return nRadius;}

protected:
	LRESULT OnPaint(WPARAM wParam,LPARAM lParam);
	LRESULT OnDestroy(WPARAM wParam,LPARAM lParam);
	LRESULT OnMsgDrawNextPos(WPARAM wParam,LPARAM lParam);
	int ResetCornerPixel(HDC hDC);
	int _FillGradient(HDC hdc,CRect const& rect, COLORREF colorStart, COLORREF colorFinish);

protected:
	CRect			m_rcWnd;
	int					m_nCornerRadius;
	COLORREF		m_clrFrame,m_clrBkGround;
	COLORREF		m_clrStart1,m_clrStart2,m_clrEnd1,m_clrEnd2;
	UINT_PTR		m_uTimerID;
	HPEN				m_hBorderPen;
	LOGBRUSH		m_logBrush;
	HBRUSH		m_hBorderBrush;
	int					m_nFilledAreaWidth,m_nFilledWidthFactor;
	int					m_nBorderWidth;
	int					m_nCurrentPos;

	static COLORREF const		DEF_FRAME_COLOR = RGB(160,160,160);
	static COLORREF const		DEF_BK_COLOR = RGB(255,255,255);
	static COLORREF const		DEF_START_COLOR = RGB(0,190,0);
	static COLORREF const		DEF_END_COLOR = RGB(240,240,240);
	static int const DEF_BORDER_WIDTH = 1;
	static int const DEF_CORNER_WIDTH = 8;
	static int const DEF_CORNER_HEIGHT = 8;
	static int const DEF_WIDTH_FACTOR = 10;
	static int const DEF_EXPIRATION_TIME = 100;	// 100ms
};

#endif //__PROGRESSWND__