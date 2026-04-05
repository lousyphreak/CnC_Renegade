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

/*************************************************************************** 
 *                                                                         * 
 *                 Project Name : Westwood Auto Registration App           * 
 *                                                                         * 
 *                    File Name : PACKET.CPP                               * 
 *                                                                         * 
 *                   Programmer : Philip W. Gorrow                         * 
 *                                                                         * 
 *                   Start Date : 04/22/96                                 * 
 *                                                                         * 
 *                  Last Update : April 24, 1996 [PWG]                     * 
 *                                                                         * 
 *-------------------------------------------------------------------------* 
 * Functions:                                                              * 
 *   *PacketClass::Find_Field -- Finds a field if it exists in the packets * 
 *   Get_Field -- Find specified name and returns data                     * 
 *   PacketClass::~PacketClass -- destroys a packet class be freeing list  * 
 *   PacketClass::Add_Field -- Adds a FieldClass entry to head of packet li* 
 * - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <cstdint>
#ifndef _WINDOWS
#include <netinet/in.h>
#else
#define Win32_Winsock
#include <windows.h>
#endif

#include "packet.h"


/************************************************************************** 
 * PACKETCLASS::~PACKETCLASS -- destroys a packet class be freeing list   * 
 *                                                                        * 
 * INPUT:    none                                                         *
 *                                                                        * 
 * OUTPUT:     none                                                       *
 *                                                                        * 
 * HISTORY:                                                               * 
 *   04/24/1996 PWG : Created.                                            * 
 *========================================================================*/
PacketClass::~PacketClass()
{
  FieldClass *current;
  FieldClass *next;

  //
  // Loop through the entire field list and delete each entry.
  //
  for (current = Head; current; current = next) {
    next = current->Next;
    delete(current);
  }
}


/************************************************************************** 
 * PACKETCLASS::ADD_FIELD -- Adds a FieldClass entry to head of packet li * 
 *                                                                        * 
 * INPUT:    FieldClass * - a properly constructed field class entry.     *
 *                                                                        * 
 * OUTPUT:     none                                                       * 
 *                                                                        * 
 * HISTORY:                                                               * 
 *   04/24/1996 PWG : Created.                                            * 
 *========================================================================*/
void PacketClass::Add_Field(FieldClass *field)
{
  field->Next = Head;
  Head = field;  
}

/************************************************************************** 
 * PACKETCLASS::PACKETCLASS -- Creates a Packet object from a COMMS packe * 
 *                                                                        * 
 * INPUT:                                                                 * 
 *                                                                        * 
 * OUTPUT:                                                                * 
 *                                                                        * 
 * WARNINGS:                                                              * 
 *                                                                        * 
 * HISTORY:                                                               * 
 *   04/22/1996 PWG : Created.                                            * 
 *========================================================================*/
PacketClass::PacketClass(uint8_t *curbuf)
{
  int remaining_size;
  //
  // Pull the size and packet ID out of the linear packet stream.
  //
  memcpy(&Size, curbuf, sizeof(Size));
  curbuf += sizeof(uint16_t);
  Size = ntohs(Size);
  memcpy(&ID, curbuf, sizeof(ID));
  curbuf += sizeof(uint16_t);
  ID   = static_cast<int16_t>(ntohs(static_cast<uint16_t>(ID)));
  Head = NULL;

  //
  // Calculate the remaining size so that we can loop through the
  //   packets and extract them.
  //
  remaining_size = Size - 4;

  //
  // Loop through the linear packet until we run out of room and
  // create a field for each.
  //
  while (remaining_size > 0)
  {
    FieldClass *field = new FieldClass;

    //
    // Copy the adjusted header into the buffer and then advance the buffer
    //
    memcpy(field, curbuf, FIELD_HEADER_SIZE);
    curbuf += FIELD_HEADER_SIZE;
    remaining_size   -= FIELD_HEADER_SIZE;

    //
    // Copy the data into the buffer
    //
    int size      = ntohs(field->Size);
    field->Data    = new uint8_t[size];
    memcpy(field->Data, curbuf, size);
    curbuf      += size;
    remaining_size  -= size;

    //
    // Make sure we allow for the pad bytes.
    //
    int pad = (4 - (ntohs(field->Size) & 3)) & 3;
    curbuf += pad;
    remaining_size   -= pad;

    //
    // Convert the field back to the host format
    //
    field->Net_To_Host();

    //
    // Finally add the field to the field list in the packet
    // structure.
    //
    Add_Field(field);
  }
}

