
////////////////////////////////////////////////////////////////////////////////////////////////////

#include "waitdlg.h"
#include "vocabularyDlg.h"
#include "tools1.h"

////////////////////////////////////////////////////////////////////////////////////////////

#ifdef _DEBUG
#define new new(_NORMAL_BLOCK, __FILE__, __LINE__)
#endif

////////////////////////////////////////////////////////////////////////////////////////////

using namespace ATL;
using namespace VocabApp;

////////////////////////////////////////////////////////////////////////////////////////////////////

extern vocabGlobal globalVar;
extern CvocabularyDlg appWnd;

////////////////////////////////////////////////////////////////////////////////////////////////////

CStringW CWaitDlg::m_strSpell, CWaitDlg::m_strNotes;
int const CWaitDlg::MSGDLG_BTN_ID[] = {0x1600, 0x1601, 0x1602};
int const CWaitDlg::DLG_MSG_WIDTH = ::GetSystemMetrics(SM_CXSCREEN) / 4 + 2 * ::GetSystemMetrics(SM_CXDLGFRAME);
int const CWaitDlg::DLG_MSG_HEIGHT = ::GetSystemMetrics(SM_CYSCREEN) / 4 + ::GetSystemMetrics(SM_CYCAPTION) + 2 * ::GetSystemMetrics(SM_CYDLGFRAME);

WCHAR const CWaitDlg::MSGDLG_BTN_TEXT[][16] = { L"&add", L"e&xit", L"&search", L"&quit" , L"done" };
WCHAR const CWaitDlg::MSGDLG_LABEL_TEXT[][16] = { L"spell:", L"explanation:", L"notes:" };
WNDPROC	CWaitDlg::_WaitDlg_oldEditProc;

////////////////////////////////////////////////////////////////////////////////////////////////////

CWaitDlg::CWaitDlg() 
{
	m_hCreationEvent = ::CreateEvent(NULL, TRUE, FALSE, NULL);
	m_bIsCancelled = FALSE;
	m_uFunc = SHOWMSG;	// use SHOWMSG as default
}

CWaitDlg::~CWaitDlg()
{
	::CloseHandle(m_hCreationEvent);
}

LRESULT CWaitDlg::OnInitDialog(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
	CRect rcClient,rcLabel;

	GetClientRect(&rcClient);
	rcLabel.left = CvocabularyDlg::X_START;
	rcLabel.right = rcClient.Width() - 2*CvocabularyDlg::X_START;
	rcLabel.top = CvocabularyDlg::Y_START;
	rcLabel.bottom = rcLabel.top + 40;
	m_label.Create(&m_label, 0, CWnd0::WND0_CLASSNAME, 0, WS_CHILD | WS_VISIBLE, rcLabel, m_hWnd, 0, globalVar.g_hInstance);
	m_label.SetPlainColor(RGB(0, 0, 100));
	m_label.SetFontHeight(140);
	m_label.ShowBorder(0);
	m_label.SetAlignment(0);
	m_label.SetFontName(CVocabApp::FONT_NAME_TAHOMA);
	m_label.SetText(CVocabConst::EMPTYSTR);
	SetWindowText(CVocabConst::EMPTYSTR);
	GetDlgItem(IDCANCEL).EnableWindow(FALSE);

	//
	// if lParam is not 0,must wait for the worker thread to set the EVENT:hResumeEvent to continue
	//
	if (0 != lParam)
	{
		HANDLE *phResumeEvent = reinterpret_cast<HANDLE*>(lParam);
		::OutputDebugString(L"#################  WAITING  ################");
		::WaitForSingleObject(*phResumeEvent, INFINITE);
		::CloseHandle(*phResumeEvent);
	}
	GetDlgItem(IDCANCEL).EnableWindow(TRUE);

	return 0;
}

LRESULT CALLBACK CWaitDlg::_WaitDlg_editProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	if (WM_KEYDOWN == uMsg && VK_RETURN == wParam)
	{
		::PostMessage(::GetParent(hWnd), WM_COMMAND, MAKELONG(CWaitDlg::MSGDLG_BTN_ID[0], BN_CLICKED), 0);
	}

	return ::CallWindowProc(_WaitDlg_oldEditProc, hWnd, uMsg, wParam, lParam);
}

