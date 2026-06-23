
#define _CRT_SECURE_NO_WARNINGS
#pragma warning(disable: 4996)

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#include "vocab.h"
#include <strsafe.h>
#include <wincrypt.h>
#include <time.h>
#include <string>
#include "oledberr.h"
#include "msdaguid.h"
#include "WorkThread.h"
#include "sqloledb.h"
#include "objbase.h"
#include "tools1.h"
#include "waitdlg.h"
#include <crtdbg.h>

////////////////////////////////////////////////////////////////////////////////////////////

#ifdef _DEBUG
#define new new(_NORMAL_BLOCK, __FILE__, __LINE__)
#endif

////////////////////////////////////////////////////////////////////////////////////////////
class CvocabularyDlg;
using namespace VocabApp;

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

extern vocabGlobal globalVar;

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

const WCHAR CVocabConst::FILENAME_EXT[][5] = {L"sdf", L"mdb"};
const WCHAR CVocabConst::RECORD_ITEM[][15] = {L"ProviderType", L"book", L"groupCnt", L"from", L"to"};
const WCHAR CVocabConst::NAME_MAPPING_OBJ[] = L"vocabularrMappingObj";
const WCHAR CVocabConst::PROVIDER_STR_ERR[] = L"error";
const WCHAR CVocabConst::PROVIDER_SPEC_PASSWORD_PROP[] = L"jet oledb:database password";
const WCHAR CVocabConst::EMPTYSTR[] = L"";
const WCHAR CVocabConst::MSGSTR_SEARCHING[] = L"Searching database service...";
const WCHAR CVocabConst::MSGSTR_READING[] = L"Reading database,please wait...";
const WCHAR CVocabConst::MSGSTR_CREATING[] = L"Creating database file...";
const WCHAR CVocabConst::MSGSTR_WRITING[] = L"Writing words to database...";
const WCHAR CVocabConst::MSGSTR_READINGTXT[] = L"Parsing text file,please wait...";
const WCHAR CVocabConst::WAITDLG_TITLE[] = L"wait";
const WCHAR CVocabConst::ERR_MSG1[] = L"An error occured while calling API:ReadFile().";
const WCHAR CVocabConst::ERR_MSG2[] = L"The size of input file is too small or too large(should less than 2048 BYTE).";
const WCHAR CVocabConst::ERR_MSG3[] = L"Can not create or open mapping file.";
const WCHAR CVocabConst::ERR_MSG4[] = L"The contents of record file is incorrect,please recreated it.";
const	WCHAR	CVocabConst::REG_VALUE_NAME[][32] = { L"ProviderGUID", L"DBPathName", L"ProviderType", L"Start Group", L"End Group", L"group count", L"rivise" };
const WCHAR CVocabConst::ZERO = '0';
const WCHAR CVocabConst::SLASH = '\\';
const WCHAR CVocabConst::COLON = ':';
const WCHAR CVocabConst::COMMA = ',';
const WCHAR CVocabConst::LEFT_MID_BRAKET = '[';
const WCHAR CVocabConst::RIGHT_MID_BRAKET = ']';
const WCHAR CVocabConst::DIVISION = '/';
const WCHAR CVocabConst::MINUS = '-';
const WCHAR CVocabConst::DOT = '.';
const WCHAR CVocabConst::SPACE = ' ';
const WCHAR CVocabConst::ACCELERATION_PREFIX = '&';
const WCHAR CVocabConst::LINEFEED = 0xa;
const WCHAR CVocabConst::CARRIAGE_RET = 0xd;

const int CVocabConst::COLUMN_TYPE[]=/* word table */
{
	DBTYPE_I4,			// index
	DBTYPE_I4,			// unfamiliar
	DBTYPE_WSTR,		// spell
	DBTYPE_WSTR,		// explanation
	DBTYPE_WSTR,		// related
	DBTYPE_WSTR		// notes
};

const int CVocabConst::COLUMN_SIZE[COLUMN_CNT]=
{
	sizeof(UINT),	// index
	sizeof(UINT), // unfamiliar(Y/N)
	CVocabConst::MAX_SPELL_LEN * sizeof(WCHAR),
	CVocabConst::MAX_SPELL_LEN * sizeof(WCHAR),
	CVocabConst::MAX_SPELL_LEN * sizeof(WCHAR),	// related
	CVocabConst::MAX_SPELL_LEN * sizeof(WCHAR)	// notes
};
const int CVocabConst::SOURCE_COLUMN_TYPE = DBTYPE_WSTR;

WCHAR CVocabConst::NAME_TABLE_WORD[] = L"words";
WCHAR CVocabConst::NAME_INDEX_MAJOR[] = L"Key";
WCHAR *CVocabConst::NAMES_COLUMN[] ={ L"index", L"unfamiliar", L"spell", L"explanation", L"related" , L"notes"};

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

VocabRecord::VocabRecord()
{
	pstrProviderGUID = new CStringW;
	pstrDBpathname = new CStringW;
	usProviderType = (USHORT)-1;
	uGroupCount = uStartGroup = uEndGroup = 0;
}

VocabRecord::~VocabRecord()
{
	delete pstrProviderGUID;
	delete pstrDBpathname;
}
 
CVocabWorkThread::CVocabWorkThread()
{
	m_hThread = 0;
	m_dwTID = 0;
	m_hCreatorWnd = 0;
	m_uWordCount = 0;
	m_bVEngineAvailable = FALSE;
}

CVocabWorkThread::~CVocabWorkThread()
{
	ClearWordsData();
}

BOOL CVocabWorkThread::ReadRegistry()
{
	HKEY hSubkey;
	LSTATUS lRet;
	DWORD dwType,cbData;
	BYTE pbuff[CVocabConst::MAX_REG_VALUE_LEN];
	WCHAR wszBuff[CVocabConst::MAX_REG_VALUE_LEN];
	CStringW str;


	if(ERROR_FILE_NOT_FOUND == RegOpenKeyEx(HKEY_CURRENT_USER, CVocabConst::REG_SUBKEY_NAME, REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS, &hSubkey))return FALSE;

	::RtlSecureZeroMemory(pbuff, CVocabConst::MAX_REG_VALUE_LEN);
	::RtlSecureZeroMemory(wszBuff, CVocabConst::MAX_REG_VALUE_LEN * sizeof(WCHAR));
	
	dwType = REG_SZ;

	lRet = RegQueryValueEx(hSubkey, CVocabConst::REG_VALUE_NAME[0], 0, &dwType, pbuff, &cbData);
	lRet = RegQueryValueEx(hSubkey, CVocabConst::REG_VALUE_NAME[0], 0, &dwType, pbuff, &cbData);
	if (0 == cbData)return FALSE;
	::RtlCopyMemory(wszBuff, pbuff, sizeof(WCHAR)*cbData);
	*m_Record.pstrProviderGUID = wszBuff;

	lRet = RegQueryValueEx(hSubkey, CVocabConst::REG_VALUE_NAME[1], 0, &dwType, pbuff, &cbData);
	::RtlCopyMemory(wszBuff, pbuff, sizeof(WCHAR)*cbData);
	*m_Record.pstrDBpathname = wszBuff;

	dwType = REG_DWORD;
	lRet = RegQueryValueEx(hSubkey, CVocabConst::REG_VALUE_NAME[2], 0, &dwType, pbuff, &cbData);
	m_Record.usProviderType = *(DWORD *)pbuff;

	lRet = RegQueryValueEx(hSubkey, CVocabConst::REG_VALUE_NAME[3], 0, &dwType, pbuff, &cbData);
	m_Record.uStartGroup = *(DWORD *)pbuff;

	lRet = RegQueryValueEx(hSubkey, CVocabConst::REG_VALUE_NAME[4], 0, &dwType, pbuff, &cbData);
	m_Record.uEndGroup = *(DWORD *)pbuff;

	lRet = RegQueryValueEx(hSubkey, CVocabConst::REG_VALUE_NAME[5], 0, &dwType, pbuff, &cbData);
	m_Record.uGroupCount = *(DWORD *)pbuff;

	lRet = RegQueryValueEx(hSubkey, CVocabConst::REG_VALUE_NAME[6], 0, &dwType, pbuff, &cbData);
	m_Record.bUsingUnfamiliar = *(BOOL *)pbuff;

	RegCloseKey(hSubkey);
	return TRUE;
}

