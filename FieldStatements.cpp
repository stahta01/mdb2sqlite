#pragma once

#include "stdafx.h"
#include "FieldStatements.h"
#include "naujastestas.h"
#include <afxdao.h>
#include <afxdb.h>
#include <vector>

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#define new DEBUG_NEW
#endif

#pragma warning(disable : 4995)

void CFieldStatements::fFields(CDaoTableDef &TableDef, CDaoTableDefInfo &tabledefinfo, std::vector <CString> &statements, std::vector <CString> &UniqueFields, CSettings &settings)
{
	 
	short nFieldCount = TableDef.GetFieldCount();        // Counts how manys fields there are in the database of a current sellected table
	CString *sFieldnames = new CString[nFieldCount];     // creating dynamic array of field names
	bool bIsText;
	for(int i1 = 0; i1 < nFieldCount; ++i1)
			  {
				  bIsText = false;
				  CDaoFieldInfo fieldinfo;                                          // Create a fieldinfo object in which we will store information about the field
				  TableDef.GetFieldInfo(i1,fieldinfo,AFX_DAO_ALL_INFO);              // We store the information about the j-th field
				  if(settings.m_bTrimTextValues) 
					  sFieldnames[i1] = fieldinfo.m_strName.TrimRight().TrimLeft(); 
				  else sFieldnames[i1] = fieldinfo.m_strName;
				  statements.back() += _T("      `");
				  if(settings.m_bTrimTextValues)
					  statements.back() += fieldinfo.m_strName.TrimLeft().TrimRight();
				  else statements.back() += fieldinfo.m_strName;
				  statements.back() += (_T("`     ")); 
				  CDaoRecordset rc;
				  rc.Open(&TableDef);
				  CDaoFieldInfo recordinfo;
				  rc.GetFieldInfo(i1,recordinfo);
				  if(settings.m_bFieldTypeAdd) 
				  { 
                     #pragma deprecated(FieldTypeAdd)
				     FieldTypeAdd(TableDef,recordinfo,statements,bIsText);
				  }
				  if(settings.m_bNotNullAdd)  
					  {
						  #pragma deprecated(NotNullAdd)
						  NotNullAdd(fieldinfo,statements);
				      }
				  if(settings.m_bDefaultValueAdd) 
					  {
						  #pragma deprecated(DefaultValueAdd)
						  DefaultValueAdd(fieldinfo,statements);
				      } 
				  if(settings.m_bAutoIncrementAdd)
	                    {
							#pragma deprecated(AutoIncrementAdd)
							AutoIncrementAdd(fieldinfo,statements);
				        }
				  if(settings.m_bUniqueFieldAdd) 
					  {
						  #pragma deprecated(UniqueFieldAdd)
						  UniqueFieldAdd(statements,fieldinfo,tabledefinfo,UniqueFields);
				       }
				  if(bIsText && settings.m_bCollateNoCaseFieldsAdd)
					  statements.back() += _T(" COLLATE NOCASE");
					if(i1 != nFieldCount-1)
						statements.back() += (_T(","));
			    	else
						statements.back() += (_T(");"));
			  }
	if(settings.m_bRecordAdd) 
		{
			#pragma deprecated(Records)
			Records(TableDef,tabledefinfo,nFieldCount,sFieldnames,statements);
	    }
}
	void CFieldStatements::NotNullAdd(const CDaoFieldInfo &fieldinfo, std::vector <CString> &statements)
{
	 if(fieldinfo.m_bRequired)
		statements.back() += _T(" NOT NULL");
}
void CFieldStatements::AutoIncrementAdd( const CDaoFieldInfo &fieldinfo, std::vector <CString> &statements)
{
	if(fieldinfo.m_lAttributes & dbAutoIncrField) // Checking if it should be autoincremented
					   statements.back() += _T(" PRIMARY KEY AUTOINCREMENT");
}
void CFieldStatements::DefaultValueAdd( const CDaoFieldInfo &fieldinfo, std::vector <CString> &statements)
{
	if(fieldinfo.m_strDefaultValue != "")
				   {
				   statements.back() += _T(" DEFAULT ");
				   statements.back() += (LPCTSTR)fieldinfo.m_strDefaultValue;
				   }
}
void CFieldStatements::FieldTypeAdd(CDaoTableDef &TableDef, const CDaoFieldInfo &recordinfo, std::vector <CString> &statements, bool &bIsText)
{
	              CDaoRecordset recordset;
			      recordset.Open(&TableDef);
				   switch(recordinfo.m_nType)
				     {
				        case dbBoolean: statements.back() += (_T("INTEGER"));break;
				        case dbByte: statements.back() += (_T("INTEGER"));  break;
				        case dbInteger: statements.back() += (_T("INTEGER"));  break;
						case dbLong : statements.back() += (_T("INTEGER"));  break;
			   	        case dbCurrency: statements.back() += (_T("INTEGER"));  break;
				        case dbSingle: statements.back() += (_T("INTEGER"));  break;
				        case dbDouble: statements.back() += (_T("REAL"));  break;
				        case dbDate: statements.back() += (_T("INTEGER")); break;
						case dbBinary: statements.back() += (_T("BLOB")); break;
				        case dbText: statements.back() += (_T("TEXT")); bIsText = true; break;
						case dbLongBinary: statements.back() += (_T("BLOB"));  break;
						case dbMemo: statements.back() += (_T("TEXT"));  bIsText = true; break;
						case dbGUID: ASSERT(FALSE); statements.back() += (_T("TEXT"));  bIsText = true; break;
						case dbBigInt: statements.back() += (_T("INTEGER"));  break;
						case dbVarBinary: statements.back() += (_T("BLOB"));  break;
						case dbChar: statements.back() += (_T("TEXT"));  bIsText = true; break;
						case dbNumeric: statements.back() += (_T("INTEGER"));  break;
						case dbDecimal: statements.back() += (_T("INTEGER"));  break;
						case dbFloat: statements.back() += (_T("REAL"));  break;
						case dbTime: statements.back() += (_T("INTEGER"));  break;
						case dbTimeStamp: ASSERT(FALSE); statements.back() += (_T("INTEGER"));  break;
				        default: break;
				      }
}
void CFieldStatements::UniqueFieldAdd(std::vector <CString> &statements, const CDaoFieldInfo &fieldinfo, const CDaoTableDefInfo &tabledefinfo, std::vector <CString> &UniqueFields)
{
	CString temp = tabledefinfo.m_strName;
	temp += fieldinfo.m_strName;
	unsigned nVectorSize = UniqueFields.size();
	for(unsigned i2 = 0; i2 < nVectorSize; ++i2)
	{
		if(!(temp.Compare(UniqueFields[i2])))
		{
			statements.back() += _T(" UNIQUE");
			break;
		}
	}
}
void CFieldStatements::Records(CDaoTableDef &TableDef, const CDaoTableDefInfo &tabledefinfo, short nFieldCount, CString *&sFieldnames, std::vector <CString> &statements)
{
		COleVariant COlevar;                                        // variable which stores variables of all types
	    CDaoRecordset recordset;
	    recordset.Open(&TableDef);
	    CString sParrent;
	    sParrent = _T("INSERT INTO   `");
	    sParrent += ((LPCTSTR)tabledefinfo.m_strName);
	    sParrent += _T("`  (");
		  for(int i1 = 0; i1 < nFieldCount; ++i1)
	      {
			  sParrent += _T("`");
			  sParrent += sFieldnames[i1]; 
			  if(i1 != nFieldCount-1)
			  	 sParrent += _T("`, ");
			  else sParrent += "`)";
		  }
			  statements.push_back(_T("BEGIN TRANSACTION"));
			  while(!recordset.IsEOF())
			     { 
					statements.push_back(sParrent);
			      	statements.back() += _T(" VALUES (");
			    	for(int i2 = 0; i2 < nFieldCount; ++i2)
			        	{
				        	recordset.GetFieldValue(sFieldnames[i2], COlevar);
							if(COlevar.vt == VT_NULL)
                               statements.back() += _T("NULL");
							else if(COlevar.vt == VT_I2 || COlevar.vt == VT_I4 || COlevar.vt == VT_R4 || COlevar.vt == VT_R8 || COlevar.vt == VT_BOOL || COlevar.vt == VT_CY || COlevar.vt == VT_UI2 || COlevar.vt == VT_UI4 || COlevar.vt ==  VT_I8 || COlevar.vt == VT_UI8 || COlevar.vt == VT_INT || COlevar.vt ==  VT_UINT || COlevar.vt == VT_BLOB)
                                  statements.back() += COlevar;
							else 
							{
								statements.back() += _T("'");
								CString sString = COlevar;
								sString.Replace(_T("'"), _T("''"));
								statements.back() += sString;
								statements.back() += _T("'");
							}
				        	if(i2 != nFieldCount - 1)
					        	statements.back() += _T(", ");
					        else statements.back() += _T(");");
				        }
					recordset.MoveNext(); 
		         }
			  statements.push_back(_T("END TRANSACTION"));
			  delete[] sFieldnames;           // deleting dynamic array
}
void CFieldStatements::FieldCollation(CDaoTableDef &TableDef, CDaoTableDefInfo &tabledefinfo, std::vector <CString> &CollateIndexFields, const bool &m_bTrimTextValues)
{
	TableDef.Open(tabledefinfo.m_strName);
	short nFieldCount = TableDef.GetFieldCount(); 
	for(int i1 = 0; i1 < nFieldCount; ++i1)
			  {
				  CDaoFieldInfo fieldinfo;                                          
				  TableDef.GetFieldInfo(i1,fieldinfo,AFX_DAO_ALL_INFO); 
				  CDaoRecordset rc;
				  rc.Open(&TableDef);
				  CDaoFieldInfo recordinfo;
				  rc.GetFieldInfo(i1,recordinfo);
				  switch(recordinfo.m_nType)
				  {
				  case dbText: CollateIndexFields.push_back(tabledefinfo.m_strName);  m_bTrimTextValues? CollateIndexFields.back() += fieldinfo.m_strName.TrimLeft().TrimRight() : CollateIndexFields.back() += fieldinfo.m_strName ; break;
				        case dbMemo: CollateIndexFields.push_back(tabledefinfo.m_strName); m_bTrimTextValues? CollateIndexFields.back() += fieldinfo.m_strName.TrimLeft().TrimRight() : CollateIndexFields.back() += fieldinfo.m_strName ; break;
						case dbGUID: CollateIndexFields.push_back(tabledefinfo.m_strName); m_bTrimTextValues? CollateIndexFields.back() += fieldinfo.m_strName.TrimLeft().TrimRight() : CollateIndexFields.back() += fieldinfo.m_strName ; break;
                        case dbChar: CollateIndexFields.push_back(tabledefinfo.m_strName); m_bTrimTextValues? CollateIndexFields.back() += fieldinfo.m_strName.TrimLeft().TrimRight() : CollateIndexFields.back() += fieldinfo.m_strName ; break;
						default: break;
				  }
	          }
}