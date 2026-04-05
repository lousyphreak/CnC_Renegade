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
 ***                            Confidential - Westwood Studios                              *** 
 *********************************************************************************************** 
 *                                                                                             * 
 *                 Project Name : Commando                                                     * 
 *                                                                                             * 
 *                     $Archive:: /Commando/Code/wwnet/networkobjectmgr.cpp                   $* 
 *                                                                                             * 
 *                      $Author:: Tom_s                                                       $* 
 *                                                                                             * 
 *                     $Modtime:: 1/07/02 10:30a                                              $* 
 *                                                                                             * 
 *                    $Revision:: 20                                                          $* 
 *                                                                                             * 
 *---------------------------------------------------------------------------------------------* 
 * Functions:                                                                                  * 
 * - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

#include "networkobjectmgr.h"
#include "networkobject.h"


////////////////////////////////////////////////////////////////
//	Static member initialization
////////////////////////////////////////////////////////////////
int											NetworkObjectMgrClass::_NewDynamicID = NETID_DYNAMIC_OBJECT_MIN;
int											NetworkObjectMgrClass::_NewClientID = 0;
bool											NetworkObjectMgrClass::_IsLevelLoading = false;

NetworkObjectMgrClass::OBJECT_LIST &
NetworkObjectMgrClass::Get_Object_List(void)
{
	static OBJECT_LIST object_list;
	return object_list;
}

NetworkObjectMgrClass::OBJECT_LIST &
NetworkObjectMgrClass::Get_Delete_Pending_List(void)
{
	static OBJECT_LIST delete_pending_list;
	return delete_pending_list;
}

////////////////////////////////////////////////////////////////
//
//	Register_Object
//
////////////////////////////////////////////////////////////////
void
NetworkObjectMgrClass::Register_Object (NetworkObjectClass *object)
{
	int object_id = object->Get_Network_ID ();
	if (object_id != 0) {

		//
		//	Check to ensure the object isn't already in the list
		//
		int index = 0;
		if (Find_Object (object_id, &index) == false) {
			
			//
			//	Insert the object into the list
			//
			Get_Object_List ().Insert (index, object);
		}
	}

	return ;
}


////////////////////////////////////////////////////////////////
//
//	Unregister_Object
//
////////////////////////////////////////////////////////////////
void
NetworkObjectMgrClass::Unregister_Object (NetworkObjectClass *object)
{
	int object_id = object->Get_Network_ID ();
	if (object_id != 0) {

		//
		//	Try to find the object in the list
		//
		int index = 0;
		if (Find_Object (object_id, &index)) {
					
			//
			//	Remove the object from the list
			//
			Get_Object_List ().Delete (index);
		}
	}

	return ;
}


////////////////////////////////////////////////////////////////
//
//	Find_Object
//
////////////////////////////////////////////////////////////////
NetworkObjectClass *
NetworkObjectMgrClass::Find_Object (int object_id)
{
	NetworkObjectClass *object = NULL;
	
	//
	//	Lookup the object in the sorted list
	//
	int index = 0;
	if (Find_Object (object_id, &index)) {
		object = Get_Object_List ()[index];
	}

	return object;
}


////////////////////////////////////////////////////////////////
//
//	Set_New_Dynamic_ID
//
////////////////////////////////////////////////////////////////
void
NetworkObjectMgrClass::Set_New_Dynamic_ID (int id)
{
	WWASSERT(id >= NETID_DYNAMIC_OBJECT_MIN && id <= NETID_DYNAMIC_OBJECT_MAX);

	_NewDynamicID = id;
}

