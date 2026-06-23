
////////////////////////////////////////////////////////////////////////////////////////////

#pragma once

////////////////////////////////////////////////////////////////////////////////////////////

#include <vector>
#include <map>
#include <list>
#include <atlstr.h>
#include <sphelper.h>
#include "oledb.h"
#include "vocabconst.h"

//////////////////////////////////////////////////////////////////////////////////////////////////

using namespace std;

//////////////////////////////////////////////////////////////////////////////////////////////////

class VocabRecord
{
public:
	VocabRecord();
	~VocabRecord();

public:
	UINT				uGroupCount, uStartGroup, uEndGroup;	// only used for words,not contains unfamiliars
	USHORT		usProviderType;
	BOOL				bUsingUnfamiliar;
	CStringW		*pstrProviderGUID;
	CStringW		*pstrDBpathname;
};

struct _Vocab_THREAD_STARTUP
{
	// following are "in" parameters to thread startup
	void* pThread;    // CWinThread for new thread
	DWORD dwTID;		// thread ID

	HANDLE hEventCreation;          // event triggered after success/non-success
	HANDLE hEventMsgQueue;         // event triggered after message queue is created

	// strictly "out" -- set after hEvent is triggered
	BOOL bError;    // TRUE if error during startup
};

///////////////////////////////////////////////////////////////////////////////////////////////////////////
//
class CVocabWorkThread
{
public:
	CVocabWorkThread();
	virtual ~CVocabWorkThread();
	BOOL CreateWorkThread(LPARAM pvoid, DWORD dwCreateFlags = 0, UINT nStackSize = 0, LPSECURITY_ATTRIBUTES lpSecurityAttrs = NULL);
	BOOL PostThreadMsg( __in UINT Msg, __in WPARAM wParam,  __in LPARAM lParam);

	inline HANDLE GetHandle(){return m_hThread;}
	inline void SetCreator(HWND hCreator){m_hCreatorWnd = hCreator;}
	inline void GetDBName(CStringW& strName){strName = *m_Record.pstrDBpathname;}
	inline void SetDBName(const CStringW& strName){*m_Record.pstrDBpathname = strName;}
	inline  VocabRecord* GetRecord() { return &m_Record; }

protected:
	void		OnMsgPrepareData(WPARAM, LPARAM);
	void		OnMsgSelectDatabase(WPARAM, LPARAM);
	void		OnMsgCreateDatabase(WPARAM, LPARAM);
	void		OnMsgSpeakWord(WPARAM, LPARAM);
	void		OnMsgSearchSpell(WPARAM, LPARAM);
	void		OnDisgardData(WPARAM, LPARAM);
	void		OnMsgRcvGroupData(WPARAM, LPARAM);
	void		OnMsgStopWorking(WPARAM, LPARAM);
	void		OnMsgSessionEnd(WPARAM, LPARAM);

protected:
	UINT ReadTxtFile(CStringW const& strFileName);
	UINT ClearWordsData();
	UINT ReadData(CComPtr<IDBInitialize>	pIDBInitialize);
	UINT WriteData(CComPtr<IDBInitialize>	pIDBInitialize);
	USHORT JudgeDatabaseType(CStringW strFileName);
	USHORT CheckStringType(CStringW const& str);
	BOOL ReadRegistry();
	BOOL WriteRegistry();
	BOOL IsAborting();
	HRESULT UpdateDatabase();
	void FeedbackMsg(UINT uMsg, WPARAM wParam, LPARAM lParam);

private:
	static DWORD WINAPI _workThreadProc(void *pParam);

public:
	vector<CStringW*>					m_strsSpell, m_strsExplanation;
	vector<CStringW*>					m_strsSpell2, m_strsExplanation2;		// used for new words to be added to DB
	vector<UINT>						m_uVecUnfamilarIndex;					// this array contains all unfamiliar words index in m_strsSpell
	vector<UINT>						m_uVecToMark, m_uVecToUnmark;	// contain marked/unmarked unfamilar index in m_strsSpell within current session
	vector<UINT>						m_uVecChangedRelated;

	map <UINT, CStringW*>			m_mapRelatedIndexString;
	map <UINT, CStringW*>			m_mapNotes;
	UINT										m_uWordCount;

protected:
	DWORD									m_dwTID;
	HWND									m_hCreatorWnd;
	HANDLE									m_hThread;
	BOOL										m_bVEngineAvailable;		// indicate if an usable voice engine exists
	VocabRecord							m_Record;

	// voice engine
	CComPtr<ISpVoice>				m_cpVoice;

	// the provider-specific name of the data type of a column,can be "longtext","ntext" etc.
	// the valid string will be obtained from the provider's PROVIDER_TYPES rowset at runtime
	// it will be used when creating table or adding column
	CStringW								m_strTypeName;
	CStringW								m_strErrMsg;
};


