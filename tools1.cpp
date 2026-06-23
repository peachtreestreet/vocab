
#include "Tools1.h"
#include "msdaguid.h"
#include <time.h>

////////////////////////////////////////////////////////////////////////////////////////////

#ifdef _DEBUG
#define new new(_NORMAL_BLOCK, __FILE__, __LINE__)
#endif

////////////////////////////////////////////////////////////////////////////////////////////

using namespace VocabApp;
class CvocabularyDlg;

////////////////////////////////////////////////////////////////////////////////////////////
// global vars

extern vocabGlobal globalVar;

////////////////////////////////////////////////////////////////////////////////////////////

const WCHAR CVocabConst::PROVIDER_STR_SSCE[] = L"sqlserver.ce.oledb";
const WCHAR CVocabConst::PROVIDER_STR_JET[] = L"microsoft.jet.oledb";

////////////////////////////////////////////////////////////////////////////////////////////

Tools1::Tools1(void){}

Tools1::~Tools1(void){}

BOOL Tools1::GetIconSize(HICON hIcon, CSize* pOutSize) 
{
	if (!hIcon || !pOutSize)return FALSE;

	ICONINFO iconInfo = {0};
	BOOL bResult = GetIconInfo(hIcon, &iconInfo);
	if (!bResult)return FALSE;

	BITMAP bmp = {0};
	// 优先尝试获取颜色位图的信息
	if (iconInfo.hbmColor)
	{
		GetObject(iconInfo.hbmColor, sizeof(BITMAP), &bmp);
		pOutSize->cx = bmp.bmWidth;
		pOutSize->cy = bmp.bmHeight;
	} 
	// 如果没有颜色位图（单色图标），则从掩码位图中计算
	else if (iconInfo.hbmMask)
	{
		GetObject(iconInfo.hbmMask, sizeof(BITMAP), &bmp);
		pOutSize->cx = bmp.bmWidth;
		// 关键点：单色图标的图像和掩码是上下堆叠的，所以高度要除以2
		pOutSize->cy = bmp.bmHeight / 2;
	} 
	else
	{
		// 没有任何有效的位图信息
		bResult = FALSE;
	}

	// 清理：必须删除 GetIconInfo 返回的位图句柄
	if (iconInfo.hbmColor) DeleteObject(iconInfo.hbmColor);
	if (iconInfo.hbmMask) DeleteObject(iconInfo.hbmMask);

	return bResult;
}

int Tools1::CalculateTextExtentPoint(HFONT hFont, CRect &rcTxt, CStringW const& str)
{
	HDC hScreenDC = ::GetDC(0);
	HDC hdc = ::CreateCompatibleDC(hScreenDC);
	::SelectObject(hdc, hFont);

	int nTxtLineHeight = ::DrawText(hdc, (LPCWSTR)str, str.GetLength(), &rcTxt, DT_SINGLELINE | DT_CALCRECT);
	::DeleteDC(hdc);
	::ReleaseDC(0, hScreenDC);

	return nTxtLineHeight;
}

int CALLBACK _tools1_EnumFont(ENUMLOGFONTEX *lpelfe, NEWTEXTMETRICEX *lpntme, DWORD FontType, LPARAM lParam)
{
	if(0 == lParam)return 0;

	LOGFONT *plogFont = reinterpret_cast<LOGFONT *>(lParam);
	CStringW strFaceName = lpelfe->elfLogFont.lfFaceName;

	if(strFaceName == plogFont->lfFaceName)
	{
		::RtlCopyMemory(plogFont, &(lpelfe->elfLogFont), sizeof(LOGFONT));
		return 0;		// stop enumerating
	}

	return TRUE;		// continue enumerating
}

int Tools1::CalculateDefFontHeight()
{
	HDC hScreenDC = ::GetDC(0);
	HDC hMemDC = ::CreateCompatibleDC(hScreenDC);

	// GetDeviceCaps(hMemDC,VERTSIZE) is screen size(vertical size) in mm
	// 1 inch = 25.4 mm
	// there are GetDeviceCaps(hMemDC,LOGPIXELSY) login points(in vertical direction) in 1 inch
	// the intend font height is 1/30 of the screen height (it's independent from the metrics,in mm or in inch)

	int nFontPoints = ::GetDeviceCaps(hMemDC, VERTSIZE) * ::GetDeviceCaps(hMemDC, LOGPIXELSY) / CVocabConst::MM_PER_INCH / CVocabApp::FONT_HEIGHT_ADJ;
	::DeleteDC(hMemDC);
	::ReleaseDC(0,hScreenDC);

	return nFontPoints;
}

BOOL Tools1::SearchSuitableFont(const CStringW& strFontName, LOGFONT& logFont)
{
	HDC hScreenDC = ::GetDC(0);
	HDC hMemDC = ::CreateCompatibleDC(hScreenDC);
	::wmemcpy_s(logFont.lfFaceName, sizeof(logFont.lfFaceName) / sizeof(WCHAR), strFontName, strFontName.GetLength());
	::EnumFontFamiliesEx(hMemDC, &logFont, (FONTENUMPROC)_tools1_EnumFont, (LPARAM)&logFont, 0);
	::DeleteDC(hMemDC);
	::ReleaseDC(0, hScreenDC);

	return 0 < logFont.lfHeight;
}

HFONT Tools1::CreatePointFont(int nPointSize, CStringW const& strFontName)
{
	LOGFONT logFont;
	memset(&logFont, 0, sizeof(LOGFONT));
	logFont.lfCharSet = DEFAULT_CHARSET;
	logFont.lfHeight = nPointSize;
	Checked::tcsncpy_s(logFont.lfFaceName, _countof(logFont.lfFaceName), strFontName, _TRUNCATE);

	HDC hDC;
	hDC = ::GetDC(NULL);

	// convert nPointSize to logical units based on pDC
	CPoint ptOrg, pt;

	// 72 points/inch, 10 decipoints/point
	ptOrg.x = ptOrg.y = 0;
	pt.y = ::MulDiv(::GetDeviceCaps(hDC, LOGPIXELSY), logFont.lfHeight, 720);
	pt.x = 0;
	::DPtoLP(hDC, &pt, 1);
	::DPtoLP(hDC, &ptOrg, 1);
	logFont.lfHeight = -abs(pt.y - ptOrg.y);
	::ReleaseDC(NULL, hDC);

	return ::CreateFontIndirect(&logFont);
}

