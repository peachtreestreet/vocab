//////////////////////////////////////////////////////////////////////////////////////////////////

#include "FlatDropList.h"
#include "Tools1.h"

////////////////////////////////////////////////////////////////////////////////////////////

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

////////////////////////////////////////////////////////////////////////////////////////////

using namespace VocabApp;

//////////////////////////////////////////////////////////////////////////////////////////////////

const COLORREF CFlatDropList::ARROWCOLOR[] = {RGB(179,179,179),RGB(86,86,86),RGB(213,213,213)};
const WCHAR CFlatDropList::FCOMBO_CLASS_NAME[] = L"flatCombo";
const WCHAR	CFlatDropList::DEF_FONT_NAME[] = L"tahoma";

//////////////////////////////////////////////////////////////////////////////////////////////////

CFlatDropList::CFlatDropList(void)
{
	m_bListWndShown = 0;
}

CFlatDropList::~CFlatDropList(void) 
{ 
}

int CFlatDropList::SetCurSel(int nIndex)
{
	m_listWnd.SetCurSel(nIndex);
	Invalidate(TRUE);
	return 0;
}

int CFlatDropList::ResetContent()
{
	return m_listWnd.Clear();
}

HWND CFlatDropList::Create(HWND hWndParent,LPCWSTR lpWindowName,CRect const& rcWnd)
{
	// this is must be a child window
	if(0 == hWndParent)return 0;
	if(!::IsWindow(hWndParent))return 0;

	CRect rc(rcWnd),rcParent;
	::GetClientRect(hWndParent,&rcParent);

	if(0 > rc.left)
	{
		rc.right = rc.right - rc.left;
		rc.left = 0;
	}
	if(rc.Width() > rcParent.Width())
	{
		rc.right = rc.left + rcParent.Width();
	}
	if(0 > rc.top)
	{
		rc.bottom = rc.bottom - rc.top;
		rc.top = 0;
	}

	// force to create a window with height=HEADER_HEIGHT
	if(HEADER_HEIGHT != rc.Height())
	{
		rc.bottom = rc.top + HEADER_HEIGHT;
	}

	m_hWndParent = hWndParent;
	rc.right = rc.left + CListWnd::THUMB_WIDTH + ARROW_WIDTH + 2*LEFT_MARGIN;
	rc.bottom = rc.top + HEADER_HEIGHT;

	// list window will be created when the first string is added to combo
	CWnd0::Create(this,0,0,lpWindowName,WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS,rc,hWndParent,0,0);
	if (0 == m_hWnd)return 0;

	m_listWnd.SetComboWnd(m_hWnd);
	return m_hWnd;
}

int CFlatDropList::AddString(CStringW const& str,HICON h)
{
	return m_listWnd.AddItem(str,h);
}

UINT CFlatDropList::GetCurrentState()
{
	CRect rcWnd;
	CPoint pt;

	if(m_bListWndShown)return CVocabConst::LBTNDOWN;
	GetWindowRect(&rcWnd);
	::GetCursorPos(&pt);
	ScreenToClient(&rcWnd);
	::MapWindowPoints(NULL ,m_hWnd,&pt,1);
	if(rcWnd.PtInRect(pt))
	{
		//::OutputDebugStringW(L"combo state : HOVERED\n");
		return CVocabConst::HOVERED;
	}
	else
	{
		//::OutputDebugStringW(L"combo state : PLAIN\n");
		return CVocabConst::PLAIN;
	}
}

LRESULT CFlatDropList::OnMsgLButtonDown(WPARAM wParam,LPARAM lParam)
{
	if(0 == m_bListWndShown)
	{
		ShowListWnd();
		::SendMessage(m_hWndParent,WM_COMMAND,MAKELONG(m_nCtrlID,CBN_SETFOCUS),(LPARAM)m_hWnd);
		::SendMessage(m_hWndParent,WM_COMMAND,MAKELONG(m_nCtrlID,CBN_DROPDOWN),(LPARAM)m_hWnd);
	}
	else
	{
		m_listWnd.ShowWindow(SW_HIDE);
		m_bListWndShown = 0;
		::SendMessage(m_hWndParent,WM_COMMAND,MAKELONG(m_nCtrlID,CBN_SELENDCANCEL),(LPARAM)m_hWnd);
		::SendMessage(m_hWndParent,WM_COMMAND,MAKELONG(m_nCtrlID,CBN_CLOSEUP),(LPARAM)m_hWnd);
	}

	return 0;
}

int CFlatDropList::ShowListWnd()
{
	CRect rc;

	if(0 != m_listWnd.m_hWnd)goto _ShowListWnd_exit;

	m_listWnd.SetComboWnd(m_hWnd);

	// we must create a popup window which can be displayed outside of the parent window(owner window)
	// portion of child window may be covered if it exceed the border of the parent window
	// the position and size will be adjusted before displaying
	m_listWnd.Create(&m_listWnd,0,0,0,WS_POPUP,rc,m_hWndParent,0,0);

_ShowListWnd_exit:
	m_listWnd.SetFontHeight(10);
	m_listWnd.ShowWindow(SW_SHOW);	// will send WM_SHOWWINDOW and then size&position will be adjusted
	m_bListWndShown = TRUE;
	
	return 0;
}