LRESULT CWaitDlg::SendMsgToParent(WORD, WORD, HWND hWnd, BOOL& bHandled)
{
	GetDlgItem(IDC_EDIT_SPELL).GetWindowText(m_strSpell);
	if(ADD_RELATED == m_uFunc || ADD_NEW == m_uFunc)	GetDlgItem(IDC_EDIT_NOTES).GetWindowText(m_strNotes);
	if(m_strSpell.IsEmpty())
	{
		MessageBox(L"Please enter the spell.", L"error", MB_OK | MB_ICONSTOP);
		return 0;
	}

	GetDlgItem(IDC_EDIT_SPELL).SetWindowText(CVocabConst::EMPTYSTR);
	if(ADD_RELATED == m_uFunc)
	{
		GetDlgItem(IDC_EDIT_NOTES).SetWindowText(CVocabConst::EMPTYSTR);
		GetParent().PostMessage(CVocabConst::MSG_ADD_RELATED, 0, 0);
	}
	if(ADD_NEW == m_uFunc)
	{
		GetDlgItem(IDC_EDIT_NOTES).SetWindowText(CVocabConst::EMPTYSTR);
		GetParent().PostMessage(CVocabConst::MSG_ADD_NEW, 0, 0);
	}
	if(SEARCH == m_uFunc)
	{
		GetParent().PostMessage(CVocabConst::MSG_RCV_SPELL, TRUE/* the word will be displayed in the 'spell' label if it could be found in word table */, (LPARAM)(new CStringW(m_strSpell)));
	}

	GetDlgItem(IDC_EDIT_SPELL).SetFocus();
	return 0;
}

