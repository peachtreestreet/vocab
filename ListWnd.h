
#ifndef		__LISTWND__
#define		__LISTWND__

//////////////////////////////////////////////////////////////////////////////////////////////////
#pragma once
//////////////////////////////////////////////////////////////////////////////////////////////////

#include "vocab.h"
#include <vector>
#include <list>
#include "wnd0.h"

//////////////////////////////////////////////////////////////////////////////////////////////////

using namespace std;

//////////////////////////////////////////////////////////////////////////////////////////////////


typedef struct tag_listItemData
{
	HICON		hIcon;
	HRGN		hRgn;
	CStringW	*pStr;
}LISTITEMDATA, *PLISTITEMDATA;

typedef struct tag_scrollCtrlArea
{
	DWORD dwHitCode;
	CRect	*prcArea;
}SCROLLCTRLAREA, *PSCROLLCTRLAREA;

class CListWnd : public CWnd0
{
	friend class CFlatDropList;

public:
	CListWnd(void);
	virtual ~CListWnd(void);
	virtual LRESULT WndProc(UINT uMsg, WPARAM wParam, LPARAM lParam);

	UINT Clear();
	UINT SetCurSel(UINT uIndex);
	int AddItem(CStringW const& str, HICON hIcon=0);
	int SetFontHeight(int dwHeight/* in mm */);
	int SetFontName(CStringW const& strName);
	inline UINT SetHeight(UINT uHeight){return m_uMaxWndHeight = uHeight;}
	inline UINT GetCurSel() const{return m_uSelectedItem;}
	inline HWND SetComboWnd(HWND hComboWnd){return m_hComboWnd = hComboWnd;}
	inline UINT GetItemCount() const {return m_vecContent.size();}

private:
	LRESULT OnMsgNcCreate(WPARAM wParam, LPARAM lParam);
	LRESULT OnMsgNcDestroy(WPARAM wParam, LPARAM lParam);
	LRESULT OnMsgNcHittest(WPARAM wParam, LPARAM lParam);
	LRESULT OnMsgPaint(WPARAM wParam, LPARAM lParam);
	LRESULT OnMsgLButtonUp(WPARAM wParam, LPARAM lParam);
	LRESULT OnMsgLButtonDown(WPARAM wParam, LPARAM lParam);
	LRESULT OnMsgMouseMove(WPARAM wParam, LPARAM lParam);
	LRESULT OnMsgMouseLeave(WPARAM wParam, LPARAM lParam);

	int AdjustPosition();
	int Show(BOOL bShow = TRUE);

	// will return TRUE if it has been redrawed
	BOOL DrawScrollbar(UINT uState);

	UINT GetDispIndexFromPoint(CPoint const& pt);
	UINT TurntoIntegerMultiple(CRect &rcWnd, BOOL bShownAbove = FALSE);

	int SingleScroll(DWORD dwHitCode);
	int DoScroll(DWORD dwHitCode);
	int UpdateScrollCtrlArea(CRect const& rcWnd);
	int DrawTopArrow(HBRUSH hBrush);
	int DrawBottomArrow(HBRUSH hBrush);
	int DragSlider();
	USHORT HitTest(CPoint const& pt);

protected:
	HWND					m_hComboWnd;
	BOOL						m_bIconExists;
	BOOL						m_bShowVScroll;
	BOOL						m_bHandleLBUp;
	BOOL						m_bDragging;

	// all items have the same size
	UINT						m_uItemHeight;

	// max item count,default value is 65535
	UINT						m_uMaxItemCnt;

	// the size of the list window can not be determined until it has been displayed
	// this is the max width of string of all items
	UINT						m_uTextWidth;

	// if this member is set,the height of list window will not exceed it
	UINT						m_uMaxWndHeight;

	// count of item which has been displayed,can not be less than 1
	UINT						m_uShownCnt;

	// for painting use
	UINT						m_uDispWidth,m_uDispHeight;	

	// index(0 to m_vecContent.size()) of previous hovered item(if exists)
	UINT						m_uPrevHovered;

	// to record the index (in m_vecContent,0 to m_vecContent.size()) of the top item
	// the initial value is 0
	UINT						m_uTopItemIndex;

	// record current selected item index,0xffffffff means invalid value
	UINT						m_uSelectedItem;

	int							m_nFontPoints;
	HDC						m_hMemDC;
	HANDLE					m_hMemBitmap;
	HBRUSH				m_hbrWndFrame, m_hbrSelectedItem, m_hbrWhite;
	HBRUSH				m_hbrScrollbarBG, m_hbrThumbHovered, m_hbrThumbLBD;
	HBRUSH				m_hbrSlider, m_hbrSliderHoverd;
	HPEN						m_hWhitePen, m_hBlackPen, m_hGrayPen;
	HFONT					m_hFont;

	// for scroll area use
	DWORD								m_dwLastScrollingTime;
	UINT									m_uScrollingDistance;
	USHORT							m_usPrevScrollState;
	CRect								m_rcScroll, m_rcSlider;
	list<PSCROLLCTRLAREA>		m_listScrollArea;

	// item data,each one is corresponding to an item
	vector<PLISTITEMDATA>	m_vecContent;

protected:
	static int SCROLLBAR_WIDTH;
	static int SCROLLBAR_HEIGHT;
	static int ICON_MARGIN;
	static int ICON_HEIGHT;
	static const UINT LEFT_MARGIN = 2;
	static const UINT TOP_MARGIN = 2;
	static const UINT GAP = 1;
	static const UINT TWO = 2;
	static const UINT SCROLLBAR_MARGIN = 1;
	static const UINT BORDER_WIDTH = 1;
	static const UINT DEF_MAX_ITEM_CNT = (2<<16) - 1;
	static const UINT THUMB_WIDTH = 18;
	static const UINT THUMB_HEIGHT = 18;
	static const UINT SLIDER_WIDTH = 14;
	static const UINT ARROW_WIDTH = 7;
	static const UINT ARROW_HEIGHT = 4;
	static const UINT ARROW_MARGIN = 5;
	static const LONG MIN_DRAG_OFFSET = 2;
	static const UINT DEF_FONT_HEIGHT = 10;
	static const UINT TIMER1_ELAPSE = 500;		// interval from LButton down to continuously scrolling(keep pressed)
	static const UINT TIMER2_ELAPSE = 33;		// interval between two scolling
	static const SHORT VK_STATE_LBTNDOWN = 0x8000;
	static const USHORT MAX_OFFSET_X = 200;

	static enum
	{
		COLOR_WHITE = RGB(255,255,255),
		CLR_FRAME = RGB(0,45,150),
		CLR_SELECTEDFRM = RGB(51,153,255),
		CLR_SCROLL_BG = RGB(241,241,241),
		CLR_THUMB_HOVERED = RGB(210,210,210),
		CLR_THUMB_LBD = RGB(120,120,120),
		CLR_SLIDER = RGB(192,192,192),
		CLR_SLIDER_HOVERED = RGB(168,168,168),
		CLR_ARROW = RGB(96,96,96)
	};
	
	static const WCHAR	DEF_FONT_NAME[];
};

#endif	// __LISTWND__