int CFlatDropList::DrawArrow(HDC hDC)
{
	int xStart,xEnd,yStart,yEnd;
	CRect rcWnd;

	HPEN hPen = ::CreatePen(PS_SOLID,PEN_WIDTH,ARROWCOLOR[0]);
	::SelectObject(hDC,hPen);
	GetWindowRect(&rcWnd);
	ScreenToClient(&rcWnd);

	xStart = rcWnd.right - ARROW_R_MARGIN;
	yStart = rcWnd.top + (HEADER_HEIGHT - ARROW_HEIGHT)/2;
	xEnd = xStart - ARROW_WIDTH;
	yEnd = yStart + ARROW_HEIGHT;

	::MoveToEx(hDC,xStart,yStart,0);
	::LineTo(hDC,xEnd,yEnd);

	xStart = xEnd;
	yStart = yEnd - OFFSET0;
	xEnd = xStart - ARROW_WIDTH;
	yEnd = yStart - ARROW_HEIGHT;
	::MoveToEx(hDC,xStart,yStart,0);
	::LineTo(hDC,xEnd,yEnd);

	xStart = rcWnd.right - ARROW_R_MARGIN;
	yStart = rcWnd.top + (HEADER_HEIGHT - ARROW_HEIGHT)/2 + OFFSET0;
	xEnd = xStart - ARROW_WIDTH;
	yEnd = yStart + ARROW_HEIGHT;
	::DeleteObject(hPen);
	hPen = ::CreatePen(PS_SOLID,OFFSET0,ARROWCOLOR[1]);
	::SelectObject(hDC,hPen);
	::MoveToEx(hDC,xStart,yStart,0);
	::LineTo(hDC,xEnd,yEnd);

	xStart = xEnd;
	yStart = yEnd - OFFSET0;
	xEnd = xStart - ARROW_WIDTH;
	yEnd = yStart - ARROW_HEIGHT;
	::MoveToEx(hDC,xStart,yStart,0);
	::LineTo(hDC,xEnd,yEnd);

	xStart = rcWnd.right - ARROW_R_MARGIN + OFFSET0;
	yStart = rcWnd.top + (HEADER_HEIGHT - ARROW_HEIGHT)/2 + OFFSET0;
	xEnd = xStart - ARROW_WIDTH - OFFSET0;
	yEnd = yStart + ARROW_HEIGHT + OFFSET0;
	::DeleteObject(hPen);
	hPen = ::CreatePen(PS_SOLID,PEN_WIDTH,ARROWCOLOR[2]);
	::SelectObject(hDC,hPen);
	::MoveToEx(hDC,xStart,yStart,0);
	::LineTo(hDC,xEnd,yEnd);

	xStart = xEnd;
	yStart = yEnd - OFFSET0;
	xEnd = xStart - ARROW_WIDTH - OFFSET0;
	yEnd = yStart - ARROW_HEIGHT - OFFSET0;
	::MoveToEx(hDC,xStart,yStart,0);
	::LineTo(hDC,xEnd,yEnd);

	return 0;
}

LRESULT CFlatDropList::OnMsgMouseLeave(WPARAM wParam,LPARAM lParam)
{
	Invalidate(TRUE);

	return 0;
}

LRESULT CFlatDropList::OnMsgMouseMove(WPARAM wParam,LPARAM lParam)
{
	if(GetCurrentState() != m_uPrevState)
	{
		Invalidate(TRUE);
	}

	TRACKMOUSEEVENT tm;
	tm.cbSize = sizeof(TRACKMOUSEEVENT);
	tm.dwFlags = TME_LEAVE;
	tm.hwndTrack = m_hWnd;
	tm.dwHoverTime = 0;
	::TrackMouseEvent(&tm);

	return 0;
}