////////////////////////////////////////////////////////////////
//
//	Get_New_Dynamic_ID
//
////////////////////////////////////////////////////////////////
int
NetworkObjectMgrClass::Get_New_Dynamic_ID (void)
{
	WWASSERT(_NewDynamicID >= NETID_DYNAMIC_OBJECT_MIN && _NewDynamicID < NETID_DYNAMIC_OBJECT_MAX);

	/*
	//TSS091001
	NetworkObjectClass * p_object = Find_Object(_NewDynamicID);
	//WWASSERT(p_object == NULL);
	if (p_object != NULL) {
		int iii;
		iii = 111;
	}
	/**/

	//TSS091201
	NetworkObjectClass * p_object = Find_Object(_NewDynamicID);
	while (p_object != NULL) {
		/*
		WWDEBUG_SAY(("NetworkObjectMgrClass::Get_New_Dynamic_ID :skipping id %d (already in use)\n", 
			_NewDynamicID));
		*/
		_NewDynamicID++;
		p_object = Find_Object(_NewDynamicID);
	}

	return _NewDynamicID++;
}

////////////////////////////////////////////////////////////////
//
//	Get_Current_Dynamic_ID
//
////////////////////////////////////////////////////////////////
int
NetworkObjectMgrClass::Get_Current_Dynamic_ID (void)
{
	WWASSERT(_NewDynamicID >= NETID_DYNAMIC_OBJECT_MIN && _NewDynamicID <= NETID_DYNAMIC_OBJECT_MAX);

	return _NewDynamicID;
}

////////////////////////////////////////////////////////////////
//
//	Init_New_Client_ID
//
////////////////////////////////////////////////////////////////
void
NetworkObjectMgrClass::Init_New_Client_ID (int client_id)
{
	WWASSERT(client_id > 0);

	_NewClientID = NETID_CLIENT_OBJECT_MIN + (client_id - 1) * 100000;
}

////////////////////////////////////////////////////////////////
//
//	Get_New_Client_ID
//
////////////////////////////////////////////////////////////////
int
NetworkObjectMgrClass::Get_New_Client_ID (void)
{
	WWASSERT(_NewClientID >= NETID_CLIENT_OBJECT_MIN && _NewClientID < NETID_CLIENT_OBJECT_MAX);

	return _NewClientID++;
}

////////////////////////////////////////////////////////////////
//
//	Find_Object
//
////////////////////////////////////////////////////////////////
bool
NetworkObjectMgrClass::Find_Object (int id_to_find, int *index)
{
	WWASSERT(index != NULL);

	bool found		= false;	
	(*index)			= 0;
	int min_index	= 0;
	OBJECT_LIST &object_list = Get_Object_List ();
	int max_index	= object_list.Count () - 1;		
	
	//
	//	Keep looping until we've closed the window of possiblity
	//
	bool keep_going = (max_index >= min_index);
	while (keep_going) {

		//
		//	Calculate what slot we are currently looking at
		//
		int curr_index	= min_index + ((max_index - min_index) / 2);
		int curr_id		= object_list[curr_index]->Get_Network_ID ();

		//
		//	Did we find the right slot?
		//
		if (id_to_find == curr_id) {
			(*index)		= curr_index;
			keep_going	= false;
			found			= true;
		} else {

			//
			//	Stop if we've narrowed the window to one entry
			//
			keep_going = (max_index > min_index);

			//
			//	Move the window to the appropriate side
			// of the test index.
			//
			if (id_to_find < curr_id) {
				max_index	= curr_index - 1;
				(*index)		= curr_index;
			} else {
				min_index	= curr_index + 1;
				(*index)		= curr_index + 1;
			}
		}
	}

	return found;
}


////////////////////////////////////////////////////////////////
//
//	Think
//
////////////////////////////////////////////////////////////////
void
NetworkObjectMgrClass::Think (void)
{
	//
	//	Simply let each object think
	//
	OBJECT_LIST &object_list = Get_Object_List ();
	for (int index = 0; index < object_list.Count (); index ++) {
		WWASSERT(object_list[index] != NULL);
		object_list[index]->Network_Think ();
	}

	return ;
}


