
////////////////////////////////////////////////////////////////////////////////////////////

#include "checkbox2.h"
#include "Tools1.h"

////////////////////////////////////////////////////////////////////////////////////////////

#ifdef _DEBUG
	#define new DEBUG_NEW
#endif

////////////////////////////////////////////////////////////////////////////////////////////

using namespace std;
using namespace VocabApp;

////////////////////////////////////////////////////////////////////////////////////////////

extern vocabGlobal globalVar;

////////////////////////////////////////////////////////////////////////////////////////////

CChkBox2::CChkBox2()
{
	m_bIsChecked = 0;
	m_bIsFocused = 0;

	m_Gradient.Vertex1 = 0;
	m_Gradient.Vertex2 = 1;
	m_Gradient.Vertex3 = 2;
	m_sAccelerationKeyCode = (SHORT)-1;
	m_sNumPadCode = m_sAccelerationKeyCode;

	m_clrTextColor = DEF_TEXT_COLOR;
	m_clrBkColor = ::GetSysColor(COLOR_BTNFACE);
	m_rcText.left = INVALID_POSITION;

	m_hbrPic = ::CreateSolidBrush(RGB(0,90,0));
	m_hbrText = ::CreateSolidBrush(RGB(0,0,0));
	//m_hFont.CreatePointFont(0,L("Consolas"));
	//m_hFont.CreatePointFont(0,L("Verdana"));
	//m_hFont.CreatePointFont(0,L("Gadugi"));
	m_hFont = Tools1::CreatePointFont(0,L"Lucida Sans Unicode");

	m_nID = 0;
	m_bAccelerationPrefixExist = 0;
	m_nState = CVocabConst::PLAIN;
}

CChkBox2::~CChkBox2(){}


///////////////////////////////////////////////////////////////////////////////////////////////////////////
//