BOOL Tools1::GenerateDatabaseFileName(CStringW &strFileName)
{
	HCRYPTPROV		hCryptoProvider;
	BYTE		*pBuff;

	pBuff = new BYTE[CVocabConst::BYTES_OF_FILENAME];
	::RtlSecureZeroMemory(pBuff, CVocabConst::BYTES_OF_FILENAME);

	::CryptAcquireContext(&hCryptoProvider, 0, 0, PROV_DSS, CRYPT_VERIFYCONTEXT);
	if(0 != hCryptoProvider)
	{
		CryptGenRandom(hCryptoProvider, CVocabConst::BYTES_OF_FILENAME, pBuff);
		strFileName.Empty();

		for (int i = 0;i < CVocabConst::BYTES_OF_FILENAME;i++)
		{
			if((pBuff[i]&0xf) >=0 && (pBuff[i]&0xf) <=9)
			{
				strFileName += static_cast<char>((pBuff[i]&0xf)+0x30);
			}
			else
			{
				strFileName+=static_cast<char>((pBuff[i]&0xf)+0x57);
			}

			if(pBuff[i]/16 >=0 && pBuff[i]/16 <= 9)
			{
				strFileName += static_cast<char>(pBuff[i]/16+0x30);
			}
			else
			{
				strFileName += static_cast<char>((pBuff[i]/16)+0x57);
			}
		}
		::CryptReleaseContext(hCryptoProvider, 0);
	}
	else
	{
		WCHAR   wszBuff[8];
		::srand(::time(0));
		::_itow_s(::rand()*::rand(),wszBuff,16);
		strFileName = wszBuff;
		while (strFileName.GetLength() < CVocabConst::FILE_NAME_LEN)strFileName.Insert(0, CVocabConst::ZERO);
	}

	delete[] pBuff;

	return strFileName.IsEmpty();
}

HRESULT Tools1::SearchProviderByEnumerator()
{
	CComPtr<ISourcesRowset>	pISourcesRowset(0);
	CComPtr<IRowset>				pIRowset(0);
	CComPtr<IAccessor>			pIAccessor(0);
	HRESULT hr;
	CComPtr<IMalloc>		pIMalloc;
	BYTE *pBuff;

	pBuff = nullptr;

	::CoGetMalloc(MEMCTX_TASK, &pIMalloc);

	// create root enumerator to enumerate provider
	hr = CoCreateInstance(CLSID_OLEDB_ENUMERATOR, NULL, CLSCTX_INPROC_SERVER, __uuidof(ISourcesRowset), (void**)&pISourcesRowset);
	if (FAILED(hr))return hr;
	hr = pISourcesRowset->GetSourcesRowset(0, __uuidof(IRowset), 0, 0, (IUnknown**)&pIRowset);
	if (FAILED(hr))return hr;
	hr = pIRowset->QueryInterface(__uuidof(IAccessor), (void **)&pIAccessor);
	if (FAILED(hr))return hr;

	DBBINDING *prgBindings(0);
	CStringW strSourceName,strDesc;

	prgBindings = (DBBINDING *)pIMalloc->Alloc(sizeof(DBBINDING) * CVocabConst::ROOT_ENUMERATOR_COLUMN_CNT);
	if(0 == prgBindings)goto _SearchProvider_cleanup;

	// column count in the rowset returned by root enumerator(ISourcesRowset::GetSourcesRowset)
	// the predefined(by OLEDB and readonly) count is 5,we need first only(SOURCES_NAME)
	prgBindings->iOrdinal = 1;
	prgBindings->obValue = 0;
	prgBindings->obLength = 0;
	prgBindings->obStatus = 0;
	prgBindings->pTypeInfo = 0;
	prgBindings->pObject = 0;
	prgBindings->pBindExt = 0;
	prgBindings->dwPart = DBPART_VALUE;
	prgBindings->dwMemOwner = DBMEMOWNER_CLIENTOWNED;
	prgBindings->eParamIO = DBPARAMIO_NOTPARAM;
	prgBindings->cbMaxLen = CVocabConst::MAX_LEN_SOURCES_NAME;
	prgBindings->dwFlags = 0;
	prgBindings->wType = CVocabConst::SOURCE_COLUMN_TYPE;
	prgBindings->bPrecision = 0;
	prgBindings->bScale = 0;

	HACCESSOR hAccessor(0);
	HROW	*phRows(0);
	DBCOUNTITEM cRowsObtained(0);
	CLSID clsID;
	BOOL		bFound(0);
	hr = pIAccessor->CreateAccessor(DBACCESSOR_ROWDATA, CVocabConst::ROOT_ENUMERATOR_COLUMN_CNT, prgBindings, CVocabConst::MAX_LEN_SOURCES_NAME, &hAccessor, 0);
	if (FAILED(hr))return hr;
	hr = pIRowset->GetNextRows(0, 0, CVocabConst::MAX_PROVIDER_CNT, &cRowsObtained, &phRows);
	if (FAILED(hr))return hr;

	pBuff = (BYTE*)pIMalloc->Alloc(CVocabConst::MAX_LEN_SOURCES_NAME);
	::memset(pBuff, 0, CVocabConst::MAX_LEN_SOURCES_NAME);

	for (DBCOUNTITEM i=0;i<cRowsObtained;i++)
	{   
		hr = pIRowset->GetData(phRows[i], hAccessor, (void*)pBuff);
		strSourceName = (WCHAR *)pBuff;
		strSourceName.MakeLower();
		if(strSourceName.Find(CVocabConst::PROVIDER_STR_JET) >= 0)
		{
			::CLSIDFromString(strSourceName, &clsID);
			bFound = (S_OK == hr) ? 1:0;
			break;
		}
		if (FAILED(hr)) break;
	}

	hr = pIRowset->ReleaseRows(cRowsObtained, phRows, NULL, NULL, NULL);

_SearchProvider_cleanup:
	pIMalloc->Free(prgBindings);
	if(nullptr != pBuff) pIMalloc->Free(pBuff);

	return hr;
}

