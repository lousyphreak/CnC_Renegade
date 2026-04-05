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

/****************************************************************************\
*        C O N F I D E N T I A L --- W E S T W O O D   S T U D I O S         *
******************************************************************************
Project Name: Carpenter  (The RedAlert ladder creator)
File Name   : configfile.h 
Author      : Neal Kettler
Start Date  : June 9, 1997
Last Update : May 13, 1999  
\***************************************************************************/

#ifndef CONFIGFILE_HEADER
#define CONFIGFILE_HEADER

#include <cstdint>

#include "wstypes.h"
#include "dictionary.h"
#include "wstring.h"
#include "critsec.h"
#include "arraylist.h"

class ConfigFile
{
 public:
             ConfigFile();
            ~ConfigFile();
 int8_t        readFile(FILE *config);
 int8_t        getString(IN Wstring &key,OUT Wstring &value, IN char *section=NULL) const;
 int8_t        getString(IN char *key,OUT Wstring &value, IN char *section=NULL) const;

 int8_t        getInt(IN Wstring &key,OUT int32_t &value, IN char *section=NULL) const;
 int8_t        getInt(IN char *key,OUT int32_t &value, IN char *section=NULL) const;

 int8_t        getInt(IN Wstring &key,OUT int16_t &value, IN char *section=NULL) const;
 int8_t        getInt(IN char *key,OUT int16_t &value, IN char *section=NULL) const;

             // Enumerate through the config lines
 int8_t        enumerate(int &index, int &offset, Wstring &key, Wstring &value, IN char *section=NULL) const;

 // Manual update of config file
 int8_t        setString(IN Wstring &key,IN Wstring &value, IN char *section=NULL);
 int8_t        setString(IN char *key,IN Wstring &value, IN char *section=NULL);
 int8_t        setInt(IN Wstring &key,IN int32_t &value, IN char *section=NULL);
 int8_t        setInt(IN char *key,IN int32_t &value, IN char *section=NULL);
 int8_t        removeEntry(IN Wstring &key, IN char *section=NULL);
 int8_t        removeEntry(IN char *key, IN char *section=NULL);
 int8_t        writeFile(FILE *config); // Does not preserve comments, etc

 ArrayList<Wstring> sectionList; // stores the names of all sections

 private:
  Dictionary<Wstring,Wstring> Dictionary_; // stores the mappings from keys
                                           //  to value strings

  // The lock is only needed around the immediate access to the dictionary, no writes
  //   are allowed so you don't need to worry about an outer lock around the enumerate
  CritSec                     Critsec_;    // lock around dictionary
};

#endif
