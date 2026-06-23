
////////////////////////////////////////////////////////////////////////////////////////////

#include "vocab.h"
#include "wingdi.h"
#include "LabelEx.h"
#include "WorkThread.h"
#include "vocabularyDlg.h"
#include "Tools1.h"

////////////////////////////////////////////////////////////////////////////////////////////

#ifdef _DEBUG
#define new new(_NORMAL_BLOCK, __FILE__, __LINE__)
#endif

////////////////////////////////////////////////////////////////////////////////////////////


////////////////////////////////////////////////////////////////////////////////////////////

using namespace VocabApp;
#pragma comment(lib,"Msimg32.lib")

////////////////////////////////////////////////////////////////////////////////////////////

extern vocabGlobal globalVar;

////////////////////////////////////////////////////////////////////////////////////////////

int const CLabelEx::ICON_WIDTH = ::GetSystemMetrics(SM_CXICON);
int const CLabelEx::POPUP_FONT_HEIGHT = ::GetSystemMetrics(SM_CYVTHUMB);		// is about equal to icon height

const WCHAR	CLabelEx::DEFAULT_FONT_NAME[] = L"Gabriola";
const WCHAR	CLabelEx::ONE_CHAR[] = L"W";

HDC			CLabelEx::_LabelEx_hTmpDC;
HANDLE		CLabelEx::_LabelEx_hMenuBitmap;
HFONT		CLabelEx::_LabelEx_hfontMenu;
HHOOK		CLabelEx::_LabelEx_hCbtHook;
HWND		CLabelEx::_LabelEx_hWndMenuOwner/* the label itself */;
HWND		CLabelEx::_LabelEx_hWndPopMenu;

////////////////////////////////////////////////////////////////////////////////////////////

CSensibleInfo::CSensibleInfo() { pwstr = 0; }

CSensibleInfo::~CSensibleInfo() { }

////////////////////////////////////////////////////////////////////////////////////////////

CLabelEx::CLabelEx()
{
	HANDLE h=GetProcessHeap();
	m_hCursor = ::LoadCursor(NULL,IDC_ARROW);
	m_nBorderWidth = DEFAULT_BORDER_WIDTH;
	m_nXmargin = DEFAULT_X_MARGIN;
	m_nYmargin = DEFAULT_Y_MARGIN;
	m_nMinWidth = 0;
	m_nMinHeight = 0;
	m_nHAlignment = HLEFT;
	m_nVAlignment = VTOP;
	m_bShowUnderline = FALSE;
	m_nFontHeight = POPUP_FONT_HEIGHT;
	m_bShowBorder = TRUE;
	m_strFontName = L"arial";

	LOGFONT logFont;
	HDC hScreenDC = ::GetWindowDC(0);
	CString str;
	str = m_strFontName;
	::RtlSecureZeroMemory(&logFont,sizeof(LOGFONT));
	::RtlCopyMemory(logFont.lfFaceName, str.GetBuffer(), str.GetLength()*sizeof(WCHAR));
	m_hfontLabel = ::CreateFontIndirect(&logFont);
	logFont.lfHeight = ::GetDeviceCaps(hScreenDC, VERTSIZE);
	logFont.lfHeight = logFont.lfHeight * ::GetDeviceCaps(hScreenDC,LOGPIXELSY)*4/3;
	logFont.lfHeight = logFont.lfHeight/25.4f * POPUP_FONT_HEIGHT/::GetSystemMetrics(SM_CYSCREEN);
	_LabelEx_hfontMenu = ::CreateFontIndirect(&logFont);
	::ReleaseDC(0,hScreenDC);

	m_clrBackgroundColor = ::GetSysColor(COLOR_BTNFACE);
	m_clrPlainColor = PLAIN_COLOR;
	m_clrHoveredColor = DEFAULT_HOVER_COLOR;
	m_clrBorderColor = DEFAULT_BORDER_COLOR;
	m_clrUnderlineColor = DEFAULT_UNDERLINE_COLOR;

	m_logBrush.lbStyle = BS_SOLID;
	m_logBrush.lbColor = m_clrBackgroundColor;
	m_logBrush.lbHatch = 0;
	m_hBrushBorder = ::CreateSolidBrush(m_clrBorderColor);
	m_hBrushBkgnd = ::CreateBrushIndirect (&m_logBrush);
	m_hBrushPopMenu = 0;

	LOGPEN logPen;
	logPen.lopnStyle = PS_SOLID;
	logPen.lopnWidth.x = DEFAULT_UNDERLINE_WIDTH;
	logPen.lopnColor = DEFAULT_UNDERLINE_COLOR;
	m_hpenUnderline = ::CreatePenIndirect(&logPen);
	logPen.lopnWidth.x = m_nBorderWidth;
	logPen.lopnColor = m_clrBorderColor;
	m_hpenBorder = ::CreatePenIndirect(&logPen);
	logPen.lopnColor = RGB(127,127,0);
	m_hpenSelItem = ::CreatePenIndirect(&logPen);

	m_ulHitMessage = CVocabConst::MSG_RCV_SPELL;
	m_pPrevHoverd = 0;
	m_hWnd = 0;
	m_hPopMenu = 0;
}

CLabelEx::~CLabelEx()
{
	ClearStringInfo();
	::DeleteObject(m_hBrushBkgnd);
	::DeleteObject(m_hBrushBorder);
	::DeleteObject(m_hMemBitmap);
	::DeleteDC(m_hMemDC);
	::DeleteObject(m_hfontLabel);
	::DeleteObject(_LabelEx_hfontMenu);
	::DeleteObject(m_hpenUnderline);
	::DeleteObject(m_hpenBorder);
	::DeleteObject(m_hpenSelItem);
	::DestroyMenu(m_hPopMenu);
	if (0 != m_hBrushPopMenu)::DeleteObject(m_hBrushPopMenu);
}

COLORREF CLabelEx::SetTextColor(COLORREF color)
{
	m_clrPlainColor = color;
	return color;
}

COLORREF CLabelEx::SetBkColor(COLORREF color)
{
	m_clrBackgroundColor = color;
	m_logBrush.lbColor = m_clrBackgroundColor;
	::DeleteObject(m_hBrushBkgnd);
	m_hBrushBkgnd = ::CreateBrushIndirect (&m_logBrush);

	return color;
}

BOOL CLabelEx::SetFontHeight(int nHeight)
{
	m_nFontHeight = nHeight;
	::DeleteObject(m_hfontLabel);
	m_hfontLabel = Tools1::CreatePointFont(m_nFontHeight, m_strFontName);
	return NULL != m_hfontLabel;
}

