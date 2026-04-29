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

#if defined(_MSC_VER)
#pragma once
#endif

#ifndef SORTING_RENDERER_H
#define SORTING_RENDERER_H

#include "always.h"
#include "ww3d.h"

struct SortingNodeStruct;
class SphereClass;

class SortingRendererClass
{
	static bool _EnableTriangleDraw;

	static void Flush_Sorting_Pool();
	static void Insert_To_Sorting_Pool(SortingNodeStruct* state);
	static void Insert_Triangles_Internal(
		const SphereClass& bounding_sphere,
		unsigned short start_index, 
		unsigned short polygon_count,
		unsigned short min_vertex_index,
		unsigned short vertex_count,
		bool use_explicit_shadow_flags,
		bool receive_shadows,
		bool cast_shadows);

public:
	static void Insert_Triangles(
		const SphereClass& bounding_sphere,
		unsigned short start_index, 
		unsigned short polygon_count,
		unsigned short min_vertex_index,
		unsigned short vertex_count);

	static void Insert_Triangles(
		const SphereClass& bounding_sphere,
		unsigned short start_index, 
		unsigned short polygon_count,
		unsigned short min_vertex_index,
		unsigned short vertex_count,
		bool receive_shadows,
		bool cast_shadows);

	static void Insert_Triangles(
		unsigned short start_index, 
		unsigned short polygon_count,
		unsigned short min_vertex_index,
		unsigned short vertex_count);

	static void Insert_Triangles(
		unsigned short start_index, 
		unsigned short polygon_count,
		unsigned short min_vertex_index,
		unsigned short vertex_count,
		bool receive_shadows,
		bool cast_shadows);

	static void Insert_Fixed_Function_Draw(
		const SphereClass& bounding_sphere,
		const WW3D::FixedFunctionSubmitDesc &submission);

	static void Flush();
	static void Deinit();

	static void _Enable_Triangle_Draw(bool enable) { _EnableTriangleDraw=enable; }
	static bool _Is_Triangle_Draw_Enabled() { return _EnableTriangleDraw; }
};

#endif