DBPROPID	Tools1::SearchPasswordPropID(CComPtr<IDBProperties> &pIDBProperties)
{
	if(0 == pIDBProperties)return E_FAIL;

	HRESULT hr;
	DBPROPID dwPropID(0);
	ULONG ulPropInfo;	
	DBPROPINFOSET     *prgPropInfoSets(0);
	OLECHAR	*pwszDesc;
	CStringW strDesc;
	CComPtr<IMalloc>		pIMalloc;

	::CoGetMalloc(MEMCTX_TASK,&pIMalloc);

	hr = pIDBProperties->GetPropertyInfo(0, 0, &ulPropInfo, &prgPropInfoSets, &pwszDesc);
	if(FAILED(hr))goto	_SearchPasswordPropID_cleanup;
	if(2 > ulPropInfo)goto	_SearchPasswordPropID_cleanup;

	for(int i=0;i<ulPropInfo;i++)
	{
		if(DBPROPSET_DBINIT == prgPropInfoSets[i].guidPropertySet)continue;
		for(int j=0;j<prgPropInfoSets[i].cPropertyInfos;j++)
		{
			strDesc = prgPropInfoSets[i].rgPropertyInfos[j].pwszDescription;
			strDesc.MakeLower();
			if(strDesc.Find(CVocabConst::PROVIDER_SPEC_PASSWORD_PROP) >= 0)
			{
				dwPropID =  prgPropInfoSets[i].rgPropertyInfos[j].dwPropertyID;
				break;
			}
		}
	}

_SearchPasswordPropID_cleanup:
	if(0 != prgPropInfoSets)pIMalloc->Free(prgPropInfoSets);

	return dwPropID;
}

HRESULT Tools1::CreateMsJetInterface(CComPtr<IDBInitialize> &pIDBInitialize, CComPtr<IRowsetChange> &pIRowsetChange, LPOLESTR strTableName)
{
	HRESULT hr;
	CComPtr<IDBCreateSession>	pIDBCreateSession;
	CComPtr<IOpenRowset>			pIOpenRowset;
	DBID dbidTable;
	DBPROPSET dbPropSets;
	DBPROP	rgProp[2];

	hr = pIDBInitialize->QueryInterface(__uuidof(IDBCreateSession), (void**)&pIDBCreateSession);
	if(FAILED(hr))return hr;
	hr = pIDBCreateSession->CreateSession(NULL, __uuidof(IOpenRowset), (IUnknown**)&pIOpenRowset);
	if(FAILED(hr))return hr;

	dbidTable.eKind = DBKIND_NAME;
	dbidTable.uName.pwszName = strTableName;

	dbPropSets.cProperties = 2;
	dbPropSets.guidPropertySet = DBPROPSET_ROWSET;
	dbPropSets.rgProperties = rgProp;

	rgProp[0].colid = DB_NULLID;
	rgProp[0].dwOptions = DBPROPOPTIONS_REQUIRED;
	rgProp[0].dwPropertyID = DBPROP_UPDATABILITY;
	rgProp[0].dwStatus = DBPROPSTATUS_OK;
    rgProp[0].vValue.vt = VT_I4;
    V_I4(&rgProp[0].vValue) = DBPROPVAL_UP_CHANGE|DBPROPVAL_UP_DELETE|DBPROPVAL_UP_INSERT;

	rgProp[1].colid = DB_NULLID;
	rgProp[1].dwOptions = DBPROPOPTIONS_REQUIRED;
	rgProp[1].dwPropertyID = DBPROP_IRowsetIndex;
	rgProp[1].dwStatus = DBPROPSTATUS_OK;
    rgProp[1].vValue.vt = VT_BOOL;
    V_BOOL(&(rgProp[1].vValue)) = VARIANT_TRUE;

	return pIOpenRowset->OpenRowset(0, &dbidTable, 0, __uuidof(IRowsetChange), 1, &dbPropSets, (IUnknown**)&pIRowsetChange);
}

HRESULT Tools1::CreateSSCEInterface(CComPtr<IDBInitialize> &pIDBInitialize, CComPtr<IRowsetIndex>&	pIIndex, LPOLESTR strTableName, LPOLESTR strIndexName)
{
	HRESULT hr;
	CComPtr<IDBCreateSession>	pIDBCreateSession;
	CComPtr<IOpenRowset>			pIOpenRowset;
	
	DBID dbidTable, dbidIndex;
	DBPROPSET dbPropSets;
	DBPROP	rgProp;

	hr = pIDBInitialize->QueryInterface(__uuidof(IDBCreateSession), (void**)&pIDBCreateSession);
	if(FAILED(hr))return hr;
	hr = pIDBCreateSession->CreateSession(0, __uuidof(IOpenRowset), (IUnknown**)&pIOpenRowset);
	if(FAILED(hr))return hr;

	// name of the table which contains the index column
	dbidTable.eKind = DBKIND_NAME;
	dbidTable.uName.pwszName = strTableName;

	dbidIndex.eKind = DBKIND_NAME;
	dbidIndex.uName.pwszName = strIndexName;

	dbPropSets.cProperties = 1;
	dbPropSets.guidPropertySet = DBPROPSET_ROWSET;
	dbPropSets.rgProperties = &rgProp;

	rgProp.colid = DB_NULLID;
	rgProp.dwOptions = DBPROPOPTIONS_REQUIRED;
	rgProp.dwPropertyID = DBPROP_IRowsetIndex;
	rgProp.dwStatus = DBPROPSTATUS_OK;
	rgProp.vValue.vt = VT_BOOL;
    V_BOOL(&(rgProp.vValue)) = VARIANT_TRUE;

	return pIOpenRowset->OpenRowset(0, &dbidTable, &dbidIndex, __uuidof(IRowsetIndex), 1, &dbPropSets, (IUnknown**)&pIIndex);
}