BOOL CVocabWorkThread::WriteRegistry()
{
	HKEY hSubkey;
	LSTATUS lRet;
	DWORD cbData, dwRivise, dwValue, dwDisposition;
	BYTE pbuff[_MAX_PATH];
	CStringW str;
	
	FeedbackMsg(CVocabConst::MSG_UPDATE_WAITDLG, 0, (LPARAM)new CStringW(L"writing registry..."));

	if(ERROR_FILE_NOT_FOUND == RegOpenKeyEx(HKEY_CURRENT_USER, CVocabConst::REG_SUBKEY_NAME, REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS, &hSubkey))
	{
		if(ERROR_SUCCESS != ::RegCreateKeyExW(HKEY_CURRENT_USER, CVocabConst::REG_SUBKEY_NAME, 0, 0, REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS, 0, &hSubkey, &dwDisposition))
		{
			return FALSE;
		}
	}
	cbData = sizeof(WCHAR)*m_Record.pstrProviderGUID->GetLength();
	::RtlSecureZeroMemory(pbuff, _MAX_PATH);
	::RtlCopyMemory(pbuff,m_Record.pstrProviderGUID->GetBuffer(), cbData);
	::RegSetValueExW(hSubkey, CVocabConst::REG_VALUE_NAME[0], 0, REG_SZ, (const BYTE*)pbuff, cbData);

	cbData = sizeof(WCHAR)*m_Record.pstrDBpathname->GetLength();
	::RtlSecureZeroMemory(pbuff,_MAX_PATH);
	::RtlCopyMemory(pbuff,m_Record.pstrDBpathname->GetBuffer(),cbData);
	::RegSetValueExW(hSubkey, CVocabConst::REG_VALUE_NAME[1], 0, REG_SZ, (const BYTE*)pbuff, cbData);

	cbData = sizeof(DWORD);
	::RtlSecureZeroMemory(pbuff,_MAX_PATH);
	dwValue = (DWORD)m_Record.usProviderType;
	::RegSetValueExW(hSubkey, CVocabConst::REG_VALUE_NAME[2], 0, REG_DWORD, (const BYTE*)&dwValue, cbData);

	cbData = sizeof(DWORD);
	::RtlSecureZeroMemory(pbuff,_MAX_PATH);
	dwValue = (DWORD)m_Record.uStartGroup;
	::RegSetValueExW(hSubkey, CVocabConst::REG_VALUE_NAME[3] ,0, REG_DWORD, (const BYTE*)&dwValue, cbData);

	cbData = sizeof(DWORD);
	::RtlSecureZeroMemory(pbuff,_MAX_PATH);
	dwValue = (DWORD)m_Record.uEndGroup;
	::RegSetValueExW(hSubkey, CVocabConst::REG_VALUE_NAME[4], 0, REG_DWORD, (const BYTE*)&dwValue, cbData);

	cbData = sizeof(DWORD);
	::RtlSecureZeroMemory(pbuff,_MAX_PATH);
	dwValue = (DWORD)m_Record.uGroupCount;
	::RegSetValueExW(hSubkey,CVocabConst::REG_VALUE_NAME[5], 0, REG_DWORD, (const BYTE*)&dwValue, cbData);

	cbData = sizeof(DWORD);
	::RtlSecureZeroMemory(pbuff,_MAX_PATH);
	dwValue = (DWORD)m_Record.bUsingUnfamiliar;
	::RegSetValueExW(hSubkey,CVocabConst::REG_VALUE_NAME[6], 0, REG_DWORD, (const BYTE*)&dwValue, cbData);


	RegCloseKey(hSubkey);
	return TRUE;
}

void CVocabWorkThread::OnMsgPrepareData(WPARAM wParam/* ptr to Wait dlg creation event handle */, LPARAM lParam/* ptr to Wait dlg resuming event handle */)
{
	::OutputDebugString(L"++++++++++++  WorkThread::OnMsgPrepareData() ++++++++++++++++++");
	HRESULT hr(0);
	CComPtr<IDBInitialize> pIDBInitialize;
	CComPtr<ISpObjectToken> cpToken;
	CStringW strErrMsg;

	HANDLE *phResumeEvent = reinterpret_cast<HANDLE*>(lParam);
	HANDLE *phEventDlgCreation = reinterpret_cast<HANDLE*>(wParam);

	if (0 != phResumeEvent)
	{
		::OutputDebugString(L"++++++++++++  setevent from WaitDlg ++++++++++++++++++");
		::SetEvent(*phResumeEvent);
	}
	::WaitForSingleObject(*phEventDlgCreation, 1000);
	::CoInitializeEx(0, COINIT_MULTITHREADED);

	FeedbackMsg(CVocabConst::MSG_UPDATE_WAITDLG, (WPARAM)new CStringW(L"preparing"), (LPARAM)new CStringW(CVocabConst::MSGSTR_READING));

	Sleep(500);
	if(FALSE == ReadRegistry())
	{
		FeedbackMsg(CVocabConst::MSG_DISENABLE_CANCEL, 0, 0);
		FeedbackMsg(CVocabConst::MSG_SHOW_ERRMSG, 0, (LPARAM)new CStringW(L"Can not read registry."));
		return;
	}

	if (IsAborting())goto _OnMsgPrepareData_cancelled;

	// no record file exsits,nothing to prepare,just quit
	if(m_Record.pstrProviderGUID->IsEmpty())
	{
		CLSID clsid;
		FeedbackMsg(CVocabConst::MSG_UPDATE_WAITDLG, 0, (LPARAM)new CStringW(CVocabConst::MSGSTR_SEARCHING));
		if (FALSE == Tools1::SearchProviderClassID(clsid, JudgeDatabaseType(*m_Record.pstrDBpathname), *m_Record.pstrProviderGUID))
		{
			strErrMsg = L"Errors occured while searching provider CLASSID,please check the registry or reinstall the provider.";
			goto _OnMsgPrepareData_err_exit;
		}
	}
	FeedbackMsg(CVocabConst::MSG_UPDATE_WAITDLG, 0, (LPARAM)new CStringW(CVocabConst::MSGSTR_READING));

	hr = Tools1::InitDataSource(pIDBInitialize, *m_Record.pstrProviderGUID, *m_Record.pstrDBpathname);
	if(S_OK != hr)goto _OnMsgPrepareData_err_exit;
	if(IsAborting())goto _OnMsgPrepareData_cancelled;

	if(0 < ReadData(pIDBInitialize))
	{
		if(IsAborting())goto _OnMsgPrepareData_cancelled;
	}
	else
	{
		FeedbackMsg(CVocabConst::MSG_SHOW_ERRMSG, 0, (LPARAM)(new CStringW(L"Errors occured when reading database,please recreate the database.")));
		goto _OnMsgPrepareData_err_exit;
	}

	if (0 == m_cpVoice.p)
	{
		hr = m_cpVoice.CoCreateInstance(CLSID_SpVoice);
		if (FAILED(hr)) goto _OnMsgPrepareData_No_Vengine;
	}

	hr = SpFindBestToken(SPCAT_VOICES, L"Language=409", L"VendorPreferred",  &cpToken);
	hr = SpGetDefaultTokenFromCategoryId(SPCAT_VOICES, &cpToken);
	if (SUCCEEDED(hr)) 
	{
		hr = m_cpVoice->SetVoice(cpToken);
		if (SUCCEEDED(hr)) m_bVEngineAvailable = TRUE;
	}
	cpToken.Release();

_OnMsgPrepareData_No_Vengine:
	FeedbackMsg(CVocabConst::MSG_DISENABLE_CANCEL, 0, 0);
	FeedbackMsg(CVocabConst::MSG_PREPARATION_COMPLETED, (WPARAM)m_Record.bUsingUnfamiliar, TRUE);
	return;

_OnMsgPrepareData_cancelled:
_OnMsgPrepareData_err_exit:
	if (FAILED(hr))
	{
		Tools1::LookupErrorMsg(strErrMsg);
	}
	FeedbackMsg(CVocabConst::MSG_SHOW_ERRMSG, 0, (LPARAM)(new CStringW(strErrMsg)));

	OnDisgardData(0, 0);
}

