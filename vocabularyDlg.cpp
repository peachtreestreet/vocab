

////////////////////////////////////////////////////////////////////////////////////////////////////

#include <windows.h>
#include <commdlg.h>
#include <shobjidl.h>
#include "vocabularyDlg.h"
#include "tools1.h"
#include <string>
#include <numeric>
#include <algorithm>
#include <random>

////////////////////////////////////////////////////////////////////////////////////////////

#ifdef _DEBUG
#define new new(_NORMAL_BLOCK, __FILE__, __LINE__)
#endif

////////////////////////////////////////////////////////////////////////////////////////////////////

using namespace std;
using namespace ATL;
using namespace VocabApp;

////////////////////////////////////////////////////////////////////////////////////////////////////

CLabelEx				_label_Tip;	// for file dialog using
CWndOnlyBorder CvocabularyDlg::_FrameWnd;
extern vocabGlobal globalVar;
extern 	CvocabularyDlg appWnd;
extern BOOL InstallCbtHook();

////////////////////////////////////////////////////////////////////////////////////////////////////

typedef BOOL(CALLBACK* InstallKBHook)(HMODULE hDllModule, HWND hWnd);
typedef BOOL(CALLBACK* UninstallKBHook)();

////////////////////////////////////////////////////////////////////////////////////////////////////


HANDLE CvocabularyDlg::_vocabDlg_Event1 = 0;
int		CvocabularyDlg::SCREEN_WIDTH = ::GetSystemMetrics(SM_CXSCREEN);
int		CvocabularyDlg::SCREEN_HEIGHT = ::GetSystemMetrics(SM_CYSCREEN);
int		CvocabularyDlg::SMICON_WIDTH = ::GetSystemMetrics(SM_CXSMICON);

int	const	CvocabularyDlg::MIN_WND_WIDTH = 600;
int	const	CvocabularyDlg::MIN_WND_HEIGHT = 768;
int const CvocabularyDlg::X_START = 10;
int const CvocabularyDlg::Y_START = 10;

float const CvocabularyDlg::CX_ADJUST = 1.1f;
float const CvocabularyDlg::CY_ADJUST = 1.3f;
float const CvocabularyDlg::ADJUST1 = 1.5f;
float const CvocabularyDlg::DLG_WIDTH_FACTOR = 0.4f;
float const CvocabularyDlg::DLG_HEIGHT_FACTOR = 0.7f;

WCHAR const CvocabularyDlg::TITLE_DLG_CREATION[] = L"select a text file";
WCHAR const CvocabularyDlg::TITLE_DLG_SELECTION[] = L"select a database file";
WCHAR const CvocabularyDlg::FILTER_STRING_CREATE[] = L"text file(*.txt)\0*.txt\0\0";
WCHAR const CvocabularyDlg::FILTER_STRING_SELECT[] = L"Compact SQL database(*.sdf)\0*.sdf\0Microsoft Access file(*.mdb)\0*.mdb\0\0";
WCHAR const CvocabularyDlg::DIRECTORY_INIT[] = L"c:\\";
WCHAR const CVocabConst::REG_SUBKEY_NAME[] = L"software\\vocab";
WCHAR const CvocabularyDlg::BTN_TEXT[][16]={L"&create", L"&select", L"&next(1)", L"&previous(2)", L"reread(4)", L"&find", L"&add", L"e&xit"};
WCHAR const CvocabularyDlg::LABEL_TEXT[][16]={L"from:",L"to:"};
WCHAR const CvocabularyDlg::WND_TITLE[] = L"vocab";
WCHAR const CvocabularyDlg::FILEDLG_TIP[] = L"only .sdb or .mdb file is supported";
WCHAR const CvocabularyDlg::APP_TITLE[] = L"vocabDlg";

////////////////////////////////////////////////////////////////////////////////////////////////////

CvocabularyDlg::CvocabularyDlg()
{
	m_pWorkThread = 0;
	m_pWaitDlg = 0;
	m_hMemDC = 0;
	m_hMemBitmap = 0;
	m_pPreviousArea = 0;
	m_vecRandomIndex.clear();
}

CvocabularyDlg::~CvocabularyDlg()
{
	if(0 != m_pWaitDlg)delete m_pWaitDlg;
	if(0 != m_pWorkThread)delete m_pWorkThread;
	m_vecRandomIndex.clear();
}

LRESULT CvocabularyDlg::OnMsgInit(WPARAM,LPARAM)
{
	SetWindowText(APP_TITLE);
	EnableWindow(FALSE);

	int nDlgWidth, nDlgHeight;

	// initialize the dialog to 3/4 size of the screen,and this is the minimize of the dialog
	nDlgWidth = ::GetSystemMetrics(SM_CXSCREEN) * DLG_WIDTH_FACTOR;
	nDlgHeight = ::GetSystemMetrics(SM_CYSCREEN) * DLG_HEIGHT_FACTOR;
	if (nDlgWidth < MIN_WND_WIDTH)nDlgWidth = MIN_WND_WIDTH;
	if (nDlgHeight < MIN_WND_HEIGHT)nDlgHeight = MIN_WND_HEIGHT;
	MoveWindow((::GetSystemMetrics(SM_CXSCREEN) - nDlgWidth)/2, (::GetSystemMetrics(SM_CYSCREEN) - nDlgHeight)/2, nDlgWidth , nDlgHeight, TRUE);
	CreateChildWnd();

	_Vocab_THREAD_STARTUP startup;
	::RtlSecureZeroMemory(&startup, sizeof(_Vocab_THREAD_STARTUP));

	startup.hEventMsgQueue = ::CreateEvent(NULL, TRUE, FALSE, NULL);
	m_pWorkThread = new CVocabWorkThread();
	if (FALSE == m_pWorkThread->CreateWorkThread((LPARAM)&startup))
	{
		::CloseHandle(startup.hEventMsgQueue);
		ShowMsgBox(L"Creation of work thread is failed,the program can not continue.", L"error");
		return 0;
	}
	if (WAIT_OBJECT_0 != ::WaitForSingleObject(startup.hEventMsgQueue, INFINITE))
	{
		::CloseHandle(startup.hEventMsgQueue);
		ShowMsgBox(L"Errors occured while creating message queue,the program can not continue.", L"error");
		return 0;
	}
	m_pWorkThread->SetCreator(m_hWnd);
	
	//STARTUPINFOW si;
	//CStringW strPath;
	//CStringA strProcName;
	//::RtlSecureZeroMemory(&si, sizeof(STARTUPINFOW));
	//si.cb = sizeof(si);
	//::GetStartupInfo(&si);
	//strPath = si.lpTitle;
	//strPath = strPath.Left(strPath.ReverseFind(CVocabConst::SLASH) + 1);
	//strPath += L"KBHook.dll";
	//HMODULE hDll = ::LoadLibrary(strPath);
	//strProcName = "InstallKBHook";
	//InstallKBHook p;
	//p = (InstallKBHook)::GetProcAddress(hDll, strProcName.GetBuffer());
	//BOOL bInstall = p(hDll, appWnd);
	globalVar.g_hHookKeyboard = ::SetWindowsHookEx(WH_KEYBOARD, _vocabDlg_KeyboardProc, 0, GetCurrentThreadId());


	/*************************************************************
	// in CWaitDlg::OnInitDialog(),the 'abort' button will be disabled and,wait for CVocabWorkThread::OnMsgPrepareData() is exectuing where hResumeEvent will be set by ::SetEvent()
	//    and then,the 'abort' button will be enabled  
	//   EVENT is used here to ensure the right execution sequence(other more proficient way maybe exist)


	// if the 'abort' has never been clicked,execution sequence is:
	//	CvocabularyDlg::OnMsgInit()->
	//	MAYBE CVocabWorkThread::OnMsgPrepareData() OR CWaitDlg::OnInitDialog(will wait for CVocabWorkThread::OnMsgPrepareData() setting Event to continue )
	//	CvocabularyDlg::OnMsgDisableCancelling()->
	//	CvocabularyDlg::OnMsgPreparationCompleted()->
	//	CWaitDlg::OnClose()->
	//	END
	//
	// if the 'abort' has been clicked before a task being executing in worker thread,execution sequence is(for exp start from CvocabularyDlg::OnMsgInit()):
	//	CvocabularyDlg::OnMsgInit()->
	//	CVocabWorkThread::OnMsgPrepareData()->
	//	CWaitDlg::OnInitDialog():disable the 'abort' and wait for CVocabWorkThread::OnMsgPrepareData() setting Event to continue ->
	//	CVocabWorkThread::OnMsgPrepareData() : SETEVENT and go on
	//	CWaitDlg::OnInitDialog():enable the 'abort' ->
	//	user click the 'abort' button ->
	//	CWaitDlg::OnAbort()->
	//	CvocabularyDlg::OnMsgCancelled()->
	//	CVocabWorkThread::OnDisgardData()->
	//	END
	//
	*************************************************************/

	if (0 == m_pWaitDlg)m_pWaitDlg = new CWaitDlg;
	HANDLE hResumeEvent = ::CreateEvent(NULL, TRUE, FALSE, NULL);
	m_pWorkThread->PostThreadMsg(CVocabConst::MSG_PREPARE_DATA, (WPARAM)&m_pWaitDlg->GetCreationEvent(), (LPARAM)&hResumeEvent);
	m_pWaitDlg->RunModalLoop(m_hWnd, &hResumeEvent);
	m_pWaitDlg->CenterWindow();

	return 0;
}

LRESULT CvocabularyDlg::OnMsgUpdateWaitDlg(WPARAM wParam, LPARAM lParam)
{
	CStringW *pstrTitle = reinterpret_cast<CStringW *>(wParam);
	CStringW *pstrMsg = reinterpret_cast<CStringW *>(lParam);
	if (0 != m_pWaitDlg)
	{
		if (m_pWaitDlg->IsWindow())
		{
			m_pWaitDlg->PostMessage(CVocabConst::VOCAB_MSG::MSG_SHOW_PROGRESS, 0, 0);
			if (0 != pstrTitle)
			{
				m_pWaitDlg->SetWindowText(*pstrTitle);
			}
			else
			{
				m_pWaitDlg->SetWindowText(CVocabConst::WAITDLG_TITLE);
			}
			if (0 != pstrMsg)
			{
				m_pWaitDlg->SetMsgString(*pstrMsg);
			}
		}
	}
	
	if (0 != pstrTitle)		delete pstrTitle;
	if (0 != pstrMsg)		delete pstrMsg;

	return 0;
}

void CvocabularyDlg::OnClickBtnExit()
{
	//
	// step1:
	// post message MSG_STOP_WORKING to work thread
	// the work thread executes ending work,if any errors occured while doing these, a messageBox will be displayed
	// maybe there is no errors and no messageBox is displayed

	// step2:
	// the work thread posts MSG_SESSION_END to this after completing the ending work,AND,wait for the _vocabDlg_Event1 to be signaled to continue
	// if no messageBox is found,set the _vocabDlg_Event1 to signaled immediately after receiving MSG_SESSION_END message
	// if a messageBox is found,set the _vocabDlg_Event1 to signaled only when messageBox is destroyed

	// step3:
	// the work thread gets WAIT_OBJECT_0 from WaitForSingleObject() and then post WM_QUIT to itself
	// CvocabularyDlg::OnMsgSessionEnd() post WM_CLOSE to this window
	// END
	//

	CvocabularyDlg::_vocabDlg_Event1 = ::CreateEvent(NULL, TRUE, FALSE, NULL);
	m_bForcedQuit = FALSE;
	::CoUninitialize();
	EnableWindow(FALSE);

	//// remove font resource
	//int n=::RemoveFontResourceEx(L"f:\\mvboli.ttf",FR_PRIVATE,0);

	::UnhookWindowsHookEx(globalVar.g_hHookKeyboard);

	if (0 == m_pWaitDlg)
	{
		m_pWaitDlg = new CWaitDlg;
	}

	m_pWaitDlg->RunModalLoop(m_hWnd, 0);
	m_pWaitDlg->CenterWindow();
	m_pWaitDlg->SetWindowText(L"exiting");
	m_pWaitDlg->SetMsgString(L"waiting for completion of all work...");
	m_pWaitDlg->GetDlgItem(IDCANCEL).ShowWindow(FALSE);

	UINT uStartGrp, uEndGrp;
	if(m_chkShowUnfamiliar.IsChecked())
	{
		uStartGrp = (m_comboGroupStart.GetCurSel() * CVocabConst::TWOE16) + m_usStartGrp;
		uEndGrp = (m_comboGroupEnd.GetCurSel() * CVocabConst::TWOE16) + m_usEndGrp;
		m_pWorkThread->GetRecord()->bUsingUnfamiliar = TRUE;
	}
	else
	{
		uStartGrp = m_comboGroupStart.GetCurSel() + m_usStartGrp2 * CVocabConst::TWOE16;
		uEndGrp = m_comboGroupEnd.GetCurSel() + m_usEndGrp2 * CVocabConst::TWOE16;
		m_pWorkThread->GetRecord()->bUsingUnfamiliar = FALSE;
	}

	m_pWorkThread->GetRecord()->uStartGroup = uStartGrp;
	m_pWorkThread->GetRecord()->uEndGroup = uEndGrp;

	m_pWorkThread->PostThreadMsg(CVocabConst::MSG_STOP_WORKING, (WPARAM)&m_pWaitDlg->GetCreationEvent(), 0);
}

int CvocabularyDlg::ShowFileDialog(UINT nFilterCnt, WCHAR const* pcszTitle, WCHAR const* pcszFilter, CStringW& strSelectedFileName)
{
	if(MAX_FILTER_CNT < nFilterCnt)return -1;

	IFileDialog *pIFileDialog;
	IShellItem *pIShellItem(0);
	HRESULT hr;

	strSelectedFileName.Empty();
	hr = ::CoCreateInstance(CLSID_FileOpenDialog, NULL, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&pIFileDialog));
	if(0 > hr)return -1;

	COMDLG_FILTERSPEC* pFilter = new COMDLG_FILTERSPEC[nFilterCnt];
	size_t nFilterIndex = 0;
	size_t filterSize;
	LPWSTR lpwstrFilter;

	while (nFilterIndex < nFilterCnt)
	{
		CStringW strTemp;

		filterSize = _tcslen(pcszFilter)+1;
		lpwstrFilter = static_cast<LPWSTR>(new WCHAR[filterSize]);
		if (lpwstrFilter == NULL)
		{
			throw;
		}
		strTemp = pcszFilter;
		memcpy_s(lpwstrFilter, (strTemp.GetLength()+1)*sizeof(WCHAR), strTemp.GetString(), (strTemp.GetLength()+1) * sizeof(WCHAR));
		pFilter[nFilterIndex].pszName = lpwstrFilter;
		pcszFilter += filterSize;

		filterSize = _tcslen(pcszFilter) + 1;
		lpwstrFilter = static_cast<LPWSTR>(new WCHAR[filterSize]);

		if (lpwstrFilter == NULL)
		{
			throw;
		}
		strTemp = pcszFilter;
		memcpy_s(lpwstrFilter, (strTemp.GetLength()+1)*sizeof(WCHAR), strTemp.GetString(), (strTemp.GetLength()+1) * sizeof(WCHAR));
		pFilter[nFilterIndex].pszSpec = lpwstrFilter;
		pcszFilter += filterSize;

		nFilterIndex ++;
	}
	hr = pIFileDialog->SetFileTypes(nFilterCnt, pFilter);
	
	for (nFilterIndex = 0; nFilterIndex < nFilterCnt; nFilterIndex++)
	{
		delete[] pFilter[nFilterIndex].pszName;
		delete[] pFilter[nFilterIndex].pszSpec;
	}
	delete[] pFilter;
	LPWSTR wcFileName(0);
	
	pIFileDialog->SetFileTypeIndex(0);
	pIFileDialog->SetTitle(pcszTitle);
	pIFileDialog->SetOptions(0);
	hr = pIFileDialog->Show(m_hWnd);

	::UnhookWindowsHookEx(globalVar.g_hHookCbt);
	::UnhookWindowsHookEx(globalVar.g_hHookFileDlg);
	globalVar.g_hHookCbt = globalVar.g_hHookFileDlg = 0;
	::DestroyWindow(_label_Tip.m_hWnd);

	if(S_OK != hr)return 0;

	hr = pIFileDialog->GetResult(&pIShellItem);
	
	if(0 != pIShellItem)
	{
		hr = pIShellItem->GetDisplayName(SIGDN_DESKTOPABSOLUTEEDITING, &wcFileName);
		pIFileDialog->Close(0);
		pIFileDialog->Release();
		CString strResult(wcFileName);
		if (SUCCEEDED(hr))
		{
			strSelectedFileName = wcFileName;
			::CoTaskMemFree(wcFileName);
			::CoUninitialize();
			return 1;
		}
	}

	return 0;
}

LRESULT CvocabularyDlg::OnMsgDisableCancelling(WPARAM, LPARAM)
{
	// maybe the user has clicked the 'abort' button,and then the m_pWaitDlg may has been DESTROYED
	if (0 != m_pWaitDlg)
	{
		if (m_pWaitDlg->IsWindow())
		{
			m_pWaitDlg->GetDlgItem(IDCANCEL).EnableWindow(FALSE);
		}
	}
	//Sleep(500);

	return 0;
}

LRESULT CvocabularyDlg::OnMsgCancelled(WPARAM, LPARAM lParam)
{
	// MSG_CANCELLED will be only posted when user click the 'abort' button
	// so the m_pWaitDlg is BEING DISPLAYED at the time

	ATLASSERT(0 != m_pWaitDlg);
	ATLASSERT(m_pWaitDlg->IsWindow());

	m_comboGroupStart.ResetContent();
	m_comboGroupEnd.ResetContent();
	m_lblSpell.SetText(CVocabConst::EMPTYSTR);
	m_lblExplanation.SetText(CVocabConst::EMPTYSTR);
	m_lblRelated.SetText(CVocabConst::EMPTYSTR);
	m_lblProgress.SetText(CVocabConst::EMPTYSTR);
	GetDlgItem(IDC_BTN_PREVIOUS).EnableWindow(FALSE);
	GetDlgItem(IDC_BTN_NEXT).EnableWindow(FALSE);
	m_pWaitDlg->PostMessage(WM_CLOSE);
	m_pWorkThread->PostThreadMsg(CVocabConst::MSG_DISGARD_DATA, 0, 0);
	EnableWindow();

	return 0;
}

void CvocabularyDlg::OnKeyPressEscape()
{
	if (0 != m_pWaitDlg)
	{
		if (m_pWaitDlg->IsWindow())
		{
			m_pWaitDlg->PostMessage(WM_CLOSE);
			EnableWindow();
		}
	}
}

void CvocabularyDlg::OnRepeatVoice()
{
	if (FALSE == m_chkTaskRandom.IsChecked())
	{
		if(FALSE == m_chkShowUnfamiliar.IsChecked())
		m_pWorkThread->PostThreadMsg(CVocabConst::MSG_SPEAK, (WPARAM)m_ulIndex_s, 0);
		else m_pWorkThread->PostThreadMsg(CVocabConst::MSG_SPEAK, (WPARAM)m_pWorkThread->m_uVecUnfamilarIndex[m_ulIndex_s], 0);
	}
	else
	{
		if(m_ulIndex_r < m_vecRandomIndex.size())
		{
			if(FALSE == m_chkShowUnfamiliar.IsChecked())m_pWorkThread->PostThreadMsg(CVocabConst::MSG_SPEAK, (WPARAM)m_vecRandomIndex[m_ulIndex_r], 0);
			else
				m_pWorkThread->PostThreadMsg(CVocabConst::MSG_SPEAK, (WPARAM)m_pWorkThread->m_uVecUnfamilarIndex[m_vecRandomIndex[m_ulIndex_r]], 0);
		}
	}
}