int CChkBox2::SetText(CStringW const& strTxt)
{
	CSize sz;
	HDC hDC;

	hDC = GetWindowDC();
	int nAccKeyPos = strTxt.Find(CVocabConst::ACCELERATION_PREFIX);
	if(0 <= nAccKeyPos && strTxt.GetLength() > nAccKeyPos)
	{
		m_cAccChar = strTxt.Mid(nAccKeyPos+1,1)[0];
		m_bAccelerationPrefixExist = TRUE;		
	}
	m_strWndText = strTxt;

	SelectObject(hDC,m_hFont);
	::GetTextExtentPoint(hDC,m_strWndText,m_strWndText.GetLength(),&sz);

	if(m_rcWnd.Width() < sz.cx)
	{
		m_rcWnd.right = sz.cx + CHECK_RECT_WIDTH + 1;
	}
	if(CHECK_RECT_WIDTH < sz.cy)
	{
		m_rcWnd.bottom = sz.cy;
	}

	m_rcPic.top = (m_rcWnd.Height() -  CHECK_RECT_HEIGHT)/2;
	m_rcPic.bottom = m_rcPic.top + CHECK_RECT_HEIGHT;
	m_rcPic.left = m_rcWnd.left;
	m_rcPic.right = m_rcWnd.left + CHECK_RECT_WIDTH;

	m_rcText.left = m_rcPic.right + PICTURE_TEXT_DISTANCE;
	m_rcText.right = m_rcText.left + sz.cx;
	m_rcText.top = sz.cy/2;
	m_rcText.bottom = m_rcText.top + sz.cy;

	// make gradient area
	m_rcPic.DeflateRect(1,1);

	m_triVertexPlain [0] .x       =  m_rcPic.left;
	m_triVertexPlain [0] .y       =  m_rcPic.top;
	m_triVertexPlain [0] .Red     =  0xd000;
	m_triVertexPlain [0] .Green   =  0xd000;
	m_triVertexPlain [0] .Blue    =  0xd000;
	m_triVertexPlain [0] .Alpha   =  0;

	m_triVertexPlain [1] .x       =  m_rcPic.right;
	m_triVertexPlain [1] .y       =  m_rcPic.top;
	m_triVertexPlain [1] .Red     =  0xf000;
	m_triVertexPlain [1] .Green   =  0xf000;
	m_triVertexPlain [1] .Blue    =  0xf000;
	m_triVertexPlain [1] .Alpha   =  0;

	m_triVertexPlain [2] .x       =  m_rcPic.left;
	m_triVertexPlain [2] .y       =  m_rcPic.bottom; 
	m_triVertexPlain [2] .Red     =  0xf000;
	m_triVertexPlain [2] .Green   =  0xf000;
	m_triVertexPlain [2] .Blue    =  0xf000;
	m_triVertexPlain [2] .Alpha   =  0;

	m_triVertexPlain [3] .x       =  m_rcPic.right;
	m_triVertexPlain [3] .y       =  m_rcPic.bottom;
	m_triVertexPlain [3] .Red     =  0xff00;
	m_triVertexPlain [3] .Green   =  0xff00;
	m_triVertexPlain [3] .Blue    =  0xff00;
	m_triVertexPlain [3] .Alpha   =  0;

	m_triVertexPlain [4] .x       =  m_rcPic.right;
	m_triVertexPlain [4] .y       =  m_rcPic.top;
	m_triVertexPlain [4] .Red     =  0xf000;
	m_triVertexPlain [4] .Green   =  0xf000;
	m_triVertexPlain [4] .Blue    =  0xf000;
	m_triVertexPlain [4] .Alpha   =  0;

	m_triVertexPlain [5] .x       =  m_rcPic.left;
	m_triVertexPlain [5] .y       =  m_rcPic.bottom; 
	m_triVertexPlain [5] .Red     =  0xf000;
	m_triVertexPlain [5] .Green   =  0xf000;
	m_triVertexPlain [5] .Blue    =  0xf000;
	m_triVertexPlain [5] .Alpha   =  0;

	m_triVertexLBtnDown [0] .x       =  m_rcPic.left;
	m_triVertexLBtnDown [0] .y       =  m_rcPic.top;
	m_triVertexLBtnDown [0] .Red     =  0xa000;
	m_triVertexLBtnDown [0] .Green   =  0xa000;
	m_triVertexLBtnDown [0] .Blue    =  0xa000;
	m_triVertexLBtnDown [0] .Alpha   =  0;

	m_triVertexLBtnDown [1] .x       =  m_rcPic.right;
	m_triVertexLBtnDown [1] .y       =  m_rcPic.top;
	m_triVertexLBtnDown [1] .Red     =  0xd000;
	m_triVertexLBtnDown [1] .Green   =  0xd000;
	m_triVertexLBtnDown [1] .Blue    =  0xd000;
	m_triVertexLBtnDown [1] .Alpha   =  0;

	m_triVertexLBtnDown [2] .x       =  m_rcPic.left;
	m_triVertexLBtnDown [2] .y       =  m_rcPic.bottom; 
	m_triVertexLBtnDown [2] .Red     =  0xd000;
	m_triVertexLBtnDown [2] .Green   =  0xd000;
	m_triVertexLBtnDown [2] .Blue    =  0xd000;
	m_triVertexLBtnDown [2] .Alpha   =  0;

	m_triVertexLBtnDown [3] .x       =  m_rcPic.right;
	m_triVertexLBtnDown [3] .y       =  m_rcPic.bottom;
	m_triVertexLBtnDown [3] .Red     =  0xf000;
	m_triVertexLBtnDown [3] .Green   =  0xf000;
	m_triVertexLBtnDown [3] .Blue    =  0xf000;
	m_triVertexLBtnDown [3] .Alpha   =  0;

	m_triVertexLBtnDown [4] .x       =  m_rcPic.right;
	m_triVertexLBtnDown [4] .y       =  m_rcPic.top;
	m_triVertexLBtnDown [4] .Red     =  0xd000;
	m_triVertexLBtnDown [4] .Green   =  0xd000;
	m_triVertexLBtnDown [4] .Blue    =  0xd000;
	m_triVertexLBtnDown [4] .Alpha   =  0;

	m_triVertexLBtnDown [5] .x       =  m_rcPic.left;
	m_triVertexLBtnDown [5] .y       =  m_rcPic.bottom; 
	m_triVertexLBtnDown [5] .Red     =  0xd000;
	m_triVertexLBtnDown [5] .Green   =  0xd000;
	m_triVertexLBtnDown [5] .Blue    =  0xd000;
	m_triVertexLBtnDown [5] .Alpha   =  0;

	m_rcPic.InflateRect(1,1);

	ReleaseDC(hDC);

	return 0;
}

