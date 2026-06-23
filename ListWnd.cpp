
//////////////////////////////////////////////////////////////////////////////////////////////////

#include "windows.h"
#include "shellapi.h"
#include "ListWnd.h"
#include "FlatDropList.h"
#include "Tools1.h"

////////////////////////////////////////////////////////////////////////////////////////////

#ifdef _DEBUG
#define new new(_NORMAL_BLOCK, __FILE__, __LINE__)
#endif

//////////////////////////////////////////////////////////////////////////////////////////////////

using namespace ATL;
using namespace VocabApp;

//////////////////////////////////////////////////////////////////////////////////////////////////

extern vocabGlobal globalVar;
WCHAR szBuff[_MAX_PATH];

//////////////////////////////////////////////////////////////////////////////////////////////////

int CListWnd::SCROLLBAR_WIDTH = ::GetSystemMetrics(SM_CXVSCROLL);
int CListWnd::SCROLLBAR_HEIGHT = ::GetSystemMetrics(SM_CYVSCROLL);
const WCHAR	CListWnd::DEF_FONT_NAME[] = L"Consolas";
int CListWnd::ICON_MARGIN = LEFT_MARGIN + ::GetSystemMetrics(SM_CXICON);
int CListWnd::ICON_HEIGHT = ::GetSystemMetrics(SM_CYICON);

//////////////////////////////////////////////////////////////////////////////////////////////////

CListWnd::CListWnd(void)
{
	m_hComboWnd = 0;
	m_uTextWidth = 0;

	m_uMaxWndHeight = 0;
	m_hMemDC = 0;
	m_hbrWndFrame = 0;
	m_hbrSelectedItem = 0;
	m_hFont = 0;
	m_bIconExists = 0;
	m_bShowVScroll = 0;
	m_bDragging = FALSE;
	m_uPrevHovered = -1;
	m_uTopItemIndex = 0;
	m_uShownCnt = 0;		// valid value must be greater than 0
	m_usPrevScrollState = CVocabConst::FORCETODRAW;
	m_uMaxItemCnt = DEF_MAX_ITEM_CNT;
	m_uSelectedItem = CVocabConst::INVALID_INDEX;
}

CListWnd::~CListWnd(void){}

UINT CListWnd::Clear()
{
	for(int i=0;i<m_vecContent.size();i++)
	{
		::DeleteObject(m_vecContent[i]->hRgn);
		delete m_vecContent[i]->pStr;
		delete m_vecContent[i];
	}
	m_vecContent.clear();

	list<PSCROLLCTRLAREA>::const_iterator iter,end;
	iter = m_listScrollArea.begin();
	end = m_listScrollArea.end();
	while(iter != end)
	{
		delete (*iter)->prcArea;
		delete (*iter);
		iter++;
	}
	m_listScrollArea.clear();

	return m_vecContent.size() + m_listScrollArea.size();
}

UINT CListWnd::SetCurSel(UINT uIndex)
{
	// this window has been not shown yet,m_uShownCnt is invalid,m_uTopItemIndex must be 0
	if(0 == m_uShownCnt)
	{
		m_uTopItemIndex = 0;
		return m_uSelectedItem = uIndex;
	}

	if(m_vecContent.size() - m_uShownCnt >= uIndex)
	{
		m_uTopItemIndex = uIndex;
	}
	else
	{
		m_uTopItemIndex = m_vecContent.size() - m_uShownCnt;
	}
	if(0 <= uIndex && m_vecContent.size() > uIndex)m_uSelectedItem = uIndex;

	return m_uSelectedItem;
}

int CListWnd::SetFontHeight(int dwHeight/* in mm */)
{
	return m_nFontPoints = dwHeight * ::GetDeviceCaps(m_hMemDC,LOGPIXELSY)/CVocabConst::MM_PER_INCH;
}

UINT CListWnd::GetDispIndexFromPoint(CPoint const& pt)
{
	UINT nIndex(CVocabConst::INVALID_INDEX);
	CRect rcItem;

	for(int i=0;i<m_uShownCnt;i++)
	{
		::GetRgnBox(m_vecContent[i]->hRgn,&rcItem);
		rcItem.right = m_uDispWidth - GAP;
		if(::PtInRect(&rcItem,pt))
		{
			nIndex = i;
			break;
		}
	}
	return nIndex;
}

int CListWnd::AddItem(CStringW const& str,HICON hIcon)
{
	LISTITEMDATA	*pItemData(0);
	CRect rcItem,rcCombo;
	CStringW *pStr(0);
	CSize szTxt;
	HDC hScreenDC,hdc;

	hScreenDC = ::GetWindowDC(0);
	hdc = ::CreateCompatibleDC(hScreenDC);
	::ReleaseDC(0,hScreenDC);

	if(0 != hIcon)m_bIconExists = TRUE;
	::SelectObject(hdc,m_hFont);

	pStr = new CStringW(str);
	pItemData = new LISTITEMDATA;
	pItemData->hIcon = hIcon;
	pItemData->pStr = pStr;
	::GetTextExtentPoint32(hdc,(LPCWSTR)str,pStr->GetLength(),&szTxt);
	::GetWindowRect(m_hComboWnd,&rcCombo);

	rcItem.left = LEFT_MARGIN;
	rcItem.right = rcItem.left + szTxt.cx + TWO*LEFT_MARGIN;

	// m_uTextWidth is the width(text width) of the most wide item of all
	if(rcItem.Width() < rcCombo.Width())rcItem.right = rcItem.left + rcCombo.Width() - TWO*LEFT_MARGIN;
	if(m_uTextWidth < rcItem.Width())m_uTextWidth = rcItem.Width();

	if(0 < m_vecContent.size())
	{
		CRect rc;
		::GetRgnBox(m_vecContent[m_vecContent.size() - 1]->hRgn,&rc);
		rcItem.top = rc.bottom;
	}
	else
	{
		rcItem.top = TOP_MARGIN;
		m_uItemHeight = max(szTxt.cy,ICON_HEIGHT);
	}
	rcItem.bottom = rcItem.top + max(szTxt.cy,ICON_HEIGHT);
	HRGN hRgn = ::CreateRectRgn(rcItem.left,rcItem.top,rcItem.right,rcItem.bottom);
	pItemData->hRgn = hRgn;
	
	m_vecContent.push_back(pItemData);

	::DeleteDC(hdc);

	return m_vecContent.size();
}