HRESULT Tools1::CheckProviderSchemaInfo(CComPtr<IDBInitialize> &pIDBInitialize, CStringW &strTypeName)
{
	strTypeName.Empty();
	if(0 == pIDBInitialize)return E_FAIL;

	CComPtr<IDBCreateSession>	pIDBCreateSession;
	CComPtr<IDBSchemaRowset>	pIDBSchemaRowset;
	CComPtr<IOpenRowset>			pIOpenRowset;
	CComPtr<IRowsetInfo>			pIRowsetInfo;
	CComPtr<IRowset>					pIRowset;
	CComPtr<IColumnsInfo>			pIColumnsInfo;
	CComPtr<IAccessor>				pIAccessor;
	CComPtr<IMalloc>		pIMalloc;
	
	HRESULT hr;
	DBORDINAL	cColumns;
	DBCOLUMNINFO		*rgColumnInfo(0);
	LPWSTR					pStringBuffer(0);

	::CoGetMalloc(MEMCTX_TASK,&pIMalloc);

	hr = pIDBInitialize->QueryInterface(__uuidof(IDBCreateSession), (void**)&pIDBCreateSession);
	if(FAILED(hr))return hr;
	hr = pIDBCreateSession->CreateSession(0, __uuidof(IOpenRowset), (IUnknown**)&pIOpenRowset);
	if(FAILED(hr))return hr;
	hr = pIOpenRowset->QueryInterface(__uuidof(IDBSchemaRowset), (void**)&pIDBSchemaRowset);
	if(FAILED(hr))return hr;
	hr = pIDBSchemaRowset->GetRowset(0, DBSCHEMA_PROVIDER_TYPES, 0, 0, __uuidof(IRowset), 0, 0, (IUnknown**)&pIRowset);
	if(FAILED(hr))return hr;
	hr = pIRowset->QueryInterface(__uuidof(IColumnsInfo), (void**)&pIColumnsInfo);
	if(FAILED(hr))return hr;
	hr = pIColumnsInfo->GetColumnInfo(&cColumns, &rgColumnInfo, &pStringBuffer);
	if(FAILED(hr))return hr;
	
	DBBINDING *prgBindings(0);
	UINT	ulDataLength(0);

	prgBindings = (DBBINDING *)pIMalloc->Alloc(sizeof(DBBINDING)*CVocabConst::SCHEMA_COLUMN_CNT);
	for(int i=0;i<CVocabConst::SCHEMA_COLUMN_CNT;i++)
	{
		prgBindings[i].iOrdinal = rgColumnInfo[i+1].iOrdinal;
		prgBindings[i].obValue = ulDataLength;
		prgBindings[i].obLength = 0;
		prgBindings[i].obStatus = 0;
		prgBindings[i].pTypeInfo = 0;
		prgBindings[i].pObject = 0;
		prgBindings[i].pBindExt = 0;
		prgBindings[i].dwPart = DBPART_VALUE;
		prgBindings[i].dwMemOwner = DBMEMOWNER_CLIENTOWNED;
		prgBindings[i].eParamIO = DBPARAMIO_NOTPARAM;
		prgBindings[i].cbMaxLen = rgColumnInfo[i+1].ulColumnSize;
		prgBindings[i].dwFlags = 0;
		prgBindings[i].wType = rgColumnInfo[i+1].wType;
		prgBindings[i].bPrecision = rgColumnInfo[i+1].bPrecision;
		prgBindings[i].bScale = rgColumnInfo[i+1].bScale;

		if(DBTYPE_UI2 == rgColumnInfo[i+1].wType)prgBindings[i].cbMaxLen =sizeof(UINT);
		if(DBTYPE_UI4 == rgColumnInfo[i+1].wType)prgBindings[i].cbMaxLen =sizeof(UINT);
		if(DBTYPE_WSTR == rgColumnInfo[i+1].wType)prgBindings[i].cbMaxLen = CVocabConst::SCHEMA_TYPENAME_LEN;
		
		ulDataLength = prgBindings[i].cbMaxLen + prgBindings[i].obValue;
	}
	if(0 >= ulDataLength || CVocabConst::MAX_NTEXT_LEN < ulDataLength)return E_FAIL;

	HACCESSOR hAcc;
	HROW	*phRows(0);
	DBCOUNTITEM ulRowsObtained(0);
	USHORT usDataType;
	UINT ulColumnSize;

	hr = pIRowset->QueryInterface(__uuidof(IAccessor), (void **)&pIAccessor);
	hr = pIAccessor->CreateAccessor(DBACCESSOR_ROWDATA, CVocabConst::SCHEMA_COLUMN_CNT, prgBindings, 0, &hAcc, 0);
	BYTE *pBuff = (BYTE *)pIMalloc->Alloc(ulDataLength);

	HRESULT hRet(E_FAIL);
	pIRowset->RestartPosition(0);
	while(S_OK == hr)
	{
		hr = pIRowset->GetNextRows(NULL, 0, 1, &ulRowsObtained, &phRows);
		if(FAILED(hr))break;
		hr = pIRowset->GetData(*phRows, hAcc, (void*)pBuff);
		if(FAILED(hr))break;
		usDataType = *((USHORT *)(pBuff + CVocabConst::SCHEMA_TYPENAME_LEN));
		ulColumnSize = *((UINT *)(pBuff + CVocabConst::SCHEMA_TYPENAME_LEN + sizeof(UINT)));

		// we have found the provider-specific data type name string
		if(DBTYPE_WSTR == usDataType && CVocabConst::MAX_NTEXT_LEN < ulColumnSize)
		{
			strTypeName = (WCHAR *)pBuff;
			hRet = S_OK;
			break;
		}
		hr = pIRowset->ReleaseRows(1, phRows, 0, 0, 0);
	}

	pIMalloc->Free(rgColumnInfo);
	pIMalloc->Free(prgBindings);
	pIMalloc->Free(pBuff);

	return hRet;
}

USHORT Tools1::JudgeProviderType(CLSID& clsID)
{
	////////////////////////////////////////////////////////////////////////////////////////
	// provider type
	//  0:SSCE,1:jet
	////////////////////////////////////////////////////////////////////////////////////////

	if(IID_NULL == clsID)return (USHORT)(-1);

	CComPtr<IDBProperties> pIDBProperties;
	CComPtr<IDBInitialize> pIDBInitialize;
	USHORT usProviderType(CVocabConst::INVALID_INDEX);
	DBPROPIDSET	dbPropIdSet;
	DBPROPID		dbPropID;
	DBPROPINFOSET *pdbInfoSet;
	ULONG	ulPropCnt;
	CStringW strProviderDesc;
	OLECHAR *pOleChar;
	BOOL bFound(0);

	::CoCreateInstance(clsID, NULL, CLSCTX_INPROC_SERVER, __uuidof(IDBInitialize), (void **)&pIDBInitialize);
	if(0 == pIDBInitialize)return CVocabConst::INVALID_INDEX;

	dbPropIdSet.cPropertyIDs = 0;
	dbPropIdSet.rgPropertyIDs = &dbPropID;
	dbPropIdSet.guidPropertySet = DBPROPSET_DATASOURCEINFOALL;
	pIDBInitialize->QueryInterface(__uuidof(IDBProperties), (void **)&pIDBProperties);
	HRESULT hr = pIDBProperties->GetPropertyInfo(0, &dbPropIdSet, &ulPropCnt, &pdbInfoSet, &pOleChar);

	for (UINT i = 0;i < ulPropCnt;i++)
	{
		for (int j = 0;j < pdbInfoSet[i].cPropertyInfos;j++)
		{
			strProviderDesc = pdbInfoSet[i].rgPropertyInfos[j].pwszDescription;
			strProviderDesc = strProviderDesc.Left(4);
			strProviderDesc.MakeLower();
			if(strProviderDesc.Find(L"ssce") >= 0)
			{
				usProviderType = 0;
				bFound = TRUE;
				break;
			}
			if(strProviderDesc.Find(L"jet") >= 0)
			{
				usProviderType = 1;
				bFound = TRUE;
				break;
			}
		}
		if(bFound)break;
	}

	return usProviderType;
}

