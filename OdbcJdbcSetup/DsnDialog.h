/*
 *  
 *     The contents of this file are subject to the Initial 
 *     Developer's Public License Version 1.0 (the "License"); 
 *     you may not use this file except in compliance with the 
 *     License. You may obtain a copy of the License at 
 *     http://www.ibphoenix.com/idpl.html. 
 *
 *     Software distributed under the License is distributed on 
 *     an "AS IS" basis, WITHOUT WARRANTY OF ANY KIND, either 
 *     express or implied.  See the License for the specific 
 *     language governing rights and limitations under the License.
 *
 *
 *  The Original Code was created by James A. Starkey for IBPhoenix.
 *
 *  Copyright (c) 1999, 2000, 2001 James A. Starkey
 *  All Rights Reserved.
 */

#if !defined(_DSNDIALOG_H_INCLUDED_)
#define _DSNDIALOG_H_INCLUDED_

// DsnDialog.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CDsnDialog dialog

class CDsnDialog
{
// Construction
public:
	const char** drivers;
	CDsnDialog(const char **jdbcDrivers);   // standard constructor
	~CDsnDialog();

// Dialog Data
	enum { IDD = IDD_DSN_PROPERTIES };
	JString	m_database;
	JString	m_name;
	JString	m_password;
	JString	m_user;
	JString	m_driver;
	JString	m_role;
	BOOL	m_readonly;
	BOOL	m_nowait;

public:
	int DoModal();

	BOOL OnFindFile();
	void UpdateData(HWND hDlg, BOOL bSaveAndValidate = TRUE );
	BOOL OnInitDialog(HWND hDlg);
};

#endif // !defined(_DSNDIALOG_H_INCLUDED_)