LRESULT CListWnd::OnMsgNcCreate(WPARAM wParam,LPARAM lParam)
{
	LOGFONT lf;
	HDC hScreenDC = ::GetWindowDC(0);
	m_hMemDC = ::CreateCompatibleDC(hScreenDC);
	::ReleaseDC(0,hScreenDC);
	if(0 == m_hMemDC)return FALSE;	// if m_hMemDC is not available,do not continue

	m_hbrWndFrame = ::CreateSolidBrush(CLR_FRAME);
	m_hbrSelectedItem = ::CreateSolidBrush(CLR_SELECTEDFRM);
	m_hbrScrollbarBG = ::CreateSolidBrush(CLR_SCROLL_BG);
	m_hbrThumbHovered = ::CreateSolidBrush(CLR_THUMB_HOVERED);
	m_hbrThumbLBD = ::CreateSolidBrush(CLR_THUMB_LBD);
	m_hbrSlider = ::CreateSolidBrush(CLR_SLIDER);
	m_hbrSliderHoverd = ::CreateSolidBrush(CLR_SLIDER_HOVERED);
	m_hbrWhite = (HBRUSH)::GetStockObject(WHITE_BRUSH);
	m_hWhitePen = (HPEN)::GetStockObject(WHITE_PEN);
	m_hGrayPen = ::CreatePen(PS_SOLID,BORDER_WIDTH, ::GetSysColor(COLOR_GRAYTEXT));
	m_hBlackPen = ::CreatePen(PS_SOLID,BORDER_WIDTH, CLR_ARROW);

	::GetObject(::GetStockObject(SYSTEM_FONT), sizeof(LOGFONT), &lf);
	m_nFontPoints = lf.lfHeight;
	::RtlSecureZeroMemory(&lf, sizeof(LOGFONT));
	lf.lfWeight = FW_NORMAL;
	lf.lfCharSet = DEFAULT_CHARSET;
	::RtlCopyMemory(lf.lfFaceName, DEF_FONT_NAME, sizeof(DEF_FONT_NAME));
	lf.lfHeight = m_nFontPoints;
	m_hFont = ::CreateFontIndirect(&lf);

	return TRUE;
}

LRESULT CListWnd::OnMsgNcDestroy(WPARAM wParam,LPARAM lParam)
{
	Clear();
	::DeleteObject(m_hbrWndFrame);
	::DeleteObject(m_hbrSelectedItem);
	::DeleteObject(m_hbrWhite);
	::DeleteObject(m_hbrScrollbarBG);
	::DeleteObject(m_hbrSlider);
	::DeleteObject(m_hFont);
	::DeleteObject(m_hWhitePen);
	::DeleteObject(m_hBlackPen);
	::DeleteObject(m_hGrayPen);
	::DeleteDC(m_hMemDC);
	return 0;
}

LRESULT CListWnd::OnMsgMouseLeave(WPARAM wParam,LPARAM lParam)
{
	//::OutputDebugString(L"\nCListWnd::MOUSELEAVE");

	m_usPrevScrollState = CVocabConst::FORCETODRAW;	// if scrollbar exists,force to redraw it
	Invalidate(TRUE);

	return 0;
}

LRESULT CListWnd::OnMsgMouseMove(WPARAM wParam,LPARAM lParam)
{
	static  int n=0;

	WCHAR szBuff[255];
	CString str;
	CPoint pt;
	UINT uDisplayIndex(-1);
	CRect rcItem;
	::_itow_s(n++,szBuff,10);
	UINT usHitCode;

	//::OutputDebugString(L"\nCListWnd::MouseMove()");
	//::_itow_s(pt.x,szBuff,10);
	//::OutputDebugString(L"\t");
	//::OutputDebugString(szBuff);
	//::_itow_s(pt.y,szBuff,10);
	//::OutputDebugString(L"\t");
	//::OutputDebugString(szBuff);

	//::OutputDebugString(L"\tHITCODE:");
	//::_itow_s(dwHitTest,szBuff,10);
	//::OutputDebugString(L"\t");
	//::OutputDebugString(szBuff);
	//::OutputDebugString(L"\n");
	
	HDC hWndDC = GetWindowDC();

	pt.x = GET_X_LPARAM(lParam);
	pt.y = GET_Y_LPARAM(lParam);
	usHitCode = HitTest(pt);
	if(HTNOWHERE == usHitCode)return 0;
	if(MK_LBUTTON == wParam && !m_bHandleLBUp)goto _CListWnd_OnMsgMouseMove_drawScrollbar;
	if(CVocabConst::UPTHUMB <= usHitCode)goto _CListWnd_OnMsgMouseMove_drawScrollbar;
	uDisplayIndex = GetDispIndexFromPoint(pt);
	

	//nIndex += m_uTopItemIndex;
	//::OutputDebugString(L"\t index:");
	//::_itow_s(pt.x,szBuff,10);
	//::OutputDebugString(L"\t");
	//::OutputDebugString(szBuff);
	//::OutputDebugString(L",");
	//::_itow_s(pt.y,szBuff,10);
	//::OutputDebugString(szBuff);
	//::OutputDebugString(L"\n");

	if(m_uShownCnt > uDisplayIndex)
	{
		//
		// the 1st step:
		// erase border of m_uPrevHovered item
		//
		if(m_vecContent.size() > m_uPrevHovered && m_uPrevHovered >=m_uTopItemIndex && m_uPrevHovered != (uDisplayIndex + m_uTopItemIndex))
		{
			::GetRgnBox(m_vecContent[m_uPrevHovered - m_uTopItemIndex]->hRgn, &rcItem);
			rcItem.right = m_uDispWidth - GAP;
			if(m_bShowVScroll)rcItem.right -= THUMB_WIDTH;
			::FillRect(m_hMemDC, &rcItem, m_hbrWhite);
			rcItem.left = rcItem.left + LEFT_MARGIN + TWO*GAP;

			::SelectObject(hWndDC, m_hFont);
			::DrawText(m_hMemDC, (LPCWSTR)(*m_vecContent[m_uPrevHovered]->pStr),
				m_vecContent[m_uPrevHovered]->pStr->GetLength(), &rcItem,
				DT_LEFT|DT_VCENTER|DT_SINGLELINE);
			rcItem.left = rcItem.left - LEFT_MARGIN - TWO*GAP;
			::BitBlt(hWndDC, LEFT_MARGIN, rcItem.top, rcItem.Width(), rcItem.Height(), m_hMemDC, rcItem.left, rcItem.top, SRCCOPY);
		}

		//
		// the 2nd step
		// draw border of hovered item
		//
		if(m_uPrevHovered != (uDisplayIndex + m_uTopItemIndex))
		{
			::GetRgnBox(m_vecContent[uDisplayIndex]->hRgn, &rcItem);
			rcItem.right = m_uDispWidth - TWO*BORDER_WIDTH;
			if(m_bShowVScroll)rcItem.right = rcItem.right - THUMB_WIDTH - TWO*BORDER_WIDTH;
			::FrameRect(m_hMemDC, &rcItem, m_hbrSelectedItem);
			::BitBlt(hWndDC, LEFT_MARGIN/* left position is fixed value */, rcItem.top, rcItem.Width(), rcItem.Height(), m_hMemDC, rcItem.left, rcItem.top, SRCCOPY);
			//::OutputDebugString(L"\nselection changed\n");
		}

		m_uPrevHovered = uDisplayIndex + m_uTopItemIndex;
	}

_CListWnd_OnMsgMouseMove_drawScrollbar:
	if(CVocabConst::UPTHUMB <= usHitCode)
	{
		DrawScrollbar(usHitCode);
		//::OutputDebugString(L"\_drawScrollbar:OTHER\n");
	}
	else
	{
		DrawScrollbar(CVocabConst::UNHOVERED);
		//::OutputDebugString(L"\_drawScrollbar:UNHOVERED\n");
	}
	::BitBlt(hWndDC, m_rcScroll.left, m_rcScroll.top, m_rcScroll.Width(), m_rcScroll.Height(), m_hMemDC, m_rcScroll.left, m_rcScroll.top, SRCCOPY);
	ReleaseDC(hWndDC);

	TRACKMOUSEEVENT tm;
	if(0xffffffff != lParam)
	{
		tm.cbSize = sizeof(TRACKMOUSEEVENT);
		tm.dwFlags = TME_LEAVE;
		tm.hwndTrack = m_hWnd;
		tm.dwHoverTime = 0;
		::TrackMouseEvent(&tm);
	}

	return 0;
}

