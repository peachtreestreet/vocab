

#ifndef __VOCABULARYDLG__
#define __VOCABULARYDLG__

#define _CRTDBG_MAP_ALLOC

////////////////////////////////////////////////////////////////////////////////////////////

#pragma once

////////////////////////////////////////////////////////////////////////////////////////////

#include "vocab.h"
#include "resource.h"
#include "wnd0.h"
#include "wnd1.h"
#include "WorkThread.h"
#include "labelex.h"
#include "FlatDropList.h"
#include "checkBox2.h"
#include "waitdlg.h"

////////////////////////////////////////////////////////////////////////////////////////////

namespace VocabApp
{
	class CvocabularyDlg : public CWnd0
	{
		typedef struct tag_wndNCD
		{
			HRGN		hRgn;
			UINT			uHitTestCode;
		}	_wndNCD, *_pwndNCD;

	public:
		CvocabularyDlg();
		~CvocabularyDlg();

		virtual LRESULT WndProc(UINT uMsg, WPARAM wParam, LPARAM lParam);

	protected:
		LRESULT OnMsgDisableCancelling(WPARAM, LPARAM);
		LRESULT OnMsgPreparationCompleted(WPARAM, LPARAM);
		LRESULT OnMsgRcvHoverdWord(WPARAM, LPARAM);
		LRESULT OnMsgGroupChanged(WPARAM, LPARAM);
		LRESULT OnMsgCancelled(WPARAM, LPARAM);
		LRESULT OnMsgRcvFileDlgHandle(WPARAM, LPARAM);
		LRESULT OnMsgInit(WPARAM, LPARAM);
		LRESULT OnMsgAddRelated(WPARAM, LPARAM);
		LRESULT OnMsgAddNewWords(WPARAM, LPARAM);
		LRESULT OnMsgShowFoundExplanation(WPARAM, LPARAM);
		LRESULT OnMsgUpdateWaitDlg(WPARAM, LPARAM);
		LRESULT OnMsgEraseBkGnd(WPARAM wParam, LPARAM lParam);
		LRESULT OnMsgPaint(WPARAM wParam, LPARAM lParam);
		LRESULT OnMsgMouseMove(WPARAM wParam, LPARAM lParam);
		LRESULT OnMsgShowErrMsg(WPARAM, LPARAM);
		LRESULT OnMsgSessionEnd(WPARAM, LPARAM);

		LRESULT OnMsgNcCalcSize(WPARAM wParam, LPARAM lParam);
		LRESULT OnMsgNcCreate(WPARAM wParam, LPARAM lParam);
		LRESULT OnMsgNcPaint(WPARAM wParam, LPARAM lParam);
		LRESULT OnMsgNcMouseMove(WPARAM wParam, LPARAM lParam);
		LRESULT OnMsgNcMouseLeave(WPARAM wParam, LPARAM lParam);
		LRESULT OnMsgNcHitTest(WPARAM wParam, LPARAM lParam);
		LRESULT OnMsgNcDestroy(WPARAM wParam, LPARAM lParam);
		LRESULT OnMsgNcLButtonDown(WPARAM wParam, LPARAM lParam);
		LRESULT OnMsgLButtonUp(WPARAM wParam, LPARAM lParam);
		LRESULT OnMsgSysCommand(WPARAM wParam, LPARAM lParam);
		UINT UpdateNcRegion(CRect *pRcWnd);
		UINT DoMove();
		UINT ChangeSize(LPARAM lParam);

	protected:
		static LRESULT CALLBACK _vocabDlg_KeyboardProc(int code, WPARAM wParam, LPARAM lParam);
		static BOOL CALLBACK CvocabularyDlg::_vocabDlg_enumWndProc(HWND hWnd, LPARAM lParam);

	private:
		void OnClickBtnExit();
		void OnClickBtnCreate();
		void OnClickBtnNext();
		void OnClickBtnPrevious();
		void OnClickBtnSelect();
		void OnKeyPressEscape();
		void OnChkShowExplaination();
		void OnChkTaskRandom();
		void OnReviseUnfamiliar();
		void OnRepeatVoice();
		void OnSearchWord();
		void OnAddWord();
		void PreviousUnfamiliar();
		void OnCmdAddRelated(void);
		void OnCmdRemoveRelated(void);