void CvocabularyDlg::OnClickBtnCreate()
{
	InstallCbtHook();

	CStringW strPathName;

	ShowFileDialog(1, TITLE_DLG_CREATION, FILTER_STRING_CREATE, strPathName);
	::DestroyWindow(_label_Tip.m_hWnd);

	if(strPathName.IsEmpty())return;

	EnableWindow(FALSE);

	HANDLE hResumeEvent = ::CreateEvent(NULL, TRUE, FALSE, NULL);
	if (0 == m_pWaitDlg)
	{
		m_pWaitDlg = new CWaitDlg;
	}

	if (FALSE == m_pWaitDlg->IsWindow())
	{
		m_pWaitDlg->RunModalLoop(m_hWnd, &hResumeEvent);
	}

	m_ulIndex_s = m_ulIndex_r = 0; 
	m_pWorkThread->SetDBName(strPathName);
	m_pWorkThread->PostThreadMsg(CVocabConst::MSG_CREATE_DATABASE, (WPARAM)(&(m_pWaitDlg->GetCreationEvent())), (LPARAM)&hResumeEvent);
}

void CvocabularyDlg::OnClickBtnSelect()
{
	CStringW strPathName;
	CStringW strCurrentDBN;

	InstallCbtHook();
	ShowFileDialog(2, TITLE_DLG_SELECTION, FILTER_STRING_SELECT, strPathName);

	if(strPathName.IsEmpty())return;
	m_pWorkThread->GetDBName(strCurrentDBN);
	if(0 == strPathName.Compare(strCurrentDBN))return;

	EnableWindow(0);
	::DestroyWindow(_label_Tip.m_hWnd);

	HANDLE hResumeEvent = ::CreateEvent(NULL, TRUE, FALSE, NULL);
	if (0 == m_pWaitDlg)
	{
		m_pWaitDlg = new CWaitDlg;
	}

	m_pWorkThread->SetDBName(strPathName);
	m_pWorkThread->PostThreadMsg(CVocabConst::MSG_SELECT_DATABASE, (WPARAM)(&(m_pWaitDlg->GetCreationEvent())), (LPARAM)&hResumeEvent);

	if (FALSE == m_pWaitDlg->IsWindow())
	{
		m_pWaitDlg->RunModalLoop(m_hWnd, &hResumeEvent);
	}
}

LRESULT CvocabularyDlg::OnMsgGroupChanged(WPARAM, LPARAM lParam)
{
	UINT uWordCnt, uIndex;
	CStringW str;
	list<CSensibleInfo*> strList;

	if(m_comboGroupStart.m_hWnd == (HWND)lParam)
	{
		if(m_comboGroupEnd.GetCurSel() < m_comboGroupStart.GetCurSel())
		{
			m_comboGroupEnd.SetCurSel(m_comboGroupStart.GetCurSel());
		}
		m_ulIndex_s = m_comboGroupStart.GetCurSel() * CVocabConst::COUNT_PER_GROUP;
		
		if(FALSE == m_chkShowUnfamiliar.IsChecked())
		{
			m_usStartGrp = m_comboGroupStart.GetCurSel();
			uIndex = m_ulIndex_s;
		}
		else
		{
			m_usStartGrp2 = m_comboGroupStart.GetCurSel();
			uIndex = m_pWorkThread->m_uVecUnfamilarIndex[m_ulIndex_s];
		}
		m_lblSpell.SetText(*m_pWorkThread->m_strsSpell[uIndex]);
		m_lblExplanation.SetText(*m_pWorkThread->m_strsExplanation[uIndex]);
		MakeRelatedWordSpell(uIndex, strList);
		m_lblRelated.SetText(CVocabConst::EMPTYSTR);
		AddWordsToLabel(strList);
	}

	if(m_comboGroupEnd.m_hWnd == (HWND)lParam)
	{
		if(m_comboGroupEnd.GetCurSel() < m_comboGroupStart.GetCurSel())
		{
			m_comboGroupStart.SetCurSel(m_comboGroupEnd.GetCurSel());
			m_ulIndex_s = m_comboGroupEnd.GetCurSel() * CVocabConst::COUNT_PER_GROUP;
			PostMessage(WM_COMMAND, MAKELONG(IDC_COMBO_GROUP_END, CBN_SELENDOK), (LPARAM)m_comboGroupEnd.m_hWnd);
		}
		if (FALSE == m_chkShowUnfamiliar.IsChecked())
		{
			m_usEndGrp = m_comboGroupEnd.GetCurSel();
		}
		else
		{
			m_usEndGrp2 = m_comboGroupEnd.GetCurSel();
		}
	}

	if(m_chkShowUnfamiliar.IsChecked())
	{
		MakeProgressString(str, m_pWorkThread->m_uVecUnfamilarIndex.size());
	}
	else
	{
		MakeProgressString(str, m_pWorkThread->m_uWordCount);
	}
	m_lblProgress.SetText(str);

	GetDlgItem(IDC_BTN_NEXT).EnableWindow();
	GetDlgItem(IDC_BTN_PREVIOUS).EnableWindow();

	// always uses m_ulIndex_s because group can not be changed when selecting random mood
	if(0 == m_ulIndex_s)GetDlgItem(IDC_BTN_PREVIOUS).EnableWindow(FALSE);
	if(FALSE == m_chkShowUnfamiliar.IsChecked())
	{
		uWordCnt = m_pWorkThread->m_uWordCount - 1;
	}
	else
	{
		uWordCnt = m_pWorkThread->m_uVecUnfamilarIndex.size() - 1;
	}
	if(uWordCnt == m_ulIndex_s)GetDlgItem(IDC_BTN_NEXT).EnableWindow(FALSE);

	return 0L;
}

LRESULT CvocabularyDlg::OnMsgShowErrMsg(WPARAM, LPARAM lParam)
{
	//::OutputDebugString(L"OnMsgShowErrMsg\n");
	if (0 != m_pWaitDlg)
	{
		if (m_pWaitDlg->IsWindow())
		{
			m_pWaitDlg->PostMessage(WM_CLOSE);
		}
	}
	if (0 != lParam)
	{
		ShowMsgBox((reinterpret_cast<CStringW *>(lParam))->GetBuffer(), L"error");
		delete reinterpret_cast<CStringW *>(lParam);
		//::OutputDebugString(L"message box closed.\n");
	}
	EnableWindow();

	return 0;
}

LRESULT CvocabularyDlg::OnMsgPreparationCompleted(WPARAM wParam/* use unfamiliar or not */, LPARAM lParam)
{
	UINT uGroupCnt, uIndex;
	CStringW str;
	list<CSensibleInfo *> strList;

	const VocabRecord *pRecord = m_pWorkThread->GetRecord();

	// if lParam is FALSE,dont change this 4 members
	if(TRUE == lParam)
	{
		m_usStartGrp = pRecord->uStartGroup & CVocabConst::LOWSHORT;
		m_usEndGrp = pRecord->uEndGroup & CVocabConst::LOWSHORT;
		m_usStartGrp2 = pRecord->uStartGroup / CVocabConst::TWOE16;
		m_usEndGrp2 = pRecord->uEndGroup / CVocabConst::TWOE16;
	}

	if (TRUE == wParam)
	{
		m_chkShowUnfamiliar.SetCheck();
		m_chkShowUnfamiliar.RedrawWindow();
		OnReviseUnfamiliar();

		goto _OnMsgPreparationCompleted_exit;
	}

	uGroupCnt = pRecord->uGroupCount & CVocabConst::LOWSHORT;
	if(0 == uGroupCnt)goto _OnMsgPreparationCompleted_exit;

	if(CVocabConst::MAX_WORD_COUNT / CVocabConst::COUNT_PER_GROUP > uGroupCnt)
	{
		m_comboGroupStart.ResetContent();
		m_comboGroupEnd.ResetContent();

		for(int i=0;i<uGroupCnt;i++)
		{
			str.Format(L"%d", i + 1);
			//_itow_s(i+1,wszBuff,10);
			m_comboGroupStart.AddString(str);
			m_comboGroupEnd.AddString(str);
		}
	}

	if(0 <= uGroupCnt && uGroupCnt > m_usStartGrp)
	{
		m_comboGroupStart.SetCurSel(m_usStartGrp);
	}
	else
	{
		m_comboGroupStart.SetCurSel(0);
	}

	if(m_usStartGrp <= m_usEndGrp && uGroupCnt > m_usEndGrp)
	{
		m_comboGroupEnd.SetCurSel(m_usEndGrp);
	}
	else
	{
		m_comboGroupEnd.SetCurSel(m_comboGroupStart.GetCurSel());
	}

	if(FALSE == m_chkTaskRandom.IsChecked())
	{
		m_ulIndex_s = CVocabConst::COUNT_PER_GROUP * m_comboGroupStart.GetCurSel();
		uIndex = m_ulIndex_s;
		ChangeBtnState(m_pWorkThread->m_uWordCount, m_ulIndex_s);
	}
	else
	{
		m_ulIndex_r = 0;
		m_ulRandomWordCnt = 0;
		m_vecRandomIndex.clear();
		m_ulRandomWordCnt = CalculateSelectedCnt();
		if(m_pWorkThread->m_uWordCount < m_ulRandomWordCnt)goto _OnMsgPreparationCompleted_exit;
		if(0 == MakeRandomList(m_pWorkThread->m_uWordCount, m_ulRandomWordCnt))goto _OnMsgPreparationCompleted_exit;
		uIndex = m_vecRandomIndex[m_ulIndex_r];
		ChangeBtnState(m_ulRandomWordCnt, m_ulIndex_r);
	}

	if (std::find(m_pWorkThread->m_uVecUnfamilarIndex.begin(), m_pWorkThread->m_uVecUnfamilarIndex.end(), uIndex) != m_pWorkThread->m_uVecUnfamilarIndex.end())m_lblSpell.SetTextColor(CVocabConst::PINK);
	else m_lblSpell.SetTextColor(CVocabConst::BLACK);

	m_pWorkThread->PostThreadMsg(CVocabConst::MSG_SPEAK, (WPARAM)uIndex, 0);
	m_lblSpell.SetText(*m_pWorkThread->m_strsSpell[uIndex]);
	m_lblExplanation.SetText(CVocabConst::EMPTYSTR);
	if(m_chkShowExplanation.IsChecked())m_lblExplanation.SetText(*m_pWorkThread->m_strsExplanation[uIndex]);
	MakeRelatedWordSpell(uIndex, strList);
	AddWordsToLabel(strList);
	m_lblRelated.RedrawWindow();
	MakeProgressString(str, m_pWorkThread->m_uWordCount);
	m_lblProgress.SetText(str);
	if (0 < m_pWorkThread->m_uVecUnfamilarIndex.size())m_chkShowUnfamiliar.EnableWindow();
	m_uFoundIndex = CVocabConst::INVALID_INDEX;

_OnMsgPreparationCompleted_exit:
	// maybe the user has clicked the 'abort' button,and then the m_pWaitDlg may has been DESTROYED
	if (0 != m_pWaitDlg)
	{
		if (m_pWaitDlg->IsWindow())
		{
			m_pWaitDlg->PostMessage(WM_CLOSE);
		}
	}
	EnableWindow();
	if (GetDlgItem(IDC_BTN_NEXT).IsWindowEnabled())GetDlgItem(IDC_BTN_NEXT).SetFocus();

	return 0L;
}

LRESULT CvocabularyDlg::OnMsgRcvHoverdWord(WPARAM wParam, LPARAM lParam /* a CStringW pointer */)
{
	if (0 == lParam)
	{
		m_lblExplanation.SetText(CVocabConst::EMPTYSTR);
		return 0;
	}
	m_pWorkThread->PostThreadMsg(CVocabConst::MSG_SEARCH_SPELL, wParam, lParam);

	return 0;
}

int CvocabularyDlg::MakeRelatedWordSpell(UINT uIndex,  list<CSensibleInfo*> &info)
{
	CStringW *pstr(0);
	std::wstring wstr;
	int nPos1, nPos2;
	map <UINT, CStringW*> :: const_iterator iter;
	CSensibleInfo *pInfo;

	nPos1 = nPos2 = 0;
	iter = m_pWorkThread->m_mapRelatedIndexString.find(uIndex);
	if(m_pWorkThread->m_mapRelatedIndexString.end() == iter)return 0;
	pstr = iter->second;

	do
	{
		nPos2 = (*pstr).Find(CVocabConst::COMMA, nPos1);
		if(0 < nPos2)
		{
			wstr = (*pstr).Mid (nPos1, nPos2 - nPos1);
			nPos1 = nPos2;
			nPos1++;
		}
		else
		{
			wstr = (*pstr).Mid (nPos1, (*pstr).GetLength() - nPos1);
		}

		if (0 < wstr.size())
		{
			pInfo = new CSensibleInfo;
			pInfo->pwstr = new CStringW(wstr.c_str());
			info.push_back(pInfo);
		}
	}
	while(nPos2 > 0);
	
	return info.size();
}

void CvocabularyDlg::OnClickBtnNext()
{
	UINT uTotalCnt, uIndex;
	CStringW str;
	list<CSensibleInfo *> strList;

	static DWORD dwBeepTime(0);

	if(FALSE == GetDlgItem(IDC_BTN_NEXT).IsWindowEnabled())
	{
		if(::GetTickCount() - dwBeepTime >1000)
		{
			::Beep(2000, 10);
			dwBeepTime = ::GetTickCount();
		}
		return;
	}

	if(FALSE == m_chkTaskRandom.IsChecked())
	{
		m_ulIndex_s++;

		if (m_chkShowUnfamiliar.IsChecked())
		{
			uTotalCnt = m_pWorkThread->m_uVecUnfamilarIndex.size();
			if(m_ulIndex_s < uTotalCnt)
			{
				uIndex = m_pWorkThread->m_uVecUnfamilarIndex[m_ulIndex_s];
				if (m_comboGroupEnd.GetCurSel() < m_ulIndex_s / CVocabConst::COUNT_PER_GROUP)
				{
					m_comboGroupEnd.SetCurSel(m_ulIndex_s / CVocabConst::COUNT_PER_GROUP);
				}
			}
		}
		else
		{
			uTotalCnt = m_pWorkThread->m_uWordCount;
			uIndex = m_ulIndex_s;
		}
		ChangeBtnState(uTotalCnt, m_ulIndex_s);
	}
	else
	{
		m_ulIndex_r++;
		uTotalCnt = m_ulRandomWordCnt;
		if(m_ulIndex_r < uTotalCnt)
		{
			if (m_chkShowUnfamiliar.IsChecked())
			{
				uIndex = m_pWorkThread->m_uVecUnfamilarIndex[m_vecRandomIndex[m_ulIndex_r]];
			}
			else
			{
				uIndex = m_vecRandomIndex[m_ulIndex_r];
			}
			ChangeBtnState(uTotalCnt, m_ulIndex_r);
		}
	}
	if (std::find(m_pWorkThread->m_uVecUnfamilarIndex.begin(), m_pWorkThread->m_uVecUnfamilarIndex.end(), uIndex) != m_pWorkThread->m_uVecUnfamilarIndex.end())m_lblSpell.SetTextColor(CVocabConst::PINK);
	else m_lblSpell.SetTextColor(CVocabConst::BLACK);

	if(uIndex < m_pWorkThread->m_uWordCount)
	{
		m_lblSpell.SetText(*m_pWorkThread->m_strsSpell[uIndex]);
		if(m_chkShowExplanation.IsChecked())
		{
			m_lblExplanation.SetText(*m_pWorkThread->m_strsExplanation[uIndex]);
		}
		else m_lblExplanation.SetText(CVocabConst::EMPTYSTR);
		m_pWorkThread->PostThreadMsg(CVocabConst::MSG_SPEAK, (WPARAM)uIndex, 0);
		MakeRelatedWordSpell(uIndex, strList);
		m_lblRelated.SetText(CVocabConst::EMPTYSTR);
		AddWordsToLabel(strList);
		MakeProgressString(str, uTotalCnt);
		m_lblProgress.SetText(str);
	}
	m_uFoundIndex = CVocabConst::INVALID_INDEX;

	if(GetDlgItem(IDC_BTN_NEXT).IsWindowEnabled())GetDlgItem(IDC_BTN_NEXT).SetFocus();
	else
	{
		if(GetDlgItem(IDC_BTN_PREVIOUS).IsWindowEnabled())GetDlgItem(IDC_BTN_PREVIOUS).SetFocus();
	}
}

UINT CvocabularyDlg::CalculateSelectedCnt()
{
	UINT uSelectedCnt(0);

	// any of these two must not be 0
	if (0 == m_comboGroupEnd.GetCount() * m_comboGroupStart.GetCount())return 0;

	uSelectedCnt = (m_comboGroupEnd.GetCurSel() - m_comboGroupStart.GetCurSel() + 1) * CVocabConst::COUNT_PER_GROUP;
	if(FALSE == m_chkShowUnfamiliar.IsChecked())
	{
		if (m_comboGroupEnd.GetCurSel() == m_pWorkThread->m_uWordCount / CVocabConst::COUNT_PER_GROUP)
		{
			uSelectedCnt = uSelectedCnt - CVocabConst::COUNT_PER_GROUP + m_pWorkThread->m_uWordCount % CVocabConst::COUNT_PER_GROUP;
		}
	}
	else
	{
		if (m_comboGroupEnd.GetCurSel() == m_pWorkThread->m_uVecUnfamilarIndex.size() / CVocabConst::COUNT_PER_GROUP)
		{
			uSelectedCnt = uSelectedCnt - CVocabConst::COUNT_PER_GROUP + m_pWorkThread->m_uVecUnfamilarIndex.size() % CVocabConst::COUNT_PER_GROUP;
		}
	}

	return uSelectedCnt;
}

