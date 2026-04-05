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

/******************************************************************************
*
* FILE
*     $Archive: /Commando/Code/Launcher/Toolkit/Support/UString.h $
*
* DESCRIPTION
*     String management class (Unicode)
*
* PROGRAMMER
*     Denzil E. Long, Jr.
*     $Author: Denzil_l $
*
* VERSION INFO
*     $Modtime: 9/23/00 6:20p $
*     $Revision: 1 $
*
******************************************************************************/

#ifndef USTRING_H
#define USTRING_H

#include "UTypes.h"
#include "RefCounted.h"

class UString
	: public RefCounted
	{
	public:
		// Constructors
		UString();
		UString(uint32_t capacity);
		UString(const char* s);
		UString(const wchar_t* ws);
		UString(const UString& s);
		virtual ~UString();

		//! Get the length of the string
		uint32_t Length(void) const;

		//! Copy string
		void Copy(const char* s);
		void Copy(const wchar_t* ws);
		void Copy(const UString& s);

		//! Concatenate string
		void Concat(const char* s);
		void Concat(const wchar_t* ws);
		void Concat(const UString& s);

		//! Compare strings
		int32_t Compare(const char* s) const;
		int32_t Compare(const wchar_t* s) const;
		int32_t Compare(const UString& s) const;

		//! Compare strings not case-sensitive
		int32_t CompareNoCase(const char* s) const;
		int32_t CompareNoCase(const wchar_t* ws) const;
		int32_t CompareNoCase(const UString& s) const;

		//! Find the first occurance of character
		int32_t Find(char c) const;
		int32_t Find(wchar_t wc) const;

		//! Find the last occurance of a character
		int32_t FindLast(char c) const;
		int32_t FindLast(wchar_t c) const;

		//! Find a substring
		UString SubString(const char* s);
		UString SubString(const wchar_t* ws);
		UString SubString(const UString& s);

		//! Extract left part of the string
		UString Left(uint32_t count);

		//! Extract middle part of the string.
		UString Middle(uint32_t first, uint32_t count);

		//! Extract right part of the string.
		UString Right(uint32_t count);

		//! Convert string to uppercase
		void ToUpper(void);
		
		//! Convert string to lowercase
		void ToLower(void);
		
		//! Reverse characters of string
		void Reverse(void);

		//! Remove leading and trailing characters from string.
		//  Returns true if any characters removed
		bool Trim(const char* trimChars);
		bool Trim(const wchar_t* trimChars);
		bool Trim(const UString& trimChars);

		//! Remove characters from left side of string
		//  Returns true if any characters removed
		bool TrimLeft(const char* trimChars);
		bool TrimLeft(const wchar_t* trimChars);
		bool TrimLeft(const UString& trimChars);

		//! Remove characters from right side of string
		//  Returns true if any characters removed
		bool TrimRight(const char* trimChars);
		bool TrimRight(const wchar_t* trimChars);
		bool TrimRight(const UString& trimChars);

		// Convert string to ANSI
		void ConvertToANSI(char* buffer, uint32_t bufferLength) const;

		//! Get the size (in bytes) of the string.
		uint32_t Size(void) const;

		//! Get the maximum number of characters this string can hold.
		uint32_t Capacity(void) const;

		//! Resize the string
		bool Resize(uint32_t size);

		const wchar_t* Get(void) const
			{return (mData != NULL) ? mData : L"";}

		//! Assignment operator
		UString operator=(const char* s)
			{Copy(s); return *this;};

		UString operator=(const wchar_t* ws)
			{Copy(ws); return *this;};
		
		UString operator=(const UString& s)
			{Copy(s); return *this;};

		//! Addition operator (concatenate)
		UString operator+(const char* s)
			{UString ns(*this); ns += s; return ns;}

		UString operator+(const wchar_t* ws)
			{UString ns(*this); ns += ws; return ns;}

		UString operator+(const UString& s)
			{UString ns(*this); ns += s; return ns;}

		UString operator+=(const char* s)
			{Concat(s); return *this;}

		UString operator+=(const wchar_t* ws)
			{Concat(ws); return *this;}

		UString operator+=(const UString& s)
			{Concat(s); return *this;}

		//! Equal operator (case sensitive compare)
		bool operator==(const char* s)
			{return (Compare(s) == 0);}

		bool operator==(const wchar_t* ws)
			{return (Compare(ws) == 0);}

		bool operator==(const UString& s)
			{return (Compare(s) == 0);}

		bool operator!=(const char* s)
			{return (Compare(s) != 0);}

		bool operator!=(const wchar_t* ws)
			{return (Compare(ws) != 0);}
		
		bool operator!=(const UString& s)
			{return (Compare(s) != 0);}

		//! Less than operator (case sensitive compare)
		bool operator<(const char* s)
			{return (Compare(s) == -1);}

		bool operator<(const wchar_t* ws)
			{return (Compare(ws) == -1);}

		bool operator<(const UString& s)
			{return (Compare(s) == -1);}

		bool operator<=(const char* s)
			{return (Compare(s) <= 0);}

		bool operator<=(const wchar_t* ws)
			{return (Compare(ws) <= 0);}

		bool operator<=(const UString& s)
			{return (Compare(s) <= 0);}

		//! Greater than operator (case sensitive compare)
		bool operator>(const char* s)
			{return (Compare(s) == 1);}

		bool operator>(const wchar_t* ws)
			{return (Compare(ws) == 1);}

		bool operator>(const UString& s)
			{return (Compare(s) == 1);}

		bool operator>=(const char* s)
			{return (Compare(s) >= 0);}

		bool operator>=(const wchar_t* ws)
			{return (Compare(ws) >= 0);}

		bool operator>=(const UString& s)
			{return (Compare(s) >= 0);}

		// Conversion operator
		operator const wchar_t*() const
			{return Get();}

	private:
		bool AllocString(uint32_t size);

		wchar_t* mData;
		uint32_t mCapacity;
	};

#endif // USTRING_H
