#define _AFXDLL
#include "stdafx.h"
#include "FieldStatements.h"
#include <afxdao.h>
#include <wx/textctrl.h>

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#define new DEBUG_NEW
#endif

CString CFieldStatements::GetDaoFieldDescription(CString &strFieldName, CString &strTableName, CDaoDatabase &db) 
{ 
    ASSERT(db.m_pDAODatabase != NULL); 
    ASSERT(!strTableName.IsEmpty()); 
    ASSERT(!strFieldName.IsEmpty()); 
    ASSERT(db.IsOpen()); 
    COleVariant varProp(_T("Description"), VT_BSTRT); 
    COleVariant varTable(strTableName, VT_BSTRT); 
    COleVariant varField(strFieldName, VT_BSTRT); 
    DAODatabase* piDB = db.m_pDAODatabase; 
    CComPtr<DAOProperties>piPC; 
    CComPtr<DAOTableDefs>piTDC; 
    CComPtr<DAOTableDef>piTD; 
    CComPtr<DAOProperty>piP; 
    CComPtr<DAOFields>piFC; 
    CComPtr<DAOField>piF; 
    COleVariant varValue; 
    CString strT; 
    try 
    { 
        DAO_CHECK(piDB->get_TableDefs(&piTDC)); 
        DAO_CHECK(piTDC->get_Item(varTable, &piTD)); 
        DAO_CHECK(piTD->get_Fields(&piFC)); 
        DAO_CHECK(piFC->get_Item(varField, &piF)); 
        DAO_CHECK(piF->get_Properties(&piPC)); 
        DAO_CHECK(piPC->get_Item(varProp, &piP)); 
        DAO_CHECK(piP->get_Value(&varValue)); 
        if (varValue.vt == VT_BSTR) 
            strT = V_BSTRT(&varValue); 
    } 
    catch(CDaoException* e) 
    { 
        e->Delete(); 
    } 
    return strT; 
} 

bool CFieldStatements::GetPrimaryKey(CString &sTableName, CString &sFieldName,std::vector<CString> &indexinfo)
{
	CString temp = sTableName;
	temp += sFieldName;

	for(const auto &it : indexinfo) 
		if( !(temp.Compare(it)) )
			return true;

	return false;
}

wxString CFieldStatements::CstringToWxString(const CString &ConversionString)
{
	CT2CA pszConvertedAnsiString (ConversionString);
	std::string strStd (pszConvertedAnsiString);
	return wxString::FromUTF8(_strdup(strStd.c_str() ) );
}