void CvocabularyDlg::PreviousUnfamiliar()
{
	UINT uTotalCnt, uIndex;
	CStringW str;
	list<CSensibleInfo *> strList;


	if(FALSE  == m_chkTaskRandom.IsChecked())
	{
		uTotalCnt = m_pWorkThread->m_uVecUnfamilarIndex.size();
		if (0 < m_ulIndex_s)m_ulIndex_s--;
		if(m_ulIndex_s < uTotalCnt)
		{
			uIndex = m_pWorkThread->m_uVecUnfamilarIndex[m_ulIndex_s];
			if (m_comboGroupEnd.GetCurSel() < m_ulIndex_s / CVocabConst::COUNT_PER_GROUP)
			{
				m_comboGroupEnd.SetCurSel(m_ulIndex_s / CVocabConst::COUNT_PER_GROUP);
			}
		}
		ChangeBtnState(uTotalCnt, m_ulIndex_s);
		if(m_comboGroupStart.GetCurSel() > m_ulIndex_s/CVocabConst::COUNT_PER_GROUP)
		{
			m_comboGroupStart.SetCurSel(m_ulIndex_s/CVocabConst::COUNT_PER_GROUP);
		}
	}
	else
	{
		uTotalCnt = m_ulRandomWordCnt;
		if (0 < m_ulIndex_r)m_ulIndex_r--;
		if(m_ulIndex_r < uTotalCnt)
		{
			if (m_vecRandomIndex[m_ulIndex_r] < m_pWorkThread->m_uVecUnfamilarIndex.size())
			{
				uIndex = m_pWorkThread->m_uVecUnfamilarIndex[m_vecRandomIndex[m_ulIndex_r]];
			}
		}
		ChangeBtnState(uTotalCnt, m_ulIndex_r);
	}

	if (std::find(m_pWorkThread->m_uVecUnfamilarIndex.begin(), m_pWorkThread->m_uVecUnfamilarIndex.end(), uIndex) != m_pWorkThread->m_uVecUnfamilarIndex.end())m_lblSpell.SetTextColor(CVocabConst::PINK);
	else m_lblSpell.SetTextColor(CVocabConst::BLACK);

	if(uIndex < m_pWorkThread->m_uWordCount)
	{
		m_lblSpell.SetText(*m_pWorkThread->m_strsSpell[uIndex]);
		if(m_chkShowExplanation.IsChecked())
		{
			m_lblExplanation.SetText(*m_pWorkThread->m_strsExplanation[uIndex]);
		}
		m_pWorkThread->PostThreadMsg(CVocabConst::MSG_SPEAK, (WPARAM)uIndex, 0);
		MakeRelatedWordSpell(uIndex, strList);
		m_lblRelated.SetText(CVocabConst::EMPTYSTR);
		AddWordsToLabel(strList);
		MakeProgressString(str, uTotalCnt);
		m_lblProgress.SetText(str);
	}

	if(GetDlgItem(IDC_BTN_NEXT).IsWindowEnabled())GetDlgItem(IDC_BTN_NEXT).SetFocus();
	else
	{
		if(GetDlgItem(IDC_BTN_PREVIOUS).IsWindowEnabled())GetDlgItem(IDC_BTN_PREVIOUS).SetFocus();
	}
}

void CvocabularyDlg::OnClickBtnPrevious()
{
	UINT uTotalCnt, uIndex;
	CStringW str;
	list<CSensibleInfo *> strList;
	static DWORD dwBeepTime(0);

	if(FALSE == GetDlgItem(IDC_BTN_PREVIOUS).IsWindowEnabled())
	{
		if(::GetTickCount() - dwBeepTime >1000)
		{
			::Beep(2000, 10);
			dwBeepTime = ::GetTickCount();
		}
		return;
	}

	if(FALSE == m_chkTaskRandom.IsChecked())
	{
		if (0 < m_ulIndex_s)m_ulIndex_s--;

		if(m_chkShowUnfamiliar.IsChecked())
		{
			uTotalCnt = m_pWorkThread->m_uVecUnfamilarIndex.size();
			if (m_ulIndex_s < uTotalCnt)
			{
				uIndex = m_pWorkThread->m_uVecUnfamilarIndex[m_ulIndex_s];
			}
		}
		else
		{
			uTotalCnt = m_pWorkThread->m_uWordCount;
			uIndex = m_ulIndex_s;
		}

		if (m_comboGroupStart.GetCurSel() > m_ulIndex_s / CVocabConst::COUNT_PER_GROUP)
		{
			m_comboGroupStart.SetCurSel(m_ulIndex_s / CVocabConst::COUNT_PER_GROUP);
		}
		if(m_ulIndex_s < uTotalCnt)
		{
			ChangeBtnState(uTotalCnt, m_ulIndex_s);
		}
	}
	else
	{
		if (0 < m_ulIndex_r)m_ulIndex_r--;
		uTotalCnt = m_ulRandomWordCnt;

		if(m_ulIndex_r < uTotalCnt)
		{
			if(m_chkShowUnfamiliar.IsChecked())
			{
				if (m_vecRandomIndex[m_ulIndex_r] < m_pWorkThread->m_uVecUnfamilarIndex.size())
				{
					uIndex = m_pWorkThread->m_uVecUnfamilarIndex[m_vecRandomIndex[m_ulIndex_r]];
				}
			}
			else
			{
				uIndex = m_vecRandomIndex[m_ulIndex_r];
			}
			ChangeBtnState(uTotalCnt, m_ulIndex_r);
		}
	}

	if (std::find(m_pWorkThread->m_uVecUnfamilarIndex.begin(), m_pWorkThread->m_uVecUnfamilarIndex.end(), uIndex) != m_pWorkThread->m_uVecUnfamilarIndex.end())m_lblSpell.SetTextColor(CVocabConst::PINK);
	else m_lblSpell.SetTextColor(CVocabConst::BLACK);

	if(uIndex < m_pWorkThread->m_uWordCount)
	{
		m_lblSpell.SetText(*m_pWorkThread->m_strsSpell[uIndex]);
		if(m_chkShowExplanation.IsChecked())
		{
			m_lblExplanation.SetText(*m_pWorkThread->m_strsExplanation[uIndex]);
		}
		m_pWorkThread->PostThreadMsg(CVocabConst::MSG_SPEAK, (WPARAM)uIndex, 0);
		MakeRelatedWordSpell(uIndex, strList);
		m_lblRelated.SetText(CVocabConst::EMPTYSTR);
		AddWordsToLabel(strList);
		MakeProgressString(str, uTotalCnt);
		m_lblProgress.SetText(str);
	}
	m_uFoundIndex = CVocabConst::INVALID_INDEX;

	if(GetDlgItem(IDC_BTN_PREVIOUS).IsWindowEnabled())GetDlgItem(IDC_BTN_PREVIOUS).SetFocus();
	else
	{
		if(GetDlgItem(IDC_BTN_NEXT).IsWindowEnabled())GetDlgItem(IDC_BTN_NEXT).SetFocus();
	}
}

void CvocabularyDlg::OnChkShowExplaination()
{
	if(m_pWorkThread->m_strsExplanation.empty())return;

	if(m_chkShowExplanation.IsChecked())
	{
		if (FALSE == m_chkTaskRandom.IsChecked())
		{
			if (FALSE == m_chkShowUnfamiliar.IsChecked())
			{
				m_lblExplanation.SetText(*m_pWorkThread->m_strsExplanation[m_ulIndex_s]);
			}
			else
			{
				m_lblExplanation.SetText(*m_pWorkThread->m_strsExplanation[m_pWorkThread->m_uVecUnfamilarIndex[m_ulIndex_s]]);
			}
		}
		else
		{
			if(m_vecRandomIndex.size() > m_ulIndex_r)
			{
				if (FALSE == m_chkShowUnfamiliar.IsChecked())
				{
					m_lblExplanation.SetText(*m_pWorkThread->m_strsExplanation[m_vecRandomIndex[m_ulIndex_r]]);
				}
				else
				{
					m_lblExplanation.SetText(*m_pWorkThread->m_strsExplanation[m_pWorkThread->m_uVecUnfamilarIndex[m_vecRandomIndex[m_ulIndex_r]]]);
				}
			}
		}
	}
	else m_lblExplanation.SetText(CVocabConst::EMPTYSTR);
}

void CvocabularyDlg::OnReviseUnfamiliar()
{
	UINT uWordCnt, uGroupCnt, uIndex;
	CStringW str;
	list<CSensibleInfo *> strList;

	if (FALSE == m_chkShowUnfamiliar.IsChecked())
	{
		m_lblSpell.SetTextColor(RGB(0, 0, 0));
		PostMessage(CVocabConst::MSG_PREPARATION_COMPLETED, 0, FALSE/* dont change the start & end group */);
		return ;
	}

	uWordCnt = m_pWorkThread->m_uVecUnfamilarIndex.size();
	if (0 == uWordCnt)
	{
		m_chkShowUnfamiliar.SetCheck(FALSE);
		m_chkShowUnfamiliar.EnableWindow(FALSE);
		m_chkShowUnfamiliar.RedrawWindow();
		ShowMsgBox(L"No unfamiliar words is marked yet.", L"error");
		return;
	}
	if(CVocabConst::MAX_WORD_COUNT < uWordCnt)
	{
		ShowMsgBox(L"Too many unfamiliar words have been marked,please check the database.", L"error");
		return;
	}

	m_comboGroupStart.ResetContent();
	m_comboGroupEnd.ResetContent();
	if (0 < uWordCnt % CVocabConst::COUNT_PER_GROUP)uGroupCnt = uWordCnt / CVocabConst::COUNT_PER_GROUP + 1;
	else uGroupCnt = uWordCnt / CVocabConst::COUNT_PER_GROUP;

	if(0 < uGroupCnt && CVocabConst::MAX_WORD_COUNT / CVocabConst::COUNT_PER_GROUP > uGroupCnt)
	{
		for (int i = 0; i < uGroupCnt; i++)
		{
			str.Format(L"%d", i + 1);
			//_itow_s(i+1,wszBuff,10);
			m_comboGroupStart.AddString(str);
			m_comboGroupEnd.AddString(str);
		}
	}
	if(0 <= m_usStartGrp2 && m_usStartGrp2 <= m_usEndGrp2 && m_usEndGrp2 < uGroupCnt)
	{
		m_comboGroupStart.SetCurSel(m_usStartGrp2);
		m_comboGroupEnd.SetCurSel(m_usEndGrp2);
	}
	else
	{
		m_comboGroupStart.SetCurSel(0);
		m_comboGroupEnd.SetCurSel(0);
	}

	m_lblSpell.SetTextColor(CVocabConst::PINK);

	if (FALSE == m_chkTaskRandom.IsChecked())
	{
		m_ulIndex_s = m_comboGroupStart.GetCurSel() * CVocabConst::COUNT_PER_GROUP;
		uIndex = m_pWorkThread->m_uVecUnfamilarIndex[m_ulIndex_s];
		uWordCnt = m_pWorkThread->m_uVecUnfamilarIndex.size();
		ChangeBtnState(uWordCnt, m_ulIndex_s);
	}
	else
	{
		m_ulIndex_r = 0;
		m_ulRandomWordCnt = CalculateSelectedCnt();
		if (0 == MakeRandomList(uWordCnt, m_ulRandomWordCnt))
		{
			ShowMsgBox(L"Generating random data failed,use seqeuential data.", L"error");
			return;
		}
		if (m_ulRandomWordCnt > m_ulIndex_r)
		{
			uIndex = m_pWorkThread->m_uVecUnfamilarIndex[m_vecRandomIndex[m_ulIndex_r]];
		}
		uWordCnt = m_ulRandomWordCnt;
		ChangeBtnState(uWordCnt, m_ulIndex_r);
	}
	if(m_pWorkThread->m_strsSpell.size() > uIndex)
	{
		m_lblSpell.SetText(*m_pWorkThread->m_strsSpell[uIndex]);
		m_lblExplanation.SetText(*m_pWorkThread->m_strsExplanation[uIndex]);
		m_pWorkThread->PostThreadMsg(CVocabConst::MSG_SPEAK, (WPARAM)(uIndex), 0);
		m_lblRelated.SetText(CVocabConst::EMPTYSTR);
		MakeRelatedWordSpell(uIndex, strList);
		AddWordsToLabel(strList);
		MakeProgressString(str, uWordCnt);
		m_lblProgress.SetText(str);
	}
	
	if (GetDlgItem(IDC_BTN_NEXT).IsWindowEnabled())GetDlgItem(IDC_BTN_NEXT).SetFocus();
}

void CvocabularyDlg::ChangeBtnState(UINT uWordCnt, UINT uIndex)
{
	if(1 >= uWordCnt)
	{
		GetDlgItem(IDC_BTN_NEXT).EnableWindow(FALSE);
		GetDlgItem(IDC_BTN_PREVIOUS).EnableWindow(FALSE);
		return;
	}
	if(0 == uIndex)GetDlgItem(IDC_BTN_PREVIOUS).EnableWindow(FALSE);
	if(0 < uIndex && FALSE == GetDlgItem(IDC_BTN_PREVIOUS).IsWindowEnabled())
	{
		GetDlgItem(IDC_BTN_PREVIOUS).EnableWindow();
	}
	if(uWordCnt - 1 == uIndex)GetDlgItem(IDC_BTN_NEXT).EnableWindow(FALSE);
	if(uIndex < uWordCnt - 1 && FALSE == GetDlgItem(IDC_BTN_NEXT).IsWindowEnabled())
	{
		GetDlgItem(IDC_BTN_NEXT).EnableWindow();
	}
}

void CvocabularyDlg::OnChkTaskRandom()
{
	UINT uWordCnt, uIndex;
	CStringW str;
	list<CSensibleInfo *> strList;

	m_comboGroupStart.EnableWindow(1 - m_chkTaskRandom.IsChecked());
	m_comboGroupEnd.EnableWindow(1 - m_chkTaskRandom.IsChecked());

	if (TRUE == m_chkShowUnfamiliar.IsChecked())
	{
		OnReviseUnfamiliar();
		return;
	}

	if(FALSE == m_chkTaskRandom.IsChecked())
	{
		uIndex = m_ulIndex_s;
		uWordCnt = m_pWorkThread->m_strsSpell.size();
		ChangeBtnState(uWordCnt, m_ulIndex_s);
	}
	else
	{
		m_ulIndex_r = 0;
		m_ulRandomWordCnt = CalculateSelectedCnt();
		if(m_pWorkThread->m_uWordCount < m_ulRandomWordCnt)return;
		if(0 == MakeRandomList(m_pWorkThread->m_uWordCount, m_ulRandomWordCnt))return;
		uIndex = m_vecRandomIndex[m_ulIndex_r];
		uWordCnt = m_ulRandomWordCnt;
		ChangeBtnState(uWordCnt, m_ulIndex_r);
	}
	m_pWorkThread->PostThreadMsg(CVocabConst::MSG_SPEAK, (WPARAM)uIndex, 0);
	if (std::find(m_pWorkThread->m_uVecUnfamilarIndex.begin(), m_pWorkThread->m_uVecUnfamilarIndex.end(), uIndex) != m_pWorkThread->m_uVecUnfamilarIndex.end())m_lblSpell.SetTextColor(CVocabConst::PINK);
	else m_lblSpell.SetTextColor(CVocabConst::BLACK);
	m_lblSpell.SetText(*m_pWorkThread->m_strsSpell[uIndex]);
	if(m_chkShowExplanation.IsChecked())
	{
		m_lblExplanation.SetText(*m_pWorkThread->m_strsExplanation[uIndex]);
	}
	m_lblRelated.SetText(CVocabConst::EMPTYSTR);
	MakeRelatedWordSpell(uIndex, strList);
	AddWordsToLabel(strList);
	MakeProgressString(str, uWordCnt);
	m_lblProgress.SetText(str);
}

UINT CvocabularyDlg::MakeRandomList(UINT uTotalCnt, UINT uSelectedCnt)
{
	UINT uStartIndex = m_comboGroupStart.GetCurSel() * CVocabConst::COUNT_PER_GROUP;
	if(0 == (uStartIndex + uSelectedCnt))return 0;
	if(uTotalCnt < (uStartIndex + uSelectedCnt))return 0;

	m_vecRandomIndex.clear();
	m_vecRandomIndex.resize(uSelectedCnt);
	std::iota(m_vecRandomIndex.begin(), m_vecRandomIndex.end(), uStartIndex);
	std::random_device rd;
	std::mt19937 g(rd());
	std::shuffle(m_vecRandomIndex.begin(), m_vecRandomIndex.end(), g);

	return m_vecRandomIndex.size();
}

int CvocabularyDlg::MakeProgressString(CStringW& strMsg, UINT uTotalCount)
{
	UINT uSelectedCnt;
	std::wstring str;

	// the word count in last group maybe be less than COUNT_PER_GROUP
	uSelectedCnt = CalculateSelectedCnt();

	if(m_chkTaskRandom.IsChecked())
	{
		str = std::to_wstring(m_ulIndex_r + 1);
		strMsg = str.c_str();

		strMsg += CVocabConst::DIVISION;
		if(uSelectedCnt <= uTotalCount)
		{
			str = std::to_wstring(uSelectedCnt);
		}
		else
		{
			str = std::to_wstring(uTotalCount);
		}
		strMsg += str.c_str();
	}
	else
	{
		if (m_ulIndex_s + 1 > uTotalCount)
		{
			m_ulIndex_s = uTotalCount - 1;
		}
		str = std::to_wstring(m_ulIndex_s + 1);
		strMsg = str.c_str();
		strMsg += CVocabConst::DIVISION;
	}

	strMsg += CVocabConst::LEFT_MID_BRAKET;
	str = std::to_wstring(m_comboGroupStart.GetCurSel() * CVocabConst::COUNT_PER_GROUP + 1);
	strMsg += str.c_str();
	strMsg += CVocabConst::MINUS;
	str = std::to_wstring(m_comboGroupStart.GetCurSel() * CVocabConst::COUNT_PER_GROUP + uSelectedCnt);
	strMsg += str.c_str();
	strMsg += CVocabConst::RIGHT_MID_BRAKET;

	return 0;
}

LRESULT	CALLBACK _vocabDlg_CommFileDlgProc(int code, WPARAM wParam, LPARAM lParam)
{
	PCWPRETSTRUCT pcwrs = reinterpret_cast<PCWPRETSTRUCT>(lParam);

	if(globalVar.g_hWndFileDlg == pcwrs->hwnd)
	{
		// the file dialog is to be shown
		if(WM_SHOWWINDOW ==	pcwrs->message)
		{
			CRect rcWnd,rcTxt;
			CPoint ptTopLeft,ptBottomRight;
			CString str(CvocabularyDlg::FILEDLG_TIP);
			HDC hdc=::GetDC(pcwrs->hwnd);
			LOGFONT lf;
			::GetObject(GetStockObject(SYSTEM_FONT), sizeof(LOGFONT),&lf);
			lf.lfWidth = 5;
			lf.lfWeight = 200;
			::memcpy(lf.lfFaceName,CVocabApp::FONT_NAME_TAHOMA,32);
			::SelectObject(hdc,::CreateFontIndirectW(&lf));

			::GetWindowRect(pcwrs->hwnd,&rcWnd);
			ptTopLeft.x = rcWnd.left;
			ptTopLeft.y = rcWnd.top;
			ptBottomRight.x = rcWnd.right;
			ptBottomRight.y = rcWnd.bottom;
			//::ScreenToClient(pcwrs->hwnd,&ptTopLeft);
			//::ScreenToClient(pcwrs->hwnd,&ptBottomRight);

			rcTxt.left=20;rcTxt.right=800;rcTxt.top=380;rcTxt.bottom=420;
			::SetTextColor(hdc,RGB(0,0,90));
			::DrawTextW(hdc,str,str.GetLength()*2,&rcTxt,DT_CALCRECT|DT_SINGLELINE);
			int nWidth,nHeight;
			nWidth= rcTxt.right - rcTxt.left;nHeight=rcTxt.bottom - rcTxt.top;
			rcTxt=rcWnd;
			rcTxt.right = rcTxt.right - rcTxt.left;
			rcTxt.bottom = rcTxt.bottom - rcTxt.top;
			rcTxt.left = rcTxt.top = 0;

			rcTxt.left=100;
			rcTxt.right = rcTxt.left+nWidth+10;
			rcTxt.top=rcTxt.bottom-60-nHeight;
			rcTxt.bottom = rcTxt.top+2*nHeight;
			::CreateWindowW(CWnd0::WND0_CLASSNAME,NULL,WS_VISIBLE | WS_CHILD,rcTxt.left,rcTxt.top,rcTxt.right-rcTxt.left,
				rcTxt.bottom-rcTxt.top,pcwrs->hwnd,0,globalVar.g_hInstance,&_label_Tip);
			_label_Tip.ShowBorder(0);
			_label_Tip.SetPlainColor(RGB(250,90,90));
			_label_Tip.SetText(str);
			//::SetActiveWindow(globalVar.g_hWndFileDlg);
		}
		if(WM_DESTROY == pcwrs->message)
		{
			::UnhookWindowsHookEx(globalVar.g_hHookFileDlg);
			globalVar.g_hHookFileDlg = 0;
		}
		if(WM_SIZE == pcwrs->message)
		{
			::DestroyWindow(_label_Tip.m_hWnd);
		}
	}

	return CallNextHookEx(globalVar.g_hHookFileDlg, code, wParam, lParam);
}

