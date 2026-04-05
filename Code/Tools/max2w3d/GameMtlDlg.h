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

/***********************************************************************************************
 ***              C O N F I D E N T I A L  ---  W E S T W O O D  S T U D I O S               ***
 ***********************************************************************************************
 *                                                                                             *
 *                 Project Name : Max2W3d                                                      *
 *                                                                                             *
 *                     $Archive:: /Commando/Code/Tools/max2w3d/GameMtlDlg.h                   $*
 *                                                                                             *
 *                       Author:: Greg Hjelstrom                                               *
 *                                                                                             *
 *                     $Modtime:: 2/08/02 11:08a                                              $*
 *                                                                                             *
 *                    $Revision:: 14                                                          $*
 *                                                                                             *
 *---------------------------------------------------------------------------------------------*
 * Functions:                                                                                  *
 * - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */




#ifndef GAMEMTLDLG_H
#define GAMEMTLDLG_H

#include <cstdint>

class GameMtl;
class GameMtlPassDlg;


////////////////////////////////////////////////////////////////////////
// GameMtlDlg
//
// Dialog box interface in the material editor for GameMtl
//	This is basically a cannibalized version of the Standard
// Max material's dialog.
//
////////////////////////////////////////////////////////////////////////
class GameMtlDlg: public ParamDlg 
{

public:
	
	////////////////////////////////////////////////////////////////////////
	// Methods
	////////////////////////////////////////////////////////////////////////
	GameMtlDlg(HWND hwMtlEdit, IMtlParams *imp, GameMtl *m); 
	~GameMtlDlg();

	// From ParamDlg:
	Class_ID				ClassID(void);
	void					SetThing(ReferenceTarget *m);
	ReferenceTarget*	GetThing(void) { return (ReferenceTarget*)TheMtl; }
	void					DeleteThis() { delete this;  }	
	void					SetTime(TimeValue t);
	void					ReloadDialog(void);
	void					ActivateDlg(int32_t onOff);

	void					Invalidate(void);
	void					Update_Display(void)	{IParams->MtlChanged();}
	////////////////////////////////////////////////////////////////////////
	// Max interface pointer
	////////////////////////////////////////////////////////////////////////

	Interface* Ip;

protected:

	void					Build_Dialog(HWND hParent = NULL);
	int32_t					DisplacementMapProc(HWND hDlg, uint32_t message, uintptr_t wParam, intptr_t lParam);
	int32_t					SurfaceTypeProc(HWND hDlg, uint32_t message, uintptr_t wParam, intptr_t lParam);
	int32_t					PassCountProc(HWND hDlg, uint32_t message, uintptr_t wParam, intptr_t lParam);
	void					Set_Pass_Count_Dialog(void);

	enum { MAX_PASSES = 4 };

	////////////////////////////////////////////////////////////////////////
	// Windows handles
	////////////////////////////////////////////////////////////////////////
	HWND					HwndEdit;		// window handle of the materials editor dialog
	HWND					HwndPassCount;	// Rollup pass count panel
	HWND					HwndSurfaceType;	// Rollup surface type panel
	HWND					HwndDisplacementMap;
	HPALETTE				HpalOld;

	GameMtlPassDlg *	PassDialog[MAX_PASSES];	

	////////////////////////////////////////////////////////////////////////
	// Material dialog interface
	////////////////////////////////////////////////////////////////////////
	IMtlParams *		IParams;			// interface to the material editor
	GameMtl *			TheMtl;			// current mtl being edited.
	
	////////////////////////////////////////////////////////////////////////
	// Member variables
	////////////////////////////////////////////////////////////////////////
	TimeValue			CurTime;
	int					IsActive;
	friend int32_t CALLBACK DisplacementMapDlgProc(HWND, uint32_t, uintptr_t,intptr_t);
	friend int32_t CALLBACK SurfaceTypePanelDlgProc(HWND, uint32_t, uintptr_t,intptr_t);
	friend int32_t CALLBACK PassCountPanelDlgProc(HWND, uint32_t, uintptr_t,intptr_t);
	friend class GameMtl;
};
	GameMtl*				GetMtlFromSelection();

#endif