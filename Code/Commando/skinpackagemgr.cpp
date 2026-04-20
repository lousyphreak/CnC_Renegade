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
 *                 Project Name : commando                                                     *
 *                                                                                             *
 *                     $Archive:: /Commando/Code/commando/skinpackagemgr.cpp                  $*
 *                                                                                             *
 *                       Author:: Patrick Smith                                                *
 *                                                                                             *
 *                     $Modtime:: 1/07/02 10:56a                                              $*
 *                                                                                             *
 *                    $Revision:: 2                                                           $*
 *                                                                                             *
 *---------------------------------------------------------------------------------------------*
 * Functions:                                                                                  *
 * - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

#include "skinpackagemgr.h"
#include "registry.h"
#include "_globals.h"
#include "../wwlib/osdep.h"

#include <cstring>

namespace {

bool Matches_Package_File(const std::string &path)
{
	StringClass extension(renegade_osdep::Get_Path_Extension(path).c_str(), true);
	return extension.Compare_No_Case(".pkg") == 0;
}

}


/////////////////////////////////////////////////////////////////////
//	Local constants
/////////////////////////////////////////////////////////////////////
static const char *	SKIN_DICTIONARY_FILENAME	= "skindictionary.ini";
static const char *	CURR_SKIN_REG_VALUE			= "CurrSkinPackage";


//////////////////////////////////////////////////////////////////////
//	Static member initialization
//////////////////////////////////////////////////////////////////////
DynamicVectorClass<SkinPackageClass>	SkinPackageMgrClass::PackageList;
SkinPackageClass								SkinPackageMgrClass::CurrentPackage;


/////////////////////////////////////////////////////////////////////
//
//	Initialize
//
//////////////////////////////////////////////////////////////////////
void
SkinPackageMgrClass::Initialize (void)
{
	Shutdown ();

	//
	//	Get the currently selected package name from the registry
	//
	RegistryClass registry (APPLICATION_SUB_KEY_NAME_OPTIONS);
	if (registry.Is_Valid ()) {

		//
		//	Get the current package name from the registry
		//
		StringClass package_filename;
		registry.Get_String (CURR_SKIN_REG_VALUE, package_filename, "default.pkg");

		//
		//	Initialize the current package
		//
		CurrentPackage.Set_Package_Filename (package_filename);
	}

	return ;
}


/////////////////////////////////////////////////////////////////////
//
//	Shutdown
//
//////////////////////////////////////////////////////////////////////
void
SkinPackageMgrClass::Shutdown (void)
{
	Reset_List ();
	return ;
}


//////////////////////////////////////////////////////////////////////
//
//	Build_List
//
//////////////////////////////////////////////////////////////////////
void
SkinPackageMgrClass::Build_List (void)
{
	char search_root[MAX_PATH] = {};
	if (::GetCurrentDirectory(sizeof(search_root), search_root) == 0) {
		return;
	}

	std::vector<std::string> entries;
	if (!renegade_osdep::Collect_Directory_Entries(search_root, entries)) {
		return;
	}

	for (const std::string &entry_name : entries) {
		const std::string entry_path = renegade_osdep::Join_Path(search_root, entry_name);
		if (!renegade_osdep::Path_Is_Regular_File(entry_path)) {
			continue;
		}

		if (!Matches_Package_File(entry_name)) {
			continue;
		}

		SkinPackageClass package;
		package.Set_Package_Filename(entry_name.c_str());
		PackageList.Add(package);
	}
	
	return ;
}


//////////////////////////////////////////////////////////////////////
//
//	Reset_List
//
//////////////////////////////////////////////////////////////////////
void
SkinPackageMgrClass::Reset_List (void)
{
	PackageList.Delete_All ();
	return ;
}


//////////////////////////////////////////////////////////////////////
//
//	Set_Current_Package
//
//////////////////////////////////////////////////////////////////////
void
SkinPackageMgrClass::Set_Current_Package (const char *package_filename)
{
	//
	//	Initialize the current package
	//
	CurrentPackage.Set_Package_Filename (package_filename);

	//
	//	Write the name of hte package to the registry
	//
	RegistryClass registry (APPLICATION_SUB_KEY_NAME_OPTIONS);
	if (registry.Is_Valid ()) {
		registry.Set_String (CURR_SKIN_REG_VALUE, package_filename);
	}

	return ;
}


//////////////////////////////////////////////////////////////////////
//
//	Set_Current_Package
//
//////////////////////////////////////////////////////////////////////
void
SkinPackageMgrClass::Set_Current_Package (int index)
{
	Set_Current_Package (PackageList[index].Get_Package_Filename ());
	return ;
}