BOOL CLabelEx::SetFontName(CStringW const& strFontName)
{
	m_strFontName.Empty();
	m_strFontName = strFontName;
	::DeleteObject(m_hfontLabel);
	m_hfontLabel = Tools1::CreatePointFont(m_nFontHeight, m_strFontName);
	return NULL != m_hfontLabel;
}

int CLabelEx::AddSensibleText(CSensibleInfo *pNewInfo)
{
	CRect rcPrev;
	CSize szStr,szComma;
	LONG lAvailableWidth;

	if (0 == pNewInfo)return 0;
	if (pNewInfo->pwstr->IsEmpty())return m_listStringInfo.size();

	lAvailableWidth = m_rcWnd.Width() - m_nBorderWidth*2 - m_nXmargin*2;
	::GetTextExtentPoint32(m_hMemDC, (LPCWSTR)(*(pNewInfo->pwstr)), pNewInfo->pwstr->GetLength(), &szStr);
	::GetTextExtentPoint32(m_hMemDC, CStringW(CVocabConst::COMMA), 1, &szComma);

	// label window is too small to show the string
	if(lAvailableWidth < szStr.cx)return 0;	

	if(0 == m_listStringInfo.size())
	{
		pNewInfo->rc.left = m_nBorderWidth + m_nXmargin;
		pNewInfo->rc.top = m_nBorderWidth + m_nYmargin;
		pNewInfo->rc.right = pNewInfo->rc.left + szStr.cx;
		pNewInfo->rc.bottom = pNewInfo->rc.top + szStr.cy;
	}

	if(m_listStringInfo.size() > 0)
	{
		LONG x;
		m_pPrevHoverd = m_listStringInfo.back();
		x = m_pPrevHoverd->rc.right + szComma.cx; // a comma will be append to previous string
		
		if(szStr.cx <= m_nAvailableWidth - x - szComma.cx)
		{
			// the string can be displayed in current line
			pNewInfo->rc.left = x;
			pNewInfo->rc.right = x + szStr.cx;
			pNewInfo->rc.top = m_pPrevHoverd->rc.top;
			pNewInfo->rc.bottom = m_pPrevHoverd->rc.bottom;
		}
		else
		{
			// draw it in next line
			pNewInfo->rc.left = m_nBorderWidth + m_nXmargin;
			pNewInfo->rc.right = pNewInfo->rc.left + szStr.cx;
			pNewInfo->rc.top = m_pPrevHoverd->rc.bottom + m_nYmargin;
			pNewInfo->rc.bottom = pNewInfo->rc.top + szStr.cy;
		}
	}

	m_listStringInfo.push_back(pNewInfo);
	::InvalidateRect(m_hWnd, &pNewInfo->rc, TRUE);

	return m_listStringInfo.size();
}

int CLabelEx::SetText(CStringW const& str)
{
	ClearStringInfo();
	m_strWndText = str;
	SplitText(str, m_listSplitText);
	if(0 != m_hWnd)	::InvalidateRect(m_hWnd, &m_rcWnd, TRUE);

	return str.GetLength();
}

COLORREF CLabelEx::SetBorderColor(COLORREF color)
{
	m_clrBorderColor = color;

	// it already exsits because it has been created in constructor
	::DeleteObject(m_hBrushBorder);
	m_hBrushBorder = ::CreateSolidBrush(m_clrBorderColor);

	return m_clrBorderColor;
}

int CLabelEx::CheckFirstChar(CStringW const& strTxtLine)
{
	TCHAR chLastChar;
	int nRet(-1);

	if(0 < strTxtLine.GetLength())
	{
		chLastChar = strTxtLine[0];
		switch(chLastChar)
		{
			case ',':
			case '.':
			case ';':
			case '£¬':
			case '¡£':
			case '£»':
			case '\\':
			case '?':
			case '!':
			case '£¡':
			case '*':
			case '¡¢':
			case ':':
			case '£º':
			case '¡­':
			case '£®':
				nRet = 1;
		}
	}

	return nRet;
}

int CLabelEx::ClearStringInfo()
{
	CSensibleInfo *pInfo;
	CStringW *pstr;

	while(m_listSplitText.size() > 0)
	{
		pstr = m_listSplitText.back();
		delete pstr;
		m_listSplitText.pop_back();
	}
	while(m_listStringInfo.size() > 0)
	{
		pInfo = m_listStringInfo.back();
		delete pInfo->pwstr;
		delete pInfo;
		m_listStringInfo.pop_back();

		// m_pPrevHoverd is equal to one of the m_listStringInfo member,
		//  when all m_listStringInfo members are destroyed,m_pPrevHoverd must be set to 0
		m_pPrevHoverd = 0;
	}
	return 0;
}

int CLabelEx::OutputString(HDC hdc, const CSensibleInfo *pInfo)
{
	if(0 == pInfo)return 0;

	CRect rc = pInfo->rc;
	::DrawText(hdc, (LPCWSTR)pInfo->pwstr->GetBuffer(), pInfo->pwstr->GetLength(), &rc, DT_LEFT|DT_VCENTER|DT_SINGLELINE);

	return pInfo->pwstr->GetLength();
}

LRESULT CLabelEx::OnCreate(WPARAM,LPARAM lParam)
{
	CPoint ptLeftTop;

	GetWindowRect(&m_rcWnd);
	ptLeftTop.x = m_rcWnd.left;
	ptLeftTop.y = m_rcWnd.top;
	ScreenToClient(&ptLeftTop);
	m_rcWnd.left = ptLeftTop.x;
	m_rcWnd.top = ptLeftTop.y;

	ptLeftTop.x = m_rcWnd.right;
	ptLeftTop.y = m_rcWnd.bottom;
	ScreenToClient(&ptLeftTop);
	m_rcWnd.right = ptLeftTop.x;
	m_rcWnd.bottom = ptLeftTop.y;

	// if weight or height of m_rcWnd is 0,do nothing
	if(0 == m_rcWnd.Width() * m_rcWnd.Height())return 0;

	HDC hScreenDC = ::GetDC(0);
	m_hMemDC = ::CreateCompatibleDC(hScreenDC);
	m_hMemBitmap = ::CreateCompatibleBitmap(hScreenDC, m_rcWnd.Width(), m_rcWnd.Height());	
	m_hBrushBkgnd = ::CreateBrushIndirect(&m_logBrush);
	::ReleaseDC(0, hScreenDC);

	CSize szOneChar;
	::SelectObject(m_hMemDC, (HFONT)m_hfontLabel);
	::GetTextExtentPoint32(m_hMemDC, CLabelEx::ONE_CHAR, 1, &szOneChar);
	m_nMinWidth = szOneChar.cx + 2*m_nBorderWidth + 2 * m_nXmargin;
	m_nMinHeight = szOneChar.cy + 2*m_nBorderWidth + 2 * m_nYmargin;

	m_nAvailableWidth = m_rcWnd.Width() - 2*(m_nBorderWidth + m_nXmargin);
	m_nAvailableHeight = m_rcWnd.Height() - 2*(m_nBorderWidth + m_nYmargin);

	return TRUE;
}