int CChkBox2::GetAccelerationVKeyCode(SHORT &sCode1,SHORT &sCode2)
{
	sCode1 = m_sAccelerationKeyCode;
	sCode2 = m_sNumPadCode;

	return 0;
}

int CChkBox2::CaculateAccelerationVKeyCode()
{
	m_sAccelerationKeyCode = ::VkKeyScan(m_cAccChar);
	m_sAccelerationKeyCode &= 0xff;

	// key in the number pad can also be used
	if('0' <= m_cAccChar && '9' >= m_cAccChar)
	{
		m_sNumPadCode = m_sAccelerationKeyCode + 0x30;
	}
	return m_sAccelerationKeyCode;
}

HWND CChkBox2::Create(HWND hWndParent,LPCWSTR lpszText,CRect const& rcWnd)
{
	m_hWnd = CWnd0::Create(this,0,CWnd0::WND0_CLASSNAME,lpszText,WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS,rcWnd,hWndParent,0,0);
	if(NULL == m_hWnd)return NULL;

	m_rcWnd = rcWnd;
	m_rcWnd.OffsetRect(-m_rcWnd.left,-m_rcWnd.top);

	m_rcPic.top = (m_rcWnd.Height() -  CHECK_RECT_HEIGHT)/2;
	m_rcPic.bottom = m_rcPic.top + CHECK_RECT_HEIGHT;
	m_rcPic.left = m_rcWnd.left;
	m_rcPic.right = m_rcWnd.left + CHECK_RECT_WIDTH;

	// make gradient area
	m_rcPic.DeflateRect(1,1);

	m_triVertexPlain [0] .x       =  m_rcPic.left;
	m_triVertexPlain [0] .y       =  m_rcPic.top;
	m_triVertexPlain [0] .Red     =  0xd000;
	m_triVertexPlain [0] .Green   =  0xd000;
	m_triVertexPlain [0] .Blue    =  0xd000;
	m_triVertexPlain [0] .Alpha   =  0;

	m_triVertexPlain [1] .x       =  m_rcPic.right;
	m_triVertexPlain [1] .y       =  m_rcPic.top;
	m_triVertexPlain [1] .Red     =  0xf000;
	m_triVertexPlain [1] .Green   =  0xf000;
	m_triVertexPlain [1] .Blue    =  0xf000;
	m_triVertexPlain [1] .Alpha   =  0;

	m_triVertexPlain [2] .x       =  m_rcPic.left;
	m_triVertexPlain [2] .y       =  m_rcPic.bottom; 
	m_triVertexPlain [2] .Red     =  0xf000;
	m_triVertexPlain [2] .Green   =  0xf000;
	m_triVertexPlain [2] .Blue    =  0xf000;
	m_triVertexPlain [2] .Alpha   =  0;

	m_triVertexPlain [3] .x       =  m_rcPic.right;
	m_triVertexPlain [3] .y       =  m_rcPic.bottom;
	m_triVertexPlain [3] .Red     =  0xff00;
	m_triVertexPlain [3] .Green   =  0xff00;
	m_triVertexPlain [3] .Blue    =  0xff00;
	m_triVertexPlain [3] .Alpha   =  0;

	m_triVertexPlain [4] .x       =  m_rcPic.right;
	m_triVertexPlain [4] .y       =  m_rcPic.top;
	m_triVertexPlain [4] .Red     =  0xf000;
	m_triVertexPlain [4] .Green   =  0xf000;
	m_triVertexPlain [4] .Blue    =  0xf000;
	m_triVertexPlain [4] .Alpha   =  0;

	m_triVertexPlain [5] .x       =  m_rcPic.left;
	m_triVertexPlain [5] .y       =  m_rcPic.bottom; 
	m_triVertexPlain [5] .Red     =  0xf000;
	m_triVertexPlain [5] .Green   =  0xf000;
	m_triVertexPlain [5] .Blue    =  0xf000;
	m_triVertexPlain [5] .Alpha   =  0;

	m_triVertexLBtnDown [0] .x       =  m_rcPic.left;
	m_triVertexLBtnDown [0] .y       =  m_rcPic.top;
	m_triVertexLBtnDown [0] .Red     =  0xa000;
	m_triVertexLBtnDown [0] .Green   =  0xa000;
	m_triVertexLBtnDown [0] .Blue    =  0xa000;
	m_triVertexLBtnDown [0] .Alpha   =  0;

	m_triVertexLBtnDown [1] .x       =  m_rcPic.right;
	m_triVertexLBtnDown [1] .y       =  m_rcPic.top;
	m_triVertexLBtnDown [1] .Red     =  0xd000;
	m_triVertexLBtnDown [1] .Green   =  0xd000;
	m_triVertexLBtnDown [1] .Blue    =  0xd000;
	m_triVertexLBtnDown [1] .Alpha   =  0;

	m_triVertexLBtnDown [2] .x       =  m_rcPic.left;
	m_triVertexLBtnDown [2] .y       =  m_rcPic.bottom; 
	m_triVertexLBtnDown [2] .Red     =  0xd000;
	m_triVertexLBtnDown [2] .Green   =  0xd000;
	m_triVertexLBtnDown [2] .Blue    =  0xd000;
	m_triVertexLBtnDown [2] .Alpha   =  0;

	m_triVertexLBtnDown [3] .x       =  m_rcPic.right;
	m_triVertexLBtnDown [3] .y       =  m_rcPic.bottom;
	m_triVertexLBtnDown [3] .Red     =  0xf000;
	m_triVertexLBtnDown [3] .Green   =  0xf000;
	m_triVertexLBtnDown [3] .Blue    =  0xf000;
	m_triVertexLBtnDown [3] .Alpha   =  0;

	m_triVertexLBtnDown [4] .x       =  m_rcPic.right;
	m_triVertexLBtnDown [4] .y       =  m_rcPic.top;
	m_triVertexLBtnDown [4] .Red     =  0xd000;
	m_triVertexLBtnDown [4] .Green   =  0xd000;
	m_triVertexLBtnDown [4] .Blue    =  0xd000;
	m_triVertexLBtnDown [4] .Alpha   =  0;

	m_triVertexLBtnDown [5] .x       =  m_rcPic.left;
	m_triVertexLBtnDown [5] .y       =  m_rcPic.bottom; 
	m_triVertexLBtnDown [5] .Red     =  0xd000;
	m_triVertexLBtnDown [5] .Green   =  0xd000;
	m_triVertexLBtnDown [5] .Blue    =  0xd000;
	m_triVertexLBtnDown [5] .Alpha   =  0;

	m_rcPic.InflateRect(1,1);

	return m_hWnd;
}

