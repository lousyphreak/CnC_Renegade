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
 *                 Project Name : WWAudio                                                      *
 *                                                                                             *
 *                     $Archive:: /Commando/Code/WWAudio/SoundBuffer.h                        $*
 *                                                                                             *
 *                       Author:: Patrick Smith                                                *
 *                                                                                             *
 *                     $Modtime:: 8/17/01 11:12a                                              $*
 *                                                                                             *
 *                    $Revision:: 7                                                           $*
 *                                                                                             *
 *---------------------------------------------------------------------------------------------*
 * Functions:                                                                                  *
 * - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */


#pragma once

#include <cstdint>

#ifndef __SOUNDBUFFER_H
#define __SOUNDBUFFER_H

#pragma warning (push, 3)
#include "compat/Mss.H"
#pragma warning (pop)

#include "refcount.h"


// Forward declarations
class FileClass;


/////////////////////////////////////////////////////////////////////////////////
//
//	SoundBufferClass
//
//	A sound buffer manages the raw sound data for any of the SoundObj types
// except for the StreamSoundClass object.
//
class SoundBufferClass : public RefCountClass
{
	public:

		//////////////////////////////////////////////////////////////////////
		//	Public constructors/destructors
		//////////////////////////////////////////////////////////////////////
		SoundBufferClass (void);
		virtual ~SoundBufferClass (void);

		//////////////////////////////////////////////////////////////////////
		//	Public operators
		//////////////////////////////////////////////////////////////////////
		operator uint8_t * (void)							{ return Get_Raw_Buffer (); }

		//////////////////////////////////////////////////////////////////////
		//	File methods
		//////////////////////////////////////////////////////////////////////
		virtual bool				Load_From_File (const char *filename);
		virtual bool				Load_From_File (FileClass &file);

		//////////////////////////////////////////////////////////////////////
		//	Memory methods
		//////////////////////////////////////////////////////////////////////
		virtual bool				Load_From_Memory (uint8_t *mem_buffer, uint32_t size);

		//////////////////////////////////////////////////////////////////////
		//	Buffer access
		//////////////////////////////////////////////////////////////////////		
		virtual uint8_t *	Get_Raw_Buffer (void) const	{ return m_Buffer; }
		virtual uint32_t	Get_Raw_Length (void) const	{ return m_Length; }

		//////////////////////////////////////////////////////////////////////
		//	Information methods
		//////////////////////////////////////////////////////////////////////
		virtual const char *		Get_Filename (void) const		{ return m_Filename; }
		virtual void				Set_Filename (const char *name);
		virtual uint32_t	Get_Duration (void) const		{ return m_Duration; }
		virtual uint32_t	Get_Rate (void) const			{ return m_Rate; }
		virtual uint32_t	Get_Bits (void) const			{ return m_Bits; }
		virtual uint32_t	Get_Channels (void) const		{ return m_Channels; }
		virtual uint32_t	Get_Type (void) const			{ return m_Type; }

		//////////////////////////////////////////////////////////////////////
		//	Type methods
		//////////////////////////////////////////////////////////////////////
		virtual bool				Is_Streaming (void) const		{ return false; }

	protected:

		//////////////////////////////////////////////////////////////////////
		//	Protected methods
		//////////////////////////////////////////////////////////////////////
		virtual void			Free_Buffer (void);
		virtual void			Determine_Stats (uint8_t *buffer);

		//////////////////////////////////////////////////////////////////////
		//	Protected member data
		//////////////////////////////////////////////////////////////////////		
		uint8_t *		m_Buffer;
		uint32_t			m_Length;
		char *					m_Filename;
		uint32_t			m_Duration;
		uint32_t			m_Rate;
		uint32_t			m_Bits;
		uint32_t			m_Channels;
		uint32_t			m_Type;
};


/////////////////////////////////////////////////////////////////////////////////
//
//	StreamSoundBufferClass
//
//	A sound buffer manages the raw sound data for any of the SoundObj types
// except for the StreamSoundClass object.
//
class StreamSoundBufferClass : public SoundBufferClass
{
	public:

		//////////////////////////////////////////////////////////////////////
		//	Public constructors/destructors
		//////////////////////////////////////////////////////////////////////
		StreamSoundBufferClass (void);
		virtual ~StreamSoundBufferClass (void);

		//////////////////////////////////////////////////////////////////////
		//	File methods
		//////////////////////////////////////////////////////////////////////
		virtual bool			Load_From_File (const char *filename);
		virtual bool			Load_From_File (FileClass &file);

		//////////////////////////////////////////////////////////////////////
		//	Memory methods
		//////////////////////////////////////////////////////////////////////
		virtual bool			Load_From_Memory (uint8_t *mem_buffer, uint32_t size) { return false; }

		//////////////////////////////////////////////////////////////////////
		//	Type methods
		//////////////////////////////////////////////////////////////////////
		virtual bool			Is_Streaming (void) const		{ return true; }

	protected:

		//////////////////////////////////////////////////////////////////////
		//	Protected methods
		//////////////////////////////////////////////////////////////////////
		virtual void			Free_Buffer (void);
		virtual bool			Load_From_File (HANDLE hfile, uint32_t size, uint32_t offset);

		//////////////////////////////////////////////////////////////////////
		//	Protected member data
		//////////////////////////////////////////////////////////////////////		
};


#endif //__SOUNDBUFFER_H