void CVocabWorkThread::OnMsgCreateDatabase(WPARAM wParam/* handle of Wait dlg creation event */, LPARAM lParam/* handle of Wait dlg resuming event */)
{
	HRESULT hr(0);
	CStringW strProviderGUID,strDBName;
	CStringW strPath,strErrMsg;
	CLSID clsid;
	CComPtr<IDBInitialize> pIDBInitialize;
	USHORT usProviderType;

	HANDLE *phResumeEvent = reinterpret_cast<HANDLE*>(lParam);
	HANDLE *phEventDlgCreation = reinterpret_cast<HANDLE*>(wParam);

	if (0 != phResumeEvent)	::SetEvent(*phResumeEvent);
	::WaitForSingleObject(*phEventDlgCreation, 1000);
	FeedbackMsg(CVocabConst::MSG_UPDATE_WAITDLG, 0, (LPARAM)new CStringW(CVocabConst::MSGSTR_READING));

	if(0 != m_Record.pstrDBpathname)
	{ 
		if (0 < m_Record.pstrDBpathname->GetLength())
		{
			m_uWordCount = ReadTxtFile(*m_Record.pstrDBpathname);
		}
	}
	else
	{
		strErrMsg = L"No database pathname is given.";
		goto _OnMsgCreateGroup_cleanup;
	}
	if(IsAborting())goto _OnMsgCreateGroup_cleanup;

	if (0 == m_uWordCount)
	{
		FeedbackMsg(CVocabConst::MSG_SHOW_ERRMSG, 0, (LPARAM)new CStringW(L"No word is in the text file,or the file format is incorrect."));
		return;
	}
	if (CVocabConst::MAX_WORD_COUNT < m_uWordCount)
	{
		FeedbackMsg(CVocabConst::MSG_SHOW_ERRMSG, 0, (LPARAM)new CStringW(L"The word count must be less than 30000."));
		return;
	}

	if(m_Record.pstrProviderGUID->IsEmpty())
	{
		FeedbackMsg(CVocabConst::MSG_UPDATE_WAITDLG, 0, (LPARAM)new CStringW(CVocabConst::MSGSTR_SEARCHING));

		hr = Tools1::SearchProviderClassID(clsid,CVocabConst::PROVIDERTYPE_MS_SSCE/* .sdb file is preferential */,strProviderGUID);
		if(S_OK == hr)
		{
			usProviderType = CVocabConst::PROVIDERTYPE_MS_SSCE;
		}
		else
		{
			hr = Tools1::SearchProviderClassID(clsid,CVocabConst::PROVIDERTYPE_MS_JET,strProviderGUID);
			if(S_OK != hr)goto _OnMsgCreateGroup_cleanup;
			else usProviderType = CVocabConst::PROVIDERTYPE_MS_JET;
		}
	}
	else
	{
		strProviderGUID = *m_Record.pstrProviderGUID;
		usProviderType = m_Record.usProviderType;
	}

	if(IsAborting())goto _OnMsgCreateGroup_cleanup;
	Tools1::GenerateDatabaseFileName(strDBName);

	STARTUPINFOW si;
	::RtlSecureZeroMemory(&si, sizeof(STARTUPINFOW));
	si.cb = sizeof(si);
	::GetStartupInfo(&si);

	strDBName += CVocabConst::DOT;
	strDBName += CVocabConst::FILENAME_EXT[usProviderType];
	strPath = si.lpTitle;
	strPath = strPath.Left(strPath.ReverseFind(CVocabConst::SLASH) + 1);
	strDBName = strPath + strDBName;

	if(strDBName.IsEmpty())goto _OnMsgCreateGroup_cleanup;
	if(IsAborting())goto _OnMsgCreateGroup_cleanup;
	FeedbackMsg(CVocabConst::MSG_UPDATE_WAITDLG, 0, (LPARAM)new CStringW(CVocabConst::MSGSTR_CREATING));

	if(S_OK != Tools1::CreateDataStore(pIDBInitialize,strProviderGUID,strDBName,CVocabConst::EMPTYSTR,usProviderType))goto _OnMsgCreateGroup_cleanup;
	if(IsAborting())goto _OnMsgCreateGroup_cleanup;
	FeedbackMsg(CVocabConst::MSG_UPDATE_WAITDLG, 0, (LPARAM)new CStringW(CVocabConst::MSGSTR_WRITING));
	if(0 == WriteData(pIDBInitialize))goto _OnMsgCreateGroup_cleanup;

	m_Record.pstrProviderGUID->SetString(strProviderGUID);
	m_Record.pstrDBpathname->SetString(strDBName);
	m_Record.usProviderType = usProviderType;
	if(0 == m_uWordCount % CVocabConst::COUNT_PER_GROUP)
	{
		m_Record.uGroupCount = m_uWordCount / CVocabConst::COUNT_PER_GROUP;
	}
	else
	{
		m_Record.uGroupCount = m_uWordCount / CVocabConst::COUNT_PER_GROUP + 1;
	}
	if(1 > m_Record.uGroupCount)m_Record.uGroupCount = 1;
	m_Record.uStartGroup = 0;
	m_Record.uEndGroup = 0;
	if(IsAborting())goto _OnMsgCreateGroup_cleanup;

	// use new created database as record
	if (FALSE == WriteRegistry())
	{
		strErrMsg = L"Can not write registry,the book info will not be stored.";
	}
	ClearWordsData();
	PostThreadMsg(CVocabConst::MSG_PREPARE_DATA, wParam, lParam);

	return;

_OnMsgCreateGroup_cleanup:
	if (S_OK != hr)
	{
		Tools1::LookupErrorMsg(strPath);
		FeedbackMsg(CVocabConst::MSG_SHOW_ERRMSG, 0, (LPARAM)new CStringW(strPath));
	}
	else 	FeedbackMsg(CVocabConst::MSG_SHOW_ERRMSG, 0, 0);
}

UINT CVocabWorkThread::ReadTxtFile(CStringW const& strFileName)
{
	HANDLE hTxtFile;
	DWORD dwFileSizeL,dwFileSizeH;
	BYTE *pvBuffer(0);
	CStringW strErrMsg;

	hTxtFile = ::CreateFile(strFileName,GENERIC_READ,FILE_SHARE_READ,NULL,OPEN_EXISTING,FILE_ATTRIBUTE_NORMAL,0);
	if(INVALID_HANDLE_VALUE == hTxtFile)
	{
		FeedbackMsg(CVocabConst::MSG_SHOW_ERRMSG, 0, (LPARAM)(new CStringW(L"Can not open the text file,try again.")));
		return 0;
	}

	dwFileSizeL = ::GetFileSize(hTxtFile,&dwFileSizeH);
	if(0<dwFileSizeL && CVocabConst::MAX_TXT_FILE_LENGTH > dwFileSizeL)
	{
		pvBuffer = new BYTE[dwFileSizeL];
		::ReadFile(hTxtFile,pvBuffer,dwFileSizeL,&dwFileSizeH,NULL);
	}
	else
	{
		FeedbackMsg(CVocabConst::MSG_SHOW_ERRMSG, 0, (LPARAM)(new CStringW(L"CThe text file size must be less than 1MB.")));
		return 0;
	}
	::CloseHandle(hTxtFile);

	int nStartPos;
	CStringA *pstrAnsiString;				// explicit ASCII string
	list<CStringA*> pstrAllTxtLine;

	// first,split whole text to text line
	nStartPos = 0;
	for(DWORD i=0;i<dwFileSizeL;i++)
	{
		if(CVocabConst::CARRIAGE_RET == pvBuffer[i])
		{
			pstrAnsiString = new CStringA;
			for(DWORD j=nStartPos;j<i;j++)
			{
				pstrAnsiString->AppendChar(static_cast<char>(pvBuffer[j]));
			}
			if(pstrAnsiString->GetLength() > 0)
			{
				pstrAllTxtLine.push_back(pstrAnsiString);
			}
			else
			{
				delete pstrAnsiString;		// empty string,will not be added to string list
			}
			nStartPos = i+2;
		}
	}
	pstrAnsiString = new CStringA;
	for(DWORD j=nStartPos;j<dwFileSizeL;j++)
	{
		pstrAnsiString->AppendChar(static_cast<char>(pvBuffer[j]));
	}
	if(pstrAnsiString->GetLength() > 0)
	{
		pstrAllTxtLine.push_back(pstrAnsiString);
	}
	else
	{
		delete pstrAnsiString;
	}
	if(0 != pvBuffer)delete[] pvBuffer;

	// be careful for memory leaking
	if(m_strsSpell.size() > 0)
	{
		int nSize = m_strsSpell.size();
		for(int i=0;i<nSize;i++)delete m_strsSpell[i];
		m_strsSpell.clear();
	}
	if(m_strsExplanation.size() > 0)
	{
		int nSize = m_strsExplanation.size();
		for(int i=0;i<nSize;i++)delete m_strsExplanation[i];
		m_strsExplanation.clear();
	}

	CStringW strExplaination,strW;

	strExplaination.Empty();	
	while(pstrAllTxtLine.size() > 0)
	{
		pstrAnsiString = pstrAllTxtLine.front();
		strW = *pstrAnsiString;
		if(0 == CheckStringType(strW))
		{
			// it is spell,just add it to the list			
			m_strsSpell.push_back(new CStringW(strW));
			strW.Empty();

			if(!strExplaination.IsEmpty())
			{
				m_strsExplanation.push_back(new CStringW(strExplaination));
				strExplaination.Empty();
			}
		}
		else
		{
			strExplaination += *pstrAnsiString;
		}
		delete pstrAnsiString;
		pstrAllTxtLine.pop_front();
	}
	if(!strExplaination.IsEmpty())m_strsExplanation.push_back(new CStringW(strExplaination));

	return  m_strsSpell.size();
}