LRESULT CvocabularyDlg::OnMsgRcvFileDlgHandle(WPARAM wParam, LPARAM lParam)
{
	globalVar.g_hWndFileDlg = (HWND)wParam;
	globalVar.g_hHookFileDlg = ::SetWindowsHookEx(WH_CALLWNDPROCRET, _vocabDlg_CommFileDlgProc, 0, ::GetCurrentThreadId());
	
	return 0 == globalVar.g_hHookFileDlg;
}

LRESULT CALLBACK CvocabularyDlg::_vocabDlg_KeyboardProc(int code, WPARAM wParam, LPARAM lParam)
{
	if (0 > code)return ::CallNextHookEx(globalVar.g_hHookKeyboard, code, wParam, lParam);
	//return ::CallNextHookEx(0,code,wParam,lParam);
	//CChkBox2 *pWnd;
	//SHORT sCode1,sCode2;
	//BOOL bMsgIsPosted(0);
	//list<CChkBox2*>::const_iterator iter,endIter;
	//iter = CChkBox2::_CChkBox2_listChkBox2Wnd.begin();
	//endIter = CChkBox2::_CChkBox2_listChkBox2Wnd.end();

	SHORT sLeftCtrl = ::GetKeyState(VK_LCONTROL);
	SHORT sLeftAlt = ::GetKeyState(VK_LMENU);

	if (::GetForegroundWindow() == appWnd.m_hWnd)	// if main window is not t foreground window,dont handle these messages
	{
		// only one key is pressed at a time,exp: number 1,2 or 3
		if (0 == (0xa0000000/* bit31 and bit 29 are 0,indicate the 'ALT' key is not pressed and a key is being pressed */ & lParam))
		{
			if (0x31 == wParam || VK_NUMPAD1 == wParam)
			{
				::PostMessage(appWnd.m_hWnd, CVocabConst::MSG_ACCKEY, IDC_BTN_NEXT, 0);
				goto _vocabDlg_KeyboardProc_exit;
			}
			if (0x32 == wParam || VK_NUMPAD2 == wParam)
			{
				::PostMessage(appWnd.m_hWnd, CVocabConst::MSG_ACCKEY, IDC_BTN_PREVIOUS, 0);
				goto _vocabDlg_KeyboardProc_exit;
			}
			if (0x33 == wParam || VK_NUMPAD3 == wParam)
			{
				::PostMessage(appWnd.m_hWnd, CVocabConst::MSG_ACCKEY, IDC_CHK_SHOW_EXPLANATION, 0);
				goto _vocabDlg_KeyboardProc_exit;
			}
			if (0x34 == wParam || VK_NUMPAD4 == wParam)
			{
				::PostMessage(appWnd.m_hWnd, CVocabConst::MSG_ACCKEY, IDC_BTN_REP, 0);
				goto _vocabDlg_KeyboardProc_exit;
			}
		}

		if ((sLeftCtrl & 0x8000/* if the high-order bit is 1, the key is down */) && (0x44 == wParam)) /* left Ctrl + D */
		{
			::PostMessage(appWnd.m_hWnd, WM_COMMAND, CVocabConst::POPUP_ITEM_ID::ADD_RELATED, 0);
			::CallNextHookEx(globalVar.g_hHookKeyboard, code, wParam, lParam);
			return TRUE;
		}

		if ((sLeftCtrl & 0x8000/* if the high-order bit is 1, the key is down */) && (0x46 == wParam)) /* left Ctrl + F */
		{
			::PostMessage(appWnd.m_hWnd, WM_COMMAND, IDC_BTN_FIND, 0);
			::CallNextHookEx(globalVar.g_hHookKeyboard, code, wParam, lParam);
			return TRUE;
		}

		if((sLeftAlt & 0x8000) && (::GetKeyState(0x52/* key:R */) & 0x8000))	// left ALT + R
		{
			::PostMessage(appWnd.m_hWnd, CVocabConst::MSG_ACCKEY, IDC_CHK_TASK_RANDOM, 0);
		}

		if((sLeftAlt & 0x8000) && (::GetKeyState(0x58/* key:R */) & 0x8000))	// left ALT + X
		{
			::PostMessage(appWnd.m_hWnd, WM_COMMAND, IDC_BTN_EXIT, 0);
		}
	}
	else
	{
		// these two keys are used when 'waiting dlg' is showing
		// only one key is pressed at a time,exp: number 1,2 or 3
		if (VK_RETURN == wParam)
		{
			return ::CallNextHookEx(globalVar.g_hHookKeyboard, code, wParam, lParam);
		}
		if (VK_ESCAPE == wParam)
		{
			::PostMessage(appWnd.m_hWnd, CVocabConst::MSG_ACCKEY, VK_ESCAPE, 0);
			::CallNextHookEx(globalVar.g_hHookKeyboard, code, wParam, lParam);
			return TRUE;
		}
	}

_vocabDlg_KeyboardProc_exit:
	return ::CallNextHookEx(globalVar.g_hHookKeyboard, code, wParam, lParam);
}

int CvocabularyDlg::CreateChildWnd()
{
	// first,create child windows

	DWORD dwStyle = WS_VISIBLE | WS_CHILD;
	::CreateWindow(CVocabConst::CLASS_NAME_STATIC, L"from:", dwStyle, 0, 0, 0, 0, m_hWnd,(HMENU)IDC_LBL_FROM, globalVar.g_hInstance, 0);
	::CreateWindow(CVocabConst::CLASS_NAME_STATIC, L"to:", dwStyle, 0, 0, 0, 0,m_hWnd, (HMENU)IDC_LBL_TO, globalVar.g_hInstance, 0);
	
	for(UINT ulID = IDC_BTN_CREATE; ulID<=IDC_BTN_EXIT; ulID++)
	{
		::CreateWindow(CVocabConst::CLASS_NAME_BTN, 0, dwStyle, 0, 0, 0, 0, m_hWnd, (HMENU)ulID, globalVar.g_hInstance, 0);
	}

	CRect rc{0};
	m_lblSpell.Create(&m_lblSpell, 0, CWnd0::WND0_CLASSNAME, 0, dwStyle, rc, m_hWnd, 0, globalVar.g_hInstance);
	m_lblRelated.Create(&m_lblRelated, 0, CWnd0::WND0_CLASSNAME, 0, dwStyle, rc, m_hWnd, 0, globalVar.g_hInstance);
	m_lblExplanation.Create(&m_lblExplanation, 0, CWnd0::WND0_CLASSNAME, 0, dwStyle, rc,m_hWnd, 0, globalVar.g_hInstance);
	m_lblNotes.Create(&m_lblNotes, 0, CWnd0::WND0_CLASSNAME, 0, dwStyle, rc, m_hWnd, 0, globalVar.g_hInstance);
	m_lblProgress.Create(&m_lblProgress, 0, CWnd0::WND0_CLASSNAME, 0, dwStyle, rc, m_hWnd, 0, globalVar.g_hInstance);
	//m_labMsg.Create(&m_labMsg, 0, CWnd0::WND0_CLASSNAME, 0, dwStyle, rc, m_hWnd, 0, globalVar.g_hInstance);

	m_comboGroupStart.Create(m_hWnd);
	m_comboGroupStart.SetID(IDC_COMBO_GROUP_START);
	m_comboGroupEnd.Create(m_hWnd);
	m_comboGroupEnd.SetID(IDC_COMBO_GROUP_END);

	m_chkShowExplanation.Create(m_hWnd);
	m_chkShowExplanation.SetID(IDC_CHK_SHOW_EXPLANATION);
	m_chkShowExplanation.SetText(L"show explanation(&3)");
	m_chkShowExplanation.SetCheck();

	m_chkTaskRandom.Create(m_hWnd);
	m_chkTaskRandom.SetText(L"&random in task");
	m_chkTaskRandom.SetID(IDC_CHK_TASK_RANDOM);

	m_chkShowUnfamiliar.Create(m_hWnd);
	m_chkShowUnfamiliar.SetText(L"review &unfamiliar");
	m_chkShowUnfamiliar.SetID(IDC_CHK_REVISE);
	m_chkShowUnfamiliar.SetCheck(FALSE);

	m_chkShowNotes.Create(m_hWnd);
	m_chkShowNotes.SetText(L"show no&tes");
	m_chkShowNotes.SetID(IDC_CHK_SHOW_EXAMPLE);
	m_chkShowNotes.SetCheck(FALSE);

	//
	// second,arrange size and position of all child window
	//  find a suitable font first,and then calculate the size of child windows
	//
	InitChildWndSizePos();

	m_lblSpell.SetFontHeight(320);
	m_lblSpell.SetAlignment(CLabelEx::HCENTER | CLabelEx::VCENTER);
	HICON hIcon = ::LoadIcon(globalVar.g_hInstance,MAKEINTRESOURCE(IDI_ICON0));
	m_lblSpell.AddPopupMenuItem(CVocabConst::POPUP_ITEM_ID::ADD_UNFAMILIAR,hIcon,L"&add to unfamiliar book\t\tCtrl+A");
	hIcon = ::LoadIcon(globalVar.g_hInstance,MAKEINTRESOURCE(IDI_ICON1));
	m_lblSpell.AddPopupMenuItem(CVocabConst::POPUP_ITEM_ID::REMOVE_UNFAMILIAR,hIcon,L"re&move from unfamiliar book\tCtrl+M");
	hIcon = ::LoadIcon(globalVar.g_hInstance,MAKEINTRESOURCE(IDI_ICON2));
	m_lblSpell.AddPopupMenuItem(CVocabConst::POPUP_ITEM_ID::ADD_RELATED,hIcon,L"add &related word\t\t\tCtrl+D");

	m_lblSpell.SetParent(m_hWnd);
	m_lblRelated.SetParent(m_hWnd);
	m_lblRelated.SetParent(m_hWnd);
	m_lblRelated.SetFontHeight(200);
	m_lblRelated.SetBorderColor(CVocabConst::COLOR3);
	m_lblRelated.SetFontName(CVocabApp::FONT_NAME_TAHOMA);
	m_lblRelated.AddPopupMenuItem(CVocabConst::POPUP_ITEM_ID::REMOVE_RELATED,0,L"&remove from related\tCtrl+R");

	m_lblExplanation.SetAlignment(CLabelEx::HLEFT);
	m_lblExplanation.SetFontHeight(200);
	m_lblExplanation.SetFontName(L"¿¬Ìå");

	m_lblNotes.SetAlignment(CLabelEx::HLEFT);
	m_lblNotes.SetFontHeight(200);
	m_lblNotes.SetFontName(L"¿¬Ìå");

	m_lblProgress.SetFontHeight(125);
	m_lblProgress.SetFontName(L"MV Boli");
	m_lblProgress.ShowBorder(0);
	m_lblProgress.SetPlainColor(::GetSysColor(COLOR_GRAYTEXT));
	m_lblProgress.SetAlignment(CLabelEx::VCENTER);

	GetDlgItem(IDC_BTN_NEXT).EnableWindow(FALSE);
	GetDlgItem(IDC_BTN_PREVIOUS).EnableWindow(FALSE);

	return 0;
}

int CvocabularyDlg::InitChildWndSizePos(void)
{
	HDC hScreenDC = ::GetDC(0);
	HDC hMemDC(0);

	hMemDC = ::CreateCompatibleDC(hScreenDC);
	if(0 == hMemDC)hMemDC = ::GetDCEx(m_hWnd, 0, DCX_WINDOW);
	if(0 == hMemDC)hMemDC = hScreenDC;

	int x, y;
	CSize szTxt;
	CRect rcCtrl, rcDlg;
	CString str;
	
	::SelectObject(hMemDC, globalVar.g_hDefEngFont);

	// arrange the benchmark window first
	// [label:from]
	str = LABEL_TEXT[0];
	::GetTextExtentPoint32(hMemDC, str.GetBuffer(), str.GetLength(), &szTxt);
	GetDlgItem(IDC_LBL_FROM).MoveWindow(X_START, Y_START, szTxt.cx * CX_ADJUST, szTxt.cy * CY_ADJUST);
	GetDlgItem(IDC_LBL_FROM).SendMessage(WM_SETFONT, (WPARAM)globalVar.g_hDefEngFont, 0);
	
	// [combo:from]
	GetClientRect(&rcDlg);
	GetDlgItem(IDC_LBL_FROM).GetWindowRect(&rcCtrl);
	ScreenToClient(&rcCtrl);
	x =  rcCtrl.right + CVocabConst::X_GAP;
	y = rcCtrl.top;
	str = L"0000";
	::GetTextExtentPoint32(hMemDC, str.GetBuffer(), str.GetLength(), &szTxt);
	m_comboGroupStart.MoveWindow(x, y, szTxt.cx + ::GetSystemMetrics(SM_CXVSCROLL), CFlatDropList::HEADER_HEIGHT);

	// [label:to]
	GetDlgItem(IDC_LBL_FROM).GetWindowRect(&rcCtrl);
	ScreenToClient(&rcCtrl);
	x = X_START;
	y = rcCtrl.bottom;
	str = LABEL_TEXT[1];
	::GetTextExtentPoint32(hMemDC, str.GetBuffer(), str.GetLength(), &szTxt);
	GetDlgItem(IDC_LBL_TO).MoveWindow(x, y, szTxt.cx * CX_ADJUST, szTxt.cy * CY_ADJUST);
	GetDlgItem(IDC_LBL_TO).SendMessage(WM_SETFONT, (WPARAM)globalVar.g_hDefEngFont, 0);

	// [combo:to]
	GetDlgItem(IDC_LBL_TO).GetWindowRect(&rcCtrl);
	ScreenToClient(&rcCtrl);
	y = rcCtrl.top;
	GetDlgItem(IDC_LBL_FROM).GetWindowRect(&rcCtrl);
	ScreenToClient(&rcCtrl);
	x = rcCtrl.right + CVocabConst::X_GAP;
	str = L"0000";
	::GetTextExtentPoint32(hMemDC, str.GetBuffer(), str.GetLength(), &szTxt);
	m_comboGroupEnd.MoveWindow(x, y, szTxt.cx + ::GetSystemMetrics(SM_CXVSCROLL), CFlatDropList::HEADER_HEIGHT);

	// [checkbox:show explanation]
	m_comboGroupStart.GetWindowRect(&rcCtrl);
	ScreenToClient(&rcCtrl);
	x = rcCtrl.right + CVocabConst::X_GAP * 3;
	y = rcCtrl.top;
	m_chkShowExplanation.SetCheck();
	rcCtrl = m_chkShowExplanation.GetWndRect();
	m_chkShowExplanation.MoveWindow(x, y ,rcCtrl.Width(), rcCtrl.Height());

	// [checkbox:random]
	m_chkShowExplanation.GetWindowRect(&rcCtrl);
	ScreenToClient(&rcCtrl);
	x = rcCtrl.right + CVocabConst::X_GAP;
	rcCtrl = m_chkTaskRandom.GetWndRect();
	m_chkTaskRandom.MoveWindow(x, y, rcCtrl.Width(), rcCtrl.Height());

	// [checkbox:unfamiliar]
	m_chkShowExplanation.GetWindowRect(&rcCtrl);
	ScreenToClient(&rcCtrl);
	x = rcCtrl.left;
	y = rcCtrl.bottom + CVocabConst::Y_GAP;
	rcCtrl = m_chkShowExplanation.GetWndRect();
	m_chkShowUnfamiliar.MoveWindow(x, y, rcCtrl.Width(), rcCtrl.Height());

	// [checkbox:notes]
	m_chkTaskRandom.GetWindowRect(&rcCtrl);
	ScreenToClient(&rcCtrl);
	x = rcCtrl.left;
	y = rcCtrl.bottom + CVocabConst::Y_GAP;
	rcCtrl = m_chkTaskRandom.GetWndRect();
	m_chkShowNotes.MoveWindow(x, y, rcCtrl.Width(), rcCtrl.Height());

	// [button:create]
	GetWindowRect(&rcDlg);
	GetDlgItem(IDC_LBL_TO).GetWindowRect(&rcCtrl);
	ScreenToClient(&rcDlg);
	ScreenToClient(&rcCtrl);
	x = rcCtrl.left;
	str = BTN_TEXT[0];
	::GetTextExtentPoint32(hMemDC, str.GetBuffer(), str.GetLength(), &szTxt);
	x = rcDlg.right - CVocabConst::X_GAP - szTxt.cx * ADJUST1;
	y = rcCtrl.bottom + CVocabConst::Y_GAP * 2;

	GetDlgItem(IDC_BTN_CREATE).SetWindowText(BTN_TEXT[0]);
	GetDlgItem(IDC_BTN_CREATE).MoveWindow(x, y, szTxt.cx * ADJUST1, szTxt.cy * CY_ADJUST);
	GetDlgItem(IDC_BTN_CREATE).SendMessage(WM_SETFONT, (WPARAM)globalVar.g_hDefEngFont, 0);

	for (UINT uID = IDC_BTN_SELECT; uID < IDC_BTN_EXIT; uID++)
	{
		GetDlgItem(uID - 1).GetWindowRect(&rcCtrl);
		ScreenToClient(&rcCtrl);
		y = rcCtrl.bottom + CVocabConst::Y_GAP;
		x = rcCtrl.left;
		GetDlgItem(uID).SetWindowText(BTN_TEXT[uID - IDC_BTN_CREATE]);
		GetDlgItem(uID).MoveWindow(x, y, szTxt.cx * ADJUST1, szTxt.cy * CY_ADJUST);
		GetDlgItem(uID).SendMessage(WM_SETFONT, (WPARAM)globalVar.g_hDefEngFont, 0);
	}

	// adjust 'exit' button
	y = rcDlg.bottom - rcCtrl.Height() - CVocabConst::Y_GAP;
	GetDlgItem(IDC_BTN_EXIT).MoveWindow(x, y, rcCtrl.Width(), rcCtrl.Height());
	GetDlgItem(IDC_BTN_EXIT).SetWindowText(BTN_TEXT[IDC_BTN_EXIT - IDC_BTN_CREATE]);
	GetDlgItem(IDC_BTN_EXIT).SendMessage(WM_SETFONT, (WPARAM)globalVar.g_hDefEngFont, 0);

	// [label:spell]
	GetDlgItem(IDC_LBL_TO).GetWindowRect(&rcCtrl);
	ScreenToClient(&rcCtrl);
	x = rcCtrl.left;
	y = rcCtrl.bottom + CVocabConst::Y_GAP * 2;
	GetDlgItem(IDC_BTN_CREATE).GetWindowRect(&rcCtrl);
	ScreenToClient(&rcCtrl);

	m_lblSpell.MoveWindow(x, y, rcCtrl.left - x - CVocabConst::X_GAP * 2, rcDlg.Height()*CVocabConst::LABEL_HEIGHT_FACTOR1);

	// [label:related]
	m_lblSpell.GetWindowRect(&rcCtrl);
	y += rcCtrl.Height() + CVocabConst::Y_GAP;
	m_lblRelated.MoveWindow(x, y, rcCtrl.Width(), rcDlg.Height()*CVocabConst::LABEL_HEIGHT_FACTOR2);

	// [label:explanation]
	m_lblRelated.GetWindowRect(&rcCtrl);
	y += rcCtrl.Height() + CVocabConst::Y_GAP;
	m_lblExplanation.MoveWindow(x, y, rcCtrl.Width(), rcDlg.Height()*CVocabConst::LABEL_HEIGHT_FACTOR2);

	// [label:notes]
	m_lblExplanation.GetWindowRect(&rcCtrl);
	y += rcCtrl.Height() + CVocabConst::Y_GAP;
	m_lblNotes.MoveWindow(x, y, rcCtrl.Width(), rcDlg.Height()*CVocabConst::LABEL_HEIGHT_FACTOR2 *CVocabConst::LABEL_HEIGHT_FACTOR3 - CVocabConst::Y_GAP);

	// adjust 'progress' label 
	::GetTextExtentPoint32(hMemDC, str.GetBuffer(), str.GetLength(), &szTxt);
	m_lblNotes.GetWindowRect(&rcCtrl);
	ScreenToClient(&rcCtrl);
	x = rcCtrl.left;
	y = rcDlg.bottom - CVocabConst::Y_GAP - szTxt.cy * CY_ADJUST;
	m_lblProgress.MoveWindow(x, y, szTxt.cx * 3, szTxt.cy * CY_ADJUST);
	m_lblProgress.SetFontHeight(125);
	m_lblProgress.SetFontName(L"MV Boli");
	m_lblProgress.ShowBorder(0);
	m_lblProgress.SetPlainColor(::GetSysColor(COLOR_GRAYTEXT));

	::ReleaseDC(0, hScreenDC);
	if(0 != hMemDC)::DeleteDC(hMemDC);

	return 0;
}

