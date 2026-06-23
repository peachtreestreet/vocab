
#ifndef __VOCABCONST__
#define __VOCABCONST__

//////////////////////////////////////////////////////////////////////////////////////////////////

#pragma once

//////////////////////////////////////////////////////////////////////////////////////////////////

class CVocabConst
{
public:
	static const UINT TWO = 2;
	static const int BYTES_OF_FILENAME = 4;
	static const int MAX_WORD_COUNT = 30000;
	static const int COUNT_PER_GROUP = 10;
	static const int MAX_TXT_FILE_LENGTH = (2<<20) - 1;				// 1M bytes max for the word source (.txt) file
	static const int MAX_RECORD_FILE_LENGTH = 2<<10;			// 1024 bytes max for the record file
	static const int COLUMN_CNT = 6;											// column count in word table

	static const int MAX_NTEXT_LEN = (2<<12) - 1;						// the max number of bytes is 4095(for column of spell,explanation and related)
	static const int MAX_SPELL_LEN = 2<<8;								// max length of a word

	static const int MAX_PROVIDER_CNT = 63;
	static const int MAX_REG_VALUE_LEN = (2<<10) - 1;
	static const int FILE_NAME_LEN = 8;
	static const int EXT_NAME_LEN = 3;
	static const int TIMER_ELAPSE = 100;					// in ms
	static const int SCHEMA_COLUMN_CNT = 3;			// we need first 3 columns only(TYPE_NAME,DATA_TYPE,COLUMN_SIZE)
	static const int SCHEMA_TYPENAME_LEN = 64;		// provider-specific data type name,an unicode string,32 is enough
	static const int FIXED_LEN_COL_CNT = 2;
	static const int INVALID_INDEX = 0xffffffff;
	static const UINT BENCHMARK_SC_HEIGHT = 1200;

	static const UINT LOWSHORT = 0xffff;
	static const UINT TWOE16 = 1 << 16;
	static const UINT TIME_OUT = 10000;	// force to quit after 10s
	static const COLORREF WHITE = RGB(255, 255, 255);
	static const COLORREF BLACK = RGB(0, 0, 0);
	static const COLORREF COLOR_CAPTION = RGB(238, 238, 242);
	static const COLORREF FRM_COLOR = RGB(0, 0, 190);
	static const COLORREF COLOR1 = RGB(63, 63, 63);
	static const COLORREF COLOR2 = RGB(0, 122, 204);
	static const COLORREF COLOR3 = RGB(0, 64, 128);
	static const COLORREF PINK = RGB(255, 0, 255);
	static const COLORREF MINBOX_HOVER = RGB(229, 229, 229);
	static const COLORREF MINBOX_LBD = RGB(200, 200, 200);
	static const COLORREF CLOSEBOX_HOVER = RGB(232, 20, 35);
	static const COLORREF CLOSEBOX_LBD = RGB(231, 112, 112);

	static const float LABEL_WIDTH_FACTOR;
	static const float LABEL_HEIGHT_FACTOR1;
	static const float LABEL_HEIGHT_FACTOR2;
	static const float LABEL_HEIGHT_FACTOR3;
	static const float MM_PER_INCH;
	static int const X_GAP;
	static int const Y_GAP;

	// column count in the rowset returned by root enumerator(ISourcesRowset::GetSourcesRowset)
	// the predefined(by OLEDB and readonly) count is 5,we need first 1 only(SOURCES_NAME)
	static const int ROOT_ENUMERATOR_COLUMN_CNT = 1;

	// the name or display name of the provider returned by root enumerator
	static const int MAX_LEN_SOURCES_NAME = (2<<8) - 1;

	// column type in the rowset returned by root enumerator,it is predefined by OLEDB and readonly
	static const int SOURCE_COLUMN_TYPE;

	static const int COLUMN_TYPE[];
	static const int COLUMN_SIZE[];

	static const	 WCHAR		ZERO;
	static const	 WCHAR		SLASH;
	static const	 WCHAR		COLON;
	static const WCHAR		COMMA;
	static const WCHAR		LEFT_MID_BRAKET;
	static const WCHAR		RIGHT_MID_BRAKET;
	static const WCHAR		SPACE;
	static const WCHAR		LINEFEED;
	static const WCHAR		CARRIAGE_RET;
	static const WCHAR		DIVISION;
	static const WCHAR		MINUS;
	static const WCHAR		DOT;
	static const WCHAR		ACCELERATION_PREFIX;

	static const WCHAR CLASS_NAME_BTN[];
	static const WCHAR CLASS_NAME_STATIC[];
	static const WCHAR CLASS_NAME_COMBO[];
	static const WCHAR CLASS_NAME_EDIT[];

