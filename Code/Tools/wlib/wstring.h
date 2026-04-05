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
File Name   : main.cpp
Author      : Neal Kettler
Start Date  : June 1, 1997
Last Update : June 17, 1997  
\****************************************************************************/

#ifndef WSTRING_HEADER
#define WSTRING_HEADER

#include <cstdint>

#include <stdio.h>
#include <stdlib.h>
#include "wstypes.h"

class Wstring
{
 public: 
           Wstring();
           Wstring(IN Wstring &other);
           Wstring(IN char *string);
          ~Wstring();

   void    clear(void);

   int8_t    cat(IN char *string);
   int8_t    cat(uint32_t size,IN char *string);
   int8_t    cat(IN Wstring &string);

   void    cellCopy(OUT char *dest, uint32_t len);
   char    remove(int32_t pos, int32_t count);
   int8_t    removeChar(char c);
   void    removeSpaces(void);
   char   *get(void) RO;
   char    get(uint32_t index) RO;
   uint32_t  length(void) RO;
   int8_t    insert(char c, uint32_t pos);
   int8_t    insert(char *instring, uint32_t pos);
   int8_t    beautifyNumber();
   int8_t    replace(IN char *replaceThis,IN char *withThis);
   char    set(IN char *str);
   char    set(uint32_t size,IN char *str);
   int8_t    set(char c, uint32_t index);
   char    setFormatted(IN char *str, ...);		// Added by Joe Howes
   void    setSize(int32_t bytes);  // create an empty string
   void    toLower(void);
   void    toUpper(void);
   int8_t    truncate(uint32_t len);
   int8_t    truncate(char c);  // trunc after char c
   int32_t  getToken(int offset,char *delim,Wstring &out) RO;
   int32_t  getLine(int offset, Wstring &out);
   void    strgrow(int length);

   int8_t    operator==(IN char *other) RO;
   int8_t    operator==(IN Wstring &other) RO;
   int8_t    operator!=(IN char *other) RO;
   int8_t    operator!=(IN Wstring &other) RO;

   Wstring  &operator=(IN char *other);
   Wstring  &operator=(IN Wstring &other);
   Wstring  &operator+=(IN char *other);
   Wstring  &operator+=(IN Wstring &other);
   Wstring   operator+(IN char *other);
   Wstring   operator+(IN Wstring &other);

   bool operator<(IN Wstring &other) RO;

 private:
   char    *str;      // Pointer to allocated string.
   int      strsize;  // allocated data length
};

#endif
