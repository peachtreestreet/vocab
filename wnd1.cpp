
////////////////////////////////////////////////////////////////////////////////////////////////////

#include "wnd1.h"
#include "vocabularyDlg.h"

////////////////////////////////////////////////////////////////////////////////////////////

#ifdef _DEBUG
#define new new(_NORMAL_BLOCK, __FILE__, __LINE__)
#endif

////////////////////////////////////////////////////////////////////////////////////////////

using namespace ATL;

////////////////////////////////////////////////////////////////////////////////////////////////////

extern vocabGlobal globalVar;

////////////////////////////////////////////////////////////////////////////////////////////////////

CWndOnlyBorder::CWndOnlyBorder(){}

CWndOnlyBorder::~CWndOnlyBorder(){}

LRESULT CWndOnlyBorder::WndProc(UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	switch(uMsg)
	{
	case WM_NCCREATE:
		return TRUE;

	case WM_NCHITTEST:
		return HTCAPTION;

case WM_NCCALCSIZE:
	{
		CRect rcWnd;

		if(0 == wParam && 0 != lParam)
		{
			CRect *prcWnd = reinterpret_cast<CRect*>(lParam);
			::CopyRect(&rcWnd, prcWnd);
			prcWnd->left+=CvocabularyDlg::BORDER_WIDTH;
			prcWnd->top+=CvocabularyDlg::BORDER_WIDTH;
			prcWnd->right-=CvocabularyDlg::BORDER_WIDTH;
			prcWnd->bottom-=CvocabularyDlg::BORDER_WIDTH;
		}
		return 0;
	}
	
	case WM_NCLBUTTONDOWN:
		{
			MSG msg;
			CPoint ptStart, ptMouse;
			CRect rcWnd, rcClient;
			HBRUSH hBrushFrame;
			HDC hWndDC;
			HRGN hRgnWnd, hRgnClient, hRgnBorder;

			hWndDC = GetWindowDC();
			GetWindowRect(&rcWnd);
			rcWnd.OffsetRect(-rcWnd.left, -rcWnd.top);
			rcClient = rcWnd;
			hBrushFrame = ::CreateSolidBrush(RGB(255, 255, 0));

			rcClient = rcWnd;
			rcClient.OffsetRect(-rcClient.left, -rcClient.top);
			rcClient.DeflateRect(CvocabularyDlg::BORDER_WIDTH, CvocabularyDlg::BORDER_WIDTH, CvocabularyDlg::BORDER_WIDTH, CvocabularyDlg::BORDER_WIDTH);
			rcWnd.OffsetRect(-rcWnd.left, -rcWnd.top);

			hRgnWnd = ::CreateRectRgn(rcWnd.left, rcWnd.top, rcWnd.right, rcWnd.bottom);
			hRgnClient = ::CreateRectRgn(rcClient.left, rcClient.top, rcClient.right, rcClient.bottom);
			hRgnBorder = ::CreateRectRgn(0, 0, 0, 0);
			::CombineRgn(hRgnBorder, hRgnWnd, hRgnClient, RGN_DIFF);
			SetWindowRgn(hRgnBorder, 0);
			::FillRect(hWndDC, &rcWnd, hBrushFrame);

			SetCapture();
			GetWindowRect(&rcWnd);
			ptMouse.x = GET_X_LPARAM(lParam);
			ptMouse.y = GET_Y_LPARAM(lParam);
			ptStart = ptMouse;

			while(TRUE)
			{
				::GetMessage (&msg,m_hWnd,0,0);

				if(WM_PAINT == msg.message)
				{
					ValidateRect(0);
					GetWindowRect(&rcClient);
					rcClient.OffsetRect(-rcClient.left, -rcClient.top);
					::FillRect(hWndDC, &rcClient, hBrushFrame);
					//::OutputDebugStringW(L"wnd1:::::::::::::::::::::WM_NCPAINT\n");
					continue;
				}
				if(WM_MOUSEMOVE == msg.message)
				{
					ptMouse.x = GET_X_LPARAM(msg.lParam);
					ptMouse.y = GET_Y_LPARAM(msg.lParam);
					ClientToScreen(&ptMouse);
					MoveWindow(rcWnd.left + (ptMouse.x - ptStart.x), rcWnd.top + (ptMouse.y - ptStart.y), rcWnd.Width(), rcWnd.Height(), TRUE);
					//::OutputDebugStringW(L"-----------------------WND1 MOVING-----------------\n");
				}
				if(WM_LBUTTONUP == msg.message)
				{
					ShowWindow(0);
					::BringWindowToTop(GetParent());
					PostMessage(WM_CLOSE);
					::PostMessageW(GetParent(), 0xff00, ptMouse.x - ptStart.x, ptMouse.y - ptStart.y);
					//::OutputDebugStringW(L"-----------------------END OF WND1 MOVING-----------------\n");
					break;
				}
			}

			::DeleteObject(hBrushFrame);
			::DeleteObject(hRgnBorder);
			::DeleteObject(hRgnClient);
			::DeleteObject(hRgnWnd);
			ReleaseDC(hWndDC);

			return 0;
		}
	}

	return ::DefWindowProc(m_hWnd, uMsg, wParam, lParam);
}