BOOL Tools1::SearchProviderClassID(CLSID& clsID, USHORT dwType, CStringW &strProviderName)
{
	if(CVocabConst::PROVIDERTYPE_INVALID == dwType)return FALSE;

	if(!strProviderName.IsEmpty())
	{
		CLSID classid;
		if(S_OK == ::CLSIDFromString(strProviderName, &classid))
		{
			if(JudgeProviderType(classid) == dwType)
			{
				clsID = classid;
				return TRUE;
			}
		}
	}

	HKEY hRegKey;

	if (::RegOpenKeyEx(HKEY_CLASSES_ROOT, L"clsid", 0, KEY_READ, &hRegKey) != ERROR_SUCCESS)return FALSE;
	DWORD dwClassLen(CVocabConst::MAX_REG_VALUE_LEN - 1);
	WCHAR	wszSubkeyName[CVocabConst::MAX_REG_VALUE_LEN];
	DWORD	dwSubkeyCnt(0);
	DWORD dwIndex;
	CStringW strProgID, strProviderDesc;;
	LPOLESTR	wszProgID;

	::RegQueryInfoKey(hRegKey, wszSubkeyName, &dwClassLen, 0, &dwSubkeyCnt, 0, 0, 0, 0, 0, 0, 0);
	if(0 ==  dwSubkeyCnt)return FALSE;

	strProviderDesc.Empty();
	if(CVocabConst::PROVIDERTYPE_MS_SSCE == dwType)strProviderDesc = CVocabConst::PROVIDER_STR_SSCE;
	if(CVocabConst::PROVIDERTYPE_MS_JET == dwType)strProviderDesc = CVocabConst::PROVIDER_STR_JET;

	for (dwIndex = 0; dwIndex < dwSubkeyCnt; dwIndex++)
	{ 
		dwClassLen = CVocabConst::MAX_REG_VALUE_LEN - 1;
		::RegEnumKeyEx(hRegKey, dwIndex, wszSubkeyName, &dwClassLen, 0, 0, 0, 0);
		::CLSIDFromString(wszSubkeyName, &clsID);
		::ProgIDFromCLSID(clsID, &wszProgID);
		strProgID = wszProgID;
		strProgID.MakeLower();
		if(strProgID.Find(strProviderDesc) >=0 && strProgID.Find(CVocabConst::PROVIDER_STR_ERR) < 0)break;
	}

	if(dwIndex < dwSubkeyCnt)strProviderName = wszSubkeyName;
	else strProviderName.Empty();

	return FALSE == strProviderName.IsEmpty();
}

HRESULT Tools1::InitDataSource(CComPtr<IDBInitialize> &pIDBInitialize, const CStringW& strProviderName, const CStringW& strDataStoreName, const CStringW& strPassword)
{
	if(strProviderName.IsEmpty())return E_FAIL;

	HRESULT hr;
	CLSID clsid;
	CComPtr<IDBProperties>  pIDbProperties;
	ULONG cPropertySets;	
	DBPROPSET	*prgPropSets(0);
	DWORD dwProviderType;
	CComPtr<IMalloc>		pIMalloc;

	::CoGetMalloc(MEMCTX_TASK, &pIMalloc);
	if(0 != pIDBInitialize)
	{
		pIDBInitialize->Uninitialize();
		pIDBInitialize.Release();
	}

	hr = ::CLSIDFromString(strProviderName, &clsid);
	if(FAILED(hr))return hr;
	dwProviderType = JudgeProviderType(clsid);

	hr = ::CoCreateInstance(clsid, 0, CLSCTX_INPROC_SERVER, __uuidof(IDBInitialize), (void**)&pIDBInitialize);
	if(FAILED(hr))return hr;
	hr = pIDBInitialize->QueryInterface(__uuidof(IDBProperties), (void **)(&pIDbProperties));
	if(FAILED(hr))return hr;

	DBPROPSET	rgPropSets;
	DBPROP		rgDBProp;

	rgPropSets.guidPropertySet = DBPROPSET_DBINIT;
	rgPropSets.cProperties = 1;
	rgPropSets.rgProperties = &rgDBProp;

	rgDBProp.colid = DB_NULLID;
	rgDBProp.dwPropertyID = DBPROP_INIT_DATASOURCE;
	rgDBProp.dwOptions = DBPROPOPTIONS_REQUIRED;
	rgDBProp.vValue.bstrVal = SysAllocString(strDataStoreName);
	rgDBProp.vValue.vt = DBTYPE_BSTR;
	rgDBProp.dwStatus = 0;
	hr = pIDbProperties->SetProperties(1, &rgPropSets);

	if(strPassword.IsEmpty())goto	_InitDataSource_cleanup;

	// set database password property(provider specific)
	hr = pIDbProperties->GetProperties(0, 0, &cPropertySets, &prgPropSets);
	if(FAILED(hr))goto	_InitDataSource_cleanup;

	DBPROPID dwPasswordPropID;
	dwPasswordPropID = SearchPasswordPropID(pIDbProperties);
	if(0 == dwPasswordPropID)goto	_InitDataSource_cleanup;

	for (int i = 0;i < cPropertySets;i++)
	{
		if(DBPROPSET_DBINIT == prgPropSets[i].guidPropertySet)continue;
		for (int j = 0;j < prgPropSets[i].cProperties;j++)
		{
			if(prgPropSets[i].rgProperties[j].dwPropertyID == dwPasswordPropID)
			{
				prgPropSets[i].rgProperties[j].vValue.bstrVal = SysAllocString(strPassword);
				break;
			}
		}
	}

	hr = pIDbProperties->SetProperties(cPropertySets, prgPropSets);

_InitDataSource_cleanup:
	if(0 != prgPropSets)pIMalloc->Free(prgPropSets);
	if(FAILED(hr))return hr;

	return pIDBInitialize->Initialize();
}