LRESULT CWaitDlg::OnMsgShow(UINT uMsg, WPARAM wParam/* contains DLG_FUNC value */, LPARAM lParam, BOOL& bHandled)
{
	CRect rcWnd;

	m_uFunc = wParam;
	GetWindowRect(&rcWnd);
	m_label.ShowWindow(FALSE);

	if(SHOWMSG == m_uFunc)m_label.ShowWindow(SW_SHOW);

	if(SEARCH == m_uFunc)
	{
		rcWnd.top += rcWnd.Height() / 2;
		rcWnd.left += rcWnd.Width() / 6;
	}
	if(ADD_RELATED == m_uFunc || ADD_NEW == m_uFunc)
	{
		rcWnd.top -= rcWnd.Height() / 2;
	}

	GetDlgItem(IDCANCEL).ShowWindow(SW_HIDE);
	SetWindowPos(0, &rcWnd, SWP_NOZORDER);

	::CreateWindow(CVocabConst::CLASS_NAME_STATIC, MSGDLG_LABEL_TEXT[0], WS_VISIBLE | WS_CHILD, 0, 0, 0, 0, m_hWnd, (HMENU)IDC_LBL_FROM, globalVar.g_hInstance, 0);
	::CreateWindow(CVocabConst::CLASS_NAME_EDIT, 0, WS_VISIBLE | WS_TABSTOP | WS_CHILD | WS_BORDER, 0, 0, 0, 0, m_hWnd, (HMENU)IDC_EDIT_SPELL, globalVar.g_hInstance, 0);
	::CreateWindow(CVocabConst::CLASS_NAME_BTN, MSGDLG_BTN_TEXT[0], WS_TABSTOP | WS_VISIBLE | WS_CHILD, 0, 0, 0, 0, m_hWnd, (HMENU)CWaitDlg::MSGDLG_BTN_ID[0], globalVar.g_hInstance, 0);
	::CreateWindow(CVocabConst::CLASS_NAME_BTN, MSGDLG_BTN_TEXT[1], WS_TABSTOP | WS_VISIBLE | WS_CHILD, 0, 0, 0, 0, m_hWnd, (HMENU)CWaitDlg::MSGDLG_BTN_ID[1], globalVar.g_hInstance, 0);

	if(ADD_RELATED == m_uFunc || ADD_NEW == m_uFunc)
	{
		::CreateWindow(CVocabConst::CLASS_NAME_STATIC, MSGDLG_LABEL_TEXT[1], WS_VISIBLE | WS_CHILD, 0, 0, 0, 0, m_hWnd, (HMENU)IDC_LBL_TO, globalVar.g_hInstance, 0);
		::CreateWindow(CVocabConst::CLASS_NAME_EDIT, 0, WS_VISIBLE | WS_TABSTOP | WS_CHILD | WS_BORDER | ES_MULTILINE | ES_WANTRETURN, 0, 0, 0, 0, m_hWnd, (HMENU)IDC_EDIT_NOTES, globalVar.g_hInstance, 0);
	}

	int x, y;
	const int X_START = ::GetSystemMetrics(SM_CXSCREEN) / 120;
	const int Y_START = ::GetSystemMetrics(SM_CXSCREEN) / 100;

	Tools1::CalculateTextExtentPoint(globalVar.g_hDefEngFont, rcWnd, MSGDLG_LABEL_TEXT[0]);
	x = X_START;
	y = Y_START;
	GetDlgItem(IDC_LBL_FROM).MoveWindow(x, y, rcWnd.Width() + CVocabConst::X_GAP/* just a little space*/, rcWnd.Height() + CVocabConst::Y_GAP / 2);

	x += rcWnd.Width() + CVocabConst::X_GAP / 2 + CLabelEx::DEFAULT_X_MARGIN;
	GetDlgItem(IDC_EDIT_SPELL).MoveWindow(x, y, DLG_MSG_WIDTH * 2 / 3, rcWnd.Height());

	if(ADD_RELATED == m_uFunc || ADD_NEW == m_uFunc)
	{
		x = X_START;
		y += rcWnd.Height() + CVocabConst::Y_GAP;
		Tools1::CalculateTextExtentPoint(globalVar.g_hDefEngFont, rcWnd, MSGDLG_LABEL_TEXT[1]);
		if(FALSE == wParam)
			GetDlgItem(IDC_LBL_TO).MoveWindow(x, y, rcWnd.Width() + CVocabConst::X_GAP, rcWnd.Height() + CVocabConst::Y_GAP / 2);
		else GetDlgItem(IDC_LBL_TO).MoveWindow(x, y, rcWnd.Width()*2 + CVocabConst::X_GAP, rcWnd.Height() + CVocabConst::Y_GAP / 2);

		y += rcWnd.Height() + CVocabConst::Y_GAP;
		GetClientRect(&rcWnd);
		GetDlgItem(IDC_EDIT_NOTES).MoveWindow(x, y, rcWnd.Width() - 2 * X_START, rcWnd.Height() / 3);
		y += rcWnd.Height() / 3;
	}

	// if the blank space is too small,adjust the dialog size
	if ((rcWnd.Height() - ::GetSystemMetrics(SM_CYICON)) < y)
	{
		GetWindowRect(&rcWnd);
		MoveWindow(rcWnd.left, rcWnd.top, rcWnd.Width(), rcWnd.Height() * 3 / 2 + ::GetSystemMetrics(SM_CYICON));
	}
	GetDlgItem(IDC_LBL_FROM).SendMessage(WM_SETFONT, (WPARAM)globalVar.g_hDefEngFont, 0);

	if(ADD_RELATED == m_uFunc || ADD_NEW == m_uFunc)
	{
		GetDlgItem(IDC_LBL_TO).SendMessage(WM_SETFONT, (WPARAM)globalVar.g_hDefEngFont, 0);
	}

	HDC hScreenDC = ::GetDC(0);
	LOGFONT logFont;
	CStringW strFontName;
	HFONT hFont;

	::RtlSecureZeroMemory(&logFont, sizeof(LOGFONT));
	logFont.lfCharSet = ANSI_CHARSET;
	strFontName = CVocabApp::FONT_NAME_TAHOMA;
	Tools1::SearchSuitableFont(strFontName, logFont);
	if (0 < logFont.lfHeight/* logFont.lfHeight will be grater than 0 if enumeration is succussful */)
	{
		GetDlgItem(IDC_EDIT_SPELL).GetWindowRect(&rcWnd);

		// calculate a proper font height,in points
		logFont.lfHeight = ::GetDeviceCaps(hScreenDC, VERTSIZE)*::GetDeviceCaps(hScreenDC, LOGPIXELSY) / CVocabConst::MM_PER_INCH*rcWnd.Height()/* for a bit margin */ * 9 / 10 / ::GetSystemMetrics(SM_CYSCREEN);
		logFont.lfWidth = 0;
		logFont.lfPitchAndFamily = 0;

		hFont = ::CreateFontIndirect(&logFont);
		GetDlgItem(IDC_EDIT_SPELL).SendMessage(WM_SETFONT, (WPARAM)hFont, 0);
	}
	else
	{
		GetDlgItem(IDC_EDIT_SPELL).SendMessage(WM_SETFONT, (WPARAM)GetStockObject(SYSTEM_FONT), 0);
	}
	::ReleaseDC(0, hScreenDC);

	if(ADD_RELATED == m_uFunc || ADD_NEW == m_uFunc)
	{
		::RtlSecureZeroMemory(&logFont, sizeof(LOGFONT));
		logFont.lfCharSet = GB2312_CHARSET;
		strFontName = CVocabApp::FONT_NAME_KAITI;
		Tools1::SearchSuitableFont(strFontName, logFont);
		if (0 < logFont.lfHeight)
		{
			logFont.lfHeight = ::GetSystemMetrics(SM_CYCAPTION) * 3 / 2;		// the font height is about 1.5X to caption font height
			logFont.lfWidth = 0;
			hFont = ::CreateFontIndirect(&logFont);
			GetDlgItem(IDC_EDIT_NOTES).SendMessage(WM_SETFONT, (WPARAM)hFont, 0);
		}
		else
		{
			GetDlgItem(IDC_EDIT_NOTES).SendMessage(WM_SETFONT, (WPARAM)GetStockObject(SYSTEM_FONT), 0);
		}
	}

	// if the initial button width(SM_CXSIZE * 3) is too large to fit in the dialog,adjust it
	int nBtnWidth(::GetSystemMetrics(SM_CXSIZE) * 3);
	int nBlankWidth;
	int nSmallIconWidth(::GetSystemMetrics(SM_CXSMICON));
	int nDlgFrmWidth(::GetSystemMetrics(SM_CXDLGFRAME));

	GetClientRect(&rcWnd);
	nBlankWidth = rcWnd.Width() - nDlgFrmWidth * 2 - nBtnWidth * 2 - CVocabConst::Y_GAP * 2;
	while (nBlankWidth < nSmallIconWidth)
	{
		nBtnWidth -= nSmallIconWidth;
		nBlankWidth = rcWnd.Width() - nDlgFrmWidth * 2 - nBtnWidth * 2 - CVocabConst::Y_GAP * 2;
	}
	x = rcWnd.Width() / 2 - CVocabConst::Y_GAP - nBtnWidth;
	y = rcWnd.Height() - CVocabConst::Y_GAP / 3 - ::GetSystemMetrics(SM_CYICON) * 3 / 2;
	GetDlgItem(CWaitDlg::MSGDLG_BTN_ID[0]).MoveWindow(x, y, nBtnWidth, ::GetSystemMetrics(SM_CYICON));
	x += nBtnWidth + CVocabConst::Y_GAP * 2;
	GetDlgItem(CWaitDlg::MSGDLG_BTN_ID[1]).MoveWindow(x, y, nBtnWidth, ::GetSystemMetrics(SM_CYICON));

	::RtlSecureZeroMemory(&logFont, sizeof(LOGFONT));
	logFont.lfCharSet = DEFAULT_CHARSET;
	strFontName = CVocabApp::DEF_FONT_NAME;
	Tools1::SearchSuitableFont(strFontName, logFont);
	if (0 < logFont.lfHeight)
	{
		logFont.lfHeight = ::GetSystemMetrics(SM_CYICON);
		logFont.lfWidth = 0;
		hFont = ::CreateFontIndirect(&logFont);
	}
	else hFont = (HFONT)::GetStockObject(SYSTEM_FONT);
	GetDlgItem(CWaitDlg::MSGDLG_BTN_ID[0]).SendMessage(WM_SETFONT, (WPARAM)hFont, 0);
	GetDlgItem(CWaitDlg::MSGDLG_BTN_ID[1]).SendMessage(WM_SETFONT, (WPARAM)hFont, 0);
	GetDlgItem(IDC_EDIT_SPELL).SetFocus();

	_WaitDlg_oldEditProc = (WNDPROC)::SetWindowLongPtr(GetDlgItem(IDC_EDIT_SPELL).m_hWnd, GWLP_WNDPROC, (LONG_PTR)_WaitDlg_editProc);

	if(ADD_NEW == m_uFunc)GetDlgItem(IDC_LBL_TO).SetWindowText(MSGDLG_LABEL_TEXT[1]);
	if(ADD_RELATED == m_uFunc)GetDlgItem(IDC_LBL_TO).SetWindowText(MSGDLG_LABEL_TEXT[2]);
	if(ADD_NEW == m_uFunc || ADD_RELATED == m_uFunc)GetDlgItem(CWaitDlg::MSGDLG_BTN_ID[1]).SetWindowText(MSGDLG_BTN_TEXT[4]);
	if(SEARCH == m_uFunc)
	{
		GetDlgItem(CWaitDlg::MSGDLG_BTN_ID[0]).SetWindowText(MSGDLG_BTN_TEXT[2]);
		GetDlgItem(CWaitDlg::MSGDLG_BTN_ID[1]).SetWindowText(MSGDLG_BTN_TEXT[3]);
	}
	CenterWindow();

	return 0;
}