LRESULT CListWnd::OnMsgPaint(WPARAM wParam, LPARAM lParam)
{
	PAINTSTRUCT ps;
	CRect rcWnd,rcItem;
	UINT nItemIndex;
	WCHAR szBuff[255];

	//::OutputDebugString(L"\nCListWnd::OnMsgPaint");
	BeginPaint(&ps);
	GetWindowRect(&rcWnd);
	rcWnd.OffsetRect(-rcWnd.left, -rcWnd.top);
	::SetBkMode(m_hMemDC,TRANSPARENT);
	::SelectObject(m_hMemDC, m_hFont);
	::SelectObject(m_hMemDC, m_hMemBitmap);
	::FillRect(m_hMemDC, &rcWnd, m_hbrWhite);
	::FrameRect(m_hMemDC, &rcWnd, m_hbrWndFrame);

	//::_itow_s(m_usPrevScrollState,szBuff,16);
	//	::OutputDebugString(L"\nm_usPrevScrollState:");
	//	::OutputDebugString(szBuff);
	//	::OutputDebugString(L"\n");

	// draw text of all shown items
	for(nItemIndex = 0;nItemIndex < m_uShownCnt;nItemIndex++)
	{
		::GetRgnBox(m_vecContent[nItemIndex]->hRgn, &rcItem);
		rcItem.left += LEFT_MARGIN;	// margin between window border and item border
		rcItem.left += TWO*GAP;	// margin between item border and item text
		::DrawText(m_hMemDC,(LPCWSTR)(*m_vecContent[nItemIndex + m_uTopItemIndex]->pStr),
			m_vecContent[nItemIndex + m_uTopItemIndex]->pStr->GetLength(),
			&rcItem, DT_LEFT|DT_VCENTER|DT_SINGLELINE);
	}

	// draw border of selected item if it exists
	if(CVocabConst::INVALID_INDEX != m_uPrevHovered)
	{
		if(m_uPrevHovered >= m_uTopItemIndex && m_uShownCnt > (m_uPrevHovered - m_uTopItemIndex))
		{
			::GetRgnBox(m_vecContent[m_uPrevHovered - m_uTopItemIndex]->hRgn, &rcItem);
			rcItem.right = m_uDispWidth - TWO*BORDER_WIDTH/* gap between item border and listwnd border*/;
			if(m_bShowVScroll)rcItem.right = rcItem.right - THUMB_WIDTH - TWO*BORDER_WIDTH/* gap between item border and scrollbar */;
			::FrameRect(m_hMemDC, &rcItem, m_hbrSelectedItem);
		}
	}

	if(m_bShowVScroll)
	{
		if(m_bDragging)
		{
			DrawScrollbar(CVocabConst::SLIDER_LBD);
		}
		else
		{
			//::OutputDebugString(L"WM_PAINT:call DrawScrollbar\n");
			SHORT sKeyState = ::GetAsyncKeyState(VK_LBUTTON);
			BOOL bLeftButtonDown = ((0 == (sKeyState & 0x8000)) ? 0:1);
			UINT usHitCode;
			CPoint pt;
			::GetCursorPos(&pt);
			ScreenToClient(&pt);
			usHitCode = HitTest(pt);
			DrawScrollbar(usHitCode+bLeftButtonDown);
		}
	}
	::BitBlt(ps.hdc, 0, 0, rcWnd.Width(), rcWnd.Height(), m_hMemDC, 0, 0, SRCCOPY);
	EndPaint(&ps);

	return 0;
}

//
// pt must be in client coordinate
//
USHORT CListWnd::HitTest(CPoint const& pt)
{
	DWORD dwCode(HTCLIENT);
	list<PSCROLLCTRLAREA>::const_iterator iter,end;
	PSCROLLCTRLAREA psca;
	CRect rcWnd,rcSlide;

	GetWindowRect(&rcWnd);
	rcWnd.OffsetRect(-rcWnd.left,-rcWnd.top);

	if(FALSE == rcWnd.PtInRect(pt))
		return HTNOWHERE;

	if(m_bShowVScroll)
	{
		if(m_rcSlider.PtInRect(pt))
		{
			return CVocabConst::SLIDER;
		}

		iter = m_listScrollArea.begin();
		end = m_listScrollArea.end();
		while(iter != end)
		{
			psca = *iter;
			if(psca->prcArea->PtInRect(pt))
			{
				dwCode = psca->dwHitCode;
				break;
			}
			iter++;
		}
		if(HTCLIENT != dwCode)
		{
			return dwCode;
		}
	}
	return HTCLIENT;
}

int CListWnd::DrawTopArrow(HBRUSH hBrush)
{
	CRect rcThumb;
	int xStart, yStart, xEnd, yEnd;

	rcThumb = m_rcScroll;
	rcThumb.bottom = rcThumb.top + THUMB_HEIGHT;
	::FillRect(m_hMemDC, &rcThumb, hBrush);

	rcThumb = m_rcScroll;
	rcThumb.left = rcThumb.right - THUMB_WIDTH;
	rcThumb.top += ARROW_MARGIN;

	xStart = rcThumb.left + (THUMB_WIDTH - ARROW_WIDTH)/TWO;
	yStart = rcThumb.top + (THUMB_HEIGHT - ARROW_HEIGHT)/TWO;
	::MoveToEx(m_hMemDC, xStart, yStart, 0);
	xEnd = xStart + ARROW_WIDTH/TWO;
	yEnd = yStart - ARROW_HEIGHT + BORDER_WIDTH;
	::LineTo(m_hMemDC, xEnd, yEnd);
	xEnd += ARROW_WIDTH/TWO + BORDER_WIDTH;
	yEnd += ARROW_HEIGHT;
	::LineTo(m_hMemDC, xEnd, yEnd);

	xStart = rcThumb.left + (THUMB_WIDTH - ARROW_WIDTH)/TWO;
	yStart = rcThumb.top + (THUMB_HEIGHT - ARROW_HEIGHT)/TWO + SCROLLBAR_MARGIN;
	::MoveToEx(m_hMemDC, xStart, yStart, 0);
	xEnd = xStart + ARROW_WIDTH/TWO;
	yEnd = yStart - ARROW_HEIGHT + BORDER_WIDTH;
	::LineTo(m_hMemDC, xEnd, yEnd);
	xEnd += ARROW_WIDTH/TWO + BORDER_WIDTH;
	yEnd += ARROW_HEIGHT;
	::LineTo(m_hMemDC, xEnd, yEnd);

	xStart = rcThumb.left + (THUMB_WIDTH - ARROW_WIDTH)/TWO;
	yStart = rcThumb.top + (THUMB_HEIGHT - ARROW_HEIGHT)/TWO + SCROLLBAR_MARGIN + SCROLLBAR_MARGIN;
	::MoveToEx(m_hMemDC, xStart, yStart, 0);
	xEnd = xStart + ARROW_WIDTH/TWO;
	yEnd = yStart - ARROW_HEIGHT + BORDER_WIDTH;
	::LineTo(m_hMemDC, xEnd, yEnd);
	xEnd += ARROW_WIDTH/TWO + BORDER_WIDTH;
	yEnd += ARROW_HEIGHT;
	::LineTo(m_hMemDC, xEnd, yEnd);

	return 0;
}