HRESULT Tools1::GetIIndexPtr(CComPtr<IDBInitialize> &pIDBInitialize, CStringW const& strTableName, CStringW const& strKeyName, CComPtr<IRowsetIndex>& pIIndex)
{
	if(strTableName.IsEmpty())return 0;
	if(0 == pIDBInitialize)return 0;

	CComPtr<IDBCreateSession>	pIDBCreateSession;
	CComPtr<IOpenRowset>			pIOpenRowset;
	CComPtr<IRowset>					pIRowset;
	DBID idTable, idIndex;
	HRESULT hr;
	CStringW str1, str2;

	CComPtr<IDBProperties>  pIDbProperties;
	hr = pIDBInitialize->QueryInterface(__uuidof(IDBCreateSession), (void **)&pIDBCreateSession);
	if(FAILED(hr))goto _GetRowsetPtr_cleanup;
	hr = pIDBCreateSession->CreateSession(NULL, __uuidof(IOpenRowset), (IUnknown**)&pIOpenRowset);
	if(FAILED(hr))goto _GetRowsetPtr_cleanup;

	// name of the table which contains the index column
	idTable.eKind = DBKIND_NAME;
	str1 = strTableName;
	idTable.uName.pwszName = str1.GetBuffer();

	// index name
	idIndex.eKind = DBKIND_NAME;
	str2 = strKeyName;
	idIndex.uName.pwszName = str2.GetBuffer();

	DBPROPSET dbPropSets;
	DBPROP	rgProp;

	dbPropSets.cProperties = 1;
	dbPropSets.guidPropertySet = DBPROPSET_ROWSET;
	dbPropSets.rgProperties = &rgProp;

	rgProp.colid = DB_NULLID;
	rgProp.dwOptions = DBPROPOPTIONS_REQUIRED;
	rgProp.dwPropertyID = DBPROP_IRowsetIndex;
	rgProp.dwStatus = DBPROPSTATUS_OK;
	rgProp.vValue.vt = VT_BOOL;
    V_BOOL(&(rgProp.vValue)) = VARIANT_TRUE;

	// use index here to get the last record which contains word count
	hr = pIOpenRowset->OpenRowset(NULL, &idTable, &idIndex, __uuidof(IRowsetIndex), 1, &dbPropSets, (IUnknown**)&pIIndex);

_GetRowsetPtr_cleanup:

	return hr;
}

HRESULT Tools1::GetIRowChangePtr(CComPtr<IDBInitialize> &pIDBInitialize, CStringW const& strTableName, CComPtr<IRowsetChange>& pIRowsetChange)
{
	if(0 == pIDBInitialize)return E_FAIL;

	HRESULT hr;
	CComPtr<IDBCreateSession>	pIDBCreateSession;
	CComPtr<IOpenRowset>			pIOpenRowset;
	DBID idTable;
	DBPROPSET dbPropSets;
	DBPROP	rgProp;
	CStringW str;

	hr = pIDBInitialize->QueryInterface(__uuidof(IDBCreateSession), (void**)&pIDBCreateSession);
	if(FAILED(hr))goto _GetIRowChangePtr_cleanup;
	hr = pIDBCreateSession->CreateSession(NULL, __uuidof(IOpenRowset), (IUnknown**)&pIOpenRowset);
	if(FAILED(hr))goto _GetIRowChangePtr_cleanup;

	// name of the table which contains the index column
	idTable.eKind = DBKIND_NAME;
	str = strTableName;
	idTable.uName.pwszName = str.GetBuffer();

	dbPropSets.cProperties = 1;
	dbPropSets.guidPropertySet = DBPROPSET_ROWSET;
	dbPropSets.rgProperties = &rgProp;

	rgProp.colid = DB_NULLID;
	rgProp.dwOptions = DBPROPOPTIONS_REQUIRED;
	rgProp.dwPropertyID = DBPROP_UPDATABILITY;
	rgProp.dwStatus = DBPROPSTATUS_OK;
    rgProp.vValue.vt = VT_I4;
    V_I4(&rgProp.vValue) = DBPROPVAL_UP_CHANGE|DBPROPVAL_UP_DELETE|DBPROPVAL_UP_INSERT;

	hr = pIOpenRowset->OpenRowset(NULL, &idTable, NULL, __uuidof(IRowsetChange), 1, &dbPropSets, (IUnknown**)&pIRowsetChange);

_GetIRowChangePtr_cleanup:
	return hr;
}

HRESULT Tools1::CreateDataStore(CComPtr<IDBInitialize> &pIDBInitialize, const CStringW& strProviderName, const CStringW& strFileName, const CStringW& strPassword, const USHORT uType)
{
	HRESULT hr;
	CComPtr<IDBDataSourceAdmin>		pIDBDataSourceAdmin;
	CComPtr<IDBProperties>					pIDBCreationProperties;
	CLSID clsid;
	CComPtr<IMalloc>		pIMalloc;
	DBPROPSET	*prgPropSets(0);
	ULONG	ulCreationPropSet;

	::CoGetMalloc(MEMCTX_TASK, &pIMalloc);

	if(0 != pIDBInitialize)
	{
		pIDBInitialize->Uninitialize();
		pIDBInitialize.Release();
	}
	if(strProviderName.IsEmpty())return E_FAIL;
	::CLSIDFromString(strProviderName,&clsid);
	hr = ::CoCreateInstance(clsid, NULL, CLSCTX_INPROC_SERVER, __uuidof(IDBInitialize), (void**)&pIDBInitialize);
	if(FAILED(hr))goto	_CreateDataSource_cleanup;
	hr = pIDBInitialize->QueryInterface(__uuidof(IDBDataSourceAdmin), (void**)&pIDBDataSourceAdmin);
	if(FAILED(hr))goto	_CreateDataSource_cleanup;
	hr = pIDBDataSourceAdmin->QueryInterface(__uuidof(IDBProperties), (void**)&pIDBCreationProperties);
	if(FAILED(hr))goto	_CreateDataSource_cleanup;

	// set datasource path name
	hr = pIDBCreationProperties->GetProperties(0, 0, &ulCreationPropSet, &prgPropSets);
	if(FAILED(hr))goto	_CreateDataSource_cleanup;

	for (int i = 0;i < ulCreationPropSet;i++)
	{
		if(DBPROPSET_DBINIT != prgPropSets[i].guidPropertySet)continue;
		for (int j = 0;j < prgPropSets[i].cProperties;j++)
		{
			if(DBPROP_INIT_DATASOURCE == prgPropSets[i].rgProperties[j].dwPropertyID)
			{
				prgPropSets[i].rgProperties[j].vValue.bstrVal = SysAllocString(strFileName);
				break;
			}
		}
	}
	if(strPassword.IsEmpty())goto	_CreateDataSource_nopassword_creation;

	// set datasource password if it exists
	DBPROPID dwPasswordPropID;
	dwPasswordPropID = SearchPasswordPropID(pIDBCreationProperties);
	if(0 == dwPasswordPropID)goto	_CreateDataSource_nopassword_creation;

	for (int i = 0;i < ulCreationPropSet;i++)
	{
		if(DBPROPSET_DBINIT == prgPropSets[i].guidPropertySet)continue;
		for (int j = 0;j < prgPropSets[i].cProperties;j++)
		{
			if(prgPropSets[i].rgProperties[j].dwPropertyID == dwPasswordPropID)
			{
				prgPropSets[i].rgProperties[j].vValue.bstrVal = SysAllocString(strPassword);
				break;
			}
		}
	}

_CreateDataSource_nopassword_creation:

	hr = pIDBCreationProperties->SetProperties(ulCreationPropSet, prgPropSets);
	if(FAILED(hr))goto	_CreateDataSource_cleanup;

	// create an empty datasource
	hr = pIDBDataSourceAdmin->CreateDataSource(ulCreationPropSet, prgPropSets, 0, IID_NULL, 0);
	if(FAILED(hr))goto	_CreateDataSource_cleanup;
	pIDBInitialize.Release();
	hr = pIDBDataSourceAdmin->QueryInterface(__uuidof(IDBInitialize), (void**)&pIDBInitialize);
	if(FAILED(hr))goto	_CreateDataSource_cleanup;

	// create table,add column and create index
	if(CVocabConst::PROVIDERTYPE_MS_SSCE == uType)
	{
		CreateTableInSqlCompact(pIDBInitialize);
	}
	if(CVocabConst::PROVIDERTYPE_MS_JET == uType)
	{
		CStringW str = strProviderName;
		CreateTableInAccessDB(pIDBInitialize);
		CheckProviderSchemaInfo(pIDBInitialize, str);

		for (int i = 0;i < CVocabConst::COLUMN_CNT;i++)
		{
			hr = AddColumnInAccessDB(pIDBInitialize, CVocabConst::NAME_TABLE_WORD, CVocabConst::NAMES_COLUMN[i], CVocabConst::COLUMN_TYPE[i], CVocabConst::COLUMN_SIZE[i]);
			if(FAILED(hr))break;
		}
		if(FAILED(hr))goto	_CreateDataSource_cleanup;
	}
	hr = CreateIndex(pIDBInitialize, CVocabConst::NAME_TABLE_WORD, CVocabConst::NAMES_COLUMN[0]);

_CreateDataSource_cleanup:
	if(0 != prgPropSets)pIMalloc->Free(prgPropSets);
	
	return hr;
}

