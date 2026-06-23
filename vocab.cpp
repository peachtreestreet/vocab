
#include "vocab.h"
#include "errhandlingapi.h"
#include "wnd0.h"
#include "vocabularyDlg.h"
#include "labelex.h"
#include "tools1.h"

////////////////////////////////////////////////////////////////////////////////////////////

#ifdef _DEBUG
#define new new(_NORMAL_BLOCK, __FILE__, __LINE__)
#endif

////////////////////////////////////////////////////////////////////////////////////////////

using namespace VocabApp;

////////////////////////////////////////////////////////////////////////////////////////////
// global vars

vocabGlobal globalVar;
CvocabularyDlg appWnd;

////////////////////////////////////////////////////////////////////////////////////////////

WCHAR const CVocabApp::DEF_FONT_NAME[] = L"Comic Sans MS";
WCHAR const CVocabApp::FONT_NAME_KAITI[] = L"¿¬Ìå";
WCHAR const CVocabApp::FONT_NAME_TAHOMA[] = L"Tahoma";
WCHAR const CVocabApp::FONT_NAME_MSSANS[] = L"Microsoft Sans Serif";
const float CVocabConst::MM_PER_INCH = 25.4f;
const float CVocabConst::LABEL_WIDTH_FACTOR = 0.6f;
const float CVocabConst::LABEL_HEIGHT_FACTOR1 = 0.1f;
const float CVocabConst::LABEL_HEIGHT_FACTOR2 = 0.2f;
const float CVocabConst::LABEL_HEIGHT_FACTOR3 = 3.0f / 5.0f;

////////////////////////////////////////////////////////////////////////////////////////////


LRESULT	CALLBACK _app_CbtHookProc(int code, WPARAM wParam, LPARAM lParam)
{
	if(0 > code)goto _app_CbtHookProc_exit;
	
	if(HCBT_CREATEWND == code)
	{
		WINDOWINFO wi;
		DWORD dwWndStyle;

		wi.cbSize = sizeof(WINDOWINFO);
		::GetWindowInfo((HWND)wParam, &wi);
		dwWndStyle = WS_POPUP | WS_SYSMENU | WS_CAPTION;
		if(dwWndStyle == (wi.dwStyle & dwWndStyle))
		{
			if(::IsWindow(appWnd.m_hWnd))
			{
				::PostMessage(appWnd.m_hWnd, CVocabConst::MSG_RCV_FILEDLG, wParam, 0);
			}
		}
	}

_app_CbtHookProc_exit:
	
	return ::CallNextHookEx(globalVar.g_hHookCbt, code, wParam, lParam);
}

LRESULT	CALLBACK _app_MouseHookProc(int code, WPARAM wParam, LPARAM lParam)
{
	if(0 > code)goto _app_MouseHookProc_exit;

	MOUSEHOOKSTRUCT *pmhs;
	pmhs = reinterpret_cast<PMOUSEHOOKSTRUCT>(lParam);

	if(WM_LBUTTONDOWN == wParam)
	{
		globalVar.g_hWndLButtonDown = pmhs->hwnd;
		globalVar.g_bIsLBtnDown = 1;
		globalVar.g_ptLBtnDown = pmhs->pt;
	}

	if(WM_LBUTTONUP == wParam)
	{
		globalVar.g_bIsLBtnDown = 0;
	}

	if(WM_RBUTTONUP == wParam)
	{
		globalVar.g_ptRButtonUp.x = pmhs->pt.x;
		globalVar.g_ptRButtonUp.y = pmhs->pt.y;
	}

_app_MouseHookProc_exit:
	return ::CallNextHookEx(globalVar.g_hHookMouse, code, wParam, lParam);
}

BOOL InstallCbtHook()
{
	if(0 != globalVar.g_hHookCbt)return TRUE;
	globalVar.g_hHookCbt = ::SetWindowsHookEx(WH_CBT, _app_CbtHookProc, globalVar.g_hInstance, ::GetCurrentThreadId());
	return 0 == globalVar.g_hHookCbt;
}

LONG CALLBACK _app_CrashHandler(_EXCEPTION_POINTERS *pExceptionInfo)
{
	::MessageBoxW(0, L"Some unknown errors occured,the program will be terminated.", L"error", MB_OK | MB_ICONERROR);
	return EXCEPTION_EXECUTE_HANDLER;
}

PTOP_LEVEL_EXCEPTION_FILTER pPreviousEF;