	static const WCHAR		FILENAME_EXT[][5];
	static const WCHAR		RECORD_ITEM[][15];
	static const	 WCHAR		NAME_MAPPING_OBJ[];
	static const	 WCHAR		PROVIDER_STR_SSCE[];
	static const	 WCHAR		PROVIDER_STR_JET[];
	static const	 WCHAR		PROVIDER_STR_ERR[];
	static const	 WCHAR		PROVIDER_SPEC_PASSWORD_PROP[];
	static const	 WCHAR		EMPTYSTR[];
	static const	 WCHAR		WAITDLG_TITLE[];
	static const	 WCHAR		MSGSTR_SEARCHING[];
	static const	 WCHAR		MSGSTR_READING[];
	static const	 WCHAR		MSGSTR_CREATING[];
	static const	 WCHAR		MSGSTR_WRITING[];
	static const	 WCHAR		MSGSTR_READINGTXT[];
	static const	 WCHAR		REG_SUBKEY_NAME[];
	static const	 WCHAR		REG_VALUE_NAME[][32];
	static const	 WCHAR		ERR_MSG1[];
	static const	 WCHAR		ERR_MSG2[];
	static const	 WCHAR		ERR_MSG3[];
	static const	 WCHAR		ERR_MSG4[];
	static const	 WCHAR		ERR_MSG5[];
	static const	 WCHAR		ERR_MSG6[];

	static WCHAR		NAME_TABLE_WORD[];
	static WCHAR		NAME_INDEX_MAJOR[];
	static WCHAR		*NAMES_COLUMN[];

	// for message communication
	static enum VOCAB_MSG
	{
		MSG_INIT = WM_APP + 0xf00,	// 0x8f00

		// hoverd related word's spell
		MSG_RCV_SPELL,

		// read registry & database
		MSG_PREPARE_DATA,

		// to create new database file
		MSG_CREATE_DATABASE,

		// to open an existing database file
		MSG_SELECT_DATABASE,

		MSG_PREPARE_UNFAMILIAR,

		// data have been read for displaying
		MSG_PREPARATION_COMPLETED,

		MSG_SHOW_ERRMSG,

		MSG_STOP_WORKING,

		MSG_WAITING_FOR_QUIT,

		MSG_SESSION_END,
		MSG_FORCED_QUIT,

		MSG_SHOW_PROGRESS,

		MSG_RCV_FILEDLG,

		MSG_RCV_MENUITEM_WND,

		MSG_ADD_UNFAMILIAR,

		MSG_REMOVE_UNFAMILIAR,

		MSG_CREATE_CHILDWND,

		MSG_CHECK_SPELL,

		MSG_ADD_RELATED,

		MSG_ADD_NEW,

		MSG_DRAW_NEXT_POS,	//8f14

		MSG_SELCHANGED,

		MSG_RCVLISTWNDSTATE,

		MSG_ACCKEY,

		MSG_SPEAK,
		MSG_SEARCH_SPELL,
		MSG_FOUND_EXPLANATION,
		MSG_GROUP_DATA,
		MSG_DISENABLE_CANCEL,
		MSG_CANCELLED,
		MSG_DISGARD_DATA,
		MSG_UPDATE_WAITDLG,
		MSG_DEL_MENUDC,
	};

	static enum
	{
		PROVIDERTYPE_MS_SSCE = 0,
		PROVIDERTYPE_MS_JET = 1,
		PROVIDERTYPE_INVALID = (USHORT)0xffff
	};

	static enum POPUP_ITEM_ID
	{
		ADD_UNFAMILIAR = 0x8e00,
		REMOVE_UNFAMILIAR = 0x8e01,
		ADD_RELATED = 0x8e02,
		REMOVE_RELATED = 0x8e03
	};

	static enum DROPLIST_HITCODE
	{
		UPTHUMB = 0x80,
		DOWNTHUMB = 0x100,
		SLIDER = 0x200,
		TRACK = 0x400
	};

	static enum COMBO_STATE
	{
		PLAIN = 0x800,
		HOVERED = 0x1000,
		LBTNDOWN = 0x2000,
		DISENABLED = 0x4000
	};

	static enum SCROLLBAR_STATE
	{
		UP_HOVERED = UPTHUMB,
		UP_LBD = UPTHUMB + 1,
		DOWN_HOVERED = DOWNTHUMB,
		DOWN_LBD = DOWNTHUMB + 1,
		SLIDER_HOVERED = SLIDER,
		SLIDER_LBD = SLIDER + 1,
		UNHOVERED = TRACK,

		FORCETODRAW = 0x8000
	};

};

#endif // __VOCABCONST__