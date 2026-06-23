
#include "vocab.h"
#include "ProgressWnd.h"

////////////////////////////////////////////////////////////////////////////////////////////

#ifdef _DEBUG
#define new new(_NORMAL_BLOCK, __FILE__, __LINE__)
#endif

////////////////////////////////////////////////////////////////////////////////////////////////////

using namespace ATL;

////////////////////////////////////////////////////////////////////////////////////////////////////

CProgressWnd::CProgressWnd()
{
	m_clrStart1 = DEF_START_COLOR;
	m_clrStart2 = DEF_START_COLOR;
	m_clrEnd1 = DEF_END_COLOR;
	m_clrEnd2 = DEF_END_COLOR;

	m_nCornerRadius = DEF_CORNER_WIDTH;
	m_nBorderWidth = DEF_BORDER_WIDTH;
	m_clrFrame = DEF_FRAME_COLOR;
	m_clrBkGround = DEF_BK_COLOR;
	m_hBorderPen = ::CreatePen(PS_SOLID, m_nBorderWidth, m_clrFrame);

	m_logBrush.lbStyle = BS_HOLLOW;
	m_logBrush.lbColor = 0;
	m_logBrush.lbHatch = 0;
	m_hBorderBrush = ::CreateBrushIndirect(&m_logBrush);

	m_nCurrentPos = 0;
	m_nFilledWidthFactor = DEF_WIDTH_FACTOR;
	m_uTimerID = 1;
}

CProgressWnd::~CProgressWnd(){}

void WINAPI _progWnd_timerProc(HWND hWnd, UINT, UINT_PTR, DWORD)
{
	::PostMessage(hWnd, CVocabConst::VOCAB_MSG::MSG_DRAW_NEXT_POS, 0, 0);
	//::OutputDebugStringW(L"CProgressWnd::_CProgressWnd_timerProc\n");
}

//
// thie piece of code is copied from the MFC source code:afxdrawmanager.cpp
//
int CProgressWnd::_FillGradient(HDC hdc,CRect const& rect, COLORREF colorStart, COLORREF colorFinish)
{
	// this will make 2^6 = 64 fountain steps
	int nShift = 6;
	int nSteps = 1 << nShift;
	HBRUSH hbr;

	for (int i = 0; i < nSteps; i++)
	{
		// do a little alpha blending
		BYTE bR = (BYTE)((GetRValue(colorStart) *(nSteps - i) + GetRValue(colorFinish) * i) >> nShift);
		BYTE bG = (BYTE)((GetGValue(colorStart) *(nSteps - i) + GetGValue(colorFinish) * i) >> nShift);
		BYTE bB = (BYTE)((GetBValue(colorStart) *(nSteps - i) + GetBValue(colorFinish) * i) >> nShift);

		hbr = ::CreateSolidBrush(RGB(bR, bG, bB));

		// then paint with the resulting color
		CRect r2 = rect;
		r2.bottom = rect.bottom - ((i * rect.Height()) >> nShift);
		r2.top = rect.bottom - (((i + 1) * rect.Height()) >> nShift);
		if (r2.Height() > 0)::FillRect(hdc, r2, hbr);

		::DeleteObject(hbr);
	}

	return 0;
}

LRESULT CProgressWnd::OnPaint(WPARAM ,LPARAM )
{
	PAINTSTRUCT ps;
	CRect rcFill;
	HDC hScreenDC,hMemDC;
	HANDLE hMemBitmap;
	HBRUSH hBrush;

	BeginPaint(&ps);
	GetWindowRect(&m_rcWnd);
	m_rcWnd.OffsetRect(-m_rcWnd.left, -m_rcWnd.top);

	hScreenDC = ::GetDC(0);
	hMemDC = ::CreateCompatibleDC(hScreenDC);
	hMemBitmap = ::CreateCompatibleBitmap(hScreenDC, m_rcWnd.Width(), m_rcWnd.Height());
	::SelectObject(hMemDC, hMemBitmap);
	hBrush = ::CreateSolidBrush(m_clrBkGround);
	::SelectObject(hMemDC, m_hBorderPen);
	::SelectObject(hMemDC, m_hBorderBrush);

	rcFill = ps.rcPaint;
	rcFill.left = m_nCurrentPos;
	rcFill.right = rcFill.left + m_rcWnd.Width() / m_nFilledWidthFactor;
	m_nFilledAreaWidth = rcFill.Width();

	::FillRect(hMemDC, &m_rcWnd, hBrush);
	CRect rectSecond = rcFill;
	rectSecond.top = rcFill.Height() / 2;
	rectSecond.bottom = rcFill.Height();
	rcFill.bottom = rcFill.top + rcFill.Height() / 2;

	_FillGradient(hMemDC, rcFill, m_clrStart1, m_clrEnd1);
	_FillGradient(hMemDC, rectSecond, m_clrEnd2, m_clrStart2);

	::RoundRect(hMemDC, m_rcWnd.left, m_rcWnd.top, m_rcWnd.right, m_rcWnd.bottom, m_nCornerRadius, m_nCornerRadius);
	ResetCornerPixel(hMemDC);

	::BitBlt(ps.hdc, 0, 0, m_rcWnd.Width(), m_rcWnd.Height(), hMemDC, 0, 0, SRCCOPY);
	if(m_nCurrentPos > ps.rcPaint.right)m_nCurrentPos = 0;

	EndPaint(&ps);
	::DeleteObject(hMemBitmap);
	::DeleteDC(hMemDC);
	::ReleaseDC(0,hScreenDC);

	return 0L;
}

