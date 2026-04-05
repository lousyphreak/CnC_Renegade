/*
**	Command & Conquer Renegade(tm)
**	Copyright 2025 Electronic Arts Inc.
**
**	This program is free software: you can redistribute it and/or modify
**	it under the terms of the GNU General Public License as published by
**	the Free Software Foundation, either version 3 of the License, or
**	(at your option) any later version.
**
**	This program is distributed in the hope that it will be useful,
**	but WITHOUT ANY WARRANTY; without even the implied warranty of
**	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
**	GNU General Public License for more details.
**
**	You should have received a copy of the GNU General Public License
**	along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#if !defined(AFX_INSTANCESPAGE_H__E37C6D92_B4C0_11D2_9FF8_00104B791122__INCLUDED_)
#define AFX_INSTANCESPAGE_H__E37C6D92_B4C0_11D2_9FF8_00104B791122__INCLUDED_

#if _MSC_VER > 1000
#pragma once

#include <cstdint>
#endif // _MSC_VER > 1000
// InstancesPage.h : header file
//

#ifndef __AFXEXT_H__
#include <afxext.h>
#endif

#include "nodetypes.h"
#include "resource.h"
#include "dialogtoolbar.h"
#include "bittype.h"
#include "vector.h"


// Forward declarations
class NodeClass;
class DefinitionFactoryClass;


//////////////////////////////////////////////////////////
//
//	InstancesPageClass
//
//////////////////////////////////////////////////////////
class InstancesPageClass : public CDialog
{
public:
	InstancesPageClass (CWnd *parent_wnd);
	virtual ~InstancesPageClass (void);

// Form Data
public:
	//{{AFX_DATA(InstancesPageClass)
	enum { IDD = IDD_INSTANCES_FORM };
	CListCtrl	m_ListCtrl;
	//}}AFX_DATA

// Attributes
public:

// Operations
public:

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(InstancesPageClass)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	virtual intptr_t WindowProc(uint32_t message, uintptr_t wParam, intptr_t lParam);
	//}}AFX_VIRTUAL

// Implementation
protected:
#ifdef _DEBUG
	virtual void AssertValid() const;
	virtual void Dump(CDumpContext& dc) const;
#endif

	// Generated message map functions
	//{{AFX_MSG(InstancesPageClass)
	afx_msg void OnSize(uint32_t nType, int cx, int cy);
	afx_msg void OnDestroy();
	afx_msg void OnEdit();
	afx_msg void OnGoto();
	afx_msg void OnShowAll();
	afx_msg void OnSelect();
	afx_msg void OnDelete();
	virtual int32_t OnInitDialog();
	afx_msg void OnDeleteitemInstanceList(NMHDR* pNMHDR, intptr_t* pResult);
	afx_msg void OnDblclkInstanceList(NMHDR* pNMHDR, intptr_t* pResult);
	afx_msg void OnItemchangedInstanceList(NMHDR* pNMHDR, intptr_t* pResult);
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

public:

	///////////////////////////////////////////////////////
	//	Public methods
	///////////////////////////////////////////////////////
	void					Add_Node (NodeClass *node);
	void					Remove_Node (NodeClass *node);
	void					Reset_List (void);

protected:
	
	///////////////////////////////////////////////////////
	//	Protected methods
	///////////////////////////////////////////////////////
	void					Set_Node_Check (int index, bool bcheck = true);

	void					Insert_Factory (LPCTSTR name, int class_id);
	void					Insert_Node (NodeClass *node);
	void					Insert_Navigator (void);
	NodeClass *			Get_Item_Node (int index);

	void					Populate_List (uint32_t class_id);
	void					Populate_List (NodeClass *node);

	void					Hide_Nodes (int index, bool hide);
	void					Hide_Node (NodeClass *node, uint32_t class_id, bool hide);
	void					Update_Button_States (void);
	bool					Does_Factory_Have_Children (uint32_t factory_id);
	void					Update_Overlays (void);

private:

	///////////////////////////////////////////////////////
	//	Private member data
	///////////////////////////////////////////////////////
	DialogToolbarClass			m_Toolbar;
	uint32_t							m_ClassID;
	DynamicVectorClass<uint32_t>	m_ClassIDStack;
};

/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_INSTANCESPAGE_H__E37C6D92_B4C0_11D2_9FF8_00104B791122__INCLUDED_)