void CVocabWorkThread::OnMsgSelectDatabase(WPARAM wParam/* ptr to Wait dlg creation event handle */, LPARAM lParam/* ptr to Wait dlg resuming event handle */)
{
	int nCount(0);
	CLSID clsid;
	HRESULT hr;
	USHORT usProviderType;
	CStringW strProviderGUID;
	CComPtr<IDBInitialize>			pIDBInitialize;

	hr = S_OK;
	HANDLE *phResumeEvent = reinterpret_cast<HANDLE*>(lParam);
	HANDLE *phEventDlgCreation = reinterpret_cast<HANDLE*>(wParam);

	if (0 != phResumeEvent)	::SetEvent(*phResumeEvent);
	DWORD dw0 = ::WaitForSingleObject(*phEventDlgCreation, 1000);

	if (0 == m_Record.pstrDBpathname)
	{
		m_strErrMsg = L"No database pathname is given.";
		goto _OnMsgSelectDatabase_exit;
	}
	else
	{
		if(! m_Record.pstrDBpathname->IsEmpty())
			usProviderType = JudgeDatabaseType(*m_Record.pstrDBpathname);
		else
		{
			m_strErrMsg = L"No database pathname is given.";
			goto _OnMsgSelectDatabase_exit;
		}
	}
	
	if(usProviderType != m_Record.usProviderType)
	{
		FeedbackMsg(CVocabConst::MSG_UPDATE_WAITDLG, 0, (LPARAM)new CStringW(CVocabConst::MSGSTR_SEARCHING));
		if(TRUE == Tools1::SearchProviderClassID(clsid,usProviderType,strProviderGUID))
		{
			m_Record.pstrProviderGUID->SetString(strProviderGUID);
			m_Record.usProviderType = usProviderType;
		}
		else
		{
			m_strErrMsg = L"Can not find database service provider.";
			goto _OnMsgSelectDatabase_exit;
		}
	}
	else
	{
		strProviderGUID = *m_Record.pstrProviderGUID;
	}
	if(IsAborting())goto _OnMsgSelectDatabase_exit;

	FeedbackMsg(CVocabConst::MSG_UPDATE_WAITDLG, 0, (LPARAM)new CStringW(CVocabConst::MSGSTR_READING));
	hr = Tools1::InitDataSource(pIDBInitialize, strProviderGUID, *m_Record.pstrDBpathname);
	if(S_OK != hr)goto _OnMsgSelectDatabase_exit;
	if(IsAborting())goto _OnMsgSelectDatabase_exit;
	
	nCount = ReadData(pIDBInitialize);
	if(0 < nCount)
	{
		if(0 < (nCount % CVocabConst::COUNT_PER_GROUP))
			m_Record.uGroupCount = nCount / CVocabConst::COUNT_PER_GROUP + 1;
		else m_Record.uGroupCount = nCount / CVocabConst::COUNT_PER_GROUP;

		m_Record.uStartGroup = m_Record.uEndGroup = 0;
		m_uWordCount = nCount;
		FeedbackMsg(CVocabConst::MSG_PREPARATION_COMPLETED, 0, 0);

		if (FALSE == WriteRegistry())
		{
			m_strErrMsg = L"Errors occured while writing registry.";
			goto _OnMsgSelectDatabase_exit;
		}
	}
	return;

_OnMsgSelectDatabase_exit:
	if (S_OK != hr)
	{
		Tools1::LookupErrorMsg(m_strErrMsg);
	}
	FeedbackMsg(CVocabConst::MSG_SHOW_ERRMSG, 0, (LPARAM)(new CStringW(m_strErrMsg)));
}

USHORT CVocabWorkThread::JudgeDatabaseType(CStringW strFileName)
{
	USHORT usType(-1);

	if(strFileName.GetLength() <= CVocabConst::EXT_NAME_LEN)return usType;
	if(strFileName.Right(CVocabConst::EXT_NAME_LEN) == CVocabConst::FILENAME_EXT[0])usType = CVocabConst::PROVIDERTYPE_MS_SSCE;
	if(strFileName.Right(CVocabConst::EXT_NAME_LEN) == CVocabConst::FILENAME_EXT[1])usType = CVocabConst::PROVIDERTYPE_MS_JET;

	return usType;
}

UINT CVocabWorkThread::ClearWordsData()
{
	UINT nCount;
	map <UINT,CStringW*> :: const_iterator iter,endIter;

	m_uWordCount = 0;
	nCount = m_strsSpell.size();
	for(UINT i=0;i<nCount;i++)delete m_strsSpell[i];
	m_strsSpell.clear();

	nCount = m_strsExplanation.size();
	for(UINT i=0;i<nCount;i++)delete m_strsExplanation[i];
	m_strsExplanation.clear();

	if(0 < m_mapRelatedIndexString.size())
	{
		iter = m_mapRelatedIndexString.begin();
		endIter = m_mapRelatedIndexString.end();
		while(endIter != iter)
		{
			delete iter->second;
			iter++;
		}
		m_mapRelatedIndexString.clear();
	}
	if(0 < m_mapNotes.size())
	{
		iter = m_mapNotes.begin();
		endIter = m_mapNotes.end();
		while(endIter != iter)
		{
			delete iter->second;
			iter++;
		}
		m_mapNotes.clear();
	}

	return nCount;
}

USHORT CVocabWorkThread::CheckStringType(CStringW const& str)
{
	int nLen = str.GetLength();
	int i;

	/* 0:spell, 1:explanation */

	if(str.Find(CVocabConst::DOT) > 0)return 1;
	for(i=0;i<nLen;i++)
	{
		if(0xff < str[i])
		{
			break;
		}
	}
	if(i < nLen)return 1;

	return 0;
}