int CProgressWnd::ResetCornerPixel(HDC hDC)
{
	int nStartX,nStartY,nEndX,nEndY;
	COLORREF clr(::GetSysColor(COLOR_BTNFACE));

	nStartX = m_rcWnd.left;
	nEndX = m_rcWnd.left + m_nCornerRadius;
	nStartY = m_rcWnd.top;
	nEndY = m_rcWnd.top + m_nCornerRadius;

	for(int i=nStartX;i<nEndX;i++)
	{
		for(int j=nStartY;j<nEndY;j++)
		{
			if (m_clrFrame != GetPixel(hDC, i, j))
			{
				SetPixel(hDC, i, j, clr);
			}
			else break;
		}
	}

	nStartY = m_rcWnd.bottom;
	nEndY = m_rcWnd.bottom - m_nCornerRadius;

	for(int i=nStartX;i<nEndX;i++)
	{
		for(int j=nStartY;j>nEndY;j--)
		{
			if (m_clrFrame != GetPixel(hDC, i, j))
			{
				SetPixel(hDC, i, j, clr);
			}
			else break;
		}
	}

	nStartX = m_rcWnd.right;
	nEndX = m_rcWnd.right - m_nCornerRadius;
	for (int i = nStartX;i > nEndX;i--)
	{
		for (int j = nStartY;j > nEndY;j--)
		{
			if (m_clrFrame != GetPixel(hDC, i, j))
			{
				SetPixel(hDC, i, j, clr);
			}
			else break;
		}
	}
	nStartY = m_rcWnd.top;
	nEndY = m_rcWnd.top + m_nCornerRadius;
	for (int i = nStartX;i > nEndX;i--)
	{
		for (int j = nStartY;j < nEndY;j++)
		{
			if (m_clrFrame != GetPixel(hDC, i, j))
			{
				SetPixel(hDC, i, j, clr);
			}
			else break;
		}
	}

	return 0;
}

LRESULT CProgressWnd::OnMsgDrawNextPos(WPARAM wParam,LPARAM lParam)
{
	m_nCurrentPos += m_nFilledAreaWidth/3;
	Invalidate(TRUE);

	return 0;
}

HWND CProgressWnd::Create(LPVOID lpParam, DWORD dwExStyle, LPCWSTR szClassName, LPCWSTR szWndName,
	DWORD dwStyle, CRect const& rcWnd, HWND hWndParent, HMENU hMenu, HINSTANCE hInstance)
{
	CWnd0::Create(lpParam, dwExStyle, szClassName, szWndName, WS_CHILD | dwStyle, rcWnd, hWndParent, hMenu, hInstance);
	if(0 != m_hWnd)	
	{
		m_uTimerID = ::SetTimer(m_hWnd, m_uTimerID, DEF_EXPIRATION_TIME, _progWnd_timerProc);
	}
	return m_hWnd;
}

LRESULT CProgressWnd::OnDestroy(WPARAM wParam,LPARAM lParam)
{
	::KillTimer(m_hWnd, m_uTimerID);
	::DestroyWindow(m_hWnd);
	//::OutputDebugStringW(L"CProgressWnd::OnDestroy\n");
	return 0;
}

LRESULT CProgressWnd::WndProc(UINT uMsg,WPARAM wParam,LPARAM lParam)
{
	switch (uMsg)
	{
	case WM_PAINT:
		return OnPaint(wParam, lParam);

	case WM_DESTROY:
		return OnDestroy(wParam, lParam);

	case CVocabConst::VOCAB_MSG::MSG_DRAW_NEXT_POS:
		return OnMsgDrawNextPos(wParam, lParam);
	}

	return ::DefWindowProc(m_hWnd, uMsg, wParam, lParam);
}
