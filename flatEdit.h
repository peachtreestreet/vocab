//////////////////////////////////////////////////////////////////////////////////////////////////
#pragma once
//////////////////////////////////////////////////////////////////////////////////////////////////

#include <vector>
#include <list>
#include "wnd0.h"

//////////////////////////////////////////////////////////////////////////////////////////////////

using namespace std;

//////////////////////////////////////////////////////////////////////////////////////////////////

#ifndef		__FLATEDIT__
#define		__FLATEDIT__


class CFlatEdit : public CWnd0
{
public:
	CFlatEdit();
	~CFlatEdit();

	BOOL SetFont(CStringW const& strFontName,LONG lHeight);

protected:
	LRESULT OnMsgPaint(WPARAM wParam,LPARAM lParam);
	LRESULT OnMsgLButtonUp(WPARAM wParam,LPARAM lParam);
	LRESULT OnMsgLButtonDown(WPARAM wParam,LPARAM lParam);
	LRESULT OnMsgMouseMove(WPARAM wParam,LPARAM lParam);
	LRESULT OnMsgMouseLeave(WPARAM wParam,LPARAM lParam);
	LRESULT OnMsgDestroy(WPARAM wParam,LPARAM lParam);
	LRESULT OnMsgNcCreate(WPARAM wParam,LPARAM lParam);
	LRESULT OnMsgSize(WPARAM wParam,LPARAM lParam);
	LRESULT OnMsgSetFocus(WPARAM wParam,LPARAM lParam);

	virtual LRESULT WndProc(UINT uMsg,WPARAM wParam,LPARAM lParam);

private:
	HBRUSH m_hbrPlainBorder,m_hbrHotBorder;
	HBRUSH m_hbrPlainRc,m_hbrHighlightRc;
	LONG	m_lSingleCharWidth,m_lSingleCharHeight;
	UINT m_uCharCnt1Line,m_uLineCnt;
	HFONT m_hFont;
	ULONG	m_ulMemSize;
	HBITMAP m_hCaret;                   // caret bitmap 
	WCHAR *m_pTextMatrix;

	static const ULONG BLOCK_SIZE = 0x1000;
	static const LONG DEF_FONT_HEIGHT = 160;
	static const COLORREF		PLAINFRM = RGB(173,173,173);
	static const COLORREF		HOTFRM = RGB(0,120,215);
	static const COLORREF		HIGHLIGHTRC = RGB(0,0,225);
	static const WCHAR			DEF_FONT_NAME[];

};

#endif // __FLATEDIT__