UINT CVocabWorkThread::ReadData(CComPtr<IDBInitialize>	pIDBInitialize)
{
	CComPtr<IRowsetIndex>			pIIndex;
	CComPtr<IRowset>					pIRowset;
	CComPtr<IAccessor>				pIAccessor;
	HRESULT hr;
	CComPtr<IMalloc>		pIMalloc;
	CStringW str, strErrMsg;
	pair<UINT, CStringW*> relatedPair;

	BOOL  bIsUnfamiliar;
	pair<UINT,CStringW*> wordIndex;
	HROW	*phRows(0);
	BYTE *pRowData(0);
	DBBINDING *prgBindings(0);
	UINT	ulValueOffset(0);

	if (S_OK != ::CoGetMalloc(MEMCTX_TASK, &pIMalloc))
	{
		strErrMsg = L"Can not allocat memory while reading database.";
		goto _ReadData_cleanup;
	}

	hr = Tools1::GetIIndexPtr(pIDBInitialize, CVocabConst::NAME_TABLE_WORD, CVocabConst::NAME_INDEX_MAJOR, pIIndex);
	if(FAILED(hr))goto _ReadData_cleanup;
	hr = pIIndex->QueryInterface(__uuidof(IAccessor), (void **)&pIAccessor);
	if(FAILED(hr))goto _ReadData_cleanup;
	hr = pIIndex->QueryInterface(__uuidof(IRowset), (void **)&pIRowset);
	if(FAILED(hr))goto _ReadData_cleanup;

	prgBindings = (DBBINDING *)pIMalloc->Alloc(sizeof(DBBINDING) * CVocabConst::COLUMN_CNT);
	if(0 == prgBindings)goto _ReadData_cleanup;

	for(int i=0;i<CVocabConst::COLUMN_CNT;i++)
	{
		prgBindings[i].iOrdinal = i+1;
		prgBindings[i].obValue = ulValueOffset;
		prgBindings[i].obLength = 0;
		prgBindings[i].obStatus = 0;
		prgBindings[i].pTypeInfo = 0;
		prgBindings[i].pObject = 0;
		prgBindings[i].pBindExt = 0;
		prgBindings[i].dwPart = DBPART_VALUE;
		prgBindings[i].dwMemOwner = DBMEMOWNER_CLIENTOWNED;
		prgBindings[i].eParamIO = DBPARAMIO_NOTPARAM;
		prgBindings[i].cbMaxLen = CVocabConst::COLUMN_SIZE[i];
		prgBindings[i].dwFlags = 0;
		prgBindings[i].wType = CVocabConst::COLUMN_TYPE[i];
		prgBindings[i].bPrecision = 0;
		prgBindings[i].bScale = 0;

		ulValueOffset += CVocabConst::COLUMN_SIZE[i];
	}

	HACCESSOR hAcc;
	DBCOUNTITEM ulRowsObtained(0);
	UINT ulTemp, ulBuffSize(0);
	UINT uWordCnt(0);
	WCHAR *pwstr;

	hr = pIAccessor->CreateAccessor(DBACCESSOR_ROWDATA, CVocabConst::COLUMN_CNT, prgBindings, 0, &hAcc, 0);
	if(FAILED(hr))goto _ReadData_cleanup;

	ulBuffSize = 0;
	for(int i=0;i<CVocabConst::COLUMN_CNT;i++)ulBuffSize += CVocabConst::COLUMN_SIZE[i];
	pRowData = (BYTE*)pIMalloc->Alloc(ulBuffSize);
	if(0 == pRowData)goto _ReadData_cleanup;
	::RtlSecureZeroMemory(pRowData, ulBuffSize);

	//
	// set CVocabConst::MAX_WORD_COUNT(will excess to the row amount) to seek a row,and then GetData() to get data of the row
	// the index of this row is the amount of words in this table
	//
	pIRowset->RestartPosition(0);
	
	ulTemp = CVocabConst::MAX_WORD_COUNT;
	::RtlCopyMemory(pRowData, &ulTemp, sizeof(UINT));
	hr = pIIndex->Seek(hAcc, 1, pRowData, DBSEEK_BEFORE);
	if(FAILED(hr))goto _ReadData_cleanup;
	hr = pIRowset->GetNextRows(NULL, 0, 1, &ulRowsObtained, &phRows);
	if(FAILED(hr))goto _ReadData_cleanup;

	hr = pIRowset->GetData(*phRows, hAcc, (void*)pRowData);
	pIRowset->ReleaseRows(1, phRows, 0, 0, 0);
	if(FAILED(hr))goto _ReadData_cleanup;

	ClearWordsData();
	uWordCnt = *(reinterpret_cast<UINT *>(pRowData)) + 1;
	if(0 < uWordCnt && CVocabConst::MAX_WORD_COUNT)
	{
		m_uWordCount = uWordCnt;
	}
	else goto _ReadData_cleanup;

	pIRowset->RestartPosition(0);

	for(int i=0;i<uWordCnt;i++)
	{
		ulValueOffset = 0;

		// column 0:index(started from 0), column 1:spell, column 2:explanation, column 3:related ,column 4:notes
		hr = pIRowset->GetNextRows(NULL, 0, 1, &ulRowsObtained, &phRows);
		if(FAILED(hr))break;
		hr = pIRowset->GetData(*phRows, hAcc, (void*)pRowData);
		if(FAILED(hr))break;

		// column 1:UNFAMILIAR(value is 0 or 1)
		ulValueOffset += sizeof(UINT);
		bIsUnfamiliar = *(reinterpret_cast<UINT *>(pRowData + ulValueOffset));
		if (TRUE == bIsUnfamiliar)m_uVecUnfamilarIndex.push_back((UINT)i);

		// spell
		ulValueOffset += sizeof(UINT);
		str = reinterpret_cast<WCHAR *>(pRowData + ulValueOffset);
		str.Replace(CVocabConst::LINEFEED, CVocabConst::SPACE);
		if(0 < str.Find(CVocabConst::LEFT_MID_BRAKET))
		{
			str = str.Left(str.Find(CVocabConst::LEFT_MID_BRAKET));
		}

		// get rid of all space at detail of the string
		while (CVocabConst::SPACE == str[str.GetLength() - 1])str = str.Left(str.GetLength() - 1);
		m_strsSpell.push_back(new CStringW(str));

		// explanation
		ulValueOffset += CVocabConst::MAX_SPELL_LEN * sizeof(WCHAR);
		str = reinterpret_cast<WCHAR *>(pRowData + ulValueOffset);
		m_strsExplanation.push_back(new CStringW(str));

		// related
		ulValueOffset += CVocabConst::MAX_SPELL_LEN * sizeof(WCHAR);
		str = reinterpret_cast<WCHAR *>(pRowData + ulValueOffset);
		if (FALSE == str.IsEmpty())
		{
			relatedPair.first = i;
			relatedPair.second = new CStringW(str);
			m_mapRelatedIndexString.insert(relatedPair);
		}

		// notes
		ulValueOffset += CVocabConst::MAX_SPELL_LEN * sizeof(WCHAR);
		str = reinterpret_cast<WCHAR *>(pRowData + ulValueOffset);
		if (FALSE == str.IsEmpty())
		{
			relatedPair.second = new CStringW(str);
			m_mapNotes.insert(relatedPair);
		}

		pIRowset->ReleaseRows(1, phRows, 0, 0, 0);
	}

_ReadData_cleanup:
	pIIndex.Release();
	pIAccessor->ReleaseAccessor(hAcc, NULL);
	pIRowset.Release();
	pIAccessor.Release();

	if(0 != pRowData)pIMalloc->Free(pRowData);
	if(0 != prgBindings)pIMalloc->Free(prgBindings);

	return uWordCnt;
}

UINT CVocabWorkThread::WriteData(CComPtr<IDBInitialize>	pIDBInitialize)
{
	CComPtr<IRowsetChange>		pIRowsetChange;
	CComPtr<IAccessor>				pIAccessor;
	CComPtr<IRowset>					pIRowset;
	CComPtr<IMalloc>		pIMalloc;
	CStringW spell, explanation, strErrMsg;
	HRESULT hr;
	BYTE *pRowData(0);

	::CoGetMalloc(MEMCTX_TASK,&pIMalloc);

	DBBINDING *prgBindings(0);
	UINT	ulValueOffset(0);

	hr = Tools1::GetIRowChangePtr(pIDBInitialize, CVocabConst::NAME_TABLE_WORD, pIRowsetChange);
	if(FAILED(hr))goto _WriteData_cleanup;
	hr = pIRowsetChange->QueryInterface(__uuidof(IRowset), (void **)&pIRowset);
	if(FAILED(hr))goto _WriteData_cleanup;
	hr = pIRowset->QueryInterface(__uuidof(IAccessor), (void **)&pIAccessor);
	if(FAILED(hr))goto _WriteData_cleanup;

	// read word table
	prgBindings = (DBBINDING *)pIMalloc->Alloc(sizeof(DBBINDING) * CVocabConst::COLUMN_CNT);
	if(0 == prgBindings)goto _WriteData_cleanup;

	for(int i=0;i<CVocabConst::COLUMN_CNT;i++)
	{
		prgBindings[i].iOrdinal = i+1;
		prgBindings[i].obValue = ulValueOffset;
		prgBindings[i].obLength = 0;
		prgBindings[i].obStatus = 0;
		prgBindings[i].pTypeInfo = 0;
		prgBindings[i].pObject = 0;
		prgBindings[i].pBindExt = 0;
		prgBindings[i].dwPart = DBPART_VALUE;
		prgBindings[i].dwMemOwner = DBMEMOWNER_CLIENTOWNED;
		prgBindings[i].eParamIO = DBPARAMIO_NOTPARAM;
		prgBindings[i].cbMaxLen = CVocabConst::COLUMN_SIZE[i];
		prgBindings[i].dwFlags = 0;
		prgBindings[i].wType = CVocabConst::COLUMN_TYPE[i];
		prgBindings[i].bPrecision = 0;
		prgBindings[i].bScale = 0;

		ulValueOffset += CVocabConst::COLUMN_SIZE[i];
	}

	HACCESSOR hDataAcc, hNullAcc;
	HROW	hRow(0);
	UINT ulBuffSize;
	UINT ulTemp;

	hr = pIAccessor->CreateAccessor(DBACCESSOR_ROWDATA, CVocabConst::COLUMN_CNT, prgBindings, CVocabConst::COLUMN_CNT * sizeof(DBBINDING), &hDataAcc, 0);
	hr = pIAccessor->CreateAccessor(DBACCESSOR_ROWDATA, 0, 0, 0, &hNullAcc, 0);
	if(FAILED(hr))goto _WriteData_cleanup;

	ulBuffSize = 0;
	for(int i=0;i<CVocabConst::COLUMN_CNT;i++)ulBuffSize += CVocabConst::COLUMN_SIZE[i];
	pRowData = (BYTE*)pIMalloc->Alloc(ulBuffSize);
	if(0 == pRowData)goto _WriteData_cleanup;
	::RtlSecureZeroMemory(pRowData, ulBuffSize);
	
	ulTemp = 0;
	for(UINT i=0;i<m_uWordCount;i++)
	{
		ulValueOffset = 0;

		::RtlCopyMemory(pRowData, &i, sizeof(UINT));		// index
		ulValueOffset += sizeof(UINT);
		::RtlCopyMemory(pRowData + ulValueOffset, &ulTemp, sizeof(UINT));		// unfamiliar number

		spell = *m_strsSpell[i];
		explanation = *m_strsExplanation[i];

		ulValueOffset += sizeof(UINT);
		::RtlCopyMemory(pRowData + ulValueOffset, spell.GetBuffer(), sizeof(WCHAR) * spell.GetLength());

		ulValueOffset += CVocabConst::MAX_SPELL_LEN * sizeof(WCHAR);
		::RtlCopyMemory(pRowData + ulValueOffset, explanation.GetBuffer(), sizeof(WCHAR) * explanation. GetLength());

		hr = pIRowsetChange->InsertRow(DB_NULL_HCHAPTER, hNullAcc, 0, &hRow);
		if(FAILED(hr))break;
		hr = pIRowsetChange->SetData(hRow, hDataAcc, pRowData);
		if(FAILED(hr))break;
		pIRowset->ReleaseRows(1,&hRow, 0, 0, 0);
		::RtlSecureZeroMemory(pRowData, ulBuffSize);
	}

_WriteData_cleanup:
	if(0 != pIAccessor)
	{
		if(0 != hDataAcc)pIAccessor->ReleaseAccessor(hDataAcc, NULL);
	}
	pIAccessor.Release();
	pIRowset.Release();
	pIRowsetChange.Release();

	if(0 != prgBindings)pIMalloc->Free(prgBindings);
	if(0 != pRowData)pIMalloc->Free(pRowData);

	return m_uWordCount;
}