LRESULT CChkBox2::OnMouseMove(WPARAM wParam,LPARAM lParam)
{
	CRect rcWnd;
	HDC hDC;
	CPoint ptMouse,ptLBtnDown;

	//
	// only HOVERED or LBTNDOWN state will be draw in this function
	//
	GetWindowRect(&rcWnd);
	ScreenToClient(&rcWnd);
	ptMouse.x = GET_X_LPARAM(lParam);
	ptMouse.y = GET_Y_LPARAM(lParam);
	hDC = GetDC();

	// draw HOVER state in two conditions
	if(CVocabConst::HOVERED != m_nState)
	{
		if(rcWnd.PtInRect(ptMouse) && MK_LBUTTON != wParam)
		{
			DrawHoveredState(hDC);
		}
		if(FALSE == rcWnd.PtInRect(ptMouse) && rcWnd.PtInRect(ptLBtnDown) && MK_LBUTTON == wParam)
		{
			DrawHoveredState(hDC);
		}
		goto _CChkBox2_OnMouseMove_exit;
	}

	// draw LBTNDOWN state only in one condition
	if(CVocabConst::LBTNDOWN != m_nState)
	{
		ptLBtnDown  = globalVar.g_ptLBtnDown;
		::MapWindowPoints(0,m_hWnd,&ptLBtnDown,1);
		if(rcWnd.PtInRect(ptMouse) && rcWnd.PtInRect(ptLBtnDown) && MK_LBUTTON == wParam)
		{
			DrawLButtonDownState(hDC);
		}
	}

_CChkBox2_OnMouseMove_exit:

	if(m_bIsChecked)DrawCheckPic(hDC);
	ReleaseDC(hDC);

	TRACKMOUSEEVENT tm;
	tm.cbSize = sizeof(TRACKMOUSEEVENT);
	tm.dwFlags = TME_LEAVE;
	tm.hwndTrack = m_hWnd;
	tm.dwHoverTime = 0;
	::TrackMouseEvent(&tm);

	return 0;
}