LRESULT CWaitDlg::OnMsgShowProgressBar(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
	CRect rcWnd;

	GetWindowRect(&rcWnd);
	ScreenToClient(rcWnd);

	CRect rc(10, 60, rcWnd.right - 10, 80);

	if (!m_progWnd.IsWindow())
	{
		m_progWnd.SetRoundRadius(16);
		m_progWnd.Create(&m_progWnd, 0, CWnd0::WND0_CLASSNAME, 0, WS_CHILD | WS_VISIBLE, rc, m_hWnd, 0, globalVar.g_hInstance);
		m_progWnd.ShowWindow(SW_SHOW);
	}
	UpdateWindow();

	return 0;
}

int CWaitDlg::SetMsgString(CStringW const& str)
{
	CRect rc;
	m_label.GetWindowRect(&rc);
	ScreenToClient(&rc);
	m_label.ShowWindow(SW_SHOW);
	m_label.SetText(str);
	m_label.UpdateWindow();

	return 0;
}

LRESULT CWaitDlg::OnClose(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
	if(m_progWnd.IsWindow())m_progWnd.DestroyWindow();

#ifdef DEBUG
	m_bModal = FALSE;
#endif // DEBUG

	DestroyWindow();
	::ResetEvent(m_hCreationEvent);

	return 0;
}

