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

/****************************************************************************
*
* FILE
*     $Archive: /Commando/Code/Launcher/Toolkit/Storage/Stream.h $
*
* DESCRIPTION
*     Base class for data streaming functionality
*
* PROGRAMMER
*     Denzil E. Long, Jr.
*     $Author: Denzil_l $
*
* VERSION INFO
*     $Modtime: 9/23/00 6:17p $
*     $Revision: 1 $
*
****************************************************************************/

#ifndef STREAM_H
#define STREAM_H

#include <Support\UTypes.h>

class Stream
	{
	public:
		// Stream marker positioning
		typedef enum
			{
			FromStart = 0,
			FromMarker,
			FromEnd,
			} EStreamFrom;

		//! Get the length of the stream
		virtual uint32_t GetLength(void) = 0;

		//! Set the length of the stream
		virtual void SetLength(uint32_t length) = 0;

		//! Get current position of stream marker
		virtual uint32_t GetMarker(void) = 0;

		//! Set position of stream marker
		virtual void SetMarker(int32_t offset, EStreamFrom from) = 0;

		//! End of stream test
		virtual bool AtEnd(void) = 0;
		
		//! Retrieve a sequence of bytes.
		virtual uint32_t GetBytes(void* ptr, uint32_t bytes) = 0;

		//! Write a sequence of bytes
		virtual uint32_t PutBytes(const void* ptr, uint32_t bytes) = 0;
		
		//! Retrieve a sequence of bytes without advancing marker.
		virtual uint32_t PeekBytes(void* ptr, uint32_t bytes) = 0;

		//! Flush the stream
		virtual void Flush(void) = 0;
	};

#endif // STREAM_H