int CvocabularyDlg::AddWordsToLabel(list<CSensibleInfo *> &strList)
{
	list<CSensibleInfo *>::const_iterator iterList;

	iterList = strList.begin();
	while (strList.end() != iterList)
	{
		m_lblRelated.AddSensibleText(*iterList);
		iterList++;
	}
	strList.clear();

	return 0;
}

LRESULT CvocabularyDlg::OnMsgAddNewWords(WPARAM, LPARAM)
{
	CStringW strSpell, strExplanation;

	CWaitDlg::GetRelated(strSpell, strExplanation /* can be empty string */ );
	m_pWorkThread->m_strsSpell2.push_back(new CStringW(strSpell));
	m_pWorkThread->m_strsExplanation2.push_back(new CStringW(strExplanation));

	return 0;
}

LRESULT CvocabularyDlg::OnMsgAddRelated(WPARAM wParam, LPARAM lParam)
{
	CStringW strSpell, strNotes, oldstr;
	CStringW *pstr(0);
	list<CSensibleInfo *> strList;
	UINT uIndex(CVocabConst::INVALID_INDEX);
	map <UINT, CStringW*> ::const_iterator iterMap;

	CWaitDlg::GetRelated(strSpell, strNotes /* can be empty string */ );

	if (CVocabConst::INVALID_INDEX != m_uFoundIndex)
	{
		uIndex = m_uFoundIndex;
	}
	else
	{
		if (FALSE == m_chkTaskRandom.IsChecked())
		{
			uIndex = m_ulIndex_s;
		}
		else
		{
			if (m_ulIndex_r < m_vecRandomIndex.size())
			{
				uIndex = m_vecRandomIndex[m_ulIndex_r];
			}
		}
	}

	iterMap = m_pWorkThread->m_mapRelatedIndexString.find(uIndex);
	if (m_pWorkThread->m_mapRelatedIndexString.end() == iterMap)	// no related exists yet
	{
		pair<UINT, CStringW*> IndexString;

		IndexString.first = uIndex;
		IndexString.second = new CStringW(strSpell);
		m_pWorkThread->m_mapRelatedIndexString.insert(IndexString);
		IndexString.second = new CStringW(strNotes);
		m_pWorkThread->m_mapNotes.insert(IndexString);
	}
	else // related words exist already,append to it
	{
		// spell
		pstr = m_pWorkThread->m_mapRelatedIndexString.find(uIndex)->second;
		if(0 < pstr->GetLength())
		{
			pstr->Append(CStringW(CVocabConst::COMMA));
		}
		pstr->Append(strSpell);

		// notes
		if(uIndex < m_pWorkThread->m_mapNotes.size())
		{
			pstr = m_pWorkThread->m_mapNotes.find(uIndex)->second;
		}
		if(0 < pstr->GetLength())
		{
			pstr->Append(CStringW(CVocabConst::COMMA));
		}
		pstr->Append(strNotes);
	}

	// record index on which related has been changed
	if (CVocabConst::INVALID_INDEX != uIndex)
	{
		if (std::find(m_pWorkThread->m_uVecChangedRelated.begin(), m_pWorkThread->m_uVecChangedRelated.end(), uIndex) == m_pWorkThread->m_uVecChangedRelated.end())
		{ 
			m_pWorkThread->m_uVecChangedRelated.push_back(uIndex);
		}
	}

	MakeRelatedWordSpell(uIndex, strList);
	m_lblRelated.SetText(CVocabConst::EMPTYSTR);
	AddWordsToLabel(strList);

	return 0;
}

int CvocabularyDlg::MarkUnfamiliar()
{
	UINT uIndex(CVocabConst::INVALID_INDEX);

	if (m_chkShowUnfamiliar.IsChecked())return 0;

	if (FALSE == m_chkTaskRandom.IsChecked())
	{
		uIndex = m_ulIndex_s;
	}
	else
	{
		if (m_vecRandomIndex.size() > m_ulIndex_r)
		{
			uIndex = m_vecRandomIndex[m_ulIndex_r];
		}
	}
	if (std::find(m_pWorkThread->m_uVecToMark.begin(), m_pWorkThread->m_uVecToMark.end(), uIndex) == m_pWorkThread->m_uVecToMark.end())
	{
		m_pWorkThread->m_uVecToMark.push_back(uIndex);
	}
	m_chkShowUnfamiliar.EnableWindow();

	return uIndex;
}

int CvocabularyDlg::UnmarkUnfamiliar()
{
	UINT uIndex(CVocabConst::INVALID_INDEX);
	if (m_chkShowUnfamiliar.IsChecked())
	{
		if (FALSE == m_chkTaskRandom.IsChecked())
		{
			uIndex = m_pWorkThread->m_uVecUnfamilarIndex[m_ulIndex_s];
		}
		else
		{
			if (m_vecRandomIndex.size() > m_ulIndex_r)
			{
				uIndex = m_pWorkThread->m_uVecUnfamilarIndex[m_vecRandomIndex[m_ulIndex_r]];
			}
		}
		if (std::find(m_pWorkThread->m_uVecToUnmark.begin(), m_pWorkThread->m_uVecToUnmark.end(), uIndex) == m_pWorkThread->m_uVecToUnmark.end())
		{
			m_pWorkThread->m_uVecToUnmark.push_back(uIndex);
		}
	}
	else
	{
		if (FALSE == m_chkTaskRandom.IsChecked())
		{
			uIndex = m_ulIndex_s;
		}
		else
		{
			if (m_vecRandomIndex.size() > m_ulIndex_r)
			{
				uIndex = m_vecRandomIndex[m_ulIndex_r];
			}
		}
		if (std::find(m_pWorkThread->m_uVecToUnmark.begin(), m_pWorkThread->m_uVecToUnmark.end(), uIndex) == m_pWorkThread->m_uVecToUnmark.end())
		{
			m_pWorkThread->m_uVecToUnmark.push_back(uIndex);
		}
	}

	return uIndex;
}

LRESULT CvocabularyDlg::OnMsgShowFoundExplanation(WPARAM wParam/* show the spell in the 'spell' label OR not */, LPARAM lParam /* contains valid index in m_pWorkThread->m_strsSpell or INVALID_INDEX */)
{
	if(CVocabConst::INVALID_INDEX == lParam)
	{
		if (0 != m_pWaitDlg && TRUE == ::IsWindow(m_pWaitDlg->m_hWnd))
			::MessageBox(m_pWaitDlg->m_hWnd, L"Not found.", L"finding", MB_OK | MB_ICONINFORMATION);
		else ShowMsgBox(L"Not found.", L"finding", MB_OK | MB_ICONINFORMATION);

		return 0;	// continue to get input until user quit
	}
	if (m_pWorkThread->m_strsSpell.size() > lParam)
	{
		list<CSensibleInfo *> strList;
		m_lblExplanation.SetText(*m_pWorkThread->m_strsExplanation[lParam]);

		if(TRUE == wParam)
		{
			m_lblSpell.SetText(*m_pWorkThread->m_strsSpell[lParam]);
			MakeRelatedWordSpell((UINT)lParam, strList);
			m_lblRelated.SetText(CVocabConst::EMPTYSTR);
			AddWordsToLabel(strList);
		}
		m_pWorkThread->PostThreadMsg(CVocabConst::MSG_SPEAK, (WPARAM)lParam, 0);
		m_uFoundIndex = (UINT)lParam;
	}

	if(0 != m_pWaitDlg && TRUE == ::IsWindow(m_pWaitDlg->m_hWnd))
		m_pWaitDlg->PostMessage(WM_CLOSE, 0, 0);
	EnableWindow();

	return 0;
}

void CvocabularyDlg::OnSearchWord()
{
	HANDLE hResumeEvent;

	if (0 == m_pWaitDlg)
	{
		m_pWaitDlg = new CWaitDlg;
	}

	// only one dialogue can be shown at a time
	if (m_pWaitDlg->IsWindow())return;

	hResumeEvent = ::CreateEvent(NULL, TRUE, FALSE, NULL);	
	m_pWaitDlg->RunModalLoop(m_hWnd, 0);
	if (WAIT_OBJECT_0 != ::WaitForSingleObject(m_pWaitDlg->GetCreationEvent(), 2000))
	{
		ShowMsgBox(L"Fail to create dialog,please retry.", L"error");
		return;
	}
	m_pWaitDlg->SetWindowText(L"search word");
	m_pWaitDlg->PostMessage(CVocabConst::MSG_CREATE_CHILDWND, CWaitDlg::SEARCH, 0);
	EnableWindow(FALSE);
}

void CvocabularyDlg::OnAddWord()
{
	HANDLE hResumeEvent;

	if (0 == m_pWaitDlg)
	{
		m_pWaitDlg = new CWaitDlg;
	}

	// only one dialogue can be shown at a time
	if (m_pWaitDlg->IsWindow())return;

	hResumeEvent = ::CreateEvent(NULL, TRUE, FALSE, NULL);	
	m_pWaitDlg->RunModalLoop(m_hWnd, 0);
	if (WAIT_OBJECT_0 != ::WaitForSingleObject(m_pWaitDlg->GetCreationEvent(), 2000))
	{
		ShowMsgBox(L"Fail to create dialog,please retry.", L"error");
		return;
	}
	m_pWaitDlg->SetWindowText(L"add new words");
	m_pWaitDlg->PostMessage(CVocabConst::MSG_CREATE_CHILDWND, CWaitDlg::ADD_NEW, 0);
	EnableWindow(FALSE);
}

void CvocabularyDlg::OnCmdAddRelated()
{
	HANDLE hResumeEvent;

	if (0 == m_pWaitDlg)
	{
		m_pWaitDlg = new CWaitDlg;
	}

	if (m_pWaitDlg->IsWindow())return;

	hResumeEvent = ::CreateEvent(NULL, TRUE, FALSE, NULL);	
	m_pWaitDlg->RunModalLoop(m_hWnd, 0);
	if (WAIT_OBJECT_0 != ::WaitForSingleObject(m_pWaitDlg->GetCreationEvent(), 2000))
	{
		ShowMsgBox(L"Fail to create dialog,please retry.", L"error");
		return;
	}
	m_pWaitDlg->SetWindowText(L"add related word");
	m_pWaitDlg->PostMessage(CVocabConst::MSG_CREATE_CHILDWND, CWaitDlg::ADD_RELATED, 0);
	EnableWindow(FALSE);
}

void CvocabularyDlg::OnCmdRemoveRelated()
{
	int nPos;
	UINT uIndex(CVocabConst::INVALID_INDEX);
	CString strSpell, oldStr;
	map <UINT, CStringW*> ::const_iterator iter;

	m_lblRelated.SearchSensibleArea(globalVar.g_ptRButtonUp, strSpell);

	// if related string already existed,just remove the spell from this string
	if (FALSE == strSpell.IsEmpty())
	{
		if (FALSE == m_chkTaskRandom.IsChecked())
		{
			uIndex = m_ulIndex_s;
		}
		else
		{
			if (m_vecRandomIndex.size() > m_ulIndex_r)
			{
				uIndex = m_vecRandomIndex[m_ulIndex_r];
			}
		}

		if (CVocabConst::INVALID_INDEX == uIndex)return;
		iter = m_pWorkThread->m_mapRelatedIndexString.find(uIndex);
		if (m_pWorkThread->m_mapRelatedIndexString.end() != iter)
		{
			oldStr = *(iter->second);
			if (oldStr == strSpell)oldStr = CVocabConst::EMPTYSTR;
			else
			{
				oldStr.Replace(strSpell + CVocabConst::COMMA, CVocabConst::EMPTYSTR);
				oldStr.Replace(CVocabConst::COMMA + strSpell, CVocabConst::EMPTYSTR);
			}
			*(iter->second) = oldStr;
			m_pWorkThread->m_uVecChangedRelated.push_back(uIndex);
			m_lblRelated.RedrawWindow();
		}
	}
}

HRGN CreateRgnToErease(CRect &rcPrev, CRect &rcNew)
{
	int nLeft, nRight, nTop, nBottom;
	CRect rcUnion;

	UnionRect(&rcUnion, &rcNew, &rcPrev);
	if(rcNew.left > rcPrev.left)
	{
		nLeft = 0;
		nRight = rcPrev.right - rcPrev.left;
	}
	else
	{
		nLeft = rcPrev.left - rcNew.left;
		nRight = rcUnion.right - rcUnion.left;
	}

	if(rcNew.top > rcPrev.top)
	{
		nTop = 0;
		nBottom = rcPrev.bottom - rcPrev.top;
	}
	else
	{
		nTop = rcPrev.top - rcNew.top;
		nBottom = rcUnion.bottom - rcUnion.top;
	}

	return ::CreateRectRgn(nLeft, nTop, nRight, nBottom);
}

HRGN CreateRgnToDraw(CRect &rcPrev, CRect &rcNew)
{
	int nLeft, nRight, nTop, nBottom;
	CRect rcUnion;

	UnionRect(&rcUnion, &rcNew, &rcPrev);
	if(rcNew.left > rcPrev.left)
	{
		nLeft = rcNew.left - rcPrev.left;
		nRight = rcUnion.right - rcUnion.left;
	}
	else
	{
		nLeft = 0;
		nRight = rcNew.right - rcNew.left;
	}

	if(rcNew.top > rcPrev.top)
	{
		nTop = rcNew.top - rcPrev.top;
		nBottom = rcUnion.bottom - rcUnion.top;
	}
	else
	{
		nTop = 0;
		nBottom = rcPrev.bottom - rcPrev.top;
	}

	return ::CreateRectRgn(nLeft, nTop, nRight, nBottom);
}

//
// ptMouse & orgRc are in screen coordinates
//
int CvocabularyDlg::CalculateNewPosSize(const CPoint& ptMouse, CRect &orgRc, const UINT uHitCode)
{
	switch(uHitCode)
	{
	case HTLEFT:
		return orgRc.left = ptMouse.x;
	case HTRIGHT:
		return orgRc.right = ptMouse.x;
	case HTTOP:
		return orgRc.top = ptMouse.y;
	case HTBOTTOM:
		return orgRc.bottom = ptMouse.y;
	}

	return 0;
}

UINT CvocabularyDlg::ChangeSize(LPARAM lParam)
{
	CPoint ptMouse, ptPrevious;
	CRect rcWnd;
	MSG msg;
	UINT uHitCode;

	ptPrevious.x = ptPrevious.y = 0;
	uHitCode = OnMsgNcHitTest(0, lParam);

	while(TRUE)
	{
		::GetMessage(&msg, m_hWnd, 0, 0);

		if(WM_PAINT == msg.message)
		{
			ValidateRect(0);
			OutputDebugStringW(L"\n+++++++++++++SIZING:   PAINT   ++++++++++++\n");
			continue;
		}

		if(WM_MOUSEMOVE == msg.message)
		{
			ptMouse.x = GET_X_LPARAM(msg.lParam);
			ptMouse.y = GET_Y_LPARAM(msg.lParam);
			ClientToScreen(&ptMouse);
			GetWindowRect(&rcWnd);

			if(ptMouse.x != ptPrevious.x || ptMouse.y != ptPrevious.y)
			{
				WCHAR sz[255];
				::_itow_s(ptMouse.x,sz,10);
				OutputDebugStringW(L"\nX-pos:");
				OutputDebugStringW(sz);
				
				CRect rcClient;
				HRGN hRgnWnd, hRgnClient, hRgnNC;

				CalculateNewPosSize(ptMouse, rcWnd, uHitCode);
				if (rcWnd.Width() < MIN_WND_WIDTH)rcWnd.right = rcWnd.left + MIN_WND_WIDTH;
				if (rcWnd.Height() < MIN_WND_HEIGHT)rcWnd.bottom = rcWnd.top + MIN_WND_HEIGHT;

				::_itow_s(rcWnd.Width(),sz,10);
				OutputDebugStringW(L"\tWidth:");
				OutputDebugStringW(sz);
				SetWindowPos(0, rcWnd.left, rcWnd.top, rcWnd.Width(), rcWnd.Height(), SWP_NOSENDCHANGING);

				//
				// reset the region of window after every size changing
				// and then,set region of the window to 4 borders only
				//
				rcWnd.OffsetRect(-rcWnd.left, -rcWnd.top);
				rcClient = rcWnd;
				CalculateClientRc(&rcClient);

				hRgnWnd = ::CreateRectRgn(rcWnd.left, rcWnd.top, rcWnd.right, rcWnd.bottom);
				hRgnClient = ::CreateRectRgn(rcClient.left, rcClient.top, rcClient.right, rcClient.bottom);
				hRgnNC = ::CreateRectRgn(0, 0, 0, 0);
				::CombineRgn(hRgnNC, hRgnWnd, hRgnClient, RGN_DIFF);
				SetWindowRgn(hRgnNC, 1);

				::DeleteObject(hRgnWnd);
				::DeleteObject(hRgnClient);
				::DeleteObject(hRgnNC);
				::OutputDebugStringW(L"-----------------------SIZING-----------------\n");
				ptPrevious = ptMouse;
			}
			continue;
		}

		if(WM_LBUTTONUP == msg.message)
		{
			//Invalidate();
			break;
		}
	}

	SendMessage(WM_EXITSIZEMOVE, 0, 0);
	InitChildWndSizePos();

	return 0;
}

