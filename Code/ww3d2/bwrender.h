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
 *                 Project Name : ww3d2                                                        *
 *                                                                                             *
 *                     $Archive:: /Commando/Code/ww3d2/bwrender.h                             $*
 *                                                                                             *
 *              Original Author:: Jani Penttinen                                               *
 *                                                                                             *
 *                      $Author:: Greg_h                                                      $*
 *                                                                                             *
 *                     $Modtime:: 4/04/01 10:36a                                              $*
 *                                                                                             *
 *                    $Revision:: 2                                                           $*
 *                                                                                             *
 *---------------------------------------------------------------------------------------------*
 * Functions:                                                                                  *
 * - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

#if defined(_MSC_VER)
#pragma once

#include <cstdint>
#endif

#ifndef BWRENDER_H
#define BWRENDER_H


#include "always.h"
#include "vector2.h"
#include "vector3.h"
#include "vector3i.h"

/**
** BWRenderClass
** This class implements a simple black-and-white triangle rasterizer which
** can be used to generate shadow textures.  It is faster than a general purpose
** software renderer due to the fact that no z-buffering or sorting is needed and
** texturing isn't supported.
** (gth) 04/02/2001 - I'm going to add render-to-texture code to Renegade so this
** class may be obsolete.
*/
class BWRenderClass
{
	// Internal pixel buffer used by the triangle renderer
	// The buffer is not allocated or freed by this class.
	class Buffer
	{
		uint8_t* buffer;
		int scale;
		int minv;
		int maxv;
	public:
		Buffer(uint8_t* buffer, int scale);
		~Buffer();

		void Set_H_Line(int start_x, int end_x, int y);
		void Fill(uint8_t c);
		inline int Scale() const { return scale; }
	} pixel_buffer;

	Vector2* vertices;

	void Render_Preprocessed_Triangle(Vector3& xcf,Vector3i& yci);

public:
	BWRenderClass(uint8_t* buffer, int scale);
	~BWRenderClass();

	void Fill(uint8_t c);
	void Set_Vertex_Locations(Vector2* vertices,int count); // Warning! Contents are modified!
	void Render_Triangles(const uint32_t* indices,int index_count);
	void Render_Triangle_Strip(const uint32_t* indices,int index_count);
};


#endif //BWRENDER_H

