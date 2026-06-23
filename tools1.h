
#ifndef			__TOOLS1__
#define			__TOOLS1__


//////////////////////////////////////////////////////////////////////////////////////////////////
#pragma once
//////////////////////////////////////////////////////////////////////////////////////////////////

#include "vocab.h"
#include "oledb.h"

//////////////////////////////////////////////////////////////////////////////////////////////////

class Tools1
{
public:
	Tools1(void);
	~Tools1(void);

public:
	static void Convert2String(int n, CStringW& str);
	static HFONT CreatePointFont(int nPointSize, CStringW const& strFontName);
	static int CalculateDefFontHeight();
	static int CalculateTextExtentPoint(HFONT hFont, CRect &rcTxt, CString const& str);
	static BOOL GetIconSize(HICON hIcon, CSize* pOutSize);
	static BOOL SearchSuitableFont(const CStringW& strFontName, LOGFONT& logFont);

	// search OLE DB provider by using ISourcesRowset
	// the ISourcesRowset interface can not find the SSCE provider,it can find JET provider only
	static HRESULT SearchProviderByEnumerator();

	// the OLEDB enumerator can not find SSCE provider,so we search registry directly
	static BOOL SearchProviderClassID(CLSID& clsID, USHORT dwType, CStringW &strProviderName);

	static HRESULT CreateDataStore(CComPtr<IDBInitialize> &pIDBInitialize, const CStringW& strProviderName, const CStringW& strFileName, const CStringW& strPassword = CVocabConst::EMPTYSTR, const USHORT nType = 0);
	static HRESULT InitDataSource(CComPtr<IDBInitialize> &pIDBInitialize, const CStringW& strProviderName, const CStringW& strFileName, const CStringW& strPassword = CVocabConst::EMPTYSTR);
	static HRESULT GetIIndexPtr(CComPtr<IDBInitialize> &pIDBInitialize, CStringW const& strTableName, CStringW const& strKeyName, CComPtr<IRowsetIndex>& pIIndex);
	static HRESULT GetIRowChangePtr(CComPtr<IDBInitialize> &pIDBInitialize, CStringW const& strTableName, CComPtr<IRowsetChange>& pIRowsetChange);
	static HRESULT CreateTableInSqlCompact(CComPtr<IDBInitialize> &pIDBInitialize);
	static HRESULT CreateTableInAccessDB(CComPtr<IDBInitialize> &pIDBInitialize);
	static HRESULT AddColumnInAccessDB(CComPtr<IDBInitialize> &pIDBInitialize, LPOLESTR wszTableName, LPOLESTR wszColumnName, DBTYPE wColumnType, DBLENGTH ulColunmSize = sizeof(long));
	static HRESULT CreateIndex(CComPtr<IDBInitialize> &pIDBInitialize, LPOLESTR wszTableName, LPOLESTR wszColumnName);
	static HRESULT CreateMsJetInterface(CComPtr<IDBInitialize> &pIDBInitialize, CComPtr<IRowsetChange>	&pIRowsetChange, LPOLESTR strTableName);
	static HRESULT CreateSSCEInterface(CComPtr<IDBInitialize> &pIDBInitialize, CComPtr<IRowsetIndex>	&pIIndex, LPOLESTR strTableName, LPOLESTR strIndexName);
	static HRESULT CheckProviderSchemaInfo(CComPtr<IDBInitialize> &pIDBInitialize, CStringW &strTypeName);
	static HRESULT LookupErrorMsg(CStringW& strErrMsg);
	static DBPROPID	SearchPasswordPropID(CComPtr<IDBProperties> &pIDBInitialize);
	static USHORT JudgeProviderType(CLSID& clsID);
	static BOOL GenerateDatabaseFileName(CStringW &strFileName);

};

#endif //__TOOLS1__
