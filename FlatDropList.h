
#ifndef			__FLATDROPLIST__
#define			__FLATDROPLIST__

//////////////////////////////////////////////////////////////////////////////////////////////////
#pragma once
//////////////////////////////////////////////////////////////////////////////////////////////////

#include "vocab.h"
#include "ListWnd.h"

//////////////////////////////////////////////////////////////////////////////////////////////////

using namespace std;

//////////////////////////////////////////////////////////////////////////////////////////////////

// support drop-list only
class CFlatDropList : public CWnd0
{
	friend class CListWnd;

public:
	CFlatDropList(void);
	virtual ~CFlatDropList(void);
	virtual LRESULT WndProc(UINT uMsg, WPARAM wParam, LPARAM lParam);
	virtual HWND Create(HWND hWndParent, LPCWSTR lpWindowName = 0, CRect const& rcWnd = CRect(0, 0, 0, 0));

	int ResetContent();
	inline UINT GetCount() const{return m_listWnd.GetItemCount();}
	int AddString(CStringW const& str,HICON h=0);
	int SetCurSel(int nIndex);
	inline UINT SetID(UINT nID){return m_nCtrlID = nID;}
	inline UINT GetID(){return m_nCtrlID;}
	inline UINT GetCurSel(){return m_listWnd.GetCurSel();}

public:
	static const WCHAR FCOMBO_CLASS_NAME[];
	static const UINT HEADER_HEIGHT = 32;
	static const UINT ARROW_R_MARGIN = 6;
	static const UINT ARROW_WIDTH = 4;
	static const UINT ARROW_HEIGHT = 4;
	static const int OFFSET0 = 1;
	static const UINT LEFT_MARGIN = 2;
	static const UINT PEN_WIDTH = 1;

protected:
	LRESULT OnMsgMouseMove(WPARAM wParam, LPARAM lParam);
	LRESULT OnMsgMouseLeave(WPARAM wParam, LPARAM lParam);
	LRESULT OnMsgNcCreate(WPARAM wParam, LPARAM lParam);
	LRESULT OnMsgDestroy(WPARAM wParam, LPARAM lParam);
	LRESULT OnMsgLButtonDown(WPARAM wParam, LPARAM lParam);
	LRESULT OnMsgEraseBkGnd(WPARAM wParam, LPARAM lParam);
	LRESULT OnMsgSelChanged(WPARAM wParam, LPARAM lParam);
	LRESULT OnMsgRcvListWndState(WPARAM wParam, LPARAM lParam);

	UINT GetCurrentState();
	int DrawArrow(HDC hDC);
	int ShowListWnd();

protected:
	BOOL			m_bListWndShown;
	UINT			m_uMinWidth;		// min height=HEADER_HEIGHT
	HBRUSH	m_hbrPlainFrame, m_hbrPlainRc;
	HBRUSH	m_hbrHotFrame, m_hbrHotRc;
	HBRUSH	m_hbrLbdFrame, m_hbrLbdRc;
	HFONT		m_hFont;
	UINT			m_nCtrlID;
	UINT			m_uPrevState;
	CListWnd	m_listWnd;

	static const COLORREF		PLAINFRM = RGB(173,173,173);
	static const COLORREF		PLAINRC = RGB(230,230,230);
	static const COLORREF		HOTFRM = RGB(0,120,215);
	static const COLORREF		HOTRC = RGB(229,241,251);
	static const COLORREF		LBDFRM = RGB(0,84,153);
	static const COLORREF		LBDRC = RGB(204,228,247);
	static const COLORREF		DISENABLERC = RGB(225,225,225);
	static const COLORREF		ARROWCOLOR[];
	static const WCHAR	DEF_FONT_NAME[];
};

#endif		// ifndef __FLATDROPLIST__