LRESULT CLabelEx::OnMouseMove(WPARAM wParam,LPARAM lParam)
{
	CRect rcWnd;

	if(0 == m_listStringInfo.size())return 0;

	// left or right button is down,do nothing
	if(MK_LBUTTON == wParam)return 0;
	if(MK_RBUTTON == wParam)return 0;

	CPoint ptMouse;
	CSensibleInfo *pInfo;
	list<CSensibleInfo*>::const_iterator iterInfo;

	ptMouse.x = LOWORD(lParam);
	ptMouse.y = HIWORD(lParam);
	iterInfo = m_listStringInfo.begin();
	pInfo = 0;

	for(int i=0;i<m_listStringInfo.size();i++,iterInfo++)
	{
		if(::PtInRect((*iterInfo)->rc,ptMouse))
		{
			pInfo = *iterInfo;
			break;
		}
	}

	HDC hDC = GetDC();

	//
	// step 1
	// when the mouse is out of any rect,repaint the previous hovered item with PLAIN color
	//
	if(0 != m_pPrevHoverd)
	{
		::SetTextColor(m_hMemDC, m_clrPlainColor);
		::FillRect(m_hMemDC, &m_pPrevHoverd->rc, m_hBrushBkgnd);
		OutputString(m_hMemDC, m_pPrevHoverd);
		::BitBlt(hDC, m_pPrevHoverd->rc.left, m_pPrevHoverd->rc.top, m_pPrevHoverd->rc.Width(), m_pPrevHoverd->rc.Height(), m_hMemDC, m_pPrevHoverd->rc.left, m_pPrevHoverd->rc.top, SRCCOPY);

		// prevent from repeat painting
		m_pPrevHoverd = 0;
		::OutputDebugStringW(L"move to another region\n");
	}

	//
	// step 2
	// when the mouse is in one rect,paint the hovered item with HOVERED color
	//  and track mouse LEAVE event
	//
	if(0 != pInfo && pInfo != m_pPrevHoverd)
	{
		// paint hoverd region
		::SetTextColor(m_hMemDC, m_clrHoveredColor);
		OutputString(m_hMemDC, pInfo);
		::BitBlt(hDC, pInfo->rc.left, pInfo->rc.top, pInfo->rc.Width(), pInfo->rc.Height(), m_hMemDC, pInfo->rc.left, pInfo->rc.top, SRCCOPY);
		m_pPrevHoverd = pInfo;
		::OutputDebugStringW(L"hoverd\n");
	}
	ReleaseDC(hDC);

	TRACKMOUSEEVENT tm;
	tm.cbSize = sizeof(TRACKMOUSEEVENT);
	tm.dwFlags = TME_LEAVE;
	tm.hwndTrack = m_hWnd;
	tm.dwHoverTime = 0;
	::TrackMouseEvent(&tm);

	return 0;
}

LRESULT CLabelEx::OnMouseLeave(WPARAM wParam,LPARAM lParam)
{
	if(0 < m_listStringInfo.size())
	{
		if (0 != m_pPrevHoverd)
		{
			HDC hDC = GetDC();
			::FillRect(m_hMemDC, &m_pPrevHoverd->rc, m_hBrushBkgnd);
			::SetTextColor(m_hMemDC, m_clrPlainColor);
			OutputString(m_hMemDC, m_pPrevHoverd);
			::BitBlt(hDC, m_pPrevHoverd->rc.left, m_pPrevHoverd->rc.top, m_pPrevHoverd->rc.Width(), m_pPrevHoverd->rc.Height(), m_hMemDC, m_pPrevHoverd->rc.left, m_pPrevHoverd->rc.top, SRCCOPY);

			// prevent from repeat painting
			m_pPrevHoverd = 0;
			ReleaseDC(hDC);
			::OutputDebugStringW(L"move leave\n");
		}
	}

	return 0;
}

LRESULT CLabelEx::OnSize(WPARAM,LPARAM lParam)
{
	int nAvailableWidth;

	// when it is moving or sizing,erase the former border
	CRect rcUpdate;
	CPoint ptTopLeft,ptBottomRight;

	rcUpdate.left = m_rcWnd.left;
	rcUpdate.top = m_rcWnd.top;
	rcUpdate.right = m_rcWnd.right;
	rcUpdate.bottom = m_rcWnd.bottom;

	nAvailableWidth = lParam&0xffff - m_nBorderWidth*2 - m_nXmargin*2;



	//rcUpdate.left = lpPos->x;
	//rcUpdate.top = lpPos->y;
	//rcUpdate.right = lpPos->x + lpPos->cx;
	//rcUpdate.bottom = lpPos->y + lpPos->cy;
	//HRGN hRgnCurrentWnd = ::CreateRectRgn(rcUpdate.left,rcUpdate.top,rcUpdate.right,rcUpdate.bottom);

	//// when it is getting smaller,erase(fill with COLOR_BTNFACE brush) the content within new frame and the old larger frame
	//// this area is not a rectagle,we must use region
	//::CombineRgn(hRgnCurrentWnd,hRgnOrgWnd,hRgnCurrentWnd,RGN_AND);
	//::CombineRgn(hRgnOrgWnd,hRgnOrgWnd,hRgnCurrentWnd,RGN_DIFF);
	//HDC hDC = ::GetDC(::GetParent(hWnd));
	//::FillRgn(hDC,hRgnOrgWnd,::CreateSolidBrush(::GetSysColor(COLOR_BTNFACE)));
	//::ReleaseDC(::GetParent(hWnd),hDC);
	//::DeleteObject(hRgnOrgWnd);
	//::DeleteObject(hRgnCurrentWnd);

	//if(lpPos->cx < m_nMinWidth)lpPos->cx = m_nMinWidth;
	//if(lpPos->cy < m_nMinHeight)lpPos->cy = m_nMinHeight;

	//HDC hScreenDC = ::GetDC(0);
	//::GetWindowRect(hWnd,&m_rcWnd);
	//::MapWindowPoints(::GetDesktopWindow(),m_hWnd,(LPPOINT)&m_rcWnd,sizeof(CRect)/sizeof(POINT));
	//::DeleteObject(m_hMemBitmap);
	//m_hMemBitmap = ::CreateCompatibleBitmap(hScreenDC,m_rcWnd.Width(), m_rcWnd.Height());
	//::ReleaseDC(0,hScreenDC);

	//if(m_bSenseMouse)ResetHotStringInfo();
	//else ResetTextInfo();
	//::InvalidateRect(hWnd,&m_rcWnd,0);

	return TRUE;		// return TRUE for this message has been processed
}