HRESULT CVocabWorkThread::UpdateDatabase()
{
	CComPtr<IDBInitialize>			pIDBInitialize;
	CComPtr<IRowsetIndex>			pIIndex;
	CComPtr<IRowset>					pIRowset;
	CComPtr<IAccessor>				pIAccessor;
	CComPtr<IRowsetChange>		pIRowsetChange;
	CComPtr<IMalloc>					pIMalloc;
	HRESULT hr(S_FALSE);
	CStringW spell, explanation, strRelatedStr;
	CStringW str;
	CLSID clsid{ 0 };

	map<UINT, CStringW*>::const_iterator  iter, endIter;
	DBBINDING *prgBindings(0);
	BYTE *pRowData(0);
	HROW *phRow(0);
	UINT	ulBuffSize(0), ulValueOffset(0);
	HACCESSOR hDataAcc(0), hNullAcc(0);
	DBCOUNTITEM ulRowsObtained(0);
	UINT uTemp, uIndex;

	::CoInitializeEx(0, COINIT_MULTITHREADED);
	::CoGetMalloc(MEMCTX_TASK, &pIMalloc);

	if(m_Record.pstrProviderGUID->IsEmpty())
	{
		if (FALSE == Tools1::SearchProviderClassID(clsid, JudgeDatabaseType(*m_Record.pstrDBpathname), *m_Record.pstrProviderGUID))
		{
			m_strErrMsg = L"Can not find database service provider.";
			FeedbackMsg(CVocabConst::MSG_UPDATE_WAITDLG, 0, (LPARAM)new CStringW(L"Did not find database service provider."));
			goto _UpdateDatabase_cleanup;
		}
	}
	if(S_OK != Tools1::InitDataSource(pIDBInitialize, *m_Record.pstrProviderGUID, *m_Record.pstrDBpathname))goto _UpdateDatabase_cleanup;
	if (0 == pIDBInitialize)
	{
		FeedbackMsg(CVocabConst::MSG_UPDATE_WAITDLG, 0, (LPARAM)new CStringW(L"Initializing data source failed."));
		goto _UpdateDatabase_cleanup;
	}

	//
	// update words table
	// create interface first
	//
	if(CVocabConst::PROVIDERTYPE_MS_JET == m_Record.usProviderType)
	{
		hr = Tools1::CreateMsJetInterface(pIDBInitialize, pIRowsetChange, CVocabConst::NAME_TABLE_WORD);
		if(FAILED(hr))goto _UpdateDatabase_cleanup;
		hr = pIRowsetChange->QueryInterface(__uuidof(IRowset), (void**)&pIRowset);
		if(FAILED(hr))goto _UpdateDatabase_cleanup;
		hr = pIRowset->QueryInterface(__uuidof(IAccessor), (void**)&pIAccessor);
		if(FAILED(hr))goto _UpdateDatabase_cleanup;
		hr = pIRowset->QueryInterface(__uuidof(IRowsetIndex), (void**)&pIIndex);
		if(FAILED(hr))goto _UpdateDatabase_cleanup;
	}
	else
	{
		hr = Tools1::CreateSSCEInterface(pIDBInitialize, pIIndex, CVocabConst::NAME_TABLE_WORD, CVocabConst::NAME_INDEX_MAJOR);
		if(FAILED(hr))goto _UpdateDatabase_cleanup;
		hr = pIIndex->QueryInterface(__uuidof(IRowsetChange), (void**)&pIRowsetChange);
		if(FAILED(hr))goto _UpdateDatabase_cleanup;
		hr = pIIndex->QueryInterface(__uuidof(IRowset), (void**)&pIRowset);
		if(FAILED(hr))goto _UpdateDatabase_cleanup;
		hr = pIIndex->QueryInterface(__uuidof(IAccessor), (void**)&pIAccessor);
		if(FAILED(hr))goto _UpdateDatabase_cleanup;
	}

	// create DBBINDING
	ulBuffSize = sizeof(DBBINDING) * CVocabConst::COLUMN_CNT;
	prgBindings = (DBBINDING *)pIMalloc->Alloc(ulBuffSize);
	::RtlSecureZeroMemory(prgBindings, ulBuffSize);
	if(0 == prgBindings)goto _UpdateDatabase_cleanup;

	ulValueOffset = 0;
	for(int i=0;i<CVocabConst::COLUMN_CNT;i++)
	{
		prgBindings[i].iOrdinal = i + 1;
		prgBindings[i].obValue = ulValueOffset;
		prgBindings[i].obLength = 0;
		prgBindings[i].obStatus = 0;
		prgBindings[i].pTypeInfo = 0;
		prgBindings[i].pObject = 0;
		prgBindings[i].pBindExt = 0;
		prgBindings[i].dwPart = DBPART_VALUE;
		prgBindings[i].dwMemOwner = DBMEMOWNER_CLIENTOWNED;
		prgBindings[i].eParamIO = DBPARAMIO_NOTPARAM;
		prgBindings[i].cbMaxLen = CVocabConst::COLUMN_SIZE[i];
		prgBindings[i].dwFlags = 0;
		prgBindings[i].wType = CVocabConst::COLUMN_TYPE[i];
		prgBindings[i].bPrecision = 0;
		prgBindings[i].bScale = 0;

		ulValueOffset += CVocabConst::COLUMN_SIZE[i];
	}

	hr = pIAccessor->CreateAccessor(DBACCESSOR_ROWDATA, CVocabConst::COLUMN_CNT, prgBindings, CVocabConst::COLUMN_CNT * sizeof(DBBINDING), &hDataAcc, 0);
	hr = pIAccessor->CreateAccessor(DBACCESSOR_ROWDATA, 0, 0, 0, &hNullAcc, 0);
	if(FAILED(hr))goto _UpdateDatabase_cleanup;

	ulBuffSize = 0;
	for(int i=0;i<CVocabConst::COLUMN_CNT;i++)ulBuffSize += CVocabConst::COLUMN_SIZE[i];
	pRowData = (BYTE*)pIMalloc->Alloc(ulBuffSize);
	if(0 == pRowData)goto _UpdateDatabase_cleanup;
	::RtlSecureZeroMemory(pRowData, ulBuffSize);

	// add new words if exists
	if (0 == m_strsSpell2.size())goto _UpdateDatabase_update_related;

	uIndex = CVocabConst::MAX_WORD_COUNT;
	::RtlCopyMemory(pRowData, &uIndex, sizeof(UINT));
	hr = pIIndex->Seek(hDataAcc, 1, pRowData, DBSEEK_BEFORE);
	if(FAILED(hr))goto _UpdateDatabase_cleanup;
	hr = pIRowset->GetNextRows(NULL, 0, 1, &ulRowsObtained, &phRow);
	if(FAILED(hr))goto _UpdateDatabase_cleanup;

	hr = pIRowset->GetData(*phRow, hDataAcc, (void*)pRowData);
	pIRowset->ReleaseRows(1, phRow, 0, 0, 0);
	if(FAILED(hr))goto _UpdateDatabase_cleanup;

	uIndex = *(reinterpret_cast<UINT *>(pRowData)) + 1;
	ulValueOffset = sizeof(ULONG) + sizeof(ULONG);
	uTemp = 0;
	for (size_t i = 0; i < m_strsSpell2.size(); i++)
	{
		::RtlSecureZeroMemory(pRowData, ulBuffSize);
		::RtlCopyMemory(pRowData, &uIndex, sizeof(ULONG));		// index
		::RtlCopyMemory(pRowData + sizeof(ULONG), &uTemp, sizeof(ULONG));		// unfamiliar number,always set to 0
		spell = *m_strsSpell2[i];
		explanation = *m_strsExplanation2[i];
		::RtlCopyMemory(pRowData + ulValueOffset, spell, sizeof(WCHAR) * spell.GetLength());
		::RtlCopyMemory(pRowData + ulValueOffset + CVocabConst::MAX_SPELL_LEN * sizeof(WCHAR), explanation, sizeof(WCHAR) * explanation.GetLength());
		hr = pIRowsetChange->InsertRow(DB_NULL_HCHAPTER, hNullAcc, 0, phRow);
		if(FAILED(hr))break;
		hr = pIRowsetChange->SetData(*phRow, hDataAcc, pRowData);
		if(FAILED(hr))break;
		pIRowset->ReleaseRows(1, phRow, 0, 0, 0);
		uIndex++;
	}


_UpdateDatabase_update_related:
	// if m_mapRelatedIndexString has not been changed,do nothing
	if (0 == m_uVecChangedRelated.size())goto _UpdateDatabase_update_Unfamiliars;

	endIter = m_mapRelatedIndexString.end();

	// caculate column data address offset of [related] in table[words]
	// it is the last row,so ulValueOffset minus CVocabConst::COLUMN_SIZE[CVocabConst::COLUMN_CNT - 1]

	// update related string:column 4:spell and column 5:notes
	ulValueOffset = sizeof(UINT) * 2 + sizeof(WCHAR) * CVocabConst::MAX_SPELL_LEN * 2;
	uTemp = sizeof(WCHAR) * CVocabConst::MAX_SPELL_LEN;
	for (size_t i = 0; i < m_uVecChangedRelated.size(); i++)
	{
		::RtlSecureZeroMemory(pRowData, ulBuffSize);	
		uIndex = m_uVecChangedRelated[i];
		iter = m_mapRelatedIndexString.find(uIndex);
		if (iter == endIter)continue;

		::RtlCopyMemory(pRowData, &uIndex, sizeof(UINT));
		hr = pIIndex->Seek(hDataAcc, 1, pRowData, DBSEEK_FIRSTEQ);
		if(FAILED(hr))continue;
		hr = pIRowset->GetNextRows(DB_NULL_HCHAPTER, 0, 1, &ulRowsObtained, &phRow);
		if(FAILED(hr))continue;
		hr = pIRowset->GetData(*phRow, hDataAcc, pRowData);
		if(FAILED(hr))continue;

		// copy related string to buffer
		::RtlCopyMemory(pRowData + ulValueOffset, iter->second->GetBuffer(), uTemp);

		iter = m_mapNotes.find(uIndex);
		if (m_mapNotes.end() != iter)
		{
			// copy notes string(maybe empty string) to buffer
			::RtlCopyMemory(pRowData + ulValueOffset + uTemp, iter->second->GetBuffer(), uTemp);
		}

		hr = pIRowsetChange->SetData(*phRow, hDataAcc, pRowData);
		pIRowset->ReleaseRows(1, phRow, 0, 0, 0);
	}
	
_UpdateDatabase_update_Unfamiliars:

	ulValueOffset = sizeof(UINT); // the second column is:FARMILIAR,so the offset is 0 + sizeof(UINT)

	// mark unfamiliar
	uTemp = TRUE;
	for (size_t i = 0; i < m_uVecToMark.size(); i++)
	{
		uTemp = m_uVecToMark[i];
		::RtlCopyMemory(pRowData, &uTemp, sizeof(UINT));
		hr = pIIndex->Seek(hDataAcc, 1, pRowData, DBSEEK_FIRSTEQ);
		if(FAILED(hr))continue;
		hr = pIRowset->GetNextRows(DB_NULL_HCHAPTER, 0, 1, &ulRowsObtained, &phRow);
		if(FAILED(hr))continue;
		hr = pIRowset->GetData(*phRow, hDataAcc, pRowData);
		if(FAILED(hr))continue;

		::RtlCopyMemory(pRowData + ulValueOffset, &uTemp, sizeof(UINT));
		hr = pIRowsetChange->SetData(*phRow, hDataAcc, pRowData);
		pIRowset->ReleaseRows(1, phRow, 0, 0, 0);
		::RtlSecureZeroMemory(pRowData, ulBuffSize);	
	}

	// UNMARK unfamiliar
	uTemp = FALSE;
	for (size_t i = 0; i < m_uVecToUnmark.size(); i++)
	{
		uTemp = m_uVecToUnmark[i];
		::RtlCopyMemory(pRowData, &uTemp, sizeof(UINT));
		hr = pIIndex->Seek(hDataAcc, 1, pRowData, DBSEEK_FIRSTEQ);
		if(FAILED(hr))continue;
		hr = pIRowset->GetNextRows(DB_NULL_HCHAPTER, 0, 1, &ulRowsObtained, &phRow);
		if(FAILED(hr))continue;
		hr = pIRowset->GetData(*phRow, hDataAcc, pRowData);
		if(FAILED(hr))continue;

		::RtlCopyMemory(pRowData + ulValueOffset, &uTemp, sizeof(UINT));
		hr = pIRowsetChange->SetData(*phRow, hDataAcc, pRowData);
		pIRowset->ReleaseRows(1, phRow, 0, 0, 0);
		::RtlSecureZeroMemory(pRowData, ulBuffSize);	
	}
_UpdateDatabase_err:
	if (S_OK != hr)
	{
		Tools1::LookupErrorMsg(m_strErrMsg);
		FeedbackMsg(CVocabConst::MSG_SHOW_ERRMSG, 0, (LPARAM)(new CStringW(m_strErrMsg)));
	}
	else FeedbackMsg(CVocabConst::MSG_SHOW_ERRMSG, 0, 0);

_UpdateDatabase_cleanup:
	if (S_OK != hr)
	{
		Tools1::LookupErrorMsg(m_strErrMsg);
	}
	FeedbackMsg(CVocabConst::MSG_SHOW_ERRMSG, 0, (LPARAM)(new CStringW(m_strErrMsg)));

	if (0 != pRowData)
	{
		pIMalloc->Free(pRowData);
		pRowData = 0;
	}
	if (0 != prgBindings)
	{
		pIMalloc->Free(prgBindings);
		prgBindings = 0;
	}
	if(0 != pIIndex)pIIndex.Release();
	if(0 != pIRowsetChange)pIRowsetChange.Release();
	if(0 != pIRowset)pIRowset.Release();
	if(0 != pIAccessor)
	{
		if(0 != hDataAcc)pIAccessor->ReleaseAccessor(hDataAcc, NULL);
		pIAccessor.Release();
	}

	return hr;
}