int CListWnd::DrawBottomArrow(HBRUSH hBrush)
{
	CRect rcThumb;
	int xStart, yStart, xEnd, yEnd;

	rcThumb = m_rcScroll;
	rcThumb.top = rcThumb.bottom - THUMB_HEIGHT;
	::FillRect(m_hMemDC, &rcThumb, hBrush);

	xStart = rcThumb.left + (THUMB_WIDTH - ARROW_WIDTH)/TWO;
	yStart = rcThumb.bottom - (THUMB_HEIGHT - ARROW_HEIGHT)/TWO - ARROW_MARGIN;
	::MoveToEx(m_hMemDC, xStart, yStart, 0);
	xEnd = xStart + ARROW_WIDTH/TWO;
	yEnd = yStart + ARROW_HEIGHT - SCROLLBAR_MARGIN;
	::LineTo(m_hMemDC, xEnd, yEnd);
	xEnd += ARROW_WIDTH/TWO + BORDER_WIDTH;
	yEnd -= ARROW_HEIGHT;
	::LineTo(m_hMemDC, xEnd, yEnd);

	xStart = rcThumb.left + (THUMB_WIDTH - ARROW_WIDTH)/TWO;
	yStart = rcThumb.bottom - (THUMB_HEIGHT - ARROW_HEIGHT)/TWO - ARROW_MARGIN - SCROLLBAR_MARGIN;
	::MoveToEx(m_hMemDC, xStart, yStart, 0);
	xEnd = xStart + ARROW_WIDTH/TWO;
	yEnd = yStart + ARROW_HEIGHT - SCROLLBAR_MARGIN;
	::LineTo(m_hMemDC, xEnd, yEnd);
	xEnd += ARROW_WIDTH/TWO + BORDER_WIDTH;
	yEnd -= ARROW_HEIGHT;
	::LineTo(m_hMemDC, xEnd, yEnd);

	xStart = rcThumb.left + (THUMB_WIDTH - ARROW_WIDTH)/TWO;
	yStart = rcThumb.bottom - (THUMB_HEIGHT - ARROW_HEIGHT)/TWO - ARROW_MARGIN - SCROLLBAR_MARGIN - SCROLLBAR_MARGIN;
	::MoveToEx(m_hMemDC, xStart, yStart, 0);
	xEnd = xStart + ARROW_WIDTH/TWO;
	yEnd = yStart + ARROW_HEIGHT - SCROLLBAR_MARGIN;
	::LineTo(m_hMemDC, xEnd, yEnd);
	xEnd += ARROW_WIDTH/TWO + BORDER_WIDTH;
	yEnd -= ARROW_HEIGHT;
	::LineTo(m_hMemDC, xEnd, yEnd);

	return 0;
}

BOOL CListWnd::DrawScrollbar(UINT uState)
{
	CRect rcScroll;
	WCHAR szBuff[_MAX_PATH];

	if(uState == m_usPrevScrollState)
	{
		//::OutputDebugString(L"\n DrawScrollbar:NO REDRAW");
		return FALSE;
	}

	UINT uMaxTopIndex = (UINT)m_vecContent.size() - m_uShownCnt;
	if(0 >= uMaxTopIndex)return 0;

	UINT uSliderHeight = m_rcSlider.Height();
	m_rcSlider.top = m_rcScroll.top + THUMB_HEIGHT + m_uTopItemIndex*m_uScrollingDistance / uMaxTopIndex;
	m_rcSlider.bottom = m_rcSlider.top + uSliderHeight;

	//::OutputDebugString(L"\DrawScrollbar():uState:");
	//::_itow_s(uState,szBuff,16);
	//::OutputDebugString(szBuff);
	////::OutputDebugString(L"\t,LBD:");
	////::_itow_s(bLeftButtonDown,szBuff,10);
	////::OutputDebugString(szBuff);
	//::OutputDebugString(L"\n");

	// whole scroll control area background
	::FillRect(m_hMemDC, &m_rcScroll, m_hbrScrollbarBG);
	//::OutputDebugString(L"\n DrawScrollbar:draw background");

	switch(uState)
	{
	case CVocabConst::UP_HOVERED:
		::SelectObject(m_hMemDC, m_hBlackPen);
		DrawTopArrow(m_hbrThumbHovered);
		DrawBottomArrow(m_hbrScrollbarBG);
		::FillRect(m_hMemDC, &m_rcSlider, m_hbrSlider);
		//::OutputDebugString(L"\n DrawScrollbar():UP_HOVERED");
		break;

	case CVocabConst::UP_LBD:
		::SelectObject(m_hMemDC, m_hWhitePen);
		DrawTopArrow(m_hbrThumbLBD);
		::SelectObject(m_hMemDC, m_hBlackPen);
		DrawBottomArrow(m_hbrScrollbarBG);
		::FillRect(m_hMemDC, &m_rcSlider, m_hbrSlider);
		//::OutputDebugString(L"\n DrawScrollbar():UP_LBD");
		break;

	case CVocabConst::DOWN_HOVERED:
		::SelectObject(m_hMemDC, m_hBlackPen);
		DrawTopArrow(m_hbrScrollbarBG);
		DrawBottomArrow(m_hbrThumbHovered);
		::FillRect(m_hMemDC, &m_rcSlider, m_hbrSlider);
		//::OutputDebugString(L"\n DrawScrollbar():DOWN_HOVERED");
		break;

	case CVocabConst::DOWN_LBD:
		::SelectObject(m_hMemDC, m_hBlackPen);
		DrawTopArrow(m_hbrScrollbarBG);
		::SelectObject(m_hMemDC, m_hWhitePen);
		DrawBottomArrow(m_hbrThumbLBD);
		::FillRect(m_hMemDC, &m_rcSlider, m_hbrSlider);
		//::OutputDebugString(L"\n DrawScrollbar():DOWN_LBD");
		break;

	case CVocabConst::SLIDER_HOVERED:
		::SelectObject(m_hMemDC, m_hBlackPen);
		DrawTopArrow(m_hbrScrollbarBG);
		DrawBottomArrow(m_hbrScrollbarBG);
		::FillRect(m_hMemDC, &m_rcSlider, m_hbrSliderHoverd);
		//::OutputDebugString(L"\n DrawScrollbar():SLIDER_HOVERED");
		break;

	case CVocabConst::SLIDER_LBD:
		::SelectObject(m_hMemDC, m_hBlackPen);
		DrawTopArrow(m_hbrScrollbarBG);
		DrawBottomArrow(m_hbrScrollbarBG);
		::FillRect(m_hMemDC, &m_rcSlider, m_hbrThumbLBD);
		//::OutputDebugString(L"\n DrawScrollbar():SLIDER_LBD");
		break;

	case CVocabConst::UNHOVERED:
	case HTNOWHERE + 1:
	case HTNOWHERE:
		::SelectObject(m_hMemDC, m_hBlackPen);
		DrawTopArrow(m_hbrScrollbarBG);
		DrawBottomArrow(m_hbrScrollbarBG);
		::FillRect(m_hMemDC, &m_rcSlider, m_hbrSlider);
		//::OutputDebugString(L"\n DrawScrollbar():TRACK");break;
	}

	m_usPrevScrollState = uState;

	return TRUE;
}