LRESULT CLabelEx::OnPosChanged(WPARAM,LPARAM lParam)
{
	LPWINDOWPOS lpPos = reinterpret_cast<LPWINDOWPOS>(lParam);
	if(0 == lpPos->cx)return 0;

	CRect rcWnd;

	m_rcWnd.left = 0;
	m_rcWnd.right  = lpPos->cx;
	m_rcWnd.top = 0;
	m_rcWnd.bottom = lpPos->cy;

	HDC hWndDC = GetWindowDC();
	if(0 != m_hMemDC)::DeleteDC(m_hMemDC);
	m_hMemDC = ::CreateCompatibleDC(hWndDC);

	if(0 != m_hMemBitmap)::DeleteObject(m_hMemBitmap);
	m_hMemBitmap = ::CreateCompatibleBitmap(hWndDC, m_rcWnd.Width(), m_rcWnd.Height());	
	ReleaseDC(hWndDC);

	CSize szOneChar;
	::SelectObject(m_hMemDC, (HFONT)m_hfontLabel);
	::GetTextExtentPoint32(m_hMemDC, CLabelEx::ONE_CHAR, 1, &szOneChar);
	m_nMinWidth = szOneChar.cx + 2*m_nBorderWidth + 2 * m_nXmargin;
	m_nMinHeight = szOneChar.cy + 2*m_nBorderWidth + 2 * m_nYmargin;
	m_nAvailableWidth = m_rcWnd.Width() - 2*(m_nBorderWidth + m_nXmargin);
	m_nAvailableHeight = m_rcWnd.Height() - 2*(m_nBorderWidth + m_nYmargin);

	SplitText(m_strWndText, m_listSplitText);
	Invalidate();

	return 0;
}

LRESULT CLabelEx::OnEraseBkGnd(WPARAM wParam,LPARAM lParam)
{
	list<CSensibleInfo*>::const_iterator iterInfo,iterEnd,iterNext;

	HDC hDC = GetWindowDC();
	CRect rcWnd;
	::CopyRect(&rcWnd, &m_rcWnd);
	rcWnd.OffsetRect(-rcWnd.left, -rcWnd.top);
	::FillRect(hDC, &rcWnd, m_hBrushBkgnd);
	ReleaseDC(hDC);

	return TRUE;
}

//
// split the string so that it can be displayed within the specified width(nAvailableWidth)
// [IN][OUT] strTrimed
// strTrimed contains string to be split
// when return, strTrimed contains string has been split
// return value:difference of char count between origin string and split string
//
int CLabelEx::TrimString(int nWidth,CStringW& strTrimed)
{
	if(strTrimed.IsEmpty())return -1;

	CSize szString;
	CStringW strOrigin, strSurplus;
	int nSurplusWidth;

	szString.cx = 0;
	strOrigin = strTrimed;
	::GetTextExtentPoint32(m_hMemDC, strOrigin, strOrigin.GetLength(), &szString);

	while(nWidth < szString.cx)
	{
		if(strTrimed.IsEmpty())break;		
		strTrimed = strTrimed.Left(strTrimed.GetLength()/2);
		::GetTextExtentPoint32(m_hMemDC, strTrimed, strTrimed.GetLength(), &szString);
	}
	if(strTrimed.IsEmpty())return -1;
	nSurplusWidth = nWidth - szString.cx;
	::GetTextExtentPoint32(m_hMemDC, ONE_CHAR, 1, &szString);
	if(nSurplusWidth < szString.cx)return strOrigin.GetLength() - strTrimed.GetLength();

	strSurplus = strOrigin.Right(strOrigin.GetLength() - strTrimed.GetLength());
	TrimString(nSurplusWidth, strSurplus);
	strTrimed = strTrimed + strSurplus;

	return strOrigin.GetLength() - strTrimed.GetLength();
}

int CLabelEx::SplitText(const CStringW& str, list<CStringW *>& partList)
{
	if(str.IsEmpty())return 0;

	int nSurplusCharCnt(0);

	CStringW strOrign, strTrimmed;

	while(partList.size() > 0)
	{
		delete partList.back();
		partList.pop_back();
	}

	strOrign = strTrimmed = str;

	do
	{
		nSurplusCharCnt = TrimString(m_nAvailableWidth, strTrimmed);

		// prevent from the first char of next line being ',' , '.' , ';' etc
		// if only one char can be displayed,show it
		if(nSurplusCharCnt > 1)
		{
			while(0 < CheckFirstChar(strOrign.Right(nSurplusCharCnt)))
			{
				// shorten strTrimmed until the 1st char of next line is not ',' , '.' , ';' etc
				nSurplusCharCnt ++;
				strTrimmed = strTrimmed.Left(strTrimmed.GetLength() - 1);
			}
		}

		partList.push_back(new CStringW(strTrimmed));
		strTrimmed = strOrign = strOrign.Right(nSurplusCharCnt);
	}
	while(nSurplusCharCnt > 0);

	return partList.size();
}

int CLabelEx::Caculate1stLineTop()
{
	if(m_strWndText.IsEmpty())return 0;

	int nTotalHeight(0);
	CSize szText;
	list<CStringW *>::const_iterator iter, endIter;
	
	iter = m_listSplitText.begin();
	endIter = m_listSplitText.end();
	::GetTextExtentPoint32(m_hMemDC, m_strWndText, 1, &szText);

	while(iter != endIter)
	{
		nTotalHeight += szText.cy;
		iter++;
	}

	if(nTotalHeight <= m_nAvailableHeight)return (m_nAvailableHeight - nTotalHeight)/2;
	return m_nBorderWidth + m_nYmargin;
}