int APIENTRY WinMain(HINSTANCE hInstance, HINSTANCE, PSTR , int)
{
	pPreviousEF = ::SetUnhandledExceptionFilter(_app_CrashHandler);

	_CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);

	globalVar.g_hInstance = hInstance;

	// register window class,main dialog & LabelEx
	WNDCLASSEX wndClass;

	wndClass.cbSize = sizeof(WNDCLASSEX);
	wndClass.style  = CS_SAVEBITS;		// to enhance performance
	wndClass.lpfnWndProc  = (WNDPROC)CWnd0::_wndProc;
	wndClass.cbClsExtra  = 0;
	wndClass.cbWndExtra  = 0;
	wndClass.hInstance  = hInstance;
	wndClass.hIcon  = 0;
	wndClass.hCursor  = 0;
	//wndClass.hbrBackground = ::GetSysColorBrush(::GetSysColor(COLOR_BTNFACE));

	//
	// prevent from flickering when redraw window frequently
	// this is referenced from website:https://blog.csdn.net/qinqian19/article/details/17333609
	//
	// end of reference
	//

	wndClass.hbrBackground = 0;
	wndClass.lpszMenuName  = 0;
	wndClass.lpszClassName = CWnd0::WND0_CLASSNAME;
	wndClass.hIconSm  = 0;

	if (0 == ::GetClassInfoEx(hInstance, CWnd0::WND0_CLASSNAME, &wndClass))
	{
		if(0 == ::RegisterClassEx(&wndClass))
		{
			::MessageBox(::GetDesktopWindow(), L"Failed to register window class,exiting now.", CVocabConst::EMPTYSTR, MB_APPLMODAL | MB_OK | MB_ICONSTOP);
			return FALSE;
		}
	}

	// if the specific font is not found,system default font will be used
	// a LOGFONT structure will be filled in when it return
	CStringW strFontName;
	LOGFONT logFont;

	::RtlSecureZeroMemory(&logFont, sizeof(LOGFONT));
	logFont.lfCharSet = DEFAULT_CHARSET;
	strFontName = CVocabApp::DEF_FONT_NAME;
	Tools1::SearchSuitableFont(strFontName, logFont);
	if(0 < logFont.lfHeight)	// specific font has been found
	{
		logFont.lfWidth = 0;
		logFont.lfHeight = logFont.lfHeight *CvocabularyDlg::SCREEN_HEIGHT / CVocabConst::BENCHMARK_SC_HEIGHT;
		globalVar.g_hDefEngFont = ::CreateFontIndirect(&logFont);
		if(0 != globalVar.g_hDefEngFont)globalVar.g_bIsProperFontFound = TRUE;
	}

	// if specific font has not been found, use SYSTEM_FONT
	if(FALSE == globalVar.g_bIsProperFontFound)
	{
		globalVar.g_hDefEngFont = (HFONT)::GetStockObject(SYSTEM_FONT);
	}

	globalVar.g_hHookMouse = ::SetWindowsHookEx(WH_MOUSE, _app_MouseHookProc, globalVar.g_hInstance, ::GetCurrentThreadId());

	CRect rcWnd;

	int nWidth, nHeight;

	// initialize the dialog to 1/2 size of the screen,and this is the minimize of the dialog
	nWidth = ::GetSystemMetrics(SM_CXSCREEN) * CvocabularyDlg::DLG_WIDTH_FACTOR;
	nHeight = ::GetSystemMetrics(SM_CYSCREEN) * CvocabularyDlg::DLG_HEIGHT_FACTOR;

	rcWnd.left = (::GetSystemMetrics(SM_CXSCREEN) - nWidth)/2;
	rcWnd.right = rcWnd.left + nWidth;
	rcWnd.top = (::GetSystemMetrics(SM_CYSCREEN) - nHeight)/2;
	rcWnd.bottom = rcWnd.top + nHeight;
	appWnd.Create(&appWnd, WS_EX_APPWINDOW, CWnd0::WND0_CLASSNAME, NULL, WS_POPUP, rcWnd, 0, 0, globalVar.g_hInstance);

	if(NULL == appWnd.m_hWnd)return ::GetLastError();

	appWnd.ShowWindow(SW_SHOW);
	appWnd.CenterWindow();
	appWnd.PostMessage(CVocabConst::MSG_INIT, 0, 0);

	MSG msg;
	while (0 < ::GetMessage(&msg, NULL, 0, 0))
	{
		::TranslateMessage(&msg);
		::DispatchMessage(&msg);
	}
	::UnhookWindowsHookEx(globalVar.g_hHookMouse);

	return 0;
}