/************************************************************************** 
 * CREATE_COMMS_PACKET -- Walks field list creating a packet              * 
 *                                                                        * 
 * INPUT:    short - the id of the packet so the server can identify it *
 *          uint16_t & - the size of the packet returned here    *
 *                                                                        * 
 * OUTPUT:     void * pointer to the linear packet data                   * 
 *                                                                        * 
 * WARNINGS:   This routine allocates memory that the user is responsible *
 *          for freeing.                                *
 *                                                                        * 
 * HISTORY:                                                               * 
 *   04/22/1996 PWG : Created.                                            * 
 *========================================================================*/
uint8_t *PacketClass::Create_Comms_Packet(int &size)
{
  FieldClass *current;

  //
  // Size starts at four because that is the size of the packet header.
  //
  size = 4;

  //
  // Take a quick spin through and calculate the size of the packet we
  //   are building.
  //
  for (current = Head; current; current=current->Next)
  {
    size += (uint16_t)FIELD_HEADER_SIZE;      // add in packet header size
    size += current->Size;        // add in data size
    size += (4 - (size & 3)) & 3;   // add in pad value to dword align next packet
  }

  //
  // Now that we know the size allocate a buffer big enough to hold the 
  // packet.
  //
  uint8_t *retval = new uint8_t[size];
  uint8_t *curbuf = retval;

  //
  // write the size into the packet header
  //
  const uint16_t net_size = static_cast<uint16_t>(htons(size));
  memcpy(curbuf, &net_size, sizeof(net_size));
  curbuf += sizeof(uint16_t);
  const uint16_t net_id = static_cast<uint16_t>(htons(static_cast<uint16_t>(ID)));
  memcpy(curbuf, &net_id, sizeof(net_id));
  curbuf += sizeof(uint16_t);

  //
  // Ok now that the actual header information has been written we need to write out
  // field information.
  //
  for (current = Head; current; current = current->Next)
  {
    //
    // Temporarily convert the packet to net format (this saves alot of
    //   effort, and seems safe...)
    //
    current->Host_To_Net();

    //
    // Copy the adjusted header into the buffer and then advance the buffer
    //
    memcpy(curbuf, current, FIELD_HEADER_SIZE);
    curbuf += FIELD_HEADER_SIZE;

    //
    // Copy the data into the buffer and then advance the buffer
    //
    memcpy(curbuf, current->Data, ntohs(current->Size));
    curbuf += ntohs(current->Size);

    //
    // Finally take care of any pad bytes by setting them to 0
    //
    int pad = (4 - (ntohs(current->Size) & 3)) & 3;
    curbuf += pad;

    current->Net_To_Host();
  }
  return(retval);
}


/************************************************************************** 
 * PACKETCLASS::FIND_FIELD -- Finds a field if it exists in the packets   * 
 *                                                                        * 
 * INPUT:    char *  - the id of the field we are looking for.            *
 *                                                                        * 
 * OUTPUT:     FieldClass * pointer to the field class                    * 
 *                                                                        * 
 * HISTORY:                                                               * 
 *   04/23/1996 PWG : Created.                                            * 
 *========================================================================*/
FieldClass *PacketClass::Find_Field(char *id)
{
  for (FieldClass *current = Head; current; current = current->Next)
  {
    if ( strncmp(id, current->ID, 4) == 0)
      return current;
  }
  return NULL;
}