int CLabelEx::DrawPlainText()
{
	if(m_strWndText.IsEmpty())return 0;

	int nAvailableHeight;
	CStringW strPartString;
	CSize szPartStr;
	CRect rcString;
	
	nAvailableHeight = m_rcWnd.Height() - m_nBorderWidth * 2 - m_nYmargin * 2;
	::SelectObject(m_hMemDC, (HFONT)m_hfontLabel);	
	::GetTextExtentPoint32(m_hMemDC, m_strWndText, 1, &szPartStr);
	::SetTextColor(m_hMemDC, m_clrPlainColor);

	// the height of available area is too small,nothing will be displayed
	if(nAvailableHeight < szPartStr.cy)return 0;

	if(m_nVAlignment & VCENTER)
	{
		rcString.top = Caculate1stLineTop();
	}
	else
	{
		rcString.top = m_nBorderWidth + m_nYmargin;
	}
	rcString.bottom = rcString.top + szPartStr.cy;

	// draw text row by row
	list<CStringW *>::const_iterator iter,endIter;
	
	iter = m_listSplitText.begin();
	endIter = m_listSplitText.end();

	while(iter != endIter)
	{
		strPartString = **iter;
		::GetTextExtentPoint32(m_hMemDC, strPartString, strPartString.GetLength(), &szPartStr);
		if(HCENTER == m_nHAlignment)
		{
			rcString.left = (m_rcWnd.Width() - 2*m_nBorderWidth - 2*m_nXmargin - szPartStr.cx)/2;
			if(rcString.left < m_nBorderWidth + m_nXmargin)rcString.left = m_nBorderWidth + m_nXmargin;
		}
		else 
		{
			rcString.left = m_nBorderWidth + m_nXmargin;
		}
		rcString.right = rcString.left + szPartStr.cx;
		::DrawText(m_hMemDC, strPartString, strPartString.GetLength(), rcString, DT_VCENTER|DT_SINGLELINE|DT_NOCLIP);
		rcString.top += rcString.Height();
		rcString.bottom = rcString.top + szPartStr.cy;

		// no enough space to display all text
		if(nAvailableHeight < rcString.bottom)break;

		iter++;
	}

	//// get rid of the line can not be displayed for inadequate space
	//while(lAvailableHeight < lTotalHeight)
	//{
	//	lTotalHeight -= m_listStringInfo.back()->m_listStrRects.back()->Height();
	//	delete m_listStringInfo.back()->m_listPartStrings.back();
	//	m_listStringInfo.back()->m_listPartStrings.pop_back();
	//	delete m_listStringInfo.back()->m_listStrRects.back();
	//	m_listStringInfo.back()->m_listStrRects.pop_back();
	//	delete m_listStringInfo.back();
	//	m_listStringInfo.pop_back();
	//}
	// adjust Y coordintes of text line for different alignment
	//if(CENTER_ALIGNMENT == m_nAlignment)
	//{
	//	list<CHotStringInfo *>::const_iterator iter,endIter;
	//	iter = m_listStringInfo.begin();
	//	endIter = m_listStringInfo.end();
	//	int y,nCurrentLineHeight;

	//	y = (lAvailableHeight - lTotalHeight) / 2;
	//	if(y < m_nBorderWidth + m_nYmargin)y = m_nBorderWidth + m_nYmargin;
	//	nCurrentLineHeight = (*iter)->m_listStrRects.back()->Height();

	//	do
	//	{
	//		(*iter)->m_listStrRects.back()->top = y;
	//		(*iter)->m_listStrRects.back()->bottom = y + nCurrentLineHeight;
	//		y = y + nCurrentLineHeight + nCurrentLineHeight/LINE_GAP_ADJUST_FACTOR;
	//		iter ++;
	//		if(iter != endIter)
	//		{
	//			nCurrentLineHeight = (*iter)->m_listStrRects.back()->Height();
	//		}
	//	}
	//	while(iter != endIter);
	//}
	
	return 0;
}

int CLabelEx::DrawSensibleText()
{
	return 0;
}

LRESULT CLabelEx::OnPaint(WPARAM, LPARAM)
{
	PAINTSTRUCT ps;
	list<CSensibleInfo*>::const_iterator iterInfo, iterEnd, iterNext;

	BeginPaint(&ps);
	::SelectObject(m_hMemDC, m_hMemBitmap);
	::FillRect(m_hMemDC, &m_rcWnd, m_hBrushBkgnd);
	if(m_bShowBorder)
	{
		::FrameRect(m_hMemDC, &m_rcWnd, m_hBrushBorder);
	}
	::SetBkMode(m_hMemDC, TRANSPARENT);
	::SelectObject(m_hMemDC, (HFONT)m_hfontLabel);

	DrawPlainText();
	if(0 == m_listStringInfo.size())goto _CLabelEx_OnPaint_exit;

	iterInfo = m_listStringInfo.begin();
	iterEnd = m_listStringInfo.end();
	::SetTextColor(m_hMemDC, m_clrPlainColor);

	CSensibleInfo	*pInfo;
	while(iterEnd != iterInfo)
	{
		pInfo = *iterInfo;
		if (pInfo->pwstr->IsEmpty())
		{
			iterInfo ++;
			continue;
		}
		OutputString(m_hMemDC, pInfo);
		iterNext = iterInfo;
		iterNext ++;
		if(iterEnd != iterNext)
		{
			::TextOut(m_hMemDC, (*iterInfo)->rc.right, (*iterInfo)->rc.top, CStringW(CVocabConst::COMMA), 1);
		}
		iterInfo ++;
	}

_CLabelEx_OnPaint_exit:
	::BitBlt(ps.hdc, 0, 0, m_rcWnd.Width(), m_rcWnd.Height(), m_hMemDC, 0, 0, SRCCOPY);
	EndPaint(&ps);
	//::GetTextExtentPoint32(m_hMemDC,CLabelEx::ONE_CHAR,1,&_sz_OneChar);
	//::GetTextExtentPoint32(m_hMemDC,CLabelEx::COMMA,1,&_sz_Comma);

	// return 0 to indicate we have processed the message
	return 0;
}

LRESULT CLabelEx::OnLeftClick(WPARAM, LPARAM lParam)
{
	if(0 == m_listStringInfo.size())return 0L;

	CPoint ptMouse;
	CSensibleInfo	*pInfo;
	list<CSensibleInfo *>::const_iterator iterInfo, iterEnd;
	iterInfo = m_listStringInfo.begin();
	iterEnd = m_listStringInfo.end();
	ptMouse.x = LOWORD(lParam);
	ptMouse.y = HIWORD(lParam);  
	pInfo = 0;

	while(iterEnd != iterInfo)
	{
		if(::PtInRect((*iterInfo)->rc,ptMouse))
		{
			pInfo = *iterInfo;
			//OutputDebugString (L("in\n"));
			break;
		}
		iterInfo++;
	}

	if(0 != pInfo)
	{
		::PostMessage(m_hWndParent, m_ulHitMessage, FALSE, // this spell will NOT be displayed in the 'spell' label
			(LPARAM)new CStringW(*(pInfo->pwstr)) 	/* this CStringW pointer will be delete later in CVocabWorkThread::OnMsgSearchSpell() */);
	}

	return 0L;
}