UINT CvocabularyDlg::DoMove()
{
	WINDOWPLACEMENT wp;
	CPoint ptMouse;
	CRect rcNew, rcFrameWnd;
	MSG msg;
	BOOL bRgnChanged(FALSE);

	GetWindowPlacement(&wp);
	if(SW_MAXIMIZE == wp.showCmd)return 0;

	while(TRUE)
	{
		::GetMessage (&msg,m_hWnd, 0, 0);

		if(WM_LBUTTONUP == msg.message)
		{
			ptMouse.x = GET_X_LPARAM(msg.lParam);
			ptMouse.y = GET_Y_LPARAM(msg.lParam);
			ClientToScreen(&ptMouse);
			if(ptMouse.x == globalVar.g_ptLBtnDown.x && ptMouse.y == globalVar.g_ptLBtnDown.y)
				break;

			if(::GetTickCount() - globalVar.g_dwTickCnt < 500)
				break;
		}

		if(0xff00 == msg.message)
		{
			GetWindowRect(&rcNew);
			MoveWindow(rcNew.left + (int)msg.wParam, rcNew.top + (int)msg.lParam, rcNew.Width(), rcNew.Height());
			break;
		}

		if(WM_MOUSEMOVE == msg.message)
		{
			ptMouse.x = GET_X_LPARAM(msg.lParam);
			ptMouse.y = GET_Y_LPARAM(msg.lParam);

			ClientToScreen(&ptMouse);

			//// it may be large than 2<<15(what 's the reason??)
			//if(ptMouse.x > SCREEN_WIDTH)ptMouse.x -= MAX_VALUE;
			//if(ptMouse.y > SCREEN_HEIGHT)ptMouse.y -= MAX_VALUE;

			rcNew.left = m_rcWndBeforeMove.left + ptMouse.x - globalVar.g_ptLBtnDown.x;
			rcNew.right = rcNew.left + m_rcWndBeforeMove.right - m_rcWndBeforeMove.left;
			rcNew.top = m_rcWndBeforeMove.top + ptMouse.y - globalVar.g_ptLBtnDown.y;
			rcNew.bottom = rcNew.top + m_rcWndBeforeMove.bottom - m_rcWndBeforeMove.top;

			if(MIN_DELTA_PIXEL <= ::abs(ptMouse.x - globalVar.g_ptLBtnDown.x) || MIN_DELTA_PIXEL <= ::abs(ptMouse.y - globalVar.g_ptLBtnDown.y))
			{
				// change region of the window when receiving first WM_MOUSEMOVE after WM_LBUTTONDOWN
				if( !bRgnChanged)
				{
					GetWindowRect(&m_rcWndBeforeMove);
					rcFrameWnd = m_rcWndBeforeMove;
					rcFrameWnd.OffsetRect(-rcFrameWnd.left, -rcFrameWnd.top);
					HRGN h1=::CreateRectRgn(0, 0, rcFrameWnd.Width(), rcFrameWnd.Height());
					CalculateClientRc(&rcFrameWnd);
					HRGN h2=::CreateRectRgn(rcFrameWnd.left, rcFrameWnd.top, rcFrameWnd.right, rcFrameWnd.bottom);
					HRGN h3=::CreateRectRgn(0, 0, 0, 0);
					::CombineRgn(h3, h1, h2, RGN_DIFF);
					SetWindowRgn(h3, TRUE);
					bRgnChanged = 1;
				}
				rcFrameWnd = m_rcWndBeforeMove;

				_FrameWnd.Create(&_FrameWnd, 0, CWnd0::WND0_CLASSNAME, 0, WS_POPUP, rcFrameWnd, m_hWnd, 0, 0);
				_FrameWnd.ShowWindow(SW_SHOWNORMAL);
				_FrameWnd.BringWindowToTop();
				_FrameWnd.SetCapture();
				_FrameWnd.SendMessage(WM_NCLBUTTONDOWN, HTCAPTION, MAKELONG(ptMouse.x, ptMouse.y));
			}
		}
	}

	HDC hWndDC = GetWindowDC();
	::BitBlt(hWndDC, 0, 0, m_rcWndBeforeMove.Width(), m_rcWndBeforeMove.Height(), m_hMemDC, 0, 0, SRCCOPY);
	::ReleaseDC(0, hWndDC);
	SendMessage(WM_EXITSIZEMOVE, 0, 0);

	return 0;
}

LRESULT CvocabularyDlg::OnMsgSysCommand(WPARAM wParam,LPARAM lParam)
{
	if(SC_MOVE == wParam)return DoMove();
	if(SC_SIZE == wParam)return ChangeSize(lParam);
	if(SC_CLOSE == wParam)PostMessage(WM_COMMAND, IDC_BTN_EXIT, 0);

	if(SC_MINIMIZE == wParam)ShowWindow(SW_MINIMIZE);
	if(SC_RESTORE == wParam)
	{
		ShowWindow(SW_SHOWNOACTIVATE);
		InitChildWndSizePos();
	}

	return 0;
}

LRESULT CvocabularyDlg::OnMsgNcLButtonDown(WPARAM wParam, LPARAM lParam)
{
	UINT uHitCode;
	_pwndNCD p;
	CRect rcWnd;

	globalVar.g_bIsLBtnDown = TRUE;
	globalVar.g_ptLBtnDown.x = GET_X_LPARAM(lParam);
	globalVar.g_ptLBtnDown.y = GET_Y_LPARAM(lParam);
	globalVar.g_dwTickCnt = ::GetTickCount();

	BringWindowToTop();
	SetCapture();
	uHitCode = OnMsgNcHitTest(wParam, lParam);
	GetWindowRect(&rcWnd);

	switch(uHitCode)
	{
	case	HTCAPTION:
		SendMessage(WM_SYSCOMMAND, SC_MOVE, lParam);
		break;

	case	HTTOP:
	case	HTLEFT:
	case	HTBOTTOM:
	case	HTRIGHT:
		SendMessage(WM_SYSCOMMAND, SC_SIZE, lParam);
		break;

	case HTMINBUTTON:
		drawMinbox(rcWnd, CVocabConst::COMBO_STATE::LBTNDOWN);
		break;

	case HTMAXBUTTON:
		drawMaxbox(rcWnd, CVocabConst::COMBO_STATE::LBTNDOWN);
		break;

	case HTCLOSE:
		drawClosebox(rcWnd, CVocabConst::COMBO_STATE::LBTNDOWN);
		break;
	}

	TRACKMOUSEEVENT tm;
	tm.cbSize = sizeof(TRACKMOUSEEVENT);
	tm.dwFlags = TME_CANCEL | TME_NONCLIENT |TME_LEAVE;
	tm.hwndTrack = m_hWnd;
	tm.dwHoverTime = 0;
	TrackMouseEvent(&tm);

	return 0;
}

LRESULT CvocabularyDlg::OnMsgNcHitTest(WPARAM , LPARAM lParam)
{
	_pwndNCD p;

	p = ncHitTest(lParam);
	if (0 != p)return p->uHitTestCode;
	return HTNOWHERE;
}

CvocabularyDlg::_pwndNCD CvocabularyDlg::ncHitTest(LPARAM lParam)
{
	CRect rc;
	CPoint ptMouse;
	_pwndNCD p(0);

	//
	// searching areas according to sequence:
	// HTMINBUTTON
	// HTMAXBUTTON
	// HTCLOSE
	// HTLEFT,HTTOP,HTRIGHT,HTBOTTOM
	// HTSYSMENU
	// HTCAPTION
	//

	ptMouse.x = GET_X_LPARAM(lParam);
	ptMouse.y = GET_Y_LPARAM(lParam);
	GetWindowRect(&rc);
	ptMouse.Offset(-rc.left, -rc.top);

	if(0 != m_pRgnList.size())
	{
		p = SearchNcArea(HTMINBUTTON);
		if (0 != p)
		{
			::GetRgnBox(p->hRgn, &rc);
			if(::PtInRect(&rc, ptMouse))
				return p;
		}
		p = SearchNcArea(HTMAXBUTTON);
		if (0 != p)
		{
			::GetRgnBox(p->hRgn, &rc);
			if(::PtInRect(&rc, ptMouse))
				return p;
		}
		p = SearchNcArea(HTCLOSE);
		if (0 != p)
		{
			::GetRgnBox(p->hRgn, &rc);
			if(::PtInRect(&rc, ptMouse))
				return p;
		}
		p = SearchNcArea(HTLEFT);
		if (0 != p)
		{
			::GetRgnBox(p->hRgn, &rc);
			if(::PtInRect(&rc, ptMouse))
				return p;
		}
		p = SearchNcArea(HTTOP);
		if (0 != p)
		{
			::GetRgnBox(p->hRgn, &rc);
			if(::PtInRect(&rc, ptMouse))
				return p;
		}
		p = SearchNcArea(HTRIGHT);
		if (0 != p)
		{
			::GetRgnBox(p->hRgn, &rc);
			if(::PtInRect(&rc, ptMouse))
				return p;
		}
		p = SearchNcArea(HTBOTTOM);
		if (0 != p)
		{
			::GetRgnBox(p->hRgn, &rc);
			if(::PtInRect(&rc, ptMouse))
				return p;
		}
		p = SearchNcArea(HTSYSMENU);
		if (0 != p)
		{
			::GetRgnBox(p->hRgn, &rc);
			if(::PtInRect(&rc, ptMouse))
				return p;
		}
		p = SearchNcArea(HTCAPTION);
		if (0 != p)
		{
			::GetRgnBox(p->hRgn, &rc);
			if(::PtInRect(&rc, ptMouse))
				return p;
		}
	}
	//::OutputDebugStringW(L"return from hittest\n");
	return 0;
}

CvocabularyDlg::_pwndNCD CvocabularyDlg::SearchNcArea(UINT uHitCode)
{
	_pwndNCD p(0);
	CRect rc;
	list<_wndNCD *>::const_iterator iter, endIter;

	iter = m_pRgnList.begin();
	endIter = m_pRgnList.end();

	while(iter != endIter)
	{
		if(uHitCode == (*iter)->uHitTestCode)
		{
			p = *iter;
			break;
		}
		iter++;
	}

	return p;
}

LRESULT CvocabularyDlg::OnMsgMouseMove(WPARAM wParam, LPARAM lParam)
{
	if(MK_LBUTTON == wParam)
	{
		CRect rcWnd;
		CPoint pt;
		static LRESULT bLastHitCode(0);

		pt.x = GET_X_LPARAM(lParam);
		pt.y = GET_Y_LPARAM(lParam);
		ClientToScreen(&pt);

		LRESULT down = OnMsgNcHitTest(wParam, MAKELONG(globalVar.g_ptLBtnDown.x, globalVar.g_ptLBtnDown.y));
		LRESULT now = OnMsgNcHitTest(wParam, MAKELONG(pt.x, pt.y));

		// prevent from repeat painting
		if (down == now && bLastHitCode == now)	return 0;

		if(down != now && now != bLastHitCode)
		{
			GetWindowRect(&rcWnd);
			if(HTMINBUTTON == down)drawMinbox(rcWnd, CVocabConst::COMBO_STATE::HOVERED);
			if(HTMAXBUTTON == down)drawMaxbox(rcWnd, CVocabConst::COMBO_STATE::HOVERED);
			if(HTCLOSE == down)drawClosebox(rcWnd, CVocabConst::COMBO_STATE::HOVERED);
			//::OutputDebugString(L"HOVERED00000000000000000000000000\n");
		}
		if(down == now && now != bLastHitCode)
		{
			GetWindowRect(&rcWnd);
			if(HTMINBUTTON == down)drawMinbox(rcWnd, CVocabConst::COMBO_STATE::LBTNDOWN);
			if(HTMAXBUTTON == down)drawMaxbox(rcWnd, CVocabConst::COMBO_STATE::LBTNDOWN);
			if(HTCLOSE == down)drawClosebox(rcWnd, CVocabConst::COMBO_STATE::LBTNDOWN);
			//::OutputDebugString(L"LBTNDOWN44444444444444444\n");
		}
		bLastHitCode = now;
	}

	return 0;
}

LRESULT CvocabularyDlg::OnMsgNcMouseLeave(WPARAM wParam, LPARAM lParam)
{
	CRect rcWnd;
	GetWindowRect(&rcWnd);

	if(HTMINBUTTON == m_pPreviousArea->uHitTestCode)drawMinbox(rcWnd, CVocabConst::COMBO_STATE::PLAIN);
	if(HTMAXBUTTON == m_pPreviousArea->uHitTestCode)drawMaxbox(rcWnd, CVocabConst::COMBO_STATE::PLAIN);
	if(HTCLOSE == m_pPreviousArea->uHitTestCode)drawClosebox(rcWnd, CVocabConst::COMBO_STATE::PLAIN);

	return 0;
}

LRESULT CvocabularyDlg::OnMsgNcMouseMove(WPARAM wParam, LPARAM lParam)
{
	UINT uHitCode;
	CRect rcWnd, rcScreen, rcBmp;
	_pwndNCD pNew(0), pTemp;
	GetWindowRect(&rcWnd);
	pNew = ncHitTest(lParam);
	std::wstring str1;
	//str1 = std::to_wstring(uHitCode);
	//::OutputDebugStringW(str1.c_str());

	// draw hovered
	if(0 != pNew && 0 == m_pPreviousArea)
	{
		m_pPreviousArea = pNew;

		if (HTMINBUTTON == pNew->uHitTestCode)
		{
			drawMinbox(rcWnd, CVocabConst::COMBO_STATE::HOVERED);
			::OutputDebugStringW(L"draw hovered 1111111111111\n");
		}
		if (HTMAXBUTTON == pNew->uHitTestCode)
		{
			drawMaxbox(rcWnd, CVocabConst::COMBO_STATE::HOVERED);
			::OutputDebugStringW(L"draw hovered 111111111111111111\n");
		}
		if (HTCLOSE == pNew->uHitTestCode)
		{
			drawClosebox(rcWnd, CVocabConst::COMBO_STATE::HOVERED);
			::OutputDebugStringW(L"draw hovered 11111111111111111\n");
		}

		//str1 = std::to_wstring(pNew->uHitTestCode);
		//::OutputDebugStringW(L"first draw:");
		//::OutputDebugStringW(str1.c_str());
		//::OutputDebugStringW(L"\n");
	}

	if(0 != pNew && 0 != m_pPreviousArea && pNew->uHitTestCode != m_pPreviousArea->uHitTestCode)
	{
		pTemp = m_pPreviousArea;
		m_pPreviousArea = pNew;

		if(HTMINBUTTON == pNew->uHitTestCode)
		{
			drawMinbox(rcWnd, CVocabConst::COMBO_STATE::HOVERED);
			//::OutputDebugStringW(L"draw hovered min\n");
		}
		if(HTMAXBUTTON == pNew->uHitTestCode)
		{
			drawMaxbox(rcWnd, CVocabConst::COMBO_STATE::HOVERED);
			//::OutputDebugStringW(L"draw hovered max\n");
		}
		if(HTCLOSE == pNew->uHitTestCode)
		{
			drawClosebox(rcWnd, CVocabConst::COMBO_STATE::HOVERED);
			//::OutputDebugStringW(L"draw hovered close\n");
		}

		// repaint previous area to plain state
		// only these 3 areas need redraw
		if(HTMINBUTTON == pTemp->uHitTestCode)
		{
			drawMinbox(rcWnd, CVocabConst::COMBO_STATE::PLAIN);
			//::OutputDebugStringW(L"draw plain min \n");
		}
		if(HTMAXBUTTON == pTemp->uHitTestCode)
		{
			drawMaxbox(rcWnd, CVocabConst::COMBO_STATE::PLAIN);
			//::OutputDebugStringW(L"draw plain max\n");
		}
		if(HTCLOSE == pTemp->uHitTestCode)
		{
			drawClosebox(rcWnd, CVocabConst::COMBO_STATE::PLAIN);
			//::OutputDebugStringW(L"draw plain close \n");
		}
	}
		TRACKMOUSEEVENT tm;
		tm.cbSize = sizeof(TRACKMOUSEEVENT);
		tm.dwFlags = TME_NONCLIENT | TME_LEAVE;
		tm.hwndTrack = m_hWnd;
		tm.dwHoverTime = 0;
		TrackMouseEvent(&tm);

	return 0;	// message has been handled
}

LRESULT CvocabularyDlg::OnMsgNcPaint(WPARAM wParam, LPARAM)
{
	CRect rcWnd, rcRegion;
	HRGN hRgnCaption, hRgnWnd, hRgnClient, hRgnNc;
	HDC hScreenDC;
	CSize iconSize;

	hScreenDC = ::GetDC(0);
	GetWindowRect(&rcWnd);
	if (0 != m_hMemBitmap)::DeleteObject(m_hMemBitmap);
	m_hMemBitmap = ::CreateCompatibleBitmap(hScreenDC, rcWnd.Width(), rcWnd.Height());
	::SelectObject(m_hMemDC, m_hMemBitmap);

	// fill nonclient area (caption and border)with blue color
	rcRegion = rcWnd;
	rcRegion.OffsetRect(-rcRegion.left, -rcRegion.top);
	hRgnWnd = ::CreateRectRgn(rcRegion.left, rcRegion.top, rcRegion.right, rcRegion.bottom);	
	CalculateClientRc(&rcRegion);
	hRgnClient = ::CreateRectRgn(rcRegion.left, rcRegion.top, rcRegion.right, rcRegion.bottom);
	hRgnNc = ::CreateRectRgn(0, 0, 0 ,0);
	::CombineRgn(hRgnNc, hRgnWnd, hRgnClient, RGN_DIFF);
	::FillRgn(m_hMemDC, hRgnNc, m_hFrameBrush);
	
	// then draw caption with white color
	if(0 != m_pRgnList.size())
	{
		list<_wndNCD *>::const_iterator iter, endIter;

		iter = m_pRgnList.begin();
		endIter = m_pRgnList.end();

		while(iter != endIter)
		{
			if(HTCAPTION == (*iter)->uHitTestCode)break;
			iter++;
		}
		if(endIter == iter)goto _OnMsgNcPaint_exit;
		hRgnCaption = (*iter)->hRgn;
		::GetRgnBox((*iter)->hRgn, &rcRegion);
		rcRegion.right = rcRegion.right - rcRegion.left - BORDER_WIDTH;
		rcRegion.left = BORDER_WIDTH;
		rcRegion.bottom = rcRegion.bottom - rcRegion.top + BORDER_WIDTH*2;
		rcRegion.top = BORDER_WIDTH;
		::FillRect(m_hMemDC, &rcRegion, m_hCaptionBrush);

		// icon
		iter = m_pRgnList.begin();
		endIter = m_pRgnList.end();
		while(iter != endIter)
		{
			if(HTSYSMENU == (*iter)->uHitTestCode)break;
			iter++;
		}
		if(endIter == iter)goto _OnMsgNcPaint_exit;
		HICON hIcon = ::LoadIcon(globalVar.g_hInstance, MAKEINTRESOURCE(IDI_APP_ICON));
		
		Tools1::GetIconSize(hIcon,&iconSize);

		::GetRgnBox((*iter)->hRgn, &rcRegion);
		rcRegion.OffsetRect(-rcRegion.left, -rcRegion.top);
		rcRegion.left = BORDER_WIDTH + BORDER_WIDTH/* gap */;
		rcRegion.right = rcRegion.left + BORDER_WIDTH + BORDER_WIDTH + iconSize.cx;
		rcRegion.top = BORDER_WIDTH + (CAPTION_HEIGHT - iconSize.cy)/2;
		rcRegion.bottom = CAPTION_HEIGHT;
		::DrawIcon(m_hMemDC, rcRegion.left, rcRegion.top, hIcon);

		// title string
		CString strTitle(WND_TITLE);
		CRect rcTitle;

		::SelectObject(m_hMemDC, globalVar.g_hDefEngFont);
		::DrawText(m_hMemDC, strTitle, strTitle.GetLength(), &rcTitle, DT_LEFT|DT_CALCRECT|DT_SINGLELINE);

		::GetRgnBox(hRgnCaption, &rcRegion);
		rcRegion.OffsetRect(-rcRegion.left, -rcRegion.top);
		rcRegion.left += BORDER_WIDTH*4/* double of border & gap */ +  iconSize.cx;
		rcRegion.right = rcRegion.left+ BORDER_WIDTH + iconSize.cx + BORDER_WIDTH/* gap */ + rcTitle.Width();
		rcRegion.top = BORDER_WIDTH + (CAPTION_HEIGHT - rcTitle.Height())/2;
		rcRegion.bottom = CAPTION_HEIGHT;
		::SetTextColor(m_hMemDC, CVocabConst::COLOR2);
		::SetBkColor(m_hMemDC, CVocabConst::COLOR_CAPTION);
		::DrawText(m_hMemDC, strTitle, strTitle.GetLength(), &rcRegion, DT_LEFT|DT_SINGLELINE|DT_VCENTER);

		if (FALSE == globalVar.g_bIsLBtnDown)
		{
			// min & max & close button
			drawMinbox(rcWnd, CVocabConst::COMBO_STATE::PLAIN);
			drawMaxbox(rcWnd, CVocabConst::COMBO_STATE::PLAIN);
			drawClosebox(rcWnd, CVocabConst::COMBO_STATE::PLAIN);
		}
	}

	::BitBlt(m_hWndDC, 0, 0, rcWnd.Width(), rcWnd.Height(), m_hMemDC, 0, 0, SRCCOPY);

_OnMsgNcPaint_exit:
	::DeleteObject(hRgnWnd);
	::DeleteObject(hRgnClient);
	::DeleteObject(hRgnNc);
	::ReleaseDC(0, hScreenDC);
   ::OutputDebugStringW(L"+++++++++++++++++++++NCPAINT()++++++++++++++++++\n");
	return 0;
}