		int CalculateClientRc(RECT *pRcWnd);
		int CalculateNewPosSize(const CPoint& ptMouse, CRect &orgRc, const UINT uHitCode);
		int MarkUnfamiliar(void);
		int UnmarkUnfamiliar(void);
		int MakeRelatedWordSpell(UINT uIndex, list<CSensibleInfo*> &info);
		UINT CalculateSelectedCnt();
		UINT MakeRandomList(UINT uTotalCnt, UINT uSelectedCnt);
		int ShowFileDialog(UINT nFilterCnt, WCHAR const* pcszTitle, WCHAR const* pcszFilter, CStringW& strPathName);
		int MakeProgressString(CStringW& strMsg, UINT uCount);
		int CreateChildWnd(void);
		int InitChildWndSizePos(void);
		int AddWordsToLabel(list<CSensibleInfo *> &strList);
		void ChangeBtnState(UINT uWordCnt, UINT uIndex);
		void ShowMsgBox(LPCWSTR lpszText, LPCWSTR lpszCaption,  UINT nType = MB_APPLMODAL | MB_OK | MB_ICONSTOP);
		_pwndNCD SearchNcArea(UINT uHitCode);
		_pwndNCD ncHitTest(LPARAM lParam);

		void drawMinbox(const CRect& rcWnd, UINT uState);
		void drawMaxbox(const CRect& rcWnd, UINT uState);
		void drawClosebox(const CRect& rcWnd, UINT uState);

		////////////////////////////////////////////////////////////////////////////////////////////

	public:
		//static HANDLE						m_hEventMsgDlg;

		static int					SCREEN_WIDTH;
		static int					SCREEN_HEIGHT;
		static int SMICON_WIDTH;

		static int const MIN_WND_WIDTH;
		static int const MIN_WND_HEIGHT;
		static int const LENGTH_PATHNAME_BUFF = 1024;
		static int const X_START;
		static int const Y_START;
		static int const CLOSEBOX_WIDTH = 36;
		static int const CLOSEBOX_HEIGHT = 30;
		static int const MINBOX_HEIGHT = 5;
		static int const BORDER_WIDTH = 3;
		static int const CAPTION_HEIGHT  = 44;
		static int const OFFSET_V = 8;
		static int const NC_RGN_CNT = 10;
		static int const MAX_VALUE = 2<<15;
		static int const MAX_FILTER_CNT = 2;
		static int const MIN_DELTA_PIXEL = 5;
		static const ATOM MSGBOX_WNDTYPE = 0x8002;
		static float const DLG_WIDTH_FACTOR;
		static float const DLG_HEIGHT_FACTOR;
		static WCHAR const FILEDLG_TIP[];

	protected:
		CFlatDropList m_comboGroupStart, m_comboGroupEnd;
		CChkBox2 m_chkShowExplanation, m_chkTaskRandom, m_chkShowUnfamiliar, m_chkShowNotes;
		CLabelEx m_lblSpell, m_lblExplanation, m_lblRelated, m_lblNotes, m_lblProgress, m_labMsg;
		CVocabWorkThread		*m_pWorkThread;
		CWaitDlg *m_pWaitDlg;

		UINT						m_ulIndex_s;		// sequential mode
		UINT						m_ulIndex_r;		// random mode
		UINT						m_uFoundIndex;	// index that has been just found by using FIND button
		UINT						m_ulRandomWordCnt;
		USHORT				m_usStartGrp, m_usEndGrp;
		USHORT				m_usStartGrp2, m_usEndGrp2;
		HICON					m_hIcon;
		HDC						m_hMemDC, m_hWndDC;
		HBITMAP				m_hMemBitmap;
		HBRUSH					m_hFrameBrush, m_hCaptionBrush, m_hWhiteBrush, m_hBlackBrush, m_hLBDBrush;
		CRect						m_rcWndBeforeMove;
		BOOL						m_bIsMsgBoxShowing, m_bForcedQuit;
		vector<UINT>			m_vecRandomIndex;
		_pwndNCD					m_pPreviousArea;
		list<_wndNCD *>		m_pRgnList;	// contains non-client regions & hit-test code

		static CWndOnlyBorder _FrameWnd;
		static HANDLE _vocabDlg_Event1;

		static float const CX_ADJUST;
		static float const CY_ADJUST;
		static float const ADJUST1;

		static WCHAR const APP_TITLE[];
		static WCHAR const TITLE_DLG_CREATION[];
		static WCHAR const TITLE_DLG_SELECTION[];
		static WCHAR const FILTER_STRING_CREATE[];
		static WCHAR const FILTER_STRING_SELECT[];
		static WCHAR const DIRECTORY_INIT[];
		static WCHAR const FONT_NAME_KAITI[];
		static WCHAR const FONT_NAME_TAHOMA[];
		static WCHAR const FONT_NAME_MS_SANS[];
		static WCHAR const WND_TITLE[];
		static WCHAR const BTN_TEXT[][16];
		static WCHAR const LABEL_TEXT[][16];
	};
}

#endif	// __VOCABULARYDLG__