BOOL CLabelEx::SearchSensibleArea(const CPoint &pt, CStringW &str)
{
	list<CSensibleInfo*>::const_iterator iterInfo;

	iterInfo = m_listStringInfo.begin();
	for(int i=0;i<m_listStringInfo.size();i++,iterInfo++)
	{
		if(::PtInRect((*iterInfo)->rc, pt))
		{
			str = *((*iterInfo)->pwstr);
			break;
		}
	}

	return str.IsEmpty();
}

int CLabelEx::AddPopupMenuItem(UINT wID,HICON hIcon,CStringW strTxt)
{
	if(0 == m_hPopMenu)m_hPopMenu = ::CreatePopupMenu();
	if(0 == m_hPopMenu)return -1;

	MENUITEMINFO	mii;
	mii.cbSize =  sizeof(MENUITEMINFO);
	mii.fMask =  MIIM_FTYPE|MIIM_ID|MIIM_DATA;
	mii.fType = MF_STRING| MFT_OWNERDRAW;
	mii.fType = MFT_OWNERDRAW;
	mii.fState = MFS_ENABLED;
	mii.wID = wID;
	mii.dwItemData = (ULONG_PTR)hIcon;
	::InsertMenuItem(m_hPopMenu, 0, 0, &mii);

	mii.fMask =  MIIM_STRING;
	mii.dwTypeData = strTxt.GetBuffer();
	mii.cch = strTxt.GetLength();
	::SetMenuItemInfo(m_hPopMenu, mii.wID, 0, &mii);

	if (0 == m_hBrushPopMenu)m_hBrushPopMenu = ::CreateSolidBrush(CVocabConst::COLOR2);
	return 0;
}

LRESULT CLabelEx::OnRightClick(WPARAM,LPARAM lParam)
{
	if(FALSE == m_hPopMenu)return 0;

	CLabelEx::_LabelEx_hWndMenuOwner = m_hWnd;

	// here we install a hook to catch the menu item window(is there any other way?) for owner-draw use
	CLabelEx::_LabelEx_hCbtHook = ::SetWindowsHookEx(WH_CBT, _LabelEx_cbt_HookProc, globalVar.g_hInstance, ::GetCurrentThreadId());

	CPoint ptMouse;
	ptMouse.x = LOWORD(lParam);
	ptMouse.y = HIWORD(lParam);  
	globalVar.g_ptRButtonUp.x = ptMouse.x;
	globalVar.g_ptRButtonUp.y = ptMouse.y;
	ClientToScreen(&ptMouse);

	int nID = ::TrackPopupMenuEx(m_hPopMenu, TPM_RIGHTBUTTON | TPM_RETURNCMD, ptMouse.x, ptMouse.y, m_hWnd, 0);
	::PostMessage(m_hWndParent, WM_COMMAND, nID, 0);

	return 0L;
}

LRESULT CLabelEx::OnDelMenuDC()
{
	::ReleaseDC(_LabelEx_hWndPopMenu, _LabelEx_hTmpDC);
	::DeleteObject(_LabelEx_hMenuBitmap);
	_LabelEx_hTmpDC = 0;
	_LabelEx_hMenuBitmap = 0;
	return 0;
}

int CLabelEx::OnMeasureMenuItem(LPARAM lParam)
{
	HDC hScreenDC(0);
	LPMEASUREITEMSTRUCT pmis = (LPMEASUREITEMSTRUCT)lParam;
	pmis->itemHeight = pmis->itemWidth = 0;

	MENUITEMINFO	mii;
	LPWSTR		pstr(0);
	CRect rcTxt;

	mii.cbSize =  sizeof(MENUITEMINFO);
	mii.fMask = MIIM_STRING;
	mii.cch = 0;
	mii.dwTypeData = 0;
	::GetMenuItemInfo(m_hPopMenu, pmis->itemID, 0, &mii);

	mii.cch++;
	if(MAX_LEN_MENUITEM > mii.cch && 0 < mii.cch)pstr = new WCHAR[mii.cch];
	else goto _OnMeasureMenuItem_exit;

	mii.dwTypeData = pstr;
	::GetMenuItemInfo(m_hPopMenu, pmis->itemID, 0, &mii);
	hScreenDC = ::GetWindowDC(0);
	::SelectObject(hScreenDC, _LabelEx_hfontMenu);
	if(MAX_LEN_MENUITEM > mii.cch && 0 < mii.cch)
	{
		::DrawTextW(hScreenDC, pstr,mii.cch, &rcTxt, DT_CALCRECT|DT_SINGLELINE|DT_EXPANDTABS|DT_NOCLIP);
	}
	else goto _OnMeasureMenuItem_exit;

	int nHeight;
	nHeight = ::GetSystemMetrics(SM_CYICON) + DEFAULT_Y_MARGIN * 2;
	if(rcTxt.Height() < nHeight)
	{
		pmis->itemHeight = nHeight;
	}
	else
	{
		pmis->itemHeight = rcTxt.Height() + DEFAULT_Y_MARGIN*2;
	}
	pmis->itemWidth = rcTxt.Width() + ::GetSystemMetrics(SM_CXICON) + DEFAULT_X_MARGIN*2;

_OnMeasureMenuItem_exit:
	if(0 != pstr)delete[] pstr;
	if(0 != hScreenDC)::ReleaseDC(0,hScreenDC);

	return TRUE;
}