void CFieldStatements::fFields(CDaoDatabase &db, CDaoTableDef &TableDef, CDaoTableDefInfo &tabledefinfo, std::vector<CString> &InsertStatements, std::vector<CString> &UniqueFields, 
	CSettings &settings, CString &sStatement,CString (&ReservedKeyWords)[124], std::vector<CString> &TableField, std::vector<CString> &IndexInfo, unsigned &nWarningCount, wxTextCtrl *PrgDlg /*= NULL*/)
{
		size_t nFieldCount = TableDef.GetFieldCount();
		std::vector<CString> sFieldnames; 
		bool bIsText;
		CString Temp2;
		for(size_t i1 = 0; i1 < nFieldCount; ++i1)
		{
			bIsText = false;
			CDaoFieldInfo fieldinfo;                                          
			TableDef.GetFieldInfo(i1, fieldinfo, AFX_DAO_ALL_INFO);
			bool isPrimary = CFieldStatements::GetPrimaryKey(tabledefinfo.m_strName,fieldinfo.m_strName,IndexInfo);
			if( !isPrimary ) 
			{
				CString Temp = tabledefinfo.m_strName;
				Temp += fieldinfo.m_strName;
				TableField.push_back(Temp);
			}

			if( settings.m_bTrimTextValues ) sFieldnames.push_back(fieldinfo.m_strName.TrimRight().TrimLeft()); 
			else sFieldnames.push_back(fieldinfo.m_strName);

			sStatement += _T("`");

			if( settings.m_bTrimTextValues ) sStatement += fieldinfo.m_strName.TrimLeft().TrimRight();
			else sStatement += fieldinfo.m_strName;

			if( settings.m_bKeyWordList && PrgDlg != NULL )
			{
				for(size_t i2 = 0; i2 < 124; ++i2 )
				{
					if( !(fieldinfo.m_strName.CompareNoCase(ReservedKeyWords[i2])) )
					{
							++nWarningCount;
							wxString ErrorMessage = wxT("WARNING: found field name sqlite keyword this could lead to unexpected behaviour, found on table: ");
							PrgDlg->SetDefaultStyle(wxTextAttr (wxNullColour, *wxYELLOW));
							ErrorMessage += CstringToWxString(tabledefinfo.m_strName);
							ErrorMessage += wxT(" field name: ");
							ErrorMessage += CstringToWxString(fieldinfo.m_strName);
							ErrorMessage += wxT("\n");
							PrgDlg->WriteText(ErrorMessage);
							PrgDlg->SetDefaultStyle(wxTextAttr (wxNullColour));
					}
				}
			}
			sStatement += (_T("` ")); 
			CDaoRecordset rc;
			rc.Open(&TableDef);
			CDaoFieldInfo recordinfo;
			rc.GetFieldInfo(i1,recordinfo);
			FieldTypeAdd(TableDef, recordinfo.m_nType, bIsText, sStatement);
			if( settings.m_bNotNullAdd )  
				NotNullAdd(fieldinfo, sStatement);
			if( settings.m_bDefaultValueAdd ) 
			{
				if( PrgDlg != NULL )
					DefaultValueAdd(fieldinfo, tabledefinfo, sStatement, recordinfo, nWarningCount, PrgDlg);
				else DefaultValueAdd(fieldinfo, tabledefinfo, sStatement, recordinfo, nWarningCount);
			}
			if( settings.m_bAutoIncrementAdd )
				AutoIncrementAdd(fieldinfo, sStatement);
			if( (!settings.m_bAutoIncrementAdd && isPrimary && settings.m_PrimaryKeySupport) || (isPrimary && settings.m_PrimaryKeySupport && !(fieldinfo.m_lAttributes & dbAutoIncrField)) )
				sStatement += _T(" PRIMARY KEY");
				
			if( settings.m_bUniqueFieldAdd ) 
			{
				CString sCmp = tabledefinfo.m_strName + fieldinfo.m_strName;
				UniqueFieldAdd(sCmp, UniqueFields, sStatement);
			}

			if( bIsText && settings.m_bCollateNoCaseFieldsAdd )
				sStatement += _T(" COLLATE NOCASE");
				if( settings.m_bAddComments )
			{
				Temp2 = GetDaoFieldDescription(fieldinfo.m_strName, tabledefinfo.m_strName, db);
				if( !Temp2.IsEmpty() )
				{
					sStatement += _T(" /*");
					sStatement += Temp2;
					sStatement += _T(" */");
				}
			}
			if( i1 != nFieldCount - 1 ) sStatement += (_T(","));
			else sStatement+= (_T(");"));
					
		}
	if( settings.m_bRecordAdd ) {
		Records(TableDef, tabledefinfo.m_strName, sFieldnames, InsertStatements);
	}
}

void CFieldStatements::NotNullAdd(const CDaoFieldInfo &fieldinfo,CString &sStatement)
{
	 if( fieldinfo.m_bRequired )
		sStatement += _T(" NOT NULL");
}

void CFieldStatements::AutoIncrementAdd(const CDaoFieldInfo &fieldinfo, CString &sStatement)
{
	if( fieldinfo.m_lAttributes & dbAutoIncrField ) 
		sStatement += _T(" PRIMARY KEY AUTOINCREMENT");
}

