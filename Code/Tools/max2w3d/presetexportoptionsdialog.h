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
 *                     $Archive:: /Commando/Code/Tools/max2w3d/presetexportoptionsdialog.h    $*
 *                                                                                             *
 *              Original Author:: Patrick Smith																  *
 *                                                                                             *
 *                      $Author:: Patrick                                                     $*
 *                                                                                             *
 *                     $Modtime:: 10/30/00 2:45p                                              $*
 *                                                                                             *
 *                    $Revision:: 2                                                           $*
 *                                                                                             *
 *---------------------------------------------------------------------------------------------*
 * Functions:                                                                                  *
 * - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */


#ifndef __PRESET_EXPORT_OPTIONS_DIALOG_H
#define __PRESET_EXPORT_OPTIONS_DIALOG_H

#include <cstdint>

#include <windows.h>
#include <max.h>
#include "w3dutil.h"


////////////////////////////////////////////////////////////////////////////////////////
//
//	PresetExportOptionsDialogClass
//
////////////////////////////////////////////////////////////////////////////////////////
class PresetExportOptionsDialogClass
{
public:

	//////////////////////////////////////////////////////////////////
	//	Public constructors/destructors
	//////////////////////////////////////////////////////////////////
	PresetExportOptionsDialogClass (Interface *maxinterface, HWND parent_wnd = NULL);
	~PresetExportOptionsDialogClass (void);


	//////////////////////////////////////////////////////////////////
	//	Public methods
	//////////////////////////////////////////////////////////////////		
	
	void			Set_Options (W3dExportOptionsStruct *options)	{ Options = options; ::memcpy (&OrigOptions, Options, sizeof (OrigOptions)); }
	int			Do_Modal (void);

private:

	//////////////////////////////////////////////////////////////////
	//	Private data types
	//////////////////////////////////////////////////////////////////
	
	enum
	{
		PANE_HLOD			= 0,
		PANE_ANIM_HLOD,
		PANE_ANIM,
		PANE_TERRAIN,
		PANE_SKELETON,
		PANE_MESH,
		PANE_MAX
	};


	//////////////////////////////////////////////////////////////////
	//	Static methods
	//////////////////////////////////////////////////////////////////
	static int32_t CALLBACK	Real_Message_Proc (HWND wnd, uint32_t message, uintptr_t wparam, intptr_t lparam);
	static int32_t CALLBACK	Settings_Pane_Message_Proc (HWND wnd, uint32_t message, uintptr_t wparam, intptr_t lparam);
	
	//////////////////////////////////////////////////////////////////
	//	Private methods
	//////////////////////////////////////////////////////////////////
	int32_t			Message_Proc (uint32_t message, uintptr_t wparam, intptr_t lparam);
	int32_t			Pane_Message_Proc (uint32_t message, uintptr_t wparam, intptr_t lparam);
	int32_t			Settings_Message_Proc (uint32_t message, uintptr_t wparam, intptr_t lparam);
	int32_t			On_Command (uintptr_t wparam, intptr_t lparam);
	void			Show_Settings_Pane (int pane_id);
	void			Create_Settings_Panes (void);
	void			Destroy_Settings_Panes (void);
	void			Determine_Preset_Type (void);
	void			Initialize_Controls (void);
	void			Update_Controls (void);
	void			Save_Settings (void);

	//////////////////////////////////////////////////////////////////
	//	Private member data
	//////////////////////////////////////////////////////////////////
	W3dExportOptionsStruct *	Options;
	W3dExportOptionsStruct		OrigOptions;
	Interface *						MaxInterface;
	HWND								Wnd;
	HWND								ParentWnd;
	HWND								PaneWnds[PANE_MAX];
	int								CurrentPane;
};


#endif //__PRESET_EXPORT_OPTIONS_DIALOG_H

