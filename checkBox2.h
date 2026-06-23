
#ifndef __CHKBOX2__
#define __CHKBOX2__

////////////////////////////////////////////////////////////////////////////////////////////

#pragma once

////////////////////////////////////////////////////////////////////////////////////////////

#include "vocab.h"
#include "wnd0.h"
#include <list>

////////////////////////////////////////////////////////////////////////////////////////////

class CChkBox2 : public CWnd0
{
public:
	CChkBox2();
	virtual ~CChkBox2();
	virtual LRESULT WndProc(UINT uMsg,WPARAM wParam,LPARAM lParam);

	virtual HWND Create(HWND hWndParent,LPCWSTR lpszText = 0,CRect const& rcWnd = CRect(0,0,0,0));
	int SetText(CStringW const& strTxt);
	int GetAccelerationVKeyCode(SHORT &sCode1,SHORT &sCode2);
	inline UINT SetID(UINT nID){return m_nID = nID;}
	inline BOOL IsChecked() const{return m_bIsChecked;}
	inline BOOL SetCheck(BOOL bChecked = TRUE){return m_bIsChecked = bChecked;}
	inline CRect& GetWndRect(){return m_rcWnd;}

protected:
	LRESULT OnPaint(WPARAM wParam,LPARAM lParam);
	LRESULT OnEraseBkgnd(WPARAM,LPARAM);
	LRESULT OnNcHitTest(WPARAM,LPARAM);
	LRESULT OnMouseMove(WPARAM,LPARAM);
	LRESULT OnMouseLeave(WPARAM,LPARAM);
	LRESULT OnLButtonDown(WPARAM,LPARAM);
	LRESULT OnLButtonUp(WPARAM,LPARAM);

	LRESULT OnKillFocus(WPARAM,LPARAM);
	LRESULT OnSetFocus(WPARAM,LPARAM);
	LRESULT OnKeyDown(WPARAM,LPARAM);
	LRESULT OnKeyUp(WPARAM,LPARAM);
	LRESULT OnCaptureChanged(WPARAM,LPARAM);

	int DrawPlainState(HDC hdc);
	int DrawHoveredState(HDC hdc);
	int DrawLButtonDownState(HDC hdc);
	int DrawCheckPic(HDC hdc);
	int CaculateAccelerationVKeyCode();

public:
	static int const CHKBOX2_TOTAL_HEIGHT = 20;
	static int const CHECK_RECT_WIDTH = 14;		// 14 pixel width & height for the check rectangle
	static int const CHECK_RECT_HEIGHT = 14;
	static int const PICTURE_TEXT_DISTANCE = 6;
	static int const Y_POS_ADJUST = 2;
	static COLORREF const DEF_TEXT_COLOR = RGB(10,10,10);
	static COLORREF const DEF_CHECKEDTOKEN_COLOR = RGB(0,0,255);
	static long const INVALID_POSITION = -1L;

protected:
	UINT				m_nID;
	UINT				m_nState;
	BOOL				m_bIsChecked,m_bIsFocused,m_bAccelerationPrefixExist;
	SHORT			m_sAccelerationKeyCode,m_sNumPadCode;
	HBRUSH		m_hbrPic,m_hbrText;
	HFONT			m_hFont;
	CRect			m_rcWnd,m_rcPic,m_rcText;
	TRIVERTEX	m_triVertexPlain[6],m_triVertexLBtnDown[6];
	GRADIENT_TRIANGLE		m_Gradient;
	COLORREF		m_clrTextColor,m_clrBkColor;
	CStringW		m_strWndText;
	WCHAR			m_cAccChar;
};

#endif //__CHKBOX2__