// gks 9/25/2000
FieldClass *PacketClass::Get_Field_At(int position)
{
	int i = 0;
	FieldClass *current = Head;
	for (; (current && (i < position)); current = current->Next, i++) {
	}
	
	if (current) return current;
	else return NULL;
}

// gks 9/25/2000
int PacketClass::Get_Num_Fields()
{
	int i = 0;
		
	for (FieldClass *current = Head; current; current = current->Next, i++) {}
	
	return i;

}


/************************************************************************** 
 * GET_FIELD -- Find specified name and returns data                      * 
 *                                                                        * 
 * INPUT:    char *   - the id of the field that holds the data.          *
 *           char &   - the reference to store the data into              *
 *                                                                        * 
 * OUTPUT:    true if the field was found, false if it was not.           *
 *                                                                        * 
 * WARNINGS:  The data reference is not changed if the field is not       *
 *          found.                                                        *
 *                                                                        * 
 * HISTORY:                                                               * 
 *   04/23/1996 PWG : Created.                                            * 
 *========================================================================*/
int8_t PacketClass::Get_Field(char *id, char &data)
{
  FieldClass *field = Find_Field(id);
  if (field) {
    data = *((char *)field->Data);
  }
  return((field) ? true : false);
}  


/************************************************************************** 
 * GET_FIELD -- Find specified name and returns data                      * 
 *                                                                        * 
 * INPUT:    char *   - the id of the field that holds the data.          *
 *           uint8_t &   - the reference to store the data into     *
 *                                                                        * 
 * OUTPUT:    true if the field was found, false if it was not.           *
 *                                                                        * 
 * WARNINGS:  The data reference is not changed if the field is not       *
 *          found.                                                        *
 *                                                                        * 
 * HISTORY:                                                               * 
 *   04/23/1996 PWG : Created.                                            * 
 *========================================================================*/
int8_t PacketClass::Get_Field(char *id, uint8_t &data)
{
  FieldClass *field = Find_Field(id);
  if (field) {
    data = *((uint8_t *)field->Data);
  }
  return((field) ? true : false);
}  


/************************************************************************** 
 * GET_FIELD -- Find specified name and returns data                      * 
 *                                                                        * 
 * INPUT:    char *   - the id of the field that holds the data.          *
 *          short &   - the reference to store the data into              *
 *                                                                        * 
 * OUTPUT:    true if the field was found, false if it was not.           *
 *                                                                        * 
 * WARNINGS:  The data reference is not changed if the field is not       *
 *          found.                                                        *
 *                                                                        * 
 * HISTORY:                                                               * 
 *   04/23/1996 PWG : Created.                                            * 
 *========================================================================*/
int8_t PacketClass::Get_Field(char *id, int16_t &data)
{
  FieldClass *field = Find_Field(id);
  if (field) {
    memcpy(&data, field->Data, sizeof(data));
  }
  return((field) ? true : false);
}  


/************************************************************************** 
 * GET_FIELD -- Find specified name and returns data                      * 
 *                                                                        * 
 * INPUT:    char *   - the id of the field that holds the data.          *
 *           uint16_t &   - the reference to store the data into    *
 *                                                                        * 
 * OUTPUT:    true if the field was found, false if it was not.           *
 *                                                                        * 
 * WARNINGS:  The data reference is not changed if the field is not       *
 *          found.                                                        *
 *                                                                        * 
 * HISTORY:                                                               * 
 *   04/23/1996 PWG : Created.                                            * 
 *========================================================================*/
int8_t PacketClass::Get_Field(char *id, uint16_t &data)
{
  FieldClass *field = Find_Field(id);
  if (field) {
    data = *((uint16_t *)field->Data);
  }
  return((field) ? true : false);
}  


