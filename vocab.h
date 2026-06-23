

#ifndef __cplusplus
	#error requires C++ compilation
#endif

#ifndef __VOCAB_APP__
#define __VOCAB_APP__

//#define _CRTDBG_MAP_ALLOC

////////////////////////////////////////////////////////////////////////////////////////////

#pragma once

////////////////////////////////////////////////////////////////////////////////////////////

#include <atlstr.h>
#include <atltypes.h>
#include <vector>
#include "windows.h"
#include "vocabConst.h"
#include "resource.h"

////////////////////////////////////////////////////////////////////////////////////////////

using namespace ATL;

////////////////////////////////////////////////////////////////////////////////////////////
//
namespace VocabApp
{
	class CVocabApp : public IUnknown
	{
	public:
		CVocabApp();
		~CVocabApp();

	public:
		static WCHAR const DEF_FONT_NAME[];
		static WCHAR const FONT_NAME_KAITI[];
		static WCHAR const FONT_NAME_TAHOMA[];
		static WCHAR const FONT_NAME_MSSANS[];
		static int const		FONT_HEIGHT_ADJ = 30;
	};

	struct vocabGlobal
	{
	public:
		HINSTANCE	g_hInstance;
		BOOL				g_bIsProperFontFound;
		HFONT			g_hDefEngFont;
		HFONT			g_hDefChnFont;

		HHOOK			g_hHookMouse;
		HHOOK			g_hHookCbt;
		HHOOK			g_hHookKeyboard;
		HHOOK			g_hHookFileDlg;

		HWND			g_hWndLButtonDown;
		HWND			g_hWndFileDlg;

		BOOL				g_bIsLBtnDown;
		CPoint			g_ptLBtnDown;
		CPoint			g_ptRButtonUp;
		DWORD			g_dwTickCnt;
	};
}

#endif	// __VOCAB_APP__