LRESULT CChkBox2::OnMouseLeave(WPARAM,LPARAM)
{
	HDC hDC = GetDC();
	DrawPlainState(hDC);
	if(m_bIsChecked)DrawCheckPic(hDC);
	ReleaseDC(hDC);

	return 0;
}

LRESULT CChkBox2::OnNcHitTest(WPARAM wParam,LPARAM lParam)
{
	return HTCLIENT;
}

LRESULT CChkBox2::OnPaint(WPARAM wParam,LPARAM lParam)
{
	PAINTSTRUCT ps;
	CRect rcTxt;

	BeginPaint(&ps);
	::SetBkColor(ps.hdc,m_clrBkColor);
	::SelectObject(ps.hdc,m_hFont);
	::SelectObject(ps.hdc,m_hbrText);
	if (FALSE == IsWindowEnabled())
	{
		::SetTextColor(ps.hdc,::GetSysColor(COLOR_GRAYTEXT));
	}
	else ::SetTextColor(ps.hdc,m_clrTextColor);

	int nTxtHeight = ::DrawTextW(ps.hdc, m_strWndText, m_strWndText.GetLength(), &rcTxt, DT_VCENTER|DT_CALCRECT|DT_SINGLELINE|DT_NOCLIP);
	nTxtHeight = nTxtHeight*2;
	m_rcText.top = (m_rcWnd.Height() - nTxtHeight) / 2 - Y_POS_ADJUST;
	m_rcText.bottom = m_rcText.top + nTxtHeight;
	::DrawTextW(ps.hdc, m_strWndText, m_strWndText.GetLength(), &m_rcText, DT_LEFT|DT_VCENTER|DT_SINGLELINE|DT_NOCLIP);

	::FrameRect(ps.hdc, &m_rcPic, m_hbrPic);
	DrawPlainState(ps.hdc);
	if(m_bIsChecked)DrawCheckPic(ps.hdc);
	EndPaint(&ps);

	return 0;
}