int CLabelEx::OnDrawMenuItem(LPARAM lParam)
{
	if (0 == CLabelEx::_LabelEx_hWndPopMenu)return 1;
	DRAWITEMSTRUCT *pdm;
	MENUITEMINFO	mii;
	CRect rcItem;
	WCHAR szBuff[_MAX_PATH];
	CString strItemTxt;
	HDC hScreenDC, hMenuWndDC;
	CRect rcPopWnd, rcPopMenu;

	hScreenDC = ::GetWindowDC(0);
	::GetWindowRect(CLabelEx::_LabelEx_hWndPopMenu, &rcPopMenu);
	hMenuWndDC = ::GetWindowDC(CLabelEx::_LabelEx_hWndPopMenu);
	::GetWindowRect(CLabelEx::_LabelEx_hWndPopMenu, &rcPopWnd);
	rcPopWnd.OffsetRect(-rcPopWnd.left, -rcPopWnd.top);

	pdm = (DRAWITEMSTRUCT *)lParam;

	mii.cbSize = sizeof(MENUITEMINFO);
	mii.fMask = MIIM_STRING | MIIM_DATA;
	mii.cch = 0;
	mii.dwTypeData = 0;
	GetMenuItemInfo(m_hPopMenu, pdm->itemID, 0, &mii);

	mii.cch++;
	mii.dwTypeData = szBuff;
	GetMenuItemInfo(m_hPopMenu, pdm->itemID, 0, &mii);
	strItemTxt = szBuff;


	if (ODA_DRAWENTIRE == pdm->itemAction)
	{
		if (0 == _LabelEx_hMenuBitmap)
		{
			//rcPopMenu.bottom += 20;
			//::SetWindowPos(CLabelEx::_LabelEx_hWndPopMenu, 0, rcPopMenu.left, rcPopMenu.top, rcPopMenu.Width(), rcPopMenu.Height(),  SWP_NOMOVE| SWP_NOACTIVATE| SWP_NOREDRAW|SWP_NOOWNERZORDER| SWP_NOSENDCHANGING);

			CLabelEx::_LabelEx_hTmpDC = ::CreateCompatibleDC(hScreenDC);
			_LabelEx_hMenuBitmap = ::CreateCompatibleBitmap(hScreenDC, rcPopMenu.Width(), rcPopMenu.Height());
			::SelectObject(CLabelEx::_LabelEx_hTmpDC, CLabelEx::_LabelEx_hMenuBitmap);
			::SetBkMode(CLabelEx::_LabelEx_hTmpDC, TRANSPARENT);
			::SelectObject(CLabelEx::_LabelEx_hTmpDC, _LabelEx_hfontMenu);

			TRIVERTEX vert[2];
			GRADIENT_RECT    grdRect;

			rcPopMenu.OffsetRect(-rcPopMenu.left, -rcPopMenu.top);

			vert[0].x = 0;
			vert[0].y = 0;
			vert[0].Red = 0xd000;
			vert[0].Green = 0xd000;
			vert[0].Blue = 0xd000;
			vert[0].Alpha = 0;

			vert[1].x = ::GetSystemMetrics(SM_CXICON) + POPMENU_MARGIN_WIDTH;
			vert[1].y = rcPopMenu.Height();
			vert[1].Red = 0xee00;
			vert[1].Green = 0xee00;
			vert[1].Blue = 0xee00;
			vert[1].Alpha = 0;

			grdRect.UpperLeft = 0;
			grdRect.LowerRight = 1;
			GradientFill(CLabelEx::_LabelEx_hTmpDC, vert, 2, &grdRect, 1, GRADIENT_FILL_RECT_V);

			vert[0].x = ::GetSystemMetrics(SM_CXICON) + POPMENU_MARGIN_WIDTH;
			vert[0].y = 0;
			vert[0].Red = 0xee00;
			vert[0].Green = 0xee00;
			vert[0].Blue = 0xee00;
			vert[0].Alpha = 0;

			vert[1].x = rcPopMenu.Width();
			vert[1].y = rcPopMenu.Height();
			vert[1].Red = 0xdd00;
			vert[1].Green = 0xdd00;
			vert[1].Blue = 0xdd00;
			vert[1].Alpha = 0;

			grdRect.UpperLeft = 0;
			grdRect.LowerRight = 1;
			GradientFill(CLabelEx::_LabelEx_hTmpDC, vert, 2, &grdRect, 1, GRADIENT_FILL_RECT_V);

			::FrameRect(CLabelEx::_LabelEx_hTmpDC, &rcPopMenu, ::CreateSolidBrush(MEMU_FRM_COLOR));
		}

		CString ss;
		::OutputDebugString(L"ENTIRE:");
		ss.Format(L"%x    %d\n", pdm->itemID, pdm->rcItem.top);
		::OutputDebugString(ss);

		// draw icon
		HICON hIcon = (HICON)mii.dwItemData;
		rcItem.CopyRect(&pdm->rcItem);
		rcItem.left += POPMENU_MARGIN_WIDTH;
		rcItem.top += POPMENU_MARGIN_WIDTH;
		rcItem.right += POPMENU_MARGIN_WIDTH;
		rcItem.bottom += POPMENU_MARGIN_WIDTH;
		if (0 != hIcon)
		{
			::DrawIconEx(_LabelEx_hTmpDC, rcItem.left, rcItem.top, hIcon, ICON_WIDTH - 2, ICON_WIDTH - 2, 0, 0, DI_NORMAL);
			::DestroyIcon(hIcon);
		}

		// draw text
		CRect rcTxt;
		::DrawText(_LabelEx_hTmpDC, (LPCWSTR)strItemTxt, strItemTxt.GetLength(), &rcTxt, DT_VCENTER | DT_CALCRECT | DT_SINGLELINE | DT_EXPANDTABS | DT_NOCLIP);
		rcItem.left += ICON_WIDTH + POPMENU_MARGIN_WIDTH;
		rcItem.right += ICON_WIDTH + POPMENU_MARGIN_WIDTH;
		::DrawText(_LabelEx_hTmpDC, (LPCWSTR)strItemTxt, strItemTxt.GetLength(), &rcItem, DT_VCENTER | DT_SINGLELINE | DT_EXPANDTABS | DT_NOCLIP);
		goto _DrawMenuItem_exit;
	}

	if (FALSE == (ODS_SELECTED & pdm->itemState))
	{
		::OutputDebugString(L"UNSELECTED: ");
		strItemTxt.Format(L"%x\n", pdm->itemID);
		::OutputDebugString(strItemTxt);

		// when an item is UNSELECTED,just paste the memory BMP to the menu
		::BitBlt(hMenuWndDC, 0, 0, rcPopWnd.Width(), rcPopWnd.Height(), _LabelEx_hTmpDC, 0, 0, SRCCOPY);
	}

	if (TRUE == (ODS_SELECTED & pdm->itemState))
	{
		::OutputDebugString(L"SELECTED\n");

		//
		// draw SELECTED state(a frame)
		// paste the memory BMP to the menu first,and then draw the frame by using hMenuWndDC directly
		//
		::BitBlt(hMenuWndDC, 0, 0, rcPopWnd.Width(), rcPopWnd.Height(), _LabelEx_hTmpDC, 0, 0, SRCCOPY);
		rcItem.CopyRect(&pdm->rcItem);
		rcItem.left += POPMENU_MARGIN_WIDTH;
		rcItem.top += POPMENU_MARGIN_WIDTH;
		rcItem.right += POPMENU_MARGIN_WIDTH;
		rcItem.bottom += POPMENU_MARGIN_WIDTH;

		::FrameRect(hMenuWndDC, &rcItem, m_hBrushPopMenu);
	}

_DrawMenuItem_exit:
	::ReleaseDC(CLabelEx::_LabelEx_hWndPopMenu, hMenuWndDC);
	::ReleaseDC(0, hScreenDC);

	return TRUE;
}