/************************************************************************** 
 * GET_FIELD -- Find specified name and returns data                      * 
 *                                                                        * 
 * INPUT:    char *   - the id of the field that holds the data.          *
 *          long &   - the reference to store the data into               *
 *                                                                        * 
 * OUTPUT:    true if the field was found, false if it was not.           *
 *                                                                        * 
 * WARNINGS:  The data reference is not changed if the field is not       *
 *          found.                                                        *
 *                                                                        * 
 * HISTORY:                                                               * 
 *   04/23/1996 PWG : Created.                                            * 
 *========================================================================*/
int8_t PacketClass::Get_Field(char *id, int32_t &data)
{
  FieldClass *field = Find_Field(id);
  if (field) {
    memcpy(&data, field->Data, sizeof(data));
  }
  return((field) ? true : false);
}  



int8_t PacketClass::Get_Field(char *id, int &data)
{
  FieldClass *field = Find_Field(id);
  if (field) {
    data = *((int *)field->Data);
  }
  return((field) ? true : false);
}



/************************************************************************** 
 * GET_FIELD -- Find specified name and returns data as a string          * 
 *                                                                        * 
 * INPUT:    char *   - the id of the field that holds the data.          *
 *           char *   - the string to store the data into                 *
 *                                                                        * 
 * OUTPUT:    true if the field was found, false if it was not.           *
 *                                                                        * 
 * WARNINGS:  The string is not changed if the field is not found.  It    *
 *          is assumed that the string variabled specified by the         *
 *          pointer is large enough to hold the data.                     *
 *                                                                        * 
 * HISTORY:                                                               * 
 *   04/23/1996 PWG : Created.                                            * 
 *========================================================================*/
int8_t PacketClass::Get_Field(char *id, char *data)
{
  FieldClass *field = Find_Field(id);
  if (field) {
    strcpy(data, (char *)field->Data);
  }
  return((field) ? true : false);
}

/************************************************************************** 
 * GET_FIELD -- Find specified name and returns data                      * 
 *                                                                        * 
 * INPUT:    char *   - the id of the field that holds the data.          *
 *          uint32_t &   - the reference to store the data into      *
 *                                                                        * 
 * OUTPUT:    true if the field was found, false if it was not.           *
 *                                                                        * 
 * WARNINGS:  The data reference is not changed if the field is not       *
 *          found.                                                        *
 *                                                                        * 
 * HISTORY:                                                               * 
 *   04/23/1996 PWG : Created.                                            * 
 *========================================================================*/
int8_t PacketClass::Get_Field(char *id, uint32_t &data)
{
  FieldClass *field = Find_Field(id);
  if (field) {
    data = *((uint32_t *)field->Data);
  }
  return((field) ? true : false);
}  

int8_t PacketClass::Get_Field(char *id, unsigned  &data)
{
  FieldClass *field = Find_Field(id);
  if (field) {
    data = *((unsigned *)field->Data);
  }
  return((field) ? true : false);
}  



/**************************************************************************
 * GET_FIELD -- Find specified name and returns data                      *
 *                                                                        *
 * INPUT:   char *   - the id of the field that holds the data.           *
 *          void * - the reference to store the data into                 *
 *          int    - the length of the buffer passed in                   *
 *                                                                        *
 * OUTPUT:          true if the field was found, false if it was not.     *
 *                                                                        *
 * WARNINGS:    The data reference is not changed if the field is not     *
 *                                      found.                            *
 *                                                                        *
 * HISTORY:                                                               *
 *   6/4/96 4:46PM ST : Created                                           *
 *========================================================================*/
int8_t PacketClass::Get_Field(char *id, void *data, int &length)
{
   FieldClass *field = Find_Field(id);
   if (field) {
     memcpy (data, field->Data, MIN(field->Size, length));
     length = (int) field->Size;
   }
   return((field) ? true : false);
}


uint16_t PacketClass::Get_Field_Size(char* id)
{
   FieldClass *field = Find_Field(id);
   if (field) 
	return field->Get_Size();
   else
	return 0;
}