int CListWnd::AdjustPosition()
{
	int nScreenWidth, nScreenHeight;
	UINT nAvailableHeight, nHeight1, nHeight2, nTotalHeight;
	CRect rcCombo, rcListWnd;
	APPBARDATA abd;
	
	nScreenWidth = ::GetSystemMetrics(SM_CXSCREEN);
	nScreenHeight = ::GetSystemMetrics(SM_CYSCREEN);
	::GetWindowRect(m_hComboWnd, &rcCombo);

	//
	// the list window should not be covered by the taskbar
	// if the taskbar is at top or bottom,decrease the height of list window
	//
	abd.cbSize = sizeof(APPBARDATA);
	abd.hWnd = 0;
	::SHAppBarMessage(ABM_GETTASKBARPOS, &abd);

	// determine the top coordinates
	if(ABE_TOP == abd.uEdge)
	{
		nHeight1 = rcCombo.top - abd.rc.bottom;
		nHeight2 = nScreenHeight - rcCombo.bottom;
	}
	if(ABE_BOTTOM == abd.uEdge)
	{
		nHeight1 = rcCombo.top;
		nHeight2 = abd.rc.top - rcCombo.bottom;
	}

	nAvailableHeight = max(nHeight1,nHeight2);

	if(0 < m_vecContent.size())
	{
		::GetRgnBox(m_vecContent[0]->hRgn, &rcListWnd);
		nTotalHeight = m_vecContent.size() * rcListWnd.Height();
	}
	else
	{
		// if there is no item,show only one blank item
		nTotalHeight = ICON_HEIGHT;
	}
	m_uDispHeight = nTotalHeight + TWO*BORDER_WIDTH + TWO*TOP_MARGIN/* top&bottom margin */;

	// if nHeight2 < nHeight1,this window will be displayed above the top of combo
	if(nHeight2 < nHeight1)
	{
		rcListWnd.bottom = rcCombo.top;
		rcListWnd.top = rcListWnd.bottom - min(nAvailableHeight, m_uDispHeight);
	}
	else
	{
		// this window will be displayed below the bottom of combo
		rcListWnd.top = rcCombo.bottom;
		rcListWnd.bottom = rcListWnd.top + min(nAvailableHeight, m_uDispHeight);
	}

	m_bShowVScroll = (nAvailableHeight < nTotalHeight);

	//
	// adjust list window size&position
	//

	// the window width is equal to the combo and the left edge align with the combo
	rcListWnd.left = rcCombo.left;
	rcListWnd.right = rcListWnd.left + rcCombo.Width();

	TurntoIntegerMultiple(rcListWnd, nHeight2 < nHeight1);
	m_uDispWidth = rcListWnd.Width();
	m_uDispHeight = rcListWnd.Height();

	MoveWindow(rcListWnd.left, rcListWnd.top, rcListWnd.Width(), rcListWnd.Height());
	if(rcListWnd.Width() * rcListWnd.Height() > 0)
	{
		::PostMessage(m_hComboWnd, CVocabConst::MSG_RCVLISTWNDSTATE, TRUE, 0);
	}

	if(TRUE == m_bShowVScroll)
	{
		UpdateScrollCtrlArea(rcListWnd);
	}

	return 0;
}

UINT CListWnd::TurntoIntegerMultiple(CRect &rcWnd, BOOL bShownAbove)
{
	if(0 == m_uItemHeight)return -1;

	// the list window should display an integer multiple count of item
	UINT uReduction;

	// substract border&margin,then the uReduction is integer multiple of m_uItemHeight
	uReduction = rcWnd.Height() - TWO*BORDER_WIDTH - TWO*TOP_MARGIN;
	m_uShownCnt = uReduction / m_uItemHeight;
	uReduction = uReduction % m_uItemHeight;

	// substract the uReduction from the window height
	if(bShownAbove)
	{
		rcWnd.top = rcWnd.top + uReduction;
	}
	else 
	{
		rcWnd.bottom = rcWnd.bottom - uReduction;
	}

	return m_uShownCnt;
}