LRESULT CWaitDlg::OnCloseWnd(WORD, WORD, HWND hWnd, BOOL& bHandled)
{
	GetParent().PostMessage(WM_ENABLE, TRUE, 0);
	PostMessage(WM_CLOSE, 0, 0);
	return 0;
}

LRESULT CWaitDlg::OnAbort(WORD , WORD , HWND hWnd, BOOL& bHandled)
{
#ifdef DEBUG
	m_bModal = FALSE;
#endif // DEBUG

	m_bIsCancelled = TRUE;
	SetWindowText(L"aborting");
	GetDlgItem(IDCANCEL).EnableWindow(FALSE);
	::PostMessage(appWnd.m_hWnd,CVocabConst::MSG_CANCELLED, 0, 0);
	m_label.SetText(L"Aborting,please wait...");
	if(m_progWnd.IsWindow())m_progWnd.DestroyWindow();

	return 0;
}

int CWaitDlg::GetRelated(CString& strSpell, CString& strNotes)
{
	strSpell = m_strSpell;
	strNotes = m_strNotes;

	return m_strSpell.GetLength();
}

int CWaitDlg::RunModalLoop(HWND hParentWnd, HANDLE *phResumeEvent)
{
	BOOL result;

#ifdef DEBUG
	m_bModal = FALSE;
#endif // DEBUG
	::OutputDebugStringW(L"RunModalLoop\n");
	result = m_thunk.Init(NULL, NULL);
	if (result == FALSE)
	{
		SetLastError(ERROR_OUTOFMEMORY);
		return -1;
	}

	_AtlWinModule.AddCreateWndData(&m_thunk.cd, this);

	m_bIsCancelled = FALSE;
	::ResetEvent(m_hCreationEvent);
	HWND hDlg = ::CreateDialogParam(globalVar.g_hInstance, MAKEINTRESOURCE(IDD), hParentWnd, ATL::CDialogImplBaseT<CWindow>::StartDialogProc, (LPARAM)phResumeEvent);
	if (0 != hDlg)
	{
		HICON hIcon = ::LoadIcon(globalVar.g_hInstance, MAKEINTRESOURCE(IDI_ICON3));
		SetIcon(hIcon,TRUE);
		::SetEvent(m_hCreationEvent);
	}

	return 0;
}