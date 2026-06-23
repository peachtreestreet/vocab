
#ifndef __LABELEX__
#define __LABELEX__

////////////////////////////////////////////////////////////////////////////////////////////

#pragma once

////////////////////////////////////////////////////////////////////////////////////////////

#include <list>
#include <map>
#include <atltypes.h>
#include "vocab.h"
#include "wnd0.h"

////////////////////////////////////////////////////////////////////////////////////////////

class CSensibleInfo
{
public:
	CSensibleInfo();
	~CSensibleInfo();

	CStringW	*pwstr;
	CRect		rc;
};

////////////////////////////////////////////////////////////////////////////////////////////

class CLabelEx : public CWnd0
{
public:
	CLabelEx();
	virtual ~CLabelEx();
	virtual LRESULT WndProc(UINT uMsg, WPARAM wParam, LPARAM lParam);


public:
	inline COLORREF		SetPlainColor(COLORREF color){return m_clrPlainColor=color;}
	inline COLORREF		SetHoveredColor(COLORREF color){return m_clrHoveredColor=color;}
	inline COLORREF		SetUnderlineColor(COLORREF color){return m_clrUnderlineColor=color;}
	inline int					SetBorderWidth(int nWidth){return m_nBorderWidth = nWidth;}
	inline BOOL			ShowUnderline(BOOL bShow){return m_bShowUnderline = bShow;}
	inline	UINT				SetHitMessage(UINT ulMsg){return m_ulHitMessage = ulMsg;}
	inline BOOL			ShowBorder(BOOL bShow=TRUE){return m_bShowBorder = bShow;}
	inline int					SetAlignment(int nAlignment){m_nHAlignment = nAlignment&0x3; m_nVAlignment = nAlignment&0xc; return nAlignment;}
	inline						CStringW const& GetText(){return m_strWndText;}
	inline						HWND SetParent(HWND hWnd){return m_hWndParent = hWnd;}

	BOOL		SetFontHeight(int nHeight);
	BOOL		SetFontName(CStringW const& strFontName);
	COLORREF SetTextColor(COLORREF color);
	COLORREF SetBkColor(COLORREF color);
	COLORREF SetBorderColor(COLORREF color);

	int SetText(CStringW const& str);	// to be used when m_bSenseMouse is FALSE
	int AddSensibleText(CSensibleInfo *pInfo);	// to be used when m_bSenseMouse is TRUE
	int AddPopupMenuItem(UINT wID, HICON hIcon, CStringW strTxt);
	BOOL SearchSensibleArea(const CPoint &pt/* in client coordinates */, CStringW &str);	// determine if a point is with in a sensible area

public:
	LRESULT OnCreate(WPARAM wParam, LPARAM lParam);
	LRESULT OnMouseMove(WPARAM wParam, LPARAM lParam);
	LRESULT OnMouseLeave(WPARAM wParam, LPARAM lParam);
	LRESULT OnEraseBkGnd(WPARAM wParam, LPARAM lParam);
	LRESULT OnPaint(WPARAM wParam, LPARAM lParam);
	LRESULT OnLeftClick(WPARAM wParam, LPARAM lParam);
	LRESULT OnRightClick(WPARAM wParam, LPARAM lParam);
	LRESULT OnPosChanged(WPARAM wParam, LPARAM lParam);
	LRESULT OnSize(WPARAM wParam, LPARAM lParam);
	LRESULT OnCommand(WPARAM wParam, LPARAM lParam);
	LRESULT OnDelMenuDC();

	int DrawMenuBkGnd();
	int OnDrawMenuItem(LPARAM lParam);
	int OnMeasureMenuItem(LPARAM lParam);
	int OnMsgRcvPopWnd(WPARAM wParam);

public:
	static int const		POPUP_FONT_HEIGHT;
	static int const		DEFAULT_BORDER_WIDTH = 1;
	static int const		DEFAULT_X_MARGIN = 4;
	static int const		DEFAULT_Y_MARGIN = 4;
	static int const		DEFAULT_UNDERLINE_WIDTH = 1;
	static int const		LINE_GAP_ADJUST_FACTOR = 10;
	static int const		POPMENU_MARGIN_WIDTH = 3;
	static int const		MAX_LEN_MENUITEM = 255;

	static enum
	{
		// only horizon-left & horizon-center is supported
		HLEFT = 0x1,
		HCENTER = 0x2,

		// only vertical-top & vertical-center is supported
		VTOP = 0x4,
		VCENTER = 0x8
	};

protected:
	int ClearStringInfo();
	int CheckFirstChar(CStringW const& strTxtLine);
	int Caculate1stLineTop();
	int OutputString(HDC hdc, const CSensibleInfo *pInfo);
	int SearchMenuShortcutChar(const CStringW& strItem);
	int DrawPlainText();
	int DrawSensibleText();
	int SplitText(const CStringW& str, list<CStringW *>& partList);
	int TrimString(int nWidth, CStringW& strTrimed);

	// ONLY ONE of these is needed at any time(no submenu)
	static HDC			_LabelEx_hTmpDC;
	static HANDLE		_LabelEx_hMenuBitmap;
	static HFONT		_LabelEx_hfontMenu;
	static HHOOK		_LabelEx_hCbtHook;
	static HWND		_LabelEx_hWndMenuOwner/* the label itself */, _LabelEx_hWndPopMenu;

	static LRESULT CALLBACK _LabelEx_cbt_HookProc(int code, WPARAM wParam, LPARAM lParam);

protected:
	int		m_nFontHeight, m_nBorderWidth;
	int		m_nXmargin, m_nYmargin;
	int		m_nMinWidth, m_nMinHeight;
	int		m_nHAlignment, m_nVAlignment;
	int		m_nAvailableWidth, m_nAvailableHeight;
	BOOL		m_bShowUnderline, m_bShowBorder;
	UINT	m_ulHitMessage;
	COLORREF	m_clrBorderColor, m_clrUnderlineColor;
	COLORREF	m_clrBackgroundColor, m_clrPlainColor, m_clrHoveredColor;
	HBRUSH	m_hBrushBkgnd, m_hBrushBorder, m_hBrushPopMenu;
	HDC			m_hMemDC;
	HANDLE		m_hMemBitmap;
	LOGBRUSH		m_logBrush;
	HCURSOR	m_hCursor;
	HMENU		m_hPopMenu;		// for popup menu

	CRect			m_rcWnd;
	HFONT			m_hfontLabel;
	HPEN				m_hpenUnderline, m_hpenBorder, m_hpenSelItem;
	CStringW		m_strWndText;
	CStringW		m_strFontName;
	CSensibleInfo		*m_pPrevHoverd;
	list<CSensibleInfo*>		m_listStringInfo;
	list<CStringW*>				m_listSplitText;

	static	int	 const ICON_WIDTH;
	static COLORREF const DEFAULT_BORDER_COLOR = RGB(80,80,80);
	static COLORREF const DEFAULT_HOVER_COLOR = RGB(70,100,0);
	static COLORREF const DEFAULT_UNDERLINE_COLOR = RGB(0,100,100);
	static COLORREF const PLAIN_COLOR = RGB(0, 0, 0);
	static COLORREF const MEMU_FRM_COLOR = RGB(0, 45, 150);
	static const WCHAR	DEFAULT_FONT_NAME[];
	static const WCHAR	ONE_CHAR[];

};	//CLabelEx

#endif	// __LABELEX__