int CLabelEx::DrawMenuBkGnd()
{
	::OutputDebugString(L"bkground\n");

	CRect rcPopMenu;

	//::GetWindowRect(CLabelEx::_LabelEx_hWndPopMenu, &rcPopMenu);


	//hPopupWndDC = ::GetWindowDC(CLabelEx::_LabelEx_hWndPopMenu);
	//::BitBlt(hPopupWndDC, 0, 0, rcPopMenu.Width(), rcPopMenu.Height(), CLabelEx::_LabelEx_hTmpDC, 0, 0, SRCCOPY);
	
	return TRUE;
}

LRESULT CLabelEx::OnCommand(WPARAM wParam,LPARAM lParam)
{
	if(0 == wParam)return 0;

	if(CVocabConst::POPUP_ITEM_ID::ADD_RELATED == wParam)
	{
		::PostMessage(m_hWndParent,WM_COMMAND,wParam,0);
	}

	return 0;
}

int CLabelEx::OnMsgRcvPopWnd(WPARAM wParam)
{
	CStringW str;
	//str.Format(L"%x\n", CLabelEx::_LabelEx_hWndPopMenu);
	//::OutputDebugString(L"RcvWnd:");
	//::OutputDebugString(str);

	CRect rcPopMenu;
	CLabelEx::_LabelEx_hWndPopMenu = (HWND)wParam;
	::GetWindowRect(CLabelEx::_LabelEx_hWndPopMenu, &rcPopMenu);

	//
	// force to redraw the popup menu
	// due to the initial displayed popup menu is predefined by system,we must repaint it exactly after it first shown
	// but the mechanism here is opaue and unknown
	// we post a WM_MOUSEMOVE to it,and then system will send WM_DRAWITEM message
	// so we can repaint it exactly after it first shown
	// better way maybe exits,but...
	//

	::PostMessage(CLabelEx::_LabelEx_hWndPopMenu, WM_MOUSEMOVE, 0, MAKELONG(rcPopMenu.left + rcPopMenu.Width() / 2, rcPopMenu.top + rcPopMenu.Height() / 2));

	return 0;
}

LRESULT CLabelEx::WndProc(UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	switch(uMsg)
	{
	case WM_NCCREATE:

		// return TRUE to continue creation of the window
		// if return FALSE, the CreateWindow function will return a NULL handle
		return 1;

	case WM_CREATE:
		return OnCreate(wParam, lParam);

	case WM_MOUSEMOVE:
		return OnMouseMove(wParam, lParam);

	case WM_MOUSELEAVE:
		return OnMouseLeave(wParam, lParam);

	case WM_NCHITTEST:	
		return HTCLIENT;

	case WM_SETCURSOR:
		return 0;

	case WM_SYSCOMMAND:

		// return 0 to indicate we have processed the message
		return 0;

	case WM_COMMAND:
		return OnCommand(wParam, lParam);

	case WM_LBUTTONDOWN:
		return 0;

	case WM_LBUTTONUP:
		return OnLeftClick(wParam, lParam);

	case WM_RBUTTONUP:

		// we must process WM_RBUTTONUP here instead of WM_CONTEXTMENU which will be sent to parent
		return OnRightClick(wParam, lParam);	// return 0 when we have finish processing this message


	case WM_PAINT:
		{
			//
			// the DC of ps is parent's DC,be careful for exceeding boundray of the this window
			//
			return OnPaint(wParam, lParam);
		}
	case WM_ERASEBKGND:
		return OnEraseBkGnd(wParam, lParam);

	case WM_WINDOWPOSCHANGING:
		return 0;

	case WM_WINDOWPOSCHANGED:

	// we process this message rather than WM_SIZE to be more efficient
		return OnPosChanged(wParam, lParam);

	case WM_SIZING:
		return OnSize(wParam, lParam);

	case CVocabConst::MSG_RCV_MENUITEM_WND:
		return OnMsgRcvPopWnd(wParam);

	case WM_DRAWITEM:
		return OnDrawMenuItem(lParam);

	case WM_MEASUREITEM:
		return OnMeasureMenuItem(lParam);

	case CVocabConst::MSG_DEL_MENUDC:
		OnDelMenuDC();
		return 0;
	}
	return ::DefWindowProc(m_hWnd, uMsg, wParam, lParam);
}

LRESULT	CALLBACK	CLabelEx::_LabelEx_cbt_HookProc(int code, WPARAM wParam, LPARAM lParam)
{
	if(0 > code)goto _LabelEx_CbtHookProc_exit;
	LPCBT_CREATEWND pCBTCreateWnd;

	if(HCBT_CREATEWND == code)
	{
		
		pCBTCreateWnd = reinterpret_cast<LPCBT_CREATEWND>(lParam);
		
		if(0x8000/* class name of popup menu */ == (int)pCBTCreateWnd->lpcs->lpszClass)
		{
			//OutputDebugString(L"hook\n");

			// when code is HCBT_CREATEWND,the wParam is the handle of window being created
			CLabelEx::_LabelEx_hWndPopMenu = (HWND)wParam;
			::PostMessage(CLabelEx::_LabelEx_hWndMenuOwner/* the label */, CVocabConst::MSG_RCV_MENUITEM_WND, wParam, 0);
		}	
	}
	if(HCBT_DESTROYWND == code)
	{
		if(0 != CLabelEx::_LabelEx_hCbtHook)
		{
			::UnhookWindowsHookEx(CLabelEx::_LabelEx_hCbtHook);
			::PostMessage(CLabelEx::_LabelEx_hWndMenuOwner, CVocabConst::MSG_DEL_MENUDC, 0, 0);
		}
	}

_LabelEx_CbtHookProc_exit:
	return ::CallNextHookEx(CLabelEx::_LabelEx_hCbtHook, code, wParam, lParam);
}