void CVocabWorkThread::FeedbackMsg(UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	::PostMessage(m_hCreatorWnd, uMsg, wParam, lParam);
}

void CVocabWorkThread::OnMsgSearchSpell(WPARAM wParam/* 0 or 1, the spell is to be displayed in the label or not */, LPARAM lParam/* a CStringW pointer */)
{
	size_t uIndex, size0;
	size0 = m_strsSpell.size();
	CStringW strSpell;

	//CStringW str;
	//str.Format(L"OnMsgSearchSpell:%x\n", lParam); ::OutputDebugString(str);
	if (0 == lParam)return;

	strSpell = *reinterpret_cast<CStringW*>(lParam);
	for (uIndex = 0; uIndex < size0; uIndex++)
	{
		if (0 == m_strsSpell[uIndex]->Find(strSpell) && m_strsSpell[uIndex]->GetLength() == strSpell.GetLength())	//exactly the same
			break;
	}
	if(size0 > uIndex)::PostMessage(m_hCreatorWnd, CVocabConst::MSG_FOUND_EXPLANATION, wParam, (WPARAM)uIndex);
	else	::PostMessage(m_hCreatorWnd, CVocabConst::MSG_FOUND_EXPLANATION, wParam, CVocabConst::INVALID_INDEX);

	// the CStringW pointer MUST be deleted every time
	delete reinterpret_cast<CStringW*>(lParam);
}