int CListWnd::UpdateScrollCtrlArea(CRect const& rcWnd)
{
	list<PSCROLLCTRLAREA>::const_iterator iter, end;
	CRect rc;
	PSCROLLCTRLAREA psca;

	iter = m_listScrollArea.begin();
	end = m_listScrollArea.end();
	while(iter != end)
	{
		delete (*iter)->prcArea;
		delete (*iter);
		iter++;
	}
	m_listScrollArea.clear();

	// up-thumb
	rc = rcWnd;
	ScreenToClient(&rc);
	rc.right = rc.right - BORDER_WIDTH;
	rc.left = rc.right - THUMB_WIDTH;
	rc.top = rc.top + BORDER_WIDTH;
	rc.bottom = rc.top + THUMB_HEIGHT;
	psca = new SCROLLCTRLAREA;
	psca->dwHitCode = CVocabConst::UPTHUMB;
	psca->prcArea = new CRect(rc);
	m_listScrollArea.push_back(psca);

	// down-thumb
	rc = rcWnd;
	ScreenToClient(&rc);
	rc.right = rc.right - BORDER_WIDTH;
	rc.left = rc.right - THUMB_WIDTH;
	rc.bottom = rc.bottom - BORDER_WIDTH;
	rc.top = rc.bottom - THUMB_HEIGHT;
	psca = new SCROLLCTRLAREA;
	psca->dwHitCode = CVocabConst::DOWNTHUMB;
	psca->prcArea = new CRect(rc);
	m_listScrollArea.push_back(psca);

	// the whole scroll control area,this area must be at the last
	m_rcScroll = rcWnd;
	ScreenToClient(&m_rcScroll);
	m_rcScroll.right = m_rcScroll.right - BORDER_WIDTH;
	m_rcScroll.left = m_rcScroll.right - THUMB_WIDTH;
	m_rcScroll.top = m_rcScroll.top + BORDER_WIDTH;
	m_rcScroll.bottom = m_rcScroll.bottom - BORDER_WIDTH;
	psca = new SCROLLCTRLAREA;
	psca->dwHitCode = CVocabConst::TRACK;
	psca->prcArea = new CRect(m_rcScroll);
	m_listScrollArea.push_back(psca);

	// slider
	// m_listScrollArea does NOT contain this area
	m_rcSlider.right = m_rcScroll.right - LEFT_MARGIN;
	m_rcSlider.left = m_rcSlider.right - SLIDER_WIDTH;
	m_rcSlider.top = m_rcScroll.top + THUMB_HEIGHT;
	m_rcSlider.bottom = m_rcSlider.top + rcWnd.Height() / 3;	// slider height:1/3 of scroll control area
	m_uScrollingDistance = m_rcScroll.bottom - m_rcSlider.bottom - THUMB_HEIGHT;

	if(m_vecContent.size() > m_uShownCnt)
	{
		m_rcSlider.top = m_rcSlider.top + m_uTopItemIndex*m_uScrollingDistance / (m_vecContent.size() - m_uShownCnt);
		m_rcSlider.bottom = m_rcSlider.top + rcWnd.Height() / 3;
	}

	return m_listScrollArea.size();
}

int CListWnd::Show(BOOL bShow)
{
	if(bShow)
	{
		// when this window is about to be displayed,do not handle the first WM_LBUTTONUP message after displaying
		m_bHandleLBUp = FALSE;

		// calculate m_uShownCnt,m_uDispWidth etc. first
		AdjustPosition();

		if((UINT)CVocabConst::INVALID_INDEX != m_uSelectedItem)
		{
			if(m_vecContent.size() - m_uShownCnt >= m_uSelectedItem)
			{
				m_uTopItemIndex = m_uSelectedItem;
			}
			else
			{
				m_uTopItemIndex = m_vecContent.size() - m_uShownCnt;
			}
			m_uPrevHovered = m_uSelectedItem;
		}

		SetCapture();	// must catch mouse
		m_dwLastScrollingTime = 0;
		m_usPrevScrollState = CVocabConst::FORCETODRAW;
		HDC hScreenDC = ::GetWindowDC(0);
		m_hMemBitmap = ::CreateCompatibleBitmap(hScreenDC, m_uDispWidth, m_uDispHeight);
		::ReleaseDC(0, hScreenDC);
		::_itow_s((int)m_hWnd, szBuff, 16);
		//::OutputDebugString(L"\nSHOW LIST:\t");::OutputDebugString(szBuff);::OutputDebugString(L"\n");
	}
	else
	{
		CRect rcCombo;
		::GetWindowRect(m_hComboWnd, &rcCombo);
		::PostMessage(m_hComboWnd, CVocabConst::MSG_RCVLISTWNDSTATE, FALSE, 0);
		ShowWindow(SW_HIDE);
		m_uPrevHovered = -1;
		if(0 != m_hMemBitmap)::DeleteObject(m_hMemBitmap);
	}
	return 0;
}

LRESULT CListWnd::OnMsgNcHittest(WPARAM wParam,LPARAM lParam)
{
	if(0 == m_listScrollArea.size())return HTCLIENT;
	CPoint pt;

	pt.x = GET_X_LPARAM(lParam);
	pt.y = GET_Y_LPARAM(lParam);
	ScreenToClient(&pt);

	// return HTVSCROLL if the mouse is in the scroll control area,otherwise return HTCLIENT
	if(m_listScrollArea.back()->prcArea->PtInRect(pt))
	{
		//::OutputDebugString(L"\nhittest = scroll\n");
		return HTVSCROLL;
	}
	
	return HTCLIENT;
}

int CListWnd::SingleScroll(DWORD dwHitCode)
{
	::OutputDebugString(L"\nsingle scroll");
	UINT uHeight;

	if(CVocabConst::UPTHUMB == dwHitCode && 0 == m_uTopItemIndex)return 0;
	if(CVocabConst::DOWNTHUMB == dwHitCode && (m_uShownCnt + m_uTopItemIndex) == m_vecContent.size())return 0;

	if(CVocabConst::UPTHUMB == dwHitCode)
	{
		m_uTopItemIndex --;
	}
	if(CVocabConst::DOWNTHUMB == dwHitCode)
	{
		m_uTopItemIndex ++;
	}
	uHeight = m_rcSlider.Height();
	m_rcSlider.top = m_rcScroll.top + THUMB_HEIGHT + m_uTopItemIndex*m_uScrollingDistance / (m_vecContent.size() - m_uShownCnt);
	m_rcSlider.bottom = m_rcSlider.top + uHeight;
	if(m_rcSlider.bottom > m_rcScroll.bottom - THUMB_HEIGHT)
	{
		m_rcSlider.bottom = m_rcScroll.bottom - THUMB_HEIGHT;
		m_rcSlider.top = m_rcSlider.bottom - uHeight;
	}
	m_usPrevScrollState = CVocabConst::FORCETODRAW;

	Invalidate(TRUE);

	return m_uTopItemIndex;
}

