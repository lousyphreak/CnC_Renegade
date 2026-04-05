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
 *                    File Name : PACKET.H                                 * 
 *                                                                         * 
 *                   Programmer : Philip W. Gorrow                         * 
 *                                                                         * 
 *                   Start Date : 04/19/96                                 * 
 *                                                                         * 
 *                  Last Update : April 19, 1996 [PWG]                     * 
 *                                                                         * 
 * This header defines the functions for the PacketClass.  The packet      *
 * class is used to create a linked list of field entries which can be     * 
 * converted to a linear packet in a COMMS API compatible format.          *
 *                                                                         *
 * Packets can be created empty and then have fields added to them or can  *
 * be created from an existing linear packet.                              *
 *                                                                         *
 *-------------------------------------------------------------------------* 
 * Functions:                                                              * 
 * - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

#include "field.h"
#include <cstdint>
#include "wlib/wstypes.h"


class PacketClass
{
  public:

    PacketClass(int16_t id = 0)
    {
      Size      = 0;
      ID        = id;
      Head      = 0;
    }
    PacketClass(uint8_t *cur_buf);
    ~PacketClass();

    //
    // This function allows us to add a field to the start of the list.  As the field is just
    //   a big linked list it makes no difference which end we add a member to.
    //
    void Add_Field(FieldClass *field);

    //
    // These conveniance functions allow us to add a field directly to the list without
    // having to worry about newing one first.
    //
    void Add_Field(char *field, char data) {Add_Field(new FieldClass(field, data));};
    void Add_Field(char *field, uint8_t data) {Add_Field(new FieldClass(field, data));};
    void Add_Field(char *field, int16_t data) {Add_Field(new FieldClass(field, data));};
    void Add_Field(char *field, uint16_t data) {Add_Field(new FieldClass(field, data));};
    void Add_Field(char *field, int32_t data) {Add_Field(new FieldClass(field, data));};
    void Add_Field(char *field, uint32_t data) {Add_Field(new FieldClass(field, data));};
    void Add_Field(char *field, char *data) {Add_Field(new FieldClass(field, data));};
    void Add_Field(char *field, void *data, int length) {Add_Field(new FieldClass(field, data, length));};

    //
    // These functions search for a field of a given name in the list and 
    // return the data via a reference value.
    //
    FieldClass *Find_Field(char *id);

    int8_t Get_Field(char *id, int &data);
    int8_t Get_Field(char *id, char &data);
    int8_t Get_Field(char *id, uint8_t &data);
    int8_t Get_Field(char *id, int16_t &data);
    int8_t Get_Field(char *id, uint16_t &data);
    int8_t Get_Field(char *id, int32_t &data);
    int8_t Get_Field(char *id, uint32_t &data);
    int8_t Get_Field(char *id, unsigned &data);
    int8_t Get_Field(char *id, char *data);
    int8_t Get_Field(char *id, void *data, int &length);
    uint16_t Get_Field_Size(char* id); 


    uint8_t *Create_Comms_Packet(int &size);
        
  private:
    uint16_t   Size;
    int16_t          ID;
    FieldClass      *Head;
    FieldClass      *Current;
};