void CVocabWorkThread::OnMsgStopWorking(WPARAM wParam, LPARAM lParam)
{
	HANDLE *phResumeEvent = reinterpret_cast<HANDLE*>(lParam);
	HANDLE *phEventDlgCreation = reinterpret_cast<HANDLE*>(wParam);
	UINT uGrpCnt;

	if (0 != phResumeEvent)	::SetEvent(*phResumeEvent);
	::WaitForSingleObject(*phEventDlgCreation, 1000);
	FeedbackMsg(CVocabConst::MSG_UPDATE_WAITDLG, 0, (LPARAM)new CStringW(L"Writing registry..."));

	if (0 < m_uVecUnfamilarIndex.size() % CVocabConst::COUNT_PER_GROUP)
		uGrpCnt = m_uVecUnfamilarIndex.size() / CVocabConst::COUNT_PER_GROUP + 1;
	else uGrpCnt = m_uVecUnfamilarIndex.size() / CVocabConst::COUNT_PER_GROUP;
	m_Record.uGroupCount = uGrpCnt * CVocabConst::TWOE16 + (m_Record.uGroupCount & CVocabConst::LOWSHORT);

	WriteRegistry();
	Sleep(200);
	FeedbackMsg(CVocabConst::MSG_UPDATE_WAITDLG, 0, (LPARAM)new CStringW(L"Updating database..."));
	UpdateDatabase();
	FeedbackMsg(CVocabConst::MSG_UPDATE_WAITDLG, 0, (LPARAM)new CStringW(L"Release resource..."));
	m_cpVoice.Release();

	// all work is done,notify the main window to detect if any messageBox exists
	::PostMessage(m_hCreatorWnd, CVocabConst::MSG_WAITING_FOR_QUIT, 0, 0);
}

void CVocabWorkThread::OnMsgSessionEnd(WPARAM, LPARAM lParam)
{
	static DWORD dwTick = ::GetTickCount();
	HANDLE hWaitEvent = *((HANDLE *)lParam);
	MSG msg;

	// wait for hWaitEvent if any messageBox is displaying
	while (WAIT_OBJECT_0 != ::WaitForSingleObject(hWaitEvent, 1000))
	{
		::PostMessage(m_hCreatorWnd, CVocabConst::MSG_SESSION_END, 0, 0);
		if (::GetTickCount() - dwTick > CVocabConst::TIME_OUT)
		{
			::SendMessage(m_hCreatorWnd, CVocabConst::MSG_FORCED_QUIT, 0, 0);
			break;
		}
	}

	// clear all message in queue and quit
	while (::PeekMessage(&msg, (HWND)-1/*  message posted by PostThreadMessage() */, 0, 0, PM_REMOVE));
	PostThreadMsg(WM_QUIT, 0, 0);
}

void CVocabWorkThread::OnMsgSpeakWord(WPARAM wParam, LPARAM lParam)
{
	CStringW strSpell;

	if (m_bVEngineAvailable)
	{
		if(CVocabConst::INVALID_INDEX != wParam)
		{
			if (0 <= wParam && wParam < m_strsSpell.size())
			{
				strSpell = *m_strsSpell[wParam];
				if (0 < strSpell.Find(CVocabConst::LEFT_MID_BRAKET))
				{
					strSpell = strSpell.Left(strSpell.Find(CVocabConst::LEFT_MID_BRAKET));
				}
				m_cpVoice->Speak(strSpell, SPF_DEFAULT, NULL);
			}
		}
		else
		{
			if(0 != lParam)m_cpVoice->Speak(*reinterpret_cast<CStringW *>(lParam), SPF_DEFAULT, NULL);
		}
	}
}

void CVocabWorkThread::OnMsgRcvGroupData(WPARAM wParam/* using unfamilar or not */, LPARAM lParam /* start & end group */)
{
	m_Record.bUsingUnfamiliar = GET_X_LPARAM(wParam);
	m_Record.uGroupCount = GET_Y_LPARAM(wParam);
	m_Record.uStartGroup = GET_X_LPARAM(lParam);
	m_Record.uEndGroup = GET_Y_LPARAM(lParam);
}

void CVocabWorkThread::OnDisgardData(WPARAM wParam, LPARAM lParam)
{
	m_bVEngineAvailable = FALSE;
	ClearWordsData();
}

BOOL CVocabWorkThread::IsAborting()
{
	MSG msg;
	::RtlSecureZeroMemory(&msg,sizeof(MSG));
	::PeekMessage(&msg,NULL/* message posted by the PostThreadMsg */,CVocabConst::MSG_CANCELLED,CVocabConst::MSG_CANCELLED,PM_NOREMOVE);
	
	return (CVocabConst::MSG_CANCELLED == msg.message);
}

BOOL CVocabWorkThread::PostThreadMsg(__in UINT Msg, __in WPARAM wParam, __in LPARAM lParam)
{
	return ::PostThreadMessage(m_dwTID, Msg, wParam, lParam);
}

BOOL CVocabWorkThread::CreateWorkThread(LPARAM pvoid, DWORD dwCreateFlags, UINT nStackSize, LPSECURITY_ATTRIBUTES lpSecurityAttrs)
{
	if (0 != m_hThread)return FALSE;
	if (0 == pvoid)return FALSE;	// startup parameter must be valid
	DWORD dwRet;
	_Vocab_THREAD_STARTUP *pStartup = reinterpret_cast<_Vocab_THREAD_STARTUP *>(pvoid);

	pStartup->pThread = this;

	// the new thread will set this event if new thread has been created successfully
	pStartup->hEventCreation = ::CreateEvent(NULL, TRUE, FALSE, NULL);

	if (NULL == pStartup->hEventCreation || NULL == pStartup->hEventMsgQueue)
	{
		if (NULL == pStartup->hEventCreation)		::CloseHandle(pStartup->hEventCreation);
		if (NULL == pStartup->hEventMsgQueue)		::CloseHandle(pStartup->hEventMsgQueue);
		return FALSE;
	}

	m_hThread = ::CreateThread(lpSecurityAttrs, nStackSize, CVocabWorkThread::_workThreadProc, (LPVOID)pStartup, 0/* no creation flags option is available */, &m_dwTID);
	if (0 == m_hThread)
	{
		::CloseHandle(pStartup->hEventCreation);
		::CloseHandle(pStartup->hEventMsgQueue);
		return FALSE;
	}
	dwRet = ::WaitForSingleObject(pStartup->hEventCreation, INFINITE);
	::CloseHandle(pStartup->hEventCreation);
	if(WAIT_OBJECT_0 != dwRet)return FALSE;

	if (pStartup->bError)
	{
		::WaitForSingleObject(m_hThread, INFINITE);
		::CloseHandle(m_hThread);
		m_hThread = NULL;
		::CloseHandle(pStartup->hEventMsgQueue);
		return FALSE;
	}
	m_dwTID = pStartup->dwTID;

	return TRUE;
}

DWORD WINAPI CVocabWorkThread::_workThreadProc(void *pParam)
{
	if(0 == pParam)return 0;

	MSG msg;
	_Vocab_THREAD_STARTUP *pStartup = reinterpret_cast<_Vocab_THREAD_STARTUP *>(pParam);
	if(0 == pStartup->pThread)return 0;

	CVocabWorkThread *pWorkThread = reinterpret_cast<CVocabWorkThread *>(pStartup->pThread);

	pStartup->bError = FALSE;
	pStartup->dwTID = ::GetCurrentThreadId();
	::SetEvent(pStartup->hEventCreation);

	//
	// force to create a message queue
	//
	::PeekMessage(&msg, NULL, WM_USER, WM_USER, PM_NOREMOVE);
	::SetEvent(pStartup->hEventMsgQueue);

	for (;;)
	{
		if(0 == ::GetMessage(&msg, (HWND)-1, NULL, NULL))break;

		switch (msg.message)
		{
		case CVocabConst::MSG_PREPARE_DATA:
			pWorkThread->OnMsgPrepareData(msg.wParam, msg.lParam);
			break;

		case CVocabConst::MSG_SELECT_DATABASE:
			pWorkThread->OnMsgSelectDatabase(msg.wParam, msg.lParam);
			break;

		case CVocabConst::MSG_CREATE_DATABASE:
			pWorkThread->OnMsgCreateDatabase(msg.wParam, msg.lParam);
			break;

		case CVocabConst::MSG_SPEAK:
			pWorkThread->OnMsgSpeakWord(msg.wParam, msg.lParam);
			break;

		case CVocabConst::MSG_SEARCH_SPELL:
			pWorkThread->OnMsgSearchSpell(msg.wParam, msg.lParam);
			break;

		case CVocabConst::MSG_GROUP_DATA:
			pWorkThread->OnMsgRcvGroupData(msg.wParam, msg.lParam);
			break;

		case CVocabConst::MSG_DISGARD_DATA:
			pWorkThread->OnDisgardData(msg.wParam, msg.lParam);
			break;

		case CVocabConst::MSG_STOP_WORKING:
			pWorkThread->OnMsgStopWorking(msg.wParam, msg.lParam);
			break;

		case CVocabConst::MSG_SESSION_END:
			pWorkThread->OnMsgSessionEnd(msg.wParam, msg.lParam);
			break;
		}
	}

	//::OutputDebugString(L"\n/////////////////work thread terminating///////////////////////\n");
	return 0;
}