int CListWnd::DragSlider()
{
	MSG msg;
	UINT uSliderHeight;
	UINT uMinScrollOffset;
	CPoint ptPrev,ptNow;
	::OutputDebugString(L"\nDragSlider()");
	uSliderHeight = m_rcSlider.Height();
	UINT uMaxTopIndex = (UINT)m_vecContent.size() - m_uShownCnt;
	if(0 >= uMaxTopIndex)return 0;

	TRACKMOUSEEVENT tm;
	tm.cbSize = sizeof(TRACKMOUSEEVENT);
	tm.dwFlags = TME_LEAVE;
	tm.hwndTrack = m_hWnd;
	tm.dwHoverTime = 0;
	::TrackMouseEvent(&tm);

	uMinScrollOffset = m_uScrollingDistance / uMaxTopIndex;
	if(MIN_DRAG_OFFSET > uMinScrollOffset)uMinScrollOffset = MIN_DRAG_OFFSET;

	// convert to client coordinates
	ptPrev = globalVar.g_ptLBtnDown;
	ScreenToClient(&ptPrev);
	m_bDragging =TRUE;
	UINT uOrgTopIndex = m_uTopItemIndex;

	while(TRUE)
	{
		::GetMessage(&msg,m_hWnd,0,0);
		
		if(WM_MOUSELEAVE == msg.message)
		{
			//::OutputDebugString(L"//////////////////////////////////////////");
			break;
		}
		if(WM_PAINT == msg.message)
		{
			OnMsgPaint(msg.wParam,msg.lParam);
			continue;
		}
		if(WM_MOUSEMOVE == msg.message)
		{
			ptNow.x = GET_X_LPARAM(msg.lParam);
			ptNow.y = GET_Y_LPARAM(msg.lParam);
			//::_itow_s(ptNow.x,szBuff,10);
			//::OutputDebugString(L":");::OutputDebugString(szBuff);::OutputDebugString(L",");
			//::_itow_s(ptNow.y,szBuff,10);::OutputDebugString(szBuff);::OutputDebugString(L"\n");
			if(MAX_OFFSET_X < abs(ptNow.x - m_rcScroll.left))
			{
				//::OutputDebugString(L"\n outof scrollbar");
				continue;
			}

			if(uMinScrollOffset <= abs(ptNow.y - ptPrev.y))
			{
				m_uTopItemIndex = uOrgTopIndex + (ptNow.y - ptPrev.y)*((LONG)uMaxTopIndex)/((LONG)m_uScrollingDistance);

				if(0x80000000 & m_uTopItemIndex)	// if 0 > m_uTopItemIndex
				{
					m_uTopItemIndex = 0;
				}					
				if(uMaxTopIndex <= m_uTopItemIndex)
				{
					m_uTopItemIndex = uMaxTopIndex;
				}

				//::_itow_s(ptNow.y - ptPrev.y,szBuff,10);
				//::OutputDebugString(L"deltaY:");::OutputDebugString(szBuff);::OutputDebugString(L"\t");
				//::_itow_s(m_uTopItemIndex,szBuff,10);
				//::OutputDebugString(L"m_uTopItemIndex:");::OutputDebugString(szBuff);::OutputDebugString(L"\n");

				m_rcSlider.top = m_rcScroll.top + THUMB_HEIGHT + m_uTopItemIndex*m_uScrollingDistance / uMaxTopIndex;
				m_rcSlider.bottom = m_rcSlider.top + uSliderHeight;
				if(m_rcSlider.bottom > m_rcScroll.bottom - THUMB_HEIGHT)
				{
					m_rcSlider.bottom = m_rcScroll.bottom - THUMB_HEIGHT;
					m_rcSlider.top = m_rcSlider.bottom - uSliderHeight;
				}
				m_usPrevScrollState = CVocabConst::FORCETODRAW;
				Invalidate(TRUE);
				//::OutputDebugString(L"scrolling\n");
			}
			else
			{
				// do not scroll,move slider only
				if(MIN_DRAG_OFFSET <= abs(ptNow.y - ptPrev.y))
				{
					m_rcSlider.top += ptNow.y - ptPrev.y;
					m_rcSlider.bottom = m_rcSlider.top + uSliderHeight;
					if(m_rcSlider.bottom > (m_rcScroll.bottom - THUMB_HEIGHT))
					{
						m_rcSlider.bottom = m_rcScroll.bottom - THUMB_HEIGHT;
						m_rcSlider.top = m_rcSlider.bottom - uSliderHeight;
					}
					m_usPrevScrollState = CVocabConst::FORCETODRAW;
					Invalidate(TRUE);
					//::OutputDebugString(L"dragging\n");
				}
			}
			
			continue;
		}

		if(WM_LBUTTONUP == msg.message)
		{
			m_bDragging = FALSE;

			// when left button is released,draw hovered state of the item
			m_usPrevScrollState = CVocabConst::FORCETODRAW;
			Invalidate(TRUE);
			uOrgTopIndex = m_uTopItemIndex;
			//ReleaseCapture();
			break;
		}
	}

	return 0;
}

int CListWnd::DoScroll(DWORD dwHitCode)
{
	MSG msg;

	CPoint pt;

	//
	// step1:do a single scroll first
	//
	SingleScroll(dwHitCode);

	//
	// step2:check mouse button state&position continuously and do more scroll
	//
	UINT uTimerID1 = (UINT)::GetCurrentProcessId();
	UINT uTimerID2 = (UINT)::GetCurrentThreadId();

	// 500ms time interval between continuously scrolling and left button first pressed
	SetTimer(uTimerID1,TIMER1_ELAPSE);

	while(TRUE)
	{
		::GetMessage(&msg, m_hWnd, 0, 0);
		if(WM_PAINT  == msg.message)
		{
			OnMsgPaint(msg.wParam, msg.lParam);
			continue;
		}

		if(WM_MOUSEMOVE == msg.message)
		{
			pt.x = GET_X_LPARAM(msg.lParam);
			pt.y = GET_Y_LPARAM(msg.lParam);
			if(dwHitCode != HitTest(pt))
			{
				::Sleep(TIMER2_ELAPSE);
				//::OutputDebugString(L"\nleave");
				continue;
			}
		}

		if(WM_LBUTTONUP == msg.message)
		{
			//SendMessage(WM_VSCROLL,SB_ENDSCROLL,0);
			//::OutputDebugString(L"\nEND scrolling,left button up");

			m_usPrevScrollState = CVocabConst::FORCETODRAW;
			Invalidate();
			break;
		}

		if(WM_TIMER == msg.message)
		{
			if(uTimerID1 == (UINT)msg.wParam)
			{
				//::OutputDebugString(L"\ntimer1");
				KillTimer(uTimerID1);
				SetTimer(uTimerID2,TIMER2_ELAPSE);
				m_usPrevScrollState = CVocabConst::FORCETODRAW;
				SingleScroll(dwHitCode);
				m_dwLastScrollingTime = ::GetTickCount();
				continue;
			}
			//if(uTimerID2 == (UINT)msg.wParam)::OutputDebugString(L"\ntimer2\n");
			GetCursorPos(&pt);
			ScreenToClient(&pt);
			if(dwHitCode != HitTest(pt))
			{
				::Sleep(TIMER2_ELAPSE);
				::OutputDebugString(L"\nmove out");
				continue;
			}

			// left button is keeping pressed
			if(::GetAsyncKeyState(VK_LBUTTON) & VK_STATE_LBTNDOWN)
			{
				if(::GetTickCount() - m_dwLastScrollingTime > TIMER2_ELAPSE)
				{
					//::OutputDebugString(L"\ncontinuously scrolling\t");
					//::_itow_s(nScrollStep++,szBuff,10);
					//::OutputDebugString(szBuff);
					//OutputDebugString(L"\n");
					SingleScroll(dwHitCode);
					m_dwLastScrollingTime = ::GetTickCount();
				}
				else
				{
					::Sleep(TIMER2_ELAPSE);	// test mouse button about 33 times per second
				}
			}
			else continue;
		}
	}

	KillTimer(uTimerID1);
	KillTimer(uTimerID2);

	return 0;
}