void CvocabularyDlg::drawMinbox(const CRect& rcWnd, UINT uState)
{
	if (0 == m_pRgnList.size())return;

	CRect rcRegion, rcScreen, rcCopy;
	list<_wndNCD *>::const_iterator iter, endIter;
	HDC hScreenDC;

	hScreenDC = ::GetWindowDC(0);

	iter = m_pRgnList.begin();
	endIter = m_pRgnList.end();
	while(iter != endIter)
	{
		if(HTMINBUTTON == (*iter)->uHitTestCode)break;
		iter++;
	}

	if (endIter == iter)return;
	::GetRgnBox((*iter)->hRgn, &rcRegion);
	GetWindowRect(&rcCopy);
	rcScreen = rcRegion;
	rcScreen.OffsetRect(rcCopy.left, rcCopy.top);
	rcRegion.right -= 2;
	rcCopy = rcRegion;

	if(CVocabConst::COMBO_STATE::PLAIN == uState)	::FillRect(m_hMemDC, &rcRegion, m_hCaptionBrush);
	if(CVocabConst::COMBO_STATE::HOVERED == uState)		::FillRect(m_hMemDC, &rcRegion, m_hWhiteBrush);
	if(CVocabConst::COMBO_STATE::LBTNDOWN == uState)		::FillRect(m_hMemDC, &rcRegion, m_hLBDBrush);
	rcRegion.left += 4;
	rcRegion.right -= 4;
	rcRegion.bottom -= 6;
	rcRegion.top = rcRegion.bottom - 4;
	if(CVocabConst::COMBO_STATE::PLAIN == uState)	::FillRect(m_hMemDC, &rcRegion, m_hBlackBrush);
	if(CVocabConst::COMBO_STATE::HOVERED == uState)	::FillRect(m_hMemDC, &rcRegion, m_hLBDBrush);
	if(CVocabConst::COMBO_STATE::LBTNDOWN == uState)	::FillRect(m_hMemDC, &rcRegion, m_hWhiteBrush);

	::BitBlt(hScreenDC, rcScreen.left, rcScreen.top, rcScreen.Width(), rcScreen.Height(), m_hMemDC, rcCopy.left, rcCopy.top, SRCCOPY);
	::ReleaseDC(0, hScreenDC);
}

void CvocabularyDlg::drawMaxbox(const CRect& rcWnd, UINT uState)
{
	if (0 == m_pRgnList.size())return;

	CRect rcRegion, rcScreen, rcCopy;
	list<_wndNCD *>::const_iterator iter, endIter;
	HDC hScreenDC;

	hScreenDC = ::GetWindowDC(0);

	iter = m_pRgnList.begin();
	endIter = m_pRgnList.end();
	while(iter != endIter)
	{
		if(HTMAXBUTTON == (*iter)->uHitTestCode)break;
		iter++;
	}

	if (endIter == iter)return;
	::GetRgnBox((*iter)->hRgn, &rcRegion);
	GetWindowRect(&rcCopy);
	rcScreen = rcRegion;
	rcScreen.OffsetRect(rcCopy.left, rcCopy.top);
	rcCopy = rcRegion;
	if(CVocabConst::COMBO_STATE::PLAIN == uState)	::FillRect(m_hMemDC, &rcRegion, m_hCaptionBrush);
	if(CVocabConst::COMBO_STATE::HOVERED == uState)		::FillRect(m_hMemDC, &rcRegion, m_hWhiteBrush);
	if(CVocabConst::COMBO_STATE::LBTNDOWN == uState)		::FillRect(m_hMemDC, &rcRegion, m_hLBDBrush);

	HBRUSH h(0);
	if (CVocabConst::COMBO_STATE::PLAIN == uState)h = m_hBlackBrush;
	if (CVocabConst::COMBO_STATE::HOVERED == uState)h = m_hLBDBrush;
	if (CVocabConst::COMBO_STATE::LBTNDOWN == uState)h = m_hWhiteBrush;
	rcRegion.DeflateRect(6, 6);
	::FrameRect(m_hMemDC, &rcRegion, h);
	rcRegion.DeflateRect(1, 1);
	::FrameRect(m_hMemDC, &rcRegion, h);
	::BitBlt(hScreenDC, rcScreen.left, rcScreen.top, rcScreen.Width(), rcScreen.Height(), m_hMemDC, rcCopy.left, rcCopy.top, SRCCOPY);
	::ReleaseDC(0, hScreenDC);
}

void CvocabularyDlg::drawClosebox(const CRect& rcWnd, UINT uState)
{
	if (0 == m_pRgnList.size())return;

	CRect rcRegion, rcScreen, rcCopy;
	list<_wndNCD *>::const_iterator iter, endIter;
	HDC hScreenDC;

	hScreenDC = ::GetWindowDC(0);

	iter = m_pRgnList.begin();
	endIter = m_pRgnList.end();
	while(iter != endIter)
	{
		if(HTCLOSE == (*iter)->uHitTestCode)break;
		iter++;
	}
	if (endIter == iter)return;
	::GetRgnBox((*iter)->hRgn, &rcRegion);
	GetWindowRect(&rcCopy);
	rcScreen = rcRegion;
	rcScreen.OffsetRect(rcCopy.left, rcCopy.top);
	rcCopy = rcRegion;
	if(CVocabConst::COMBO_STATE::PLAIN == uState)	::FillRect(m_hMemDC, &rcRegion, m_hCaptionBrush);
	if(CVocabConst::COMBO_STATE::HOVERED == uState)		::FillRect(m_hMemDC, &rcRegion, m_hWhiteBrush);
	if(CVocabConst::COMBO_STATE::LBTNDOWN == uState)		::FillRect(m_hMemDC, &rcRegion, m_hLBDBrush);

	rcRegion.left += OFFSET_V;
	rcRegion.right -= OFFSET_V;
	rcRegion.top += OFFSET_V;
	rcRegion.bottom -= OFFSET_V;

	// make width=height,then the line will be 45¡ã
	if(rcRegion.Width() > rcRegion.Height())
	{
		while(rcRegion.Width() > rcRegion.Height())
		{
			rcRegion.right -= 1;
		}
	}
	if(CVocabConst::COMBO_STATE::PLAIN == uState)
	{
		for(int i=rcRegion.left;i<=rcRegion.right;i++)
			for(int k=rcRegion.top;k<=rcRegion.bottom;i++,k++)
				::SetPixel(m_hMemDC,i,k,CVocabConst::BLACK);
		for(int i=rcRegion.left;i<=rcRegion.right;i++)
			for(int k=rcRegion.bottom;k>=rcRegion.top;i++,k--)
				::SetPixel(m_hMemDC,i,k,CVocabConst::BLACK);

		for(int i=rcRegion.left+1;i<=rcRegion.right+1;i++)
			for(int k=rcRegion.top;k<=rcRegion.bottom;i++,k++)
				::SetPixel(m_hMemDC,i,k,CVocabConst::BLACK);
		for(int i=rcRegion.left+2;i<=rcRegion.right+2;i++)
			for(int k=rcRegion.top;k<=rcRegion.bottom;i++,k++)
				::SetPixel(m_hMemDC,i,k,CVocabConst::BLACK);

		for(int i=rcRegion.left;i<=rcRegion.right;i++)
			for(int k=rcRegion.bottom;k>=rcRegion.top;i++,k--)
				::SetPixel(m_hMemDC,i,k,CVocabConst::BLACK);
		for(int i=rcRegion.left+1;i<=rcRegion.right+1;i++)
			for(int k=rcRegion.bottom;k>=rcRegion.top;i++,k--)
				::SetPixel(m_hMemDC,i,k,CVocabConst::BLACK);
		for(int i=rcRegion.left+2;i<=rcRegion.right+2;i++)
			for(int k=rcRegion.bottom;k>=rcRegion.top;i++,k--)
				::SetPixel(m_hMemDC,i,k,CVocabConst::BLACK);
	}
	if(CVocabConst::COMBO_STATE::HOVERED == uState)
	{
		for(int i=rcRegion.left;i<=rcRegion.right;i++)
			for(int k=rcRegion.top;k<=rcRegion.bottom;i++,k++)
				::SetPixel(m_hMemDC,i,k,CVocabConst::COLOR2);
		for(int i=rcRegion.left;i<=rcRegion.right;i++)
			for(int k=rcRegion.bottom;k>=rcRegion.top;i++,k--)
				::SetPixel(m_hMemDC,i,k,CVocabConst::COLOR2);

		for(int i=rcRegion.left+1;i<=rcRegion.right+1;i++)
			for(int k=rcRegion.top;k<=rcRegion.bottom;i++,k++)
				::SetPixel(m_hMemDC,i,k,CVocabConst::COLOR2);
		for(int i=rcRegion.left+2;i<=rcRegion.right+2;i++)
			for(int k=rcRegion.top;k<=rcRegion.bottom;i++,k++)
				::SetPixel(m_hMemDC,i,k,CVocabConst::COLOR2);

		for(int i=rcRegion.left;i<=rcRegion.right;i++)
			for(int k=rcRegion.bottom;k>=rcRegion.top;i++,k--)
				::SetPixel(m_hMemDC,i,k,CVocabConst::COLOR2);
		for(int i=rcRegion.left+1;i<=rcRegion.right+1;i++)
			for(int k=rcRegion.bottom;k>=rcRegion.top;i++,k--)
				::SetPixel(m_hMemDC,i,k,CVocabConst::COLOR2);
		for(int i=rcRegion.left+2;i<=rcRegion.right+2;i++)
			for(int k=rcRegion.bottom;k>=rcRegion.top;i++,k--)
				::SetPixel(m_hMemDC,i,k,CVocabConst::COLOR2);
	}
	if(CVocabConst::COMBO_STATE::LBTNDOWN == uState)
	{
		for(int i=rcRegion.left;i<=rcRegion.right;i++)
			for(int k=rcRegion.top;k<=rcRegion.bottom;i++,k++)
				::SetPixel(m_hMemDC,i,k,CVocabConst::WHITE);
		for(int i=rcRegion.left;i<=rcRegion.right;i++)
			for(int k=rcRegion.bottom;k>=rcRegion.top;i++,k--)
				::SetPixel(m_hMemDC,i,k,CVocabConst::WHITE);

		for(int i=rcRegion.left+1;i<=rcRegion.right+1;i++)
			for(int k=rcRegion.top;k<=rcRegion.bottom;i++,k++)
				::SetPixel(m_hMemDC,i,k,CVocabConst::WHITE);
		for(int i=rcRegion.left+2;i<=rcRegion.right+2;i++)
			for(int k=rcRegion.top;k<=rcRegion.bottom;i++,k++)
				::SetPixel(m_hMemDC,i,k,CVocabConst::WHITE);

		for(int i=rcRegion.left;i<=rcRegion.right;i++)
			for(int k=rcRegion.bottom;k>=rcRegion.top;i++,k--)
				::SetPixel(m_hMemDC,i,k,CVocabConst::WHITE);
		for(int i=rcRegion.left+1;i<=rcRegion.right+1;i++)
			for(int k=rcRegion.bottom;k>=rcRegion.top;i++,k--)
				::SetPixel(m_hMemDC,i,k,CVocabConst::WHITE);
		for(int i=rcRegion.left+2;i<=rcRegion.right+2;i++)
			for(int k=rcRegion.bottom;k>=rcRegion.top;i++,k--)
				::SetPixel(m_hMemDC,i,k,CVocabConst::WHITE);
	}

	::BitBlt(hScreenDC, rcScreen.left, rcScreen.top, rcScreen.Width(), rcScreen.Height(), m_hMemDC, rcCopy.left, rcCopy.top, SRCCOPY);
	::ReleaseDC(0, hScreenDC);
}

LRESULT CvocabularyDlg::OnMsgNcCreate(WPARAM wParam, LPARAM lParam)
{
	m_pPreviousArea = 0;
	m_hWhiteBrush = (HBRUSH)::GetStockObject(WHITE_BRUSH);
	m_hBlackBrush = (HBRUSH)::GetStockObject(BLACK_BRUSH);
	m_hCaptionBrush = ::CreateSolidBrush(CVocabConst::COLOR_CAPTION);
	m_hFrameBrush = ::CreateSolidBrush(CVocabConst::FRM_COLOR);
	m_hLBDBrush = ::CreateSolidBrush(CVocabConst::COLOR2);
	m_hWndDC = GetWindowDC();
	HDC hScreenDC = ::GetWindowDC(0);
	GetWindowRect(&m_rcWndBeforeMove);
	m_hMemDC = ::CreateCompatibleDC(hScreenDC);
	m_hMemBitmap = ::CreateCompatibleBitmap(hScreenDC, m_rcWndBeforeMove.Width(), m_rcWndBeforeMove.Height());
	::SelectObject(m_hMemDC, m_hMemBitmap);
	::ReleaseDC(0, hScreenDC);

	return TRUE;
}

int CvocabularyDlg::CalculateClientRc(RECT *pRcWnd)
{
	pRcWnd->left += BORDER_WIDTH;
	pRcWnd->top += CAPTION_HEIGHT + BORDER_WIDTH * 2 /* border & gap */;
	pRcWnd->right -= BORDER_WIDTH;
	pRcWnd->bottom -= BORDER_WIDTH;

	return 0;
}

LRESULT CvocabularyDlg::OnMsgNcCalcSize(WPARAM wParam, LPARAM lParam)
{
	CRect rcWnd;
	NCCALCSIZE_PARAMS *pParams;

	if(0 == wParam && 0 != lParam)
	{
		CRect *prcWnd = reinterpret_cast<CRect*>(lParam);
		::CopyRect(&rcWnd, prcWnd);
		CalculateClientRc(prcWnd);
		//::OutputDebugString(L"\n$$$$$$$$$$$$ OnMsgNcCalcSize:FALSE   call:CalculateClientRc()$$$$$$$$$$$$$$$$$\n");
	}
	if(TRUE == wParam)
	{
		pParams = reinterpret_cast<NCCALCSIZE_PARAMS*>(lParam);
		rcWnd = pParams->rgrc[0];
		CalculateClientRc(&pParams->rgrc[0]);
		::CopyRect(&pParams->rgrc[1], &pParams->rgrc[0]);
		//::OutputDebugString(L"\n$$$$$$$$$$$$ OnMsgNcCalcSize:TRUE\n");
	}

	return UpdateNcRegion(&rcWnd);
}

LRESULT CvocabularyDlg::OnMsgLButtonUp(WPARAM wParam, LPARAM lParam)
{
	CPoint pt;
	//::OutputDebugStringW(L"up\n");
	pt.x = GET_X_LPARAM(lParam);
	pt.y = GET_Y_LPARAM(lParam);
	ClientToScreen(&pt);
	LRESULT lDown = OnMsgNcHitTest(wParam, MAKELONG(globalVar.g_ptLBtnDown.x, globalVar.g_ptLBtnDown.y));
	LRESULT lUp = OnMsgNcHitTest(wParam, MAKELONG(pt.x, pt.y));

	// up & down must at the same control button
	if(lUp == lDown)	
	{
		if(HTCLOSE == lUp)PostMessage(WM_SYSCOMMAND, SC_CLOSE, MAKELONG(pt.x, pt.y));
		if(HTMINBUTTON == lUp)PostMessage(WM_SYSCOMMAND, SC_MINIMIZE, MAKELONG(pt.x, pt.y));
		if(HTMAXBUTTON == lUp)PostMessage(WM_SYSCOMMAND, SC_MAXIMIZE, MAKELONG(pt.x, pt.y));
	}
	//else ::DefWindowProc(m_hWnd,WM_LBUTTONUP,wParam,lParam);
	globalVar.g_bIsLBtnDown = FALSE;
	ReleaseCapture();
	PostMessage(WM_NCMOUSELEAVE);

	return 0;
}

LRESULT CvocabularyDlg::OnMsgNcDestroy(WPARAM wParam, LPARAM lParam)
{
	_wndNCD *pwe;

	if(0 != m_pRgnList.size())
	{
		while(m_pRgnList.size() > 0)
		{
			pwe = m_pRgnList.back();
			delete pwe;
			m_pRgnList.pop_back();
		}
	}
	::DeleteObject(m_hFrameBrush);
	::DeleteObject(m_hLBDBrush);
	::DeleteObject(m_hWhiteBrush);
	::DeleteObject(m_hBlackBrush);
	::DeleteObject(m_hCaptionBrush);
	if(0 != m_hMemDC)::DeleteDC(m_hMemDC);
	if(0 != m_hMemBitmap)::DeleteObject(m_hMemBitmap);
	ReleaseDC(m_hWndDC);
	::CloseHandle(CvocabularyDlg::_vocabDlg_Event1);

	return 0;
}