LRESULT CChkBox2::OnLButtonDown(WPARAM wParam,LPARAM lParam)
{
	CRect rcWnd;
	CPoint ptMouse;

	GetWindowRect(&rcWnd);
	ScreenToClient(&rcWnd);
	ptMouse.x = GET_X_LPARAM(lParam);
	ptMouse.y = GET_Y_LPARAM(lParam);

	if(rcWnd.PtInRect(ptMouse))
	{
		if(CVocabConst::LBTNDOWN != m_nState)
		{
			HDC hDC = GetDC();
			DrawLButtonDownState(hDC);
			if(m_bIsChecked)DrawCheckPic(hDC);
			ReleaseDC(hDC);
			SetCapture();
			m_nState = CVocabConst::LBTNDOWN;
		}
	}

	return 0;
}

LRESULT CChkBox2::OnLButtonUp(WPARAM,LPARAM lParam)
{
	CRect rcWnd;
	CPoint ptMouse;
	HDC hDC = GetDC();

	GetWindowRect(&rcWnd);
	ScreenToClient(&rcWnd);
	ptMouse.x = GET_X_LPARAM(lParam);
	ptMouse.y = GET_Y_LPARAM(lParam);

	if(rcWnd.PtInRect(ptMouse))
	{
		ptMouse = globalVar.g_ptLBtnDown;
		::MapWindowPoints(0,m_hWnd,&ptMouse,1);
		if(rcWnd.PtInRect(ptMouse))
		{
			m_bIsChecked = 1- m_bIsChecked;
			::PostMessage(m_hWndParent,WM_COMMAND,m_nID,0);
		}
		//else
		//{
		//	PostMessage(WM_MOUSEMOVE,0,0);
		//}
		DrawHoveredState(hDC);
		if(m_bIsChecked)DrawCheckPic(hDC);
	}
	ReleaseDC(hDC);
	::ReleaseCapture();

	return 0;
}

int CChkBox2::DrawPlainState(HDC hDC)
{
	if(0 == hDC)return 0;

	if (FALSE == IsWindowEnabled())
	{
		HBRUSH h = ::GetSysColorBrush(COLOR_GRAYTEXT);
		HBRUSH h2 = ::CreateSolidBrush(RGB(255, 255, 255));
		
		::FillRect(hDC, &m_rcPic, h2);
		::FrameRect(hDC,&m_rcPic,h);
		::DeleteObject(h2);
		return 0;
	}
	else
	{
		GradientFill(hDC,m_triVertexPlain,3,&m_Gradient,1,GRADIENT_FILL_TRIANGLE);
		GradientFill(hDC,m_triVertexPlain+3,3,&m_Gradient,1,GRADIENT_FILL_TRIANGLE);	
		::FrameRect(hDC,&m_rcPic,m_hbrPic);
	}
	
	return m_nState = CVocabConst::PLAIN;
}

int CChkBox2::DrawHoveredState(HDC hDC)
{
	if(0 == hDC)return 0;

	m_rcPic.DeflateRect(1,1);
	GradientFill(hDC,m_triVertexPlain,3,&m_Gradient,1,GRADIENT_FILL_TRIANGLE);
	GradientFill(hDC,m_triVertexPlain+3,3,&m_Gradient,1,GRADIENT_FILL_TRIANGLE);	
	::FrameRect(hDC,&m_rcPic,m_hbrPic);
	m_rcPic.InflateRect(1,1);

	return m_nState = CVocabConst::HOVERED;
}

int CChkBox2::DrawLButtonDownState(HDC hDC)
{
	if(0 == hDC)return 0;

	GradientFill(hDC,m_triVertexLBtnDown,3,&m_Gradient,1,GRADIENT_FILL_TRIANGLE);
	GradientFill(hDC,m_triVertexLBtnDown+3,3,&m_Gradient,1,GRADIENT_FILL_TRIANGLE);
	SetFocus();

	return m_nState = CVocabConst::LBTNDOWN;
}

