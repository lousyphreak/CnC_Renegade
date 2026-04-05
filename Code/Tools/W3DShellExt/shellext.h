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

#ifndef _SHELLEXT_H
#define _SHELLEXT_H

#include <cstdint>

#define MAX_TEXTURES_INFILE	32
#define MAX_TEXUTRE_NAME_LEN	32
#define MAX_MESH					128
#define MAX_ANIMS_INFILE		16

//#define ODS(sz) OutputDebugString(sz)
//#define ENABLE_MSG2 
//#define ENABLE_MSG 
//#define	ENABLE_MSG3
#if defined ENABLE_MSG
#define ODS(sz)  MessageBox(NULL, sz, "Debug Message", MB_OK)
#else 
#define ODS(sz)
#endif


#if defined ENABLE_MSG2
#define ODS2(sz)  MessageBox(NULL, sz, "Debug Message", MB_OK)
#else 
#define ODS2(sz)
#endif

#if defined ENABLE_MSG3
#define ODS3(sz)  MessageBox(NULL, sz, "Debug Message", MB_OK)
#else 
#define ODS3(sz)
#endif

#include "W3D_File.h"
#include "wdump.h"
#include "WdumpDoc.h"
//#include "w3d2dat.h"			/// LFeenanEA: Header file missing, perhaps this tool is outdated?


class CWdumpDoc;
// {556F8779-49C4-4e88-9CEF-0AC2CFD6B763}
DEFINE_GUID(CLSID_ShellExtension, 0x556f8779L, 0x49c4, 0x4e88, 0x9c, 0xef, 0x0a, 0xc2, 0xcf, 0xd6, 0xb7, 0x63 );
class CShellExtClassFactory : public IClassFactory
{
protected:
	uint32_t	m_cRef;

public:
	CShellExtClassFactory();
	~CShellExtClassFactory();

	//IUnknown members
	STDMETHODIMP			QueryInterface(REFIID, LPVOID FAR *);
	STDMETHODIMP_(uint32_t)	AddRef();
	STDMETHODIMP_(uint32_t)	Release();

	//IClassFactory members
	STDMETHODIMP		CreateInstance(LPUNKNOWN, REFIID, LPVOID FAR *);
	STDMETHODIMP		LockServer(int32_t);

};
typedef CShellExtClassFactory *LPCSHELLEXTCLASSFACTORY;
class CShellExt : public IContextMenu, 
                         IShellExtInit, 
                         IExtractIcon, 
                         IPersistFile, 
                         IShellPropSheetExt,
                         ICopyHook{
public:
    char         m_SelectedFile[MAX_PATH];
void Read_SelectedFile();
protected:
//	ITEMIDLIST m_idFolder;
	uint32_t        m_cRef;
	LPDATAOBJECT m_pDataObj;
    char         m_szFileUserClickedOn[MAX_PATH];
	STDMETHODIMP DoW3DMenu1(HWND hParent, LPCSTR pszWorkingDir, LPCSTR pszCmd,LPCSTR pszParam, int iShowCmd);
public:
	bool NotAdded(char* name);
	void Read_Selection(W3dMeshHeader3Struct&, W3D_HTree&, W3D_HAnim** );
	CShellExt();
	~CShellExt();

	//IUnknown members
	STDMETHODIMP			QueryInterface(REFIID, LPVOID FAR *);
	STDMETHODIMP_(uint32_t)	AddRef();
	STDMETHODIMP_(uint32_t)	Release();

	//IShell members
	STDMETHODIMP			QueryContextMenu(HMENU hMenu, uint32_t indexMenu, uint32_t idCmdFirst, uint32_t idCmdLast, uint32_t uFlags);
	STDMETHODIMP			InvokeCommand(LPCMINVOKECOMMANDINFO lpcmi);
	STDMETHODIMP			GetCommandString(uint32_t idCmd, uint32_t uFlags, uint32_t FAR *reserved, LPSTR pszName, uint32_t cchMax);
	//IShellExtInit methods
	STDMETHODIMP		    Initialize(LPCITEMIDLIST pIDFolder, LPDATAOBJECT pDataObj, HKEY hKeyID);
    //IExtractIcon methods
    STDMETHODIMP GetIconLocation(uint32_t   uFlags,LPSTR  szIconFile,uint32_t   cchMax,int   *piIndex,uint32_t  *pwFlags);
    STDMETHODIMP Extract(LPCSTR pszFile,uint32_t   nIconIndex,HICON  *phiconLarge,HICON  *phiconSmall,uint32_t   nIconSize);
    //IPersistFile methods
    STDMETHODIMP GetClassID(LPCLSID lpClassID);
    STDMETHODIMP IsDirty();
    STDMETHODIMP Load(LPCOLESTR lpszFileName, uint32_t grfMode);
    STDMETHODIMP Save(LPCOLESTR lpszFileName, int32_t fRemember);
    STDMETHODIMP SaveCompleted(LPCOLESTR lpszFileName);
    STDMETHODIMP GetCurFile(LPOLESTR FAR* lplpszFileName);
    //IShellPropSheetExt methods
    STDMETHODIMP AddPages(LPFNADDPROPSHEETPAGE lpfnAddPage, intptr_t lParam);
    STDMETHODIMP ReplacePage(uint32_t uPageID, LPFNADDPROPSHEETPAGE lpfnReplaceWith, intptr_t lParam);
    //ICopyHook method
    STDMETHODIMP_(uint32_t) CopyCallback(HWND hwnd, uint32_t wFunc, uint32_t wFlags, LPCSTR pszSrcFile, uint32_t dwSrcAttribs,LPCSTR pszDestFile, uint32_t dwDestAttribs);
public:
	W3dAnimHeaderStruct	m_AnimInfos[MAX_ANIMS_INFILE];
	W3dHierarchyStruct	m_Hierarchies[MAX_ANIMS_INFILE];
	CString					m_Textures[MAX_TEXTURES_INFILE];
	int						m_NumAdded;
	bool						m_FileInMemory;
	CWdumpDoc				m_WdumpDocument;
	int						m_FoundMeshes;
	W3dMeshHeader3Struct m_Meshes[MAX_MESH];
};

typedef CShellExt *LPCSHELLEXT;
#endif // _SHELLEXT_H