UINT CvocabularyDlg::UpdateNcRegion(CRect *pRcWnd)
{
	CRect rcWndNew{0};
	HRGN hRgn(0);
	int nLeft, nRight, nTop, nBottom;

	_wndNCD *pwe;

	::CopyRect(&rcWndNew, pRcWnd);
	//if(rcWndNew.right - rcWndNew.left < MIN_WND_WIDTH)rcWndNew.right = rcWndNew.left + MIN_WND_WIDTH;
	//if(rcWndNew.bottom - rcWndNew.top < MIN_WND_HEIGHT)rcWndNew.bottom = rcWndNew.top + MIN_WND_HEIGHT;

	while(m_pRgnList.size() > 0)
	{
		pwe = m_pRgnList.back();
		::DeleteObject(pwe->hRgn);
		delete pwe;
		m_pRgnList.pop_back();
	}

	rcWndNew.OffsetRect(-rcWndNew.left, -rcWndNew.top);
	nLeft = rcWndNew.right - BORDER_WIDTH - 3 * CLOSEBOX_WIDTH;
	nRight = nLeft + CLOSEBOX_WIDTH;
	nTop = rcWndNew.top + BORDER_WIDTH + (CAPTION_HEIGHT - CLOSEBOX_HEIGHT) / 2;
	nBottom = nTop + CLOSEBOX_HEIGHT;
	hRgn = ::CreateRectRgn(nLeft, nTop, nRight, nBottom);
	if(0 == hRgn)goto _UpdateNcRegion_error;
	pwe = new _wndNCD;
	pwe->hRgn = hRgn;
	pwe->uHitTestCode = HTMINBUTTON;
	m_pRgnList.push_back(pwe);

	nLeft = nLeft + CLOSEBOX_WIDTH;
	nRight = nLeft + CLOSEBOX_WIDTH;
	//nTop = rcWndNew.top + BORDER_WIDTH + BORDER_WIDTH + (CAPTION_HEIGHT - BORDER_WIDTH - CLOSEBOX_HEIGHT) / 2;
	//nBottom = nTop + CLOSEBOX_HEIGHT;
	hRgn = ::CreateRectRgn(nLeft, nTop, nRight, nBottom);
	if(0 == hRgn)goto _UpdateNcRegion_error;
	pwe = new _wndNCD;
	pwe->hRgn = hRgn;
	pwe->uHitTestCode = HTMAXBUTTON;
	m_pRgnList.push_back(pwe);

	nLeft = nLeft + CLOSEBOX_WIDTH;
	nRight = nLeft + CLOSEBOX_WIDTH;
	hRgn = ::CreateRectRgn(nLeft, nTop, nRight, nBottom);
	if(0 == hRgn)goto _UpdateNcRegion_error;
	pwe = new _wndNCD;
	pwe->hRgn = hRgn;
	pwe->uHitTestCode = HTCLOSE;
	m_pRgnList.push_back(pwe);

	hRgn = ::CreateRectRgn(rcWndNew.left + BORDER_WIDTH,
		rcWndNew.top + BORDER_WIDTH,
		rcWndNew.left + BORDER_WIDTH + SMICON_WIDTH,
		rcWndNew.top + BORDER_WIDTH+SMICON_WIDTH);
	if(0 == hRgn)goto _UpdateNcRegion_error;
	pwe = new _wndNCD;
	pwe->hRgn = hRgn;
	pwe->uHitTestCode = HTSYSMENU;
	m_pRgnList.push_back(pwe);

	hRgn = ::CreateRectRgn(rcWndNew.left, 
		rcWndNew.top + BORDER_WIDTH, 
		rcWndNew.right, 
		rcWndNew.top + BORDER_WIDTH + CAPTION_HEIGHT);
	if(0 == hRgn)goto _UpdateNcRegion_error;
	pwe = new _wndNCD;
	pwe->hRgn = hRgn;
	pwe->uHitTestCode = HTCAPTION;
	m_pRgnList.push_back(pwe);

	hRgn = ::CreateRectRgn(rcWndNew.left,
		rcWndNew.top,
		rcWndNew.left + BORDER_WIDTH,
		rcWndNew.bottom);
	if(0 == hRgn)goto _UpdateNcRegion_error;
	pwe = new _wndNCD;
	pwe->hRgn = hRgn;
	pwe->uHitTestCode = HTLEFT;
	m_pRgnList.push_back(pwe);

	hRgn = ::CreateRectRgn(rcWndNew.right - BORDER_WIDTH, rcWndNew.top, rcWndNew.right, rcWndNew.bottom);
	if(0 == hRgn)goto _UpdateNcRegion_error;
	pwe = new _wndNCD;
	pwe->hRgn = hRgn;
	pwe->uHitTestCode = HTRIGHT;
	m_pRgnList.push_back(pwe);

	hRgn = ::CreateRectRgn(rcWndNew.left, rcWndNew.bottom - BORDER_WIDTH, rcWndNew.right, rcWndNew.bottom);
	if(0 == hRgn)goto _UpdateNcRegion_error;
	pwe = new _wndNCD;
	pwe->hRgn = hRgn;
	pwe->uHitTestCode = HTBOTTOM;
	m_pRgnList.push_back(pwe);

	hRgn = ::CreateRectRgn(rcWndNew.left, rcWndNew.top, rcWndNew.right, rcWndNew.top + BORDER_WIDTH);
	if(0 == hRgn)goto _UpdateNcRegion_error;
	pwe = new _wndNCD;
	pwe->hRgn = hRgn;
	pwe->uHitTestCode = HTTOP;
	m_pRgnList.push_back(pwe);

	hRgn = ::CreateRectRgn(rcWndNew.left, rcWndNew.bottom, rcWndNew.right, rcWndNew.bottom - BORDER_WIDTH);
	if(0 == hRgn)goto _UpdateNcRegion_error;
	pwe = new _wndNCD;
	pwe->hRgn = hRgn;
	pwe->uHitTestCode = HTBOTTOM;
	m_pRgnList.push_back(pwe);

	if(NC_RGN_CNT == m_pRgnList.size())return NC_RGN_CNT;

_UpdateNcRegion_error:
	::OutputDebugStringW(L"++++++++++++++++_UpdateNcRegion_error++++++++++\n");

	while(m_pRgnList.size() > 0)
	{
		pwe = m_pRgnList.back();
		::DeleteObject(pwe->hRgn);
		delete pwe;
		m_pRgnList.pop_back();
	}
	return -1;
}

LRESULT CvocabularyDlg::OnMsgEraseBkGnd(WPARAM wParam, LPARAM lParam)
{
	CRect rcClient;
	GetClientRect(&rcClient);
	::FillRect((HDC)wParam, &rcClient, ::GetSysColorBrush(COLOR_3DFACE));

	return 0;
}

LRESULT CvocabularyDlg::OnMsgPaint(WPARAM wParam, LPARAM lParam)
{
	PAINTSTRUCT ps;
	CRect rcClient, rcCtrl;
	HPEN hPen;
	
	BeginPaint(&ps);
	
	GetWindowRect(&rcClient);
	rcClient.OffsetRect(-rcClient.left, -rcClient.top);
	::FillRect(m_hMemDC, &rcClient, ::GetSysColorBrush(COLOR_3DFACE));
	//::BitBlt(ps.hdc, rcClient.left, rcClient.top, rcClient.Width(), rcClient.Height(), m_hMemDC, 0, 0, SRCCOPY);

	::GetWindowRect(GetDlgItem(IDC_LBL_TO), &rcCtrl);
	ScreenToClient(&rcCtrl);
	hPen = ::CreatePen(PS_SOLID, 1, RGB(0,0,127));
	::SelectObject(m_hMemDC, hPen);
	::MoveToEx(m_hMemDC, 0, rcCtrl.bottom, 0);
	::LineTo(m_hMemDC, rcClient.Width(), rcCtrl.bottom);
	::BitBlt(ps.hdc, rcClient.left, rcClient.top, rcClient.Width(), rcClient.Height(), m_hMemDC, 0, 0, SRCCOPY);
	EndPaint(&ps);
	
	////::OutputDebugStringW(L"CvocabularyDlg::WM_PAINT\n");
	return 0;
}

void CvocabularyDlg::ShowMsgBox(LPCWSTR lpszText, LPCWSTR lpszCaption, UINT nType)
{
	CStringW strTxt;
	strTxt = lpszText;
	if (strTxt.IsEmpty())return;
	MessageBox(lpszText, lpszCaption, nType);
}

BOOL CALLBACK CvocabularyDlg::_vocabDlg_enumWndProc(HWND hWnd, LPARAM lParam)
{
	CvocabularyDlg *pWnd = reinterpret_cast<CvocabularyDlg*>(lParam);

	LONG_PTR lp = WS_POPUP | WS_CAPTION;	// style of messageBox window
	WINDOWINFO wi{ 0 };
	wi.cbSize = sizeof(WINDOWINFO);

	pWnd->m_bIsMsgBoxShowing = FALSE;

	if (0 == (::GetWindowLongPtr(hWnd, GWL_STYLE) &  lp))return TRUE;
	else
	{
		if (pWnd->m_hWnd == ::GetParent(hWnd))
		{
			::GetWindowInfo(hWnd, &wi);
			if (CvocabularyDlg::MSGBOX_WNDTYPE == wi.atomWindowType)
			{
				if (pWnd->m_bForcedQuit)
				{
					MSG msg;
					::PostMessage(hWnd, WM_CLOSE, 0, 0);
					while (0 < ::GetMessage(&msg, hWnd, 0, 0))
					{
						::TranslateMessage(&msg);
						::DispatchMessage(&msg);
						if (FALSE == ::IsWindowVisible(hWnd))	break;
					}
					pWnd->PostMessage(WM_CLOSE);
				}
				pWnd->m_bIsMsgBoxShowing = TRUE;
				return FALSE;
			}
		}
	}

	return TRUE;
}

LRESULT CvocabularyDlg::OnMsgSessionEnd(WPARAM wParam, LPARAM)
{
	BOOL bRet = ::EnumWindows(_vocabDlg_enumWndProc, (LPARAM)this);

	// if no specific windows (with style WS_POPUP and WS_CAPTION,and its parent window is this)could be found,
	// the messageBox window has been destroyed
	if(0 != bRet && FALSE == m_bIsMsgBoxShowing)
	{
		::SetEvent(CvocabularyDlg::_vocabDlg_Event1);
		::WaitForSingleObject(m_pWorkThread->GetHandle(), INFINITE);
		PostMessage(WM_CLOSE);
	}

	return 0;
}

LRESULT CvocabularyDlg::WndProc(UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	CRect rcWnd;

	switch (uMsg)
	{
	case CVocabConst::MSG_WAITING_FOR_QUIT:
		m_pWorkThread->PostThreadMsg(CVocabConst::MSG_SESSION_END, 0, (LPARAM)&CvocabularyDlg::_vocabDlg_Event1);
		return 0;

	case CVocabConst::MSG_SESSION_END:
		return OnMsgSessionEnd(wParam, lParam);

	case CVocabConst::MSG_FORCED_QUIT:
		m_bForcedQuit = TRUE;
		return 0;

	case WM_WINDOWPOSCHANGED:
		GetWindowRect(&rcWnd);
		UpdateNcRegion(&rcWnd);
		Invalidate(TRUE);
		return 0;

	case WM_DESTROY:
		PostQuitMessage(0);
		return 0;

	case WM_ENABLE:
		return EnableWindow(wParam);

	//case WM_ACTIVATE:
	//	//return 1;
	//	return ::DefWindowProc(m_hWnd, WM_ACTIVATE, wParam, lParam);

	case WM_MOUSEACTIVATE:
		return MA_ACTIVATE;

	case WM_NCACTIVATE:
		return ::DefWindowProc(m_hWnd, WM_NCACTIVATE, wParam, lParam);

	//case WM_ACTIVATEAPP:
	//	//return 1;
	//	return ::DefWindowProc(m_hWnd, WM_ACTIVATEAPP, wParam, lParam);

	case WM_GETMINMAXINFO:
		{
			PMINMAXINFO pmm;
			APPBARDATA abd;

			pmm = reinterpret_cast<PMINMAXINFO>(lParam);
			abd.cbSize = sizeof(APPBARDATA);
			abd.hWnd = 0;
			::SHAppBarMessage(ABM_GETTASKBARPOS,&abd);
			pmm->ptMaxPosition.x = 0;
			pmm->ptMaxPosition.y = 0;
			pmm->ptMaxSize.y = ::GetSystemMetrics(SM_CYSCREEN) - (abd.rc.bottom - abd.rc.top);
			pmm->ptMinTrackSize.x = ::GetSystemMetrics(SM_CXMINTRACK);
			pmm->ptMinTrackSize.y = ::GetSystemMetrics(SM_CYMINTRACK);
			pmm->ptMaxTrackSize.x = ::GetSystemMetrics(SM_CXMAXTRACK);
			pmm->ptMaxTrackSize.y = ::GetSystemMetrics(SM_CYMAXTRACK);
			ReleaseCapture();
		}
		return 0;

	case WM_QUERYOPEN:
		return TRUE;

	case WM_SIZE:
		//PostMessage(WM_NCCALCSIZE, TRUE, 0);
		//PostMessage(WM_NCPAINT, 0, 0);
		//PostMessage(WM_ERASEBKGND, 0, 0);
		//InitChildWndSizePos();

		return 0;

	case WM_PAINT:
		return OnMsgPaint(wParam, lParam);

	case WM_ERASEBKGND:
		return OnMsgEraseBkGnd(wParam, lParam);
		
	case WM_NCCALCSIZE:
		return OnMsgNcCalcSize(wParam, lParam);

	case WM_NCCREATE:
		return OnMsgNcCreate(wParam, lParam);

	case WM_NCPAINT:
		return OnMsgNcPaint(wParam, lParam);
		
	case WM_NCHITTEST:
		return OnMsgNcHitTest(wParam, lParam);

	case WM_NCDESTROY:
		return OnMsgNcDestroy(wParam, lParam);

	case WM_NCLBUTTONDOWN:
		return OnMsgNcLButtonDown(wParam, lParam);

	case WM_NCMOUSELEAVE:
		//::SetCursor(::LoadCursorW(0, IDC_ARROW));
		return OnMsgNcMouseLeave(wParam, lParam);

	case WM_NCMOUSEMOVE:
		return OnMsgNcMouseMove(wParam, lParam);

	case WM_MOUSEMOVE:
		return OnMsgMouseMove(wParam, lParam);

	case WM_LBUTTONUP:
		return OnMsgLButtonUp(wParam, lParam);

	case WM_SYSCOMMAND:
		return OnMsgSysCommand(wParam, lParam);

	case WM_CAPTURECHANGED:
		{
			if(0 == (HWND)lParam)::ReleaseCapture();
		}
		return 0;

	case WM_EXITSIZEMOVE:
		{
			CPoint ptMouse;
			::GetCursorPos(&ptMouse);
			
			if(MIN_DELTA_PIXEL <= ::abs(ptMouse.x - globalVar.g_ptLBtnDown.x) || MIN_DELTA_PIXEL <= ::abs(ptMouse.y - globalVar.g_ptLBtnDown.y))
			{
				CRect rcWnd;
				HRGN hRgn;

				GetWindowRect(&rcWnd);
				rcWnd.OffsetRect(-rcWnd.left, -rcWnd.top);
				hRgn = ::CreateRectRgn(rcWnd.left, rcWnd.top, rcWnd.right, rcWnd.bottom);
				SetWindowRgn(hRgn);
				::DeleteObject(hRgn);
			}

			// must repaint nc
			OnMsgNcPaint(0, 0);
			ReleaseCapture();
		}
		return 0;

	case CVocabConst::MSG_INIT:
		return OnMsgInit(wParam, lParam);

	case CVocabConst::MSG_ACCKEY:
		{
			if (VK_ESCAPE == wParam)
			{
				OnKeyPressEscape();
			}
			if(IDC_BTN_NEXT == wParam)
			{
				OnClickBtnNext();
			}
			if (IDC_BTN_REP == wParam)
			{
				OnRepeatVoice();
			}
			if(IDC_BTN_PREVIOUS == wParam)
			{
				OnClickBtnPrevious();
			}
			if(IDC_CHK_TASK_RANDOM == wParam)
			{
				m_chkTaskRandom.SetCheck(1 - m_chkTaskRandom.IsChecked());
				m_chkTaskRandom.RedrawWindow();
				OnChkTaskRandom();
			}
			if(IDC_CHK_SHOW_EXPLANATION == wParam)
			{
				m_chkShowExplanation.SetCheck(1 - m_chkShowExplanation.IsChecked());
				m_chkShowExplanation.RedrawWindow();
				OnChkShowExplaination();
			}
		}
		return 0;

	case WM_COMMAND:
		{
			switch(wParam)
			{
				case IDC_BTN_CREATE:
					OnClickBtnCreate();
					break;

				case IDC_BTN_SELECT:
					OnClickBtnSelect();
					break;

				case IDC_BTN_NEXT:
					OnClickBtnNext();
					break;

				case IDC_BTN_PREVIOUS:
					OnClickBtnPrevious();
					break;

				case IDC_BTN_REP:
					OnRepeatVoice();
					break;

				case IDC_BTN_FIND:
					OnSearchWord();
					break;

				case IDC_BTN_ADD:
					OnAddWord();
					break;

				case IDC_BTN_EXIT:
					OnClickBtnExit();
					break;

				case IDC_CHK_TASK_RANDOM:
					OnChkTaskRandom();
					break;

				case IDC_CHK_REVISE:
					OnReviseUnfamiliar();
					break;

				case IDC_CHK_SHOW_EXPLANATION:
					OnChkShowExplaination();
					break;

				case CVocabConst::POPUP_ITEM_ID::ADD_UNFAMILIAR:
					MarkUnfamiliar();
					break;

				case CVocabConst::POPUP_ITEM_ID::REMOVE_UNFAMILIAR:
					UnmarkUnfamiliar();
					break;

				case CVocabConst::POPUP_ITEM_ID::ADD_RELATED:
					OnCmdAddRelated();
					break;

				case CVocabConst::POPUP_ITEM_ID::REMOVE_RELATED:
					OnCmdRemoveRelated();
					break;

				case MAKELONG(IDC_COMBO_GROUP_START,CBN_SELENDOK):
				case MAKELONG(IDC_COMBO_GROUP_END,CBN_SELENDOK):
					OnMsgGroupChanged(wParam, lParam);
			}
		}
		return 0;

	case CVocabConst::MSG_RCV_FILEDLG:
		return OnMsgRcvFileDlgHandle(wParam, lParam);

	case CVocabConst::MSG_DISENABLE_CANCEL:
		return OnMsgDisableCancelling(wParam, lParam);

	case CVocabConst::MSG_PREPARATION_COMPLETED:
		return OnMsgPreparationCompleted(wParam, lParam);

	case CVocabConst::MSG_CANCELLED:
		return OnMsgCancelled(wParam, lParam);

	case CVocabConst::MSG_ADD_RELATED:
		return OnMsgAddRelated(wParam, lParam);

	case CVocabConst::MSG_ADD_NEW:
		return OnMsgAddNewWords(wParam, lParam);

	case	CVocabConst::MSG_FOUND_EXPLANATION:
		return OnMsgShowFoundExplanation(wParam, lParam);

	case CVocabConst::MSG_RCV_SPELL:
		return OnMsgRcvHoverdWord(wParam, lParam);

	case CVocabConst::MSG_UPDATE_WAITDLG:
		return OnMsgUpdateWaitDlg(wParam, lParam);

	case CVocabConst::MSG_SHOW_ERRMSG:
		return OnMsgShowErrMsg(wParam, lParam);
	}

	return ::DefWindowProc(m_hWnd, uMsg, wParam, lParam);
}