void CFieldStatements::DefaultValueAdd( const CDaoFieldInfo &fieldinfo, CDaoTableDefInfo &tabledefinfo, CString &sStatement, CDaoFieldInfo &recordinfo, unsigned &nWarningCount, 
	                                    wxTextCtrl *PrgDlg /*NULL*/)
{
	if( !(fieldinfo.m_strDefaultValue.IsEmpty()) )
	{
		if ( recordinfo.m_nType == dbBoolean )
		{
			if( !(fieldinfo.m_strDefaultValue.CompareNoCase(_T("Yes"))) )
			{
				sStatement += _T(" DEFAULT 1");
				return;
			}
			if( !(fieldinfo.m_strDefaultValue.CompareNoCase(_T("No"))) )
			{
				sStatement += _T(" DEFAULT 0");
				return;
			}
			if( !(fieldinfo.m_strDefaultValue.CompareNoCase(_T("true"))) )
			{
				sStatement += _T(" DEFAULT 1");
				return;
			}
			if( !(fieldinfo.m_strDefaultValue.CollateNoCase(_T("false"))) )
			{
				sStatement += _T(" DEFAULT 0");
				return;
			}
			if( !(fieldinfo.m_strDefaultValue.CollateNoCase(_T("1"))) )
			{
				sStatement += _T(" DEFAULT 1");
				return;
			}
			if( !(fieldinfo.m_strDefaultValue.CollateNoCase(_T("0"))) )
			{
				sStatement += _T(" DEFAULT 0");
				return;
			}
			if(PrgDlg != NULL)
			{
				++nWarningCount;
				PrgDlg->SetDefaultStyle(wxTextAttr (*wxRED));
				wxString ErrorMessage = wxT("Unable to recognize default value: ");
				ErrorMessage += CstringToWxString(fieldinfo.m_strDefaultValue);
				ErrorMessage += wxT(" on table: ");
				ErrorMessage += CstringToWxString(tabledefinfo.m_strName);
				ErrorMessage += wxT(" column: ");
				ErrorMessage += CstringToWxString(fieldinfo.m_strName);
				ErrorMessage += wxT("\n");
				PrgDlg->WriteText(ErrorMessage);
				PrgDlg->SetDefaultStyle(wxTextAttr (wxNullColour));
			}
		}
		else 
		{
			sStatement += _T(" DEFAULT ");
			sStatement += fieldinfo.m_strDefaultValue;
		}
	}
}
void CFieldStatements::FieldTypeAdd(CDaoTableDef &TableDef, const short nType, bool &bIsText, CString &sStatement)
{
	CDaoRecordset recordset;
	recordset.Open(&TableDef);
	CString sInt = L"INTEGER";
	CString sReal = L"REAL";
	CString sText = L"TEXT";
	CString sBlob = L"BLOB";

	switch(nType)
	{
		case dbBoolean: 
				sStatement += sInt;
				break;
		case dbByte: 
				sStatement += sInt; 
				break;
		case dbInteger: 
				sStatement += sInt;  
				break;
		case dbLong : 
				sStatement += sInt;  
				break;
		case dbCurrency: 
				sStatement += sInt; 
				break;
		case dbSingle: 
				sStatement += sInt; 
				break;
		case dbDouble: 
				sStatement += sReal; 
				break;
		case dbDate: 
				sStatement += sInt; 
				break;
		case dbBinary: 
				sStatement += sBlob; 
				break;
		case dbText: 
				sStatement += sText; 
				bIsText = true; 
				break;
		case dbLongBinary: 
				sStatement += sBlob;  
				break;
		case dbMemo: 
				sStatement += sText; 
				bIsText = true;
				break;
		case dbGUID: 
				ASSERT(FALSE); 
				sStatement += sText;  
				bIsText = true; 
				break;
		case dbBigInt: 
				sStatement += sInt;  
				break;
		case dbVarBinary: 
				sStatement += sBlob; 
				break;
		case dbChar: 
				sStatement += sText; 
				bIsText = true; 
				break;
		case dbNumeric: 
				sStatement += sInt;
				break;
		case dbDecimal: 
				sStatement += sInt; 
				break;
		case dbFloat: 
				sStatement += sReal; 
				break;
		case dbTime: 
				sStatement += sInt; 
				break;
		case dbTimeStamp: 
				ASSERT(FALSE); 
				sStatement += sInt; 
				break;
		default:
				break;
	}
}

void CFieldStatements::UniqueFieldAdd(const CString &sCmp, std::vector<CString> &UniqueFields, CString &sStatement)
{
	for(const auto &it : UniqueFields) {
		if( !(sCmp.Compare(it)) )
		{
			sStatement += _T(" UNIQUE");
			break;
		}
	}
}