HRESULT Tools1::CreateTableInSqlCompact(CComPtr<IDBInitialize> &pIDBInitialize)
{
	if(0 == pIDBInitialize)return E_FAIL;

	HRESULT hr;
	CComPtr<ITableDefinition>		pITableDefinition;
	CComPtr<IDBCreateSession>	pIDBCreateSession;
	DBID idTable,*pidNewTable(0);
	DBCOLUMNDESC		rgColumnDescs[CVocabConst::COLUMN_CNT];
	CStringW strTypeName;

	hr = pIDBInitialize->QueryInterface(__uuidof(IDBCreateSession), (void**)&pIDBCreateSession);
	if(FAILED(hr))return hr;
	hr = pIDBCreateSession->CreateSession(NULL, __uuidof(ITableDefinition), (IUnknown**)&pITableDefinition);
	if(FAILED(hr))return hr;

	idTable.eKind = DBKIND_NAME;
	idTable.uName.pwszName = CVocabConst::NAME_TABLE_WORD;

	for (int i = 0;i < CVocabConst::COLUMN_CNT;i++)
	{
		rgColumnDescs[i].pwszTypeName = 0;
		rgColumnDescs[i].cPropertySets = 0;
		rgColumnDescs[i].rgPropertySets = 0;
		
		rgColumnDescs[i].bPrecision = 0;
		rgColumnDescs[i].bScale = 0;
		rgColumnDescs[i].wType = CVocabConst::COLUMN_TYPE[i];
		rgColumnDescs[i].ulColumnSize = CVocabConst::COLUMN_SIZE[i];
		rgColumnDescs[i].pTypeInfo = 0;
		rgColumnDescs[i].pclsid = 0;
		rgColumnDescs[i].dbcid.eKind = DBKIND_NAME;
		rgColumnDescs[i].dbcid.uName.pwszName = CVocabConst::NAMES_COLUMN[i];
	}

	// if provider-specific type name is obtained,use it
	HRESULT hRet = CheckProviderSchemaInfo(pIDBInitialize, strTypeName);;
	if(S_OK == hRet)
	{
		for (int i = CVocabConst::FIXED_LEN_COL_CNT/* the types of first 2 column are int */;i < CVocabConst::COLUMN_CNT;i++)
		{
			rgColumnDescs[i].ulColumnSize = 0;
			rgColumnDescs[i].pwszTypeName = strTypeName.GetBuffer();
		}
	}

	// creation of table with no column is not allowed in SSCE database
	hr = pITableDefinition->CreateTable(0, &idTable, CVocabConst::COLUMN_CNT, rgColumnDescs, IID_NULL, 0, 0, &pidNewTable, 0);

	return hr;
}

HRESULT Tools1::CreateTableInAccessDB(CComPtr<IDBInitialize> &pIDBInitialize)
{
	if(0 == pIDBInitialize)return E_FAIL;

	HRESULT hr;
	CComPtr<ITableDefinition>		pITableDefinition;
	CComPtr<IDBCreateSession>	pIDBCreateSession;
	DBID idTable,*pidNewTable(0);

	hr = pIDBInitialize->QueryInterface(__uuidof(IDBCreateSession), (void**)&pIDBCreateSession);
	if(FAILED(hr))return hr;
	hr = pIDBCreateSession->CreateSession(NULL, __uuidof(ITableDefinition), (IUnknown**)&pITableDefinition);
	if(FAILED(hr))return hr;

	idTable.eKind = DBKIND_NAME;
	idTable.uName.pwszName = CVocabConst::NAME_TABLE_WORD;

	// empty table can be created in Microsoft Jet DB
	hr = pITableDefinition->CreateTable(0, &idTable, 0, 0, IID_NULL, 0, 0, &pidNewTable, 0);
	if(FAILED(hr))return hr;

	return hr;
}