int CChkBox2::DrawCheckPic(HDC hDC)
{
	if(0 == hDC)return 0;

	int xStart,yStart;
	int xEnd,yEnd;

	xStart = m_rcPic.left+2;
	yStart = m_rcPic.top+6;

	::MoveToEx(hDC,xStart,yStart,0);
	::LineTo(hDC,xStart+3,yStart+3);
	::MoveToEx(hDC,xStart,yStart+1,0);
	::LineTo(hDC,xStart+3,yStart+4);

	::MoveToEx(hDC,xStart+3,yStart+3,0);
	xEnd = m_rcPic.right - 2;
	yEnd = m_rcPic.top + 2;
	::LineTo(hDC,xEnd,yEnd);
	::MoveToEx(hDC,xStart+3,yStart+4,0);
	yEnd = m_rcPic.top+3;
	::LineTo(hDC,xEnd,yEnd);
	::SetPixel(hDC,xStart+3,yStart+4,DEF_CHECKEDTOKEN_COLOR);

	return 0;
}

LRESULT CChkBox2::OnKillFocus(WPARAM wParam,LPARAM lParam)
{
	if(m_bIsFocused)
	{
		HDC hDC = GetDC();
		::DrawFocusRect(hDC,&m_rcText);
		PostMessage(WM_MOUSEMOVE,0,-1);
		m_bIsFocused = FALSE;
		ReleaseDC(hDC);
	}
	
	return ::DefWindowProc(m_hWnd,WM_KILLFOCUS,wParam,lParam);
}

LRESULT CChkBox2::OnSetFocus(WPARAM wParam,LPARAM lParam)
{
	if(!m_bIsFocused)
	{
		HDC hDC = GetDC();
		::DrawFocusRect(hDC,&m_rcText);
		m_bIsFocused = TRUE;
		ReleaseDC(hDC);
	}
	return ::DefWindowProc(m_hWnd,WM_SETFOCUS,wParam,lParam);
}

LRESULT CChkBox2::OnKeyDown(WPARAM wParam,LPARAM lParam)
{
	if(VK_SPACE == wParam && m_hWnd == GetFocus())	
	{
		HDC hDC = GetDC();
		DrawLButtonDownState(hDC);
		if(m_bIsChecked)DrawCheckPic(hDC);
		ReleaseDC(hDC);
	}
	return ::DefWindowProc(m_hWnd,WM_KEYDOWN,wParam,lParam);
}

LRESULT CChkBox2::OnKeyUp(WPARAM wParam,LPARAM lParam)
{
	HDC hDC = GetDC();
	if(VK_SPACE == wParam && m_hWnd == GetFocus())
	{
		CRect rc;
		CPoint pt;
		
		GetWindowRect(&rc);
		GetCursorPos(&pt);
		m_bIsChecked = 1 - m_bIsChecked;

		if(rc.PtInRect(pt))DrawHoveredState(hDC);
		else	DrawPlainState(hDC);

		if(m_bIsChecked)
		{
			DrawCheckPic(hDC);
		}
		::PostMessage(m_hWndParent,WM_COMMAND,m_nID,0);		
		goto _OnKeyUp_exit;
	}

	if(!m_bAccelerationPrefixExist)goto _OnKeyUp_exit;

	if(m_sAccelerationKeyCode == wParam || m_sNumPadCode == wParam)
	{
		CRect rc;
		CPoint pt;

		GetWindowRect(&rc);
		GetCursorPos(&pt);
		m_bIsChecked = 1 - m_bIsChecked;

		if(rc.PtInRect(pt))DrawHoveredState(hDC);
		else	DrawPlainState(hDC);

		if(m_bIsChecked)
		{
			DrawCheckPic(hDC);
		}
		SetFocus();
		::PostMessage(m_hWndParent,WM_COMMAND,m_nID,0);
	}

_OnKeyUp_exit:
	ReleaseDC(hDC);

	return 0;
}