LRESULT CListWnd::OnMsgLButtonDown(WPARAM wParam,LPARAM lParam)
{
	//::OutputDebugString(L"\nCListWnd::OnMsgLButtonDown");

	CRect rcWnd;
	CPoint pt;
	DWORD dwHitCode;

	pt.x = GET_X_LPARAM(lParam);
	pt.y = GET_Y_LPARAM(lParam);

	dwHitCode = HitTest(pt);

	// when left button has been clicked out of this window,hide this window
	if(HTNOWHERE == dwHitCode)
	{
		//::OutputDebugString(L"\nCListWnd::OnMsgLButtonDown : HTNOWHERE");
		::ReleaseCapture();
		Show(0);
		return 0;
	}

	if(CVocabConst::UPTHUMB == dwHitCode || CVocabConst::DOWNTHUMB == dwHitCode)
	{
		//::OutputDebugString(L"\nCListWnd::OnMsgLButtonDown : call DoScroll");
		DoScroll(dwHitCode);
	}
	if(CVocabConst::SLIDER == dwHitCode)
	{
		//::OutputDebugString(L"\nCListWnd::OnMsgLButtonDown : call DragSlider");
		InvalidateRect(&m_rcSlider);
		DragSlider();
	}

	return 0;
}

LRESULT CListWnd::OnMsgLButtonUp(WPARAM wParam,LPARAM lParam)
{
	//::OutputDebugString(L"\nCListWnd::OnMsgLButtonUp");
	CFlatDropList *pCom = (CFlatDropList *)CWnd0::FromHandle(m_hComboWnd);
	if(FALSE == m_bHandleLBUp)
	{
		m_bHandleLBUp =TRUE;
		//::OutputDebugString(L"\nCListWnd::OnMsgLButtonUp do not handled");
		return 0;
	}

	CRect rcWnd;
	CPoint pt;
	DWORD dwHitCode;

	pt.x = GET_X_LPARAM(lParam);
	pt.y = GET_Y_LPARAM(lParam);

	// must be in client coordinate
	dwHitCode = HitTest(pt);

	if(HTNOWHERE == dwHitCode)
	{
		goto _OnMsgLButtonDown_exit;
	}

	if(HTCLIENT != dwHitCode)
	{
		//::OutputDebugString(L"\nLB UP in SCROLL BAR\n");
		return 0;
	}

	GetWindowRect(&rcWnd);
	ScreenToClient(&rcWnd);
	if(rcWnd.PtInRect(pt))
	{
		UINT uDisplayIndex = GetDispIndexFromPoint(pt);
		if(0 <= uDisplayIndex && m_vecContent.size() > uDisplayIndex)
		{
			m_uSelectedItem = uDisplayIndex + m_uTopItemIndex;
			m_uPrevHovered = m_uSelectedItem;
			if(m_vecContent.size() - m_uShownCnt >= m_uSelectedItem)
			{
				m_uTopItemIndex = m_uSelectedItem;
			}
			else
			{
				m_uTopItemIndex = m_vecContent.size() - m_uShownCnt;
			}

			::PostMessage(m_hComboWnd, CVocabConst::MSG_SELCHANGED, 0, m_uSelectedItem);

			// to notify combo's parent window(main window or dialog etc.) and then hide this window
			::PostMessage(::GetParent(m_hComboWnd),WM_COMMAND,MAKELONG((0 == pCom) ? 0:pCom->GetID(),CBN_SELENDOK),(LPARAM)m_hComboWnd);
			//::PostMessage(::GetParent(m_hComboWnd),WM_COMMAND,MAKELONG((0 == pCom) ? 0:pCom->GetID(),CBN_CLOSEUP),(LPARAM)m_hComboWnd);
			//::PostMessage(::GetParent(m_hComboWnd),WM_COMMAND,MAKELONG((0 == pCom) ? 0:pCom->GetID(),CBN_SELCHANGE),(LPARAM)m_hComboWnd);
		}
	}
	

_OnMsgLButtonDown_exit:
	::PostMessage(m_hComboWnd, CVocabConst::MSG_RCVLISTWNDSTATE, FALSE, 0);
	::ReleaseCapture();
	Show(FALSE);

	return 0;
}

LRESULT CListWnd::WndProc(UINT uMsg,WPARAM wParam,LPARAM lParam)
{
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
	case WM_CAPTURECHANGED:
		Show(FALSE);
		return 0;

	case WM_NCCREATE:
		return OnMsgNcCreate(wParam, lParam);

	//case WM_ERASEBKGND:
	//	::OutputDebugString(L"\nCListWnd::WM_ERASEBKGND");
	//	return TRUE;

	case WM_PAINT:
		return OnMsgPaint(wParam, lParam);

	case WM_NCDESTROY:
		return OnMsgNcDestroy(wParam, lParam);

	case WM_MOUSEMOVE:
		return OnMsgMouseMove(wParam, lParam);

	case WM_MOUSEWHEEL:
		{
			SHORT zDelta = GET_WHEEL_DELTA_WPARAM(wParam);
			if(0x8000 & zDelta)SingleScroll(CVocabConst::DOWNTHUMB);
			else SingleScroll(CVocabConst::UPTHUMB);
			return 0;
		}

	case WM_MOUSELEAVE:
		return OnMsgMouseLeave(wParam, lParam);

	case WM_LBUTTONUP:
		return OnMsgLButtonUp(wParam, lParam);

	case WM_LBUTTONDOWN:
		return OnMsgLButtonDown(wParam, lParam);

	case WM_NCHITTEST:
		return OnMsgNcHittest(wParam, lParam);

	case WM_DISPLAYCHANGE:
		CListWnd::SCROLLBAR_WIDTH = ::GetSystemMetrics(SM_CXVSCROLL);
		CListWnd::SCROLLBAR_HEIGHT = ::GetSystemMetrics(SM_CYVSCROLL);
		return 0;

	case WM_SHOWWINDOW:
		if(0 != wParam)Show();
		return 0;

	//case WM_ACTIVATEAPP:
	//	if(0 == wParam)
	//	{
	//		Show(0);
	//	}
	//	return 0;

	//case WM_SETCURSOR:
	//	CPoint pt;DWORD dwCode;
	//	GetCursorPos(&pt);

	//	// pt must be in client coordinate
	//	ScreenToClient(&pt);
	//	dwCode = HitTest(pt);
	//	switch(dwCode)
	//	{
	//	case UPTHUMB:
	//	case DOWNTHUMB:
	//	case SLIDER:
	//	case TRACK:
	//		return ::PostMessage(m_hComboWnd,WM_SETCURSOR,(WPARAM)m_hComboWnd,MAKELONG(HTVSCROLL,WM_MOUSEMOVE));
	//	}
	//	return ::PostMessage(m_hComboWnd,WM_SETCURSOR,(WPARAM)m_hComboWnd,MAKELONG(HTCLIENT,WM_MOUSEMOVE));

	}

	return ::DefWindowProcW(m_hWnd, uMsg, wParam, lParam);
}