HRESULT Tools1::CreateIndex(CComPtr<IDBInitialize> &pIDBInitialize, LPOLESTR wszTableName, LPOLESTR wszColumnName)
{
	if(0 == pIDBInitialize)return E_FAIL;

	HRESULT hr;
	CComPtr<IIndexDefinition>		pIIndexDefinition;
	CComPtr<IDBCreateSession>	pIDBCreateSession;
	CComPtr<IOpenRowset>			pIOpenRowset;
	CComPtr<IRowset>					pIRowset;
	CComPtr<IRowsetIndex>			pIRowsetIndex;
	CComPtr<IAccessor>				pIAccessor;

	DBID idTable,idIndex;
	DBID dbidBaseTableCol;
	DBINDEXCOLUMNDESC		indexColDesc;

	hr = pIDBInitialize->QueryInterface(__uuidof(IDBCreateSession), (void**)&pIDBCreateSession);
	if(FAILED(hr))return hr;
	hr = pIDBCreateSession->CreateSession(NULL, __uuidof(IOpenRowset), (IUnknown**)&pIOpenRowset);
	if(FAILED(hr))return hr;
	hr = pIDBCreateSession->CreateSession(NULL, __uuidof(IIndexDefinition), (IUnknown**)&pIIndexDefinition);
	if(FAILED(hr))return hr;
	
	// name of the table which contains the index column
	idTable.eKind = DBKIND_NAME;
	idTable.uName.pwszName = wszTableName;

	// index name
	idIndex.eKind = DBKIND_NAME;
	idIndex.uName.pwszName = CVocabConst::NAME_INDEX_MAJOR;

	// name of the column on which index will be created
	dbidBaseTableCol.eKind = DBKIND_NAME;
	dbidBaseTableCol.uName.pwszName = wszColumnName;

	indexColDesc.eIndexColOrder = DBINDEX_COL_ORDER_ASC;
	indexColDesc.pColumnID = &dbidBaseTableCol;
	
	DBPROP	rgProp;
	rgProp.colid = DB_NULLID;
	rgProp.dwOptions = DBPROPOPTIONS_REQUIRED;
	rgProp.dwPropertyID = DBPROP_IRowsetIndex;
	rgProp.dwStatus = DBPROPSTATUS_OK;
	V_VT(&(rgProp.vValue)) = VT_BOOL;

	return pIIndexDefinition->CreateIndex(&idTable, &idIndex, 1, &indexColDesc, 0, 0, 0);
}

HRESULT Tools1::AddColumnInAccessDB(CComPtr<IDBInitialize>	&pIDBInitialize, LPOLESTR wszTableName, LPOLESTR wszColumnName, DBTYPE wColumnType, DBLENGTH ulColunmSize)
{
	if(0 == pIDBInitialize)return E_FAIL;

	HRESULT hr;
	DBCOLUMNDESC		rgColumnDesc;
	DBID idTable;
	CComPtr<ITableDefinition>		pITableDefinition;
	CComPtr<IDBCreateSession>	pIDBCreateSession;
	CStringW strTypeName;

	hr = pIDBInitialize->QueryInterface(__uuidof(IDBCreateSession), (void**)&pIDBCreateSession);
	if(FAILED(hr))return hr;
	hr = pIDBCreateSession->CreateSession(NULL, __uuidof(ITableDefinition), (IUnknown**)&pITableDefinition);
	if(FAILED(hr))return hr;

	idTable.eKind = DBKIND_NAME;
	idTable.uName.pwszName = wszTableName;
	
	rgColumnDesc.pwszTypeName = 0;
	rgColumnDesc.ulColumnSize = ulColunmSize;
	rgColumnDesc.cPropertySets = 0;
	rgColumnDesc.rgPropertySets = 0;
	rgColumnDesc.wType = wColumnType;
	rgColumnDesc.pTypeInfo = 0;
	rgColumnDesc.bPrecision = 0;
	rgColumnDesc.bScale = 0;
	rgColumnDesc.pclsid = 0;
	rgColumnDesc.dbcid.eKind = DBKIND_NAME;
	rgColumnDesc.dbcid.uName.pwszName = wszColumnName;

	hr = CheckProviderSchemaInfo(pIDBInitialize, strTypeName);
	if(DBTYPE_WSTR == wColumnType)
	{
		if(!strTypeName.IsEmpty())
		{
			rgColumnDesc.pwszTypeName = strTypeName.GetBuffer();
			rgColumnDesc.ulColumnSize = 0;
		}
	}
	
	return pITableDefinition->AddColumn(&idTable, &rgColumnDesc, 0);
}

HRESULT Tools1::LookupErrorMsg(CStringW& strErrMsg)
{
	HRESULT hr;
	BSTR pwsz;
	CComPtr<IErrorInfo>	pIErrorInfo(0);

	hr = ::GetErrorInfo(0, &pIErrorInfo);
	if(0 != pIErrorInfo)
	{
		hr = pIErrorInfo->GetDescription(&pwsz);
		if(S_OK == hr)strErrMsg = pwsz;
	}

	return hr;
}

void Tools1::Convert2String(int n, CStringW& str)
{
	WCHAR chr;
	int n2;
	str.Empty();
	for (int i = 0;i < 3;i++)
	{
		n2 = n / 10 * 10;
		n2 = n - n2;
		chr = n2 + 0x30;
		str = str + chr;
		n = n / 10;
	}
	str.MakeReverse();
	while(1 < str.GetLength() && 0x30 == str[0])
	{
		str = str.Right(str.GetLength() - 1);
	}
}

HANDLE hStdOut = 0;

void IdleProcess(bool state) {
	// TODO
	static int cnt = 0;
	WCHAR tmp[64] = { 0 };
	swprintf_s(tmp, 64, L"Idle Process. Message Pump State: %s.", state ? L"In System" : L"Normal");
	WriteConsoleW(hStdOut, tmp, static_cast<DWORD>(wcslen(tmp)), 0, 0);
	for (int i = 0; i < cnt; ++i)
		tmp[i] = L'.';
	tmp[cnt] = '\n';
	tmp[cnt + 1] = '\0';
	cnt = (cnt + 1) % 16;
	WriteConsoleW(hStdOut, tmp, static_cast<DWORD>(wcslen(tmp)), 0, 0);
	return;
}

//LPVOID lpMsgBuf;
//::FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER | 
//	FORMAT_MESSAGE_FROM_SYSTEM | 
//	FORMAT_MESSAGE_IGNORE_INSERTS,
//	0,GetLastError(),0, (LPTSTR) &lpMsgBuf, 0,NULL);
//::OutputDebugStringW((LPCWSTR)lpMsgBuf);
//::LocalFree(lpMsgBuf);

//	static int n=0;
//	WCHAR szBuff[255];
//	::_itow_s(n++,szBuff,16);
//	::OutputDebugStringW(L"\n");
//	::OutputDebugStringW(szBuff);