LRESULT CChkBox2::OnEraseBkgnd(WPARAM wParam,LPARAM lParam)
{
	CRect rcWnd,rcTxt;
	HDC hDC = (HDC)wParam;
	HDC hScreenDC = ::GetDC(0);

	GetWindowRect(&rcWnd);
	ScreenToClient(&rcWnd);

	HDC hMemDC = ::CreateCompatibleDC(hScreenDC);
	HANDLE hOldBitmap;
	HANDLE hMemBitmap = ::CreateCompatibleBitmap(hScreenDC,rcWnd.Width(), rcWnd.Height());	
	hOldBitmap = ::SelectObject(hMemDC,hMemBitmap);

	LOGBRUSH logBrush;
	HBRUSH	hBrush;
	logBrush.lbStyle = BS_SOLID;
	logBrush.lbColor = ::GetSysColor(COLOR_BTNFACE);
	logBrush.lbHatch = 0;
	hBrush = ::CreateBrushIndirect (&logBrush);
	::FillRect(hMemDC,&rcWnd,hBrush);	
		
	::SetBkColor(hMemDC,m_clrBkColor);
	::SetTextColor(hMemDC,m_clrTextColor);
	::SelectObject(hMemDC,m_hFont);
	::SelectObject(hMemDC,m_hbrText);

	int nTxtHeight = ::DrawTextW(hMemDC,m_strWndText,m_strWndText.GetLength(),&rcTxt,DT_VCENTER|DT_CALCRECT|DT_SINGLELINE|DT_NOCLIP);
	nTxtHeight *= 2;
	m_rcText.top = (m_rcWnd.Height() - nTxtHeight) / 2 - Y_POS_ADJUST;
	m_rcText.bottom = m_rcText.top + nTxtHeight;
	::DrawTextW(hMemDC,m_strWndText,m_strWndText.GetLength(),&m_rcText,DT_LEFT|DT_VCENTER|DT_SINGLELINE|DT_NOCLIP);
	::FrameRect(hMemDC,&m_rcPic,m_hbrPic);
	DrawPlainState(hMemDC);
	if(m_bIsChecked)DrawCheckPic(hMemDC);
	if(::GetFocus() == m_hWnd)::DrawFocusRect(hMemDC,&m_rcText);

	::BitBlt(hDC,rcWnd.left,rcWnd.top,rcWnd.right-rcWnd.left,rcWnd.bottom-rcWnd.top,hMemDC,rcWnd.left,rcWnd.top,SRCCOPY);
	::ReleaseDC(0,hScreenDC);

	return 0;
}

LRESULT CChkBox2::OnCaptureChanged(WPARAM wParam,LPARAM lParam)
{
	::DefWindowProc(m_hWnd,WM_CAPTURECHANGED,wParam,lParam);
	PostMessage(WM_MOUSEMOVE,-1,-1);

	return 0;
}

LRESULT CChkBox2::WndProc(UINT uMsg,WPARAM wParam,LPARAM lParam)
{
	switch(uMsg)
	{
	case WM_PAINT:
		return OnPaint(wParam,lParam);

	case WM_ERASEBKGND:
		return OnEraseBkgnd(wParam,lParam);

	case WM_MOUSEMOVE:
		return OnMouseMove(wParam,lParam);

	case WM_MOUSELEAVE:
		ReleaseCapture();
		OnMouseLeave(0,0);
		return 0;

	case WM_NCHITTEST:
		return HTCLIENT;

	case WM_LBUTTONDOWN:
		return OnLButtonDown(wParam,lParam);

	case WM_LBUTTONUP:
		return OnLButtonUp(wParam,lParam);

	case WM_KEYDOWN:
		return OnKeyDown(wParam,lParam);

	case WM_KEYUP:
		return OnKeyUp(wParam,lParam);

	case WM_KILLFOCUS:
		return OnKillFocus(wParam,lParam);

	case WM_SETFOCUS:
		return OnSetFocus(wParam,lParam);

	case WM_CAPTURECHANGED:
		return OnCaptureChanged(wParam,lParam);
	}
	return ::DefWindowProc(m_hWnd,uMsg,wParam,lParam);
}