////////////////////////////////////////////////////////////////
//
//	Set_All_Delete_Pending
//
// This is for cleanup only.
//
////////////////////////////////////////////////////////////////
void
NetworkObjectMgrClass::Set_All_Delete_Pending (void)
{
	WWDEBUG_SAY(("NetworkObjectMgrClass::Set_All_Delete_Pending\n"));

	//
	//	Mark all netobjects as delete pending
	//
	OBJECT_LIST &object_list = Get_Object_List ();
	for (int index = 0; index < object_list.Count (); index ++) {
		object_list[index]->Set_Delete_Pending();
	}

	return ;
}


////////////////////////////////////////////////////////////////
//
//	Delete_Pending
//
////////////////////////////////////////////////////////////////
void
NetworkObjectMgrClass::Delete_Pending (void)
{
	if (_IsLevelLoading) {
		return ;
	}

	//WWDEBUG_SAY(("NetworkObjectMgrClass::Delete_Pending\n"));

	//
	//	Delete each object that is pending...
	//
	OBJECT_LIST &delete_pending_list = Get_Delete_Pending_List ();
	for (int index = 0; index < delete_pending_list.Count (); index ++) {
		WWASSERT(delete_pending_list[index] != NULL);
		if (delete_pending_list[index]->Is_Delete_Pending ()) {
			delete_pending_list[index]->Delete ();
		}
	}

//	if (_DeletePendingList.Count()) {
//		_DeletePendingList.Delete_All ();
//	}
	delete_pending_list.Reset_Active();	// No need to resize the vector back to zero...
	return ;
}


////////////////////////////////////////////////////////////////
//
//	Delete_Client_Objects
//
////////////////////////////////////////////////////////////////
void
NetworkObjectMgrClass::Delete_Client_Objects (int client_id)
{
	WWASSERT(client_id > 0);

	//
	//	Delete each object that belongs to the given client
	//

	OBJECT_LIST &object_list = Get_Object_List ();
	for (int index = 0; index < object_list.Count (); index ++) {
		WWASSERT(object_list[index] != NULL);
		if (object_list[index]->Belongs_To_Client (client_id)) {
			//TSS092301 _ObjectList[index]->Delete ();
			object_list[index]->Set_Delete_Pending();
		}
	}

	return ;
}


////////////////////////////////////////////////////////////////
//
//	Restore_Dirty_Bits
//
////////////////////////////////////////////////////////////////
void
NetworkObjectMgrClass::Restore_Dirty_Bits (int client_id)
{
	WWASSERT(client_id > 0);

	//
	// If a guy quits, we need to restore the dirty bits on each object so that 
	// if he rejoins he will be told about stuff again.
	// For now I am going to use the topmost client id...
	//

	OBJECT_LIST &object_list = Get_Object_List ();
	for (int index = 0; index < object_list.Count (); index ++) {
		NetworkObjectClass * p_object = object_list[index];
		WWASSERT(p_object != NULL);
		uint8_t generic_bits = p_object->Get_Object_Dirty_Bits(NetworkObjectClass::MAX_CLIENT_COUNT - 1);//TSS2001e
		p_object->Set_Object_Dirty_Bits(client_id, generic_bits);
	}

	return ;
}


////////////////////////////////////////////////////////////////
//
//	Register_Object_For_Deletion
//
////////////////////////////////////////////////////////////////
void
NetworkObjectMgrClass::Register_Object_For_Deletion (NetworkObjectClass *object)
{
	OBJECT_LIST &delete_pending_list = Get_Delete_Pending_List ();
	if (delete_pending_list.ID (object) == -1) {
		delete_pending_list.Add (object);
	}

	return ;
}


////////////////////////////////////////////////////////////////
//
//	Reset_Import_State_Counts
//
////////////////////////////////////////////////////////////////
void 
NetworkObjectMgrClass::Reset_Import_State_Counts(void)
{
	OBJECT_LIST &object_list = Get_Object_List ();
	for (int index = 0; index < object_list.Count (); index ++) {

		NetworkObjectClass * p_object = object_list[index];
		WWASSERT(p_object != NULL);
			
		//
		//	Reset its import state count
		//
		p_object->Reset_Import_State_Count();
	}
}

