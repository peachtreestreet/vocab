
#ifndef __WAITDLG__
#define __WAITDLG__

////////////////////////////////////////////////////////////////////////////////////////////

#pragma once

////////////////////////////////////////////////////////////////////////////////////////////

#include "vocab.h"
#include "LabelEx.h"
#include "ProgressWnd.h"

////////////////////////////////////////////////////////////////////////////////////////////

class CWaitDlg : public CDialogImpl<CWaitDlg>
{
public:
	enum {IDD = IDD_DLG_MSGBOX};
	static enum DLG_FUNC
	{
		SHOWMSG = 0x10,
		SEARCH = 0x20,
		ADD_RELATED = 0x30,
		ADD_NEW = 0x40,
	};

	BEGIN_MSG_MAP(CWaitDlg)
		MESSAGE_HANDLER(WM_INITDIALOG, OnInitDialog)
		MESSAGE_HANDLER(WM_CLOSE, OnClose)
		MESSAGE_HANDLER(CVocabConst::MSG_CREATE_CHILDWND, OnMsgShow)
		MESSAGE_HANDLER(CVocabConst::MSG_SHOW_PROGRESS, OnMsgShowProgressBar)
		COMMAND_HANDLER(MSGDLG_BTN_ID[0], 0, SendMsgToParent)
		COMMAND_HANDLER(MSGDLG_BTN_ID[1], 0, OnCloseWnd)
		COMMAND_HANDLER(IDCANCEL, 0, OnAbort)
	END_MSG_MAP()

public:
	CWaitDlg();
	virtual ~CWaitDlg();

	BOOL	IsCancelled() { return m_bIsCancelled; }
	int RunModalLoop(HWND hParentWnd, HANDLE *phResumeEvent);
	int SetMsgString(CStringW const& str);
	HANDLE const& GetCreationEvent() {return m_hCreationEvent; }

public:
	static int GetRelated(CString& strSpell, CString& m_strNotes);
	static CStringW m_strSpell, m_strNotes;

protected:
	virtual LRESULT OnInitDialog(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
	LRESULT OnClose(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
	LRESULT OnMsgShow(UINT uMsg, WPARAM wParam/* contains DLG_FUNC value */, LPARAM lParam, BOOL& bHandled); 
	//LRESULT OnMsgShowSearch(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled); 
	LRESULT OnAbort(WORD , WORD , HWND hWnd, BOOL& bHandled);
	LRESULT OnCloseWnd(WORD, WORD, HWND hWnd, BOOL& bHandled);
	LRESULT OnMsgShowProgressBar(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
	LRESULT SendMsgToParent(WORD, WORD, HWND hWnd, BOOL& bHandled);

	static LRESULT CALLBACK _WaitDlg_editProc(HWND hwnd, UINT uMsg, WPARAM wParam,	LPARAM lParam);
	static	WNDPROC	_WaitDlg_oldEditProc;

protected:
	BOOL					m_bIsCancelled;
	UINT					m_uFunc;
	HANDLE				m_hCreationEvent;
	CLabelEx			m_label;
	CProgressWnd	m_progWnd;


	static int const DLG_MSG_WIDTH;
	static int const DLG_MSG_HEIGHT;
	static int const MSGDLG_BTN_ID[];

	static WCHAR const MSGDLG_BTN_TEXT[][16];
	static WCHAR const MSGDLG_LABEL_TEXT[][16];
};

#endif	// __WAITDLG__
