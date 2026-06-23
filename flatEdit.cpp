
//////////////////////////////////////////////////////////////////////////////////////////////////

#include "StdAfx.h"
#include "flatEdit.h"
#include "ss6.h"

extern Css6App theApp;

#define TEXTMATRIX(x, y)		*(m_pTextMatrix + (y * m_uCharCnt1Line) + x) 

//////////////////////////////////////////////////////////////////////////////////////////////////


HFONT CreatePointFont(int nPointSize,CString strFontName)
{
	LOGFONT logFont;
	memset(&logFont, 0, sizeof(LOGFONT));
	logFont.lfCharSet = DEFAULT_CHARSET;
	logFont.lfHeight = nPointSize;
	Checked::tcsncpy_s(logFont.lfFaceName, _countof(logFont.lfFaceName), strFontName, _TRUNCATE);

	HDC hDC;
	hDC = ::GetDC(NULL);

	// convert nPointSize to logical units based on pDC
	CPoint ptOrg,pt;

	// 72 points/inch, 10 decipoints/point
	ptOrg.x = ptOrg.y = 0;
	pt.y = ::MulDiv(::GetDeviceCaps(hDC, LOGPIXELSY), logFont.lfHeight, 720);
	pt.x = 0;
	::DPtoLP(hDC, &pt, 1);
	::DPtoLP(hDC, &ptOrg, 1);
	logFont.lfHeight = -abs(pt.y - ptOrg.y);
	::ReleaseDC(NULL, hDC);

	return ::CreateFontIndirect(&logFont);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

const WCHAR	CFlatEdit::DEF_FONT_NAME[] = L"tahoma";

CFlatEdit::CFlatEdit()
{
	m_hFont = 0;
	m_ulMemSize = BLOCK_SIZE;
}

CFlatEdit::~CFlatEdit(){}

//////////////////////////////////////////////////////////////////////////////////////////////////

BOOL CFlatEdit::SetFont(CStringW const& strFontName,LONG lHeight)
{
	HDC hScreenDC = ::GetDC(0);
	HDC hMemDC = ::CreateCompatibleDC(hScreenDC);
	TEXTMETRIC tm;
	m_hFont = CreatePointFont(lHeight,strFontName);
		::SelectObject(hMemDC,m_hFont);
		GetTextMetrics(hMemDC, &tm);
		m_lSingleCharWidth = tm.tmAveCharWidth;
		m_lSingleCharHeight = tm.tmHeight;

	return 0 == m_hFont;
}

LRESULT CFlatEdit::OnMsgPaint(WPARAM wParam,LPARAM lParam)
{
	PAINTSTRUCT ps;
	CRect rcWnd,rcText;
	CPoint pt;
	BOOL bHovered;

	GetWindowRect(&rcWnd);
	GetCursorPos(&pt);
	ScreenToClient(&rcWnd);
	ScreenToClient(&pt);
	bHovered = rcWnd.PtInRect(pt);
	HBRUSH h;
	BeginPaint(&ps);	

	rcWnd = ps.rcPaint;
	rcWnd.DeflateRect(1,1,1,1);
	::FillRect(ps.hdc,&rcWnd,(HBRUSH)::GetStockObject(WHITE_BRUSH));

	if(bHovered)
	{
		::FrameRect(ps.hdc,&ps.rcPaint,::GetSysColorBrush(COLOR_HIGHLIGHT));
		::SelectObject(ps.hdc,m_hFont);

		::DrawTextW(ps.hdc,L"some text",9,&rcWnd,DT_LEFT); 
	}
	else
	{
		h=::CreateSolidBrush(RGB(173,173,173));
		::FrameRect(ps.hdc,&ps.rcPaint,h);
		::DeleteObject(h);
		::DrawTextW(ps.hdc,L"some text",9,&rcWnd,DT_LEFT|DT_VCENTER|DT_SINGLELINE);
	}

	EndPaint(&ps);

	return 0;
}

LRESULT CFlatEdit::OnMsgMouseMove(WPARAM wParam,LPARAM lParam)
{
	CRect rcWnd;
	CPoint pt;

	GetWindowRect(&rcWnd);
	ScreenToClient(&rcWnd);

	pt.x = GET_X_LPARAM(lParam);
	pt.y = GET_Y_LPARAM(lParam);

	if(rcWnd.PtInRect(pt))
	{
		//if(0 == m_bHoverd)
		{
			//m_bHoverd = TRUE;
			InvalidateRect(0);
		}
		TRACKMOUSEEVENT tm;
		tm.cbSize = sizeof(TRACKMOUSEEVENT);
		tm.dwFlags = TME_LEAVE;
		tm.hwndTrack = m_hWnd;
		tm.dwHoverTime = 0;
		::TrackMouseEvent(&tm);
	}
	return 0;
}

LRESULT CFlatEdit::OnMsgMouseLeave(WPARAM wParam,LPARAM lParam)
{
	Invalidate(TRUE);
	return 0;
}

LRESULT CFlatEdit::OnMsgNcCreate(WPARAM wParam,LPARAM lParam)
{
	m_ulMemSize = BLOCK_SIZE;
	m_pTextMatrix = (WCHAR *)::VirtualAlloc(0,m_ulMemSize,MEM_COMMIT,PAGE_READWRITE);
	SetFont(DEF_FONT_NAME,DEF_FONT_HEIGHT);
	return 0 != m_hFont;
}

LRESULT CFlatEdit::OnMsgDestroy(WPARAM wParam,LPARAM lParam)
{
	::DeleteObject(m_hbrPlainBorder);
	::DeleteObject(m_hbrHotBorder);
	::DeleteObject(m_hbrPlainRc);
	::DeleteObject(m_hbrHighlightRc);
	::DeleteObject(m_hFont);
	::VirtualFree(m_pTextMatrix,m_ulMemSize,MEM_DECOMMIT);
	return 0;
}

LRESULT CFlatEdit::OnMsgSize(WPARAM wParam,LPARAM lParam)
{
	UINT uClientWidth,uClientHeight;

	uClientWidth = LOWORD(lParam); 
    m_uCharCnt1Line = max(1, uClientWidth/m_lSingleCharWidth); 
	uClientHeight = HIWORD(lParam); 
    m_uLineCnt = max(1, uClientHeight/m_lSingleCharHeight);
	if(m_uLineCnt * m_uCharCnt1Line * sizeof(WCHAR) > BLOCK_SIZE)
	{
		::VirtualFree(m_pTextMatrix,m_ulMemSize,MEM_DECOMMIT);
		m_ulMemSize = (m_uLineCnt * m_uCharCnt1Line * sizeof(WCHAR)) / BLOCK_SIZE + 1;
		m_pTextMatrix = (WCHAR *)::VirtualAlloc(0,m_ulMemSize,MEM_COMMIT,PAGE_READWRITE);
		::RtlSecureZeroMemory(m_pTextMatrix,m_ulMemSize);
	}
	
	//BOOL bs=ShowCaret();
	//DWORD dw=::GetLastError();
 //   SetCaretPos(0, 0);
	return 0;
}

LRESULT CFlatEdit::OnMsgSetFocus(WPARAM wParam,LPARAM lParam)
{
	BOOL bs=::CreateCaret(m_hWnd,0,1,m_lSingleCharHeight*3/4);
	bs=SetCaretPos(150,2); 
	SetCaretBlinkTime(500);
    bs=ShowCaret(); 
	return 0;
}

LRESULT CFlatEdit::WndProc(UINT uMsg,WPARAM wParam,LPARAM lParam)
{
	WCHAR sz[200];
	static int nn=0;

	//::OutputDebugString(L"\nwndproc\t");
	//		::_itow_s(nn++,sz,10);
	//		::OutputDebugString(sz);
	//		::OutputDebugString(L"\tmsg:");
	//		::_itow_s(uMsg,sz,16);
	//		::OutputDebugString(sz);
	//		::OutputDebugString(L"\n");

	switch (uMsg)
	{
		case WM_NCCREATE:
		return OnMsgNcCreate(wParam,lParam);

		case WM_DESTROY:
		return OnMsgDestroy(wParam,lParam);

		case WM_NCHITTEST:
			return HTCLIENT;

		case WM_PAINT:
			return OnMsgPaint(wParam,lParam);

		case WM_SETFOCUS:
			return OnMsgSetFocus(wParam,lParam);

		case WM_MOUSEMOVE:
			return OnMsgMouseMove(wParam,lParam);

		case WM_MOUSELEAVE:
			return OnMsgMouseLeave(wParam,lParam);

		case WM_SIZE:
			return OnMsgSize(wParam,lParam);

		case WM_LBUTTONDOWN:
			return OnMsgSetFocus(wParam,lParam);

		case WM_KILLFOCUS:
			return 0;
	}

	return 0;
}