LRESULT CFlatDropList::OnMsgEraseBkGnd(WPARAM wParam,LPARAM lParam)
{
	HDC hDC;
	CRect rcWnd,rcText;
	HDC hScreenDC,hMemDC;
	HBITMAP hBmp;
	CPoint pt;
	UINT uState;

	GetWindowRect(&rcWnd);
	rcWnd.OffsetRect(-rcWnd.left,-rcWnd.top);
	hScreenDC = ::GetDC(0);
	hDC = GetWindowDC();
	hMemDC = ::CreateCompatibleDC(hScreenDC);
	hBmp = ::CreateCompatibleBitmap(hScreenDC,rcWnd.Width(),rcWnd.Height());
	::SelectObject(hMemDC,hBmp);
	uState = GetCurrentState();

	switch(uState)
	{
	case CVocabConst::PLAIN:
		::FillRect(hMemDC,&rcWnd,m_hbrPlainRc);
		::FrameRect(hMemDC,&rcWnd,m_hbrPlainFrame);
		break;

	case CVocabConst::HOVERED:
		::FillRect(hMemDC,&rcWnd,m_hbrHotRc);
		::FrameRect(hMemDC,&rcWnd,m_hbrHotFrame);
		break;

	case CVocabConst::LBTNDOWN:
		::FillRect(hMemDC,&rcWnd,m_hbrLbdRc);
		::FrameRect(hMemDC,&rcWnd,m_hbrLbdFrame);
		break;
	}

	DrawArrow(hMemDC);
	if(m_listWnd.m_vecContent.size() > 0)
	{
		int nCurSel;
		::SetBkMode(hMemDC,TRANSPARENT);
		::SelectObject(hMemDC,m_hFont);
		rcText = rcWnd;
		rcText.left += LEFT_MARGIN*2;
		nCurSel = m_listWnd.GetCurSel();
		if(0 <= nCurSel && nCurSel < m_listWnd.m_vecContent.size())
		{
			if(!IsWindowEnabled())
			{
				::SetTextColor(hMemDC,::GetSysColor(COLOR_GRAYTEXT));
			}
			else
			{
				::SetTextColor(hMemDC,RGB(0,127,127));
			}
			::DrawText(hMemDC,*m_listWnd.m_vecContent[nCurSel]->pStr,m_listWnd.m_vecContent[nCurSel]->pStr->GetLength(),&rcText,DT_LEFT|DT_VCENTER|DT_SINGLELINE);
		}
	}
	
	::BitBlt(hDC,rcWnd.left,rcWnd.top,rcWnd.Width(),rcWnd.Height(),hMemDC,0,0,SRCCOPY);
	::DeleteDC(hMemDC);
	::DeleteObject(hBmp);
	::ReleaseDC(0,hScreenDC);
	ReleaseDC(hDC);
	//::OutputDebugStringW(L"CFlatDropList::OnMsgEraseBkGnd\n");
	m_uPrevState = uState;

	return TRUE;
}

LRESULT CFlatDropList::OnMsgNcCreate(WPARAM wParam,LPARAM lParam)
{
	m_hbrPlainFrame = ::CreateSolidBrush(PLAINFRM);
	m_hbrPlainRc = ::CreateSolidBrush(PLAINRC);
	m_hbrHotFrame = ::CreateSolidBrush(HOTFRM);
	m_hbrHotRc = ::CreateSolidBrush(HOTRC);
	m_hbrLbdFrame = ::CreateSolidBrush(LBDFRM);
	m_hbrLbdRc = ::CreateSolidBrush(LBDRC);
	m_hFont = Tools1::CreatePointFont(0, CVocabApp::DEF_FONT_NAME);

	return TRUE;
}

LRESULT CFlatDropList::OnMsgDestroy(WPARAM wParam,LPARAM lParam)
{
	CFlatDropList	*pWnd;
	pWnd = (CFlatDropList*)CWnd0::FromHandle(m_hWnd);

	ResetContent(); 
	if(m_listWnd.IsWindow())	m_listWnd.DestroyWindow();
	if(0 != pWnd)
	{
		::DeleteObject(m_hbrPlainFrame);
		::DeleteObject(m_hbrPlainRc);
		::DeleteObject(m_hbrHotFrame);
		::DeleteObject(m_hbrHotRc);
	}
	return 0;
}

LRESULT CFlatDropList::OnMsgSelChanged(WPARAM wParam,LPARAM lParam)
{
	Invalidate(FALSE);
	return 0;
}

LRESULT CFlatDropList::OnMsgRcvListWndState(WPARAM wParam,LPARAM lParam)
{
//static int nn=0;
	//WCHAR sz[200];
	//::OutputDebugString(L"\nListWndState\t");
	//		::_itow_s(wParam,sz,10);
	//		::OutputDebugString(sz);
	//		::OutputDebugString(L"\n");
	m_bListWndShown = wParam;
	Invalidate(TRUE);
	return 0;
}

LRESULT CFlatDropList::WndProc(UINT uMsg,WPARAM wParam,LPARAM lParam)
{
	switch (uMsg)
	{
	case WM_ERASEBKGND:
		return OnMsgEraseBkGnd(wParam,lParam);

	case WM_PAINT:
		{
			PAINTSTRUCT ps;
			BeginPaint(&ps);	
			EndPaint(&ps);
		}
		return 0;

	case WM_ENABLE:
		Invalidate(TRUE);
		return 0;

	case WM_MOUSEMOVE:
		return OnMsgMouseMove(wParam,lParam);

	case WM_MOUSELEAVE:
		return OnMsgMouseLeave(wParam,lParam);

	case WM_NCCREATE:
		return OnMsgNcCreate(wParam,lParam);

	case WM_NCHITTEST:
		return HTCLIENT;

	case WM_DESTROY:
		return OnMsgDestroy(wParam,lParam);

	case WM_LBUTTONDOWN:
		return OnMsgLButtonDown(wParam,lParam);

	case CVocabConst::MSG_SELCHANGED:
		return OnMsgSelChanged(wParam,lParam);

	case CVocabConst::MSG_RCVLISTWNDSTATE:
		return OnMsgRcvListWndState(wParam,lParam);
	}

	return ::DefWindowProc(m_hWnd,uMsg,wParam,lParam);
}