void CFieldStatements::Records(CDaoTableDef &TableDef, LPCTSTR sTable, std::vector<CString> &sFieldNames, std::vector <CString> &InsertStatements)
{
		COleVariant COlevar;                                        
	    CDaoRecordset recordset;
	    recordset.Open(&TableDef);
	    CString sParrent = L"INSERT INTO   `";
	    sParrent += sTable;
	    sParrent += L"`  (";
		size_t nFieldCount = sFieldNames.size();

		for(size_t i1 = 0; i1 < nFieldCount; ++i1)
		{
				sParrent += L"`";
				sParrent += sFieldNames[i1]; 
				if( i1 != nFieldCount - 1 ) sParrent += L"`, ";
				else sParrent += L"`)";
		}

		CString sStatement;
		InsertStatements.push_back(L"BEGIN TRANSACTION");

		while( !recordset.IsEOF() )
		{ 
			sStatement = sParrent;
			sStatement += L" VALUES (";
			for(size_t i2 = 0; i2 < nFieldCount; ++i2)
			{
				recordset.GetFieldValue(sFieldNames[i2], COlevar);
				if( COlevar.vt == VT_NULL ) sStatement += L"NULL";
				else if( COlevar.vt == VT_I2 || COlevar.vt == VT_I4 || COlevar.vt == VT_R4 || COlevar.vt == VT_R8 || COlevar.vt == VT_BOOL || COlevar.vt == VT_CY || COlevar.vt == VT_UI2 || COlevar.vt == VT_UI4 || COlevar.vt ==  VT_I8 || COlevar.vt == VT_UI8 || COlevar.vt == VT_INT || COlevar.vt ==  VT_UINT || COlevar.vt == VT_BLOB ) sStatement += COlevar;
				else 
				{
					sStatement += L"'";
					CString sString = COlevar;
					sString.Replace(L"'", L"''");
					sStatement += sString;
					sStatement += L"'";
				}
				if( i2 != nFieldCount - 1 ) sStatement += L", ";
				else 
				{ 
					sStatement += L");";
					InsertStatements.push_back(sStatement);
				}
			}

			recordset.MoveNext(); 
		}

		InsertStatements.push_back(_T("END TRANSACTION"));          
}

void CFieldStatements::FieldCollation(CDaoTableDef &TableDef, CDaoTableDefInfo &tabledefinfo, std::vector<CString> &CollateIndexFields, const bool &m_bTrimTextValues)
{
	size_t nFieldCount = TableDef.GetFieldCount(); 
	CString sStatement;

	for(size_t i1 = 0; i1 < nFieldCount; ++i1)
	{
		CDaoFieldInfo fieldinfo;                                          
		TableDef.GetFieldInfo(i1, fieldinfo, AFX_DAO_ALL_INFO); 
		CDaoRecordset rc;
		rc.Open(&TableDef);
		CDaoFieldInfo recordinfo;
		rc.GetFieldInfo(i1, recordinfo);
		switch( recordinfo.m_nType )
		{
			case dbText: 
					sStatement = tabledefinfo.m_strName;  
					m_bTrimTextValues? sStatement += fieldinfo.m_strName.TrimLeft().TrimRight() : sStatement += fieldinfo.m_strName ;
					break;
			case dbMemo: 
					sStatement = tabledefinfo.m_strName; 
					m_bTrimTextValues? sStatement += fieldinfo.m_strName.TrimLeft().TrimRight() : sStatement += fieldinfo.m_strName ; 
					break;
			case dbGUID: 
					sStatement = tabledefinfo.m_strName;
					m_bTrimTextValues? sStatement += fieldinfo.m_strName.TrimLeft().TrimRight() : sStatement += fieldinfo.m_strName ;
					break;
            case dbChar: 
					sStatement = tabledefinfo.m_strName;
					m_bTrimTextValues? sStatement += fieldinfo.m_strName.TrimLeft().TrimRight() : sStatement += fieldinfo.m_strName ;
					break;
			default: 
					break;
		}

		CollateIndexFields.push_back(sStatement);
	}
}
