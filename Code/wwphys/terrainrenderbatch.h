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
**	MERCHANTIBILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
**	GNU General Public License for more details.
**
**	You should have received a copy of the GNU General Public License
**	along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#pragma once

#include "always.h"
#include "shader.h"
#include "ww3d.h"

#include <unordered_map>
#include <vector>

class IndexBufferClass;
class MaterialPassClass;
class RenegadeTerrainPatchClass;
class RenderIndexBufferClass;
class RenderInfoClass;
class RenderVertexBufferClass;
class TextureClass;
class VertexMaterialClass;
class VertexBufferClass;

class TerrainRenderBatchPageClass
{
public:
	struct DrawRange
	{
		int MaterialIndex = 0;
		int PassType = 0;
		unsigned short StartIndex = 0;
		unsigned short PolygonCount = 0;
		unsigned short MinVertexIndex = 0;
		unsigned short VertexCount = 0;
		TextureClass *Textures[2] = { NULL, NULL };
		VertexMaterialClass *Material = NULL;
		ShaderClass Shader;
		bool ReceiveShadows = true;
		bool CastShadows = false;
	};

	TerrainRenderBatchPageClass();
	~TerrainRenderBatchPageClass();

	bool Is_Valid() const { return Valid; }
	bool Has_Draws() const { return !DrawRanges.empty(); }
	RenegadeTerrainPatchClass *Peek_Patch() const { return Patch; }

private:
	friend class RenegadeTerrainPatchClass;
	friend class TerrainRenderBatchManagerClass;

	void Reset();

	RenegadeTerrainPatchClass *Patch;
	RenderVertexBufferClass *VertexBuffer;
	RenderIndexBufferClass *IndexBuffer;
	std::vector<DrawRange> DrawRanges;
	bool Valid;
};

class TerrainRenderBatchManagerClass
{
public:
	TerrainRenderBatchManagerClass();
	~TerrainRenderBatchManagerClass();

	bool Prepare_Patch(RenegadeTerrainPatchClass *patch);
	void Unregister_Patch(RenegadeTerrainPatchClass *patch);
	void Reset();

	bool Render_Patch(RenegadeTerrainPatchClass *patch, RenderInfoClass &rinfo);
	bool Render_Patch_Material_Passes(
		RenegadeTerrainPatchClass *patch,
		RenderInfoClass &rinfo,
		MaterialPassClass * const *passes,
		int pass_count);

	static bool Render_Immediate_Patch(RenegadeTerrainPatchClass *patch, RenderInfoClass &rinfo);
	static bool Render_Immediate_Patch_Material_Passes(
		RenegadeTerrainPatchClass *patch,
		RenderInfoClass &rinfo,
		MaterialPassClass * const *passes,
		int pass_count);

private:
	TerrainRenderBatchPageClass *Find_Or_Create_Page(RenegadeTerrainPatchClass *patch);
	TerrainRenderBatchPageClass *Find_Page(RenegadeTerrainPatchClass *patch);
	bool Submit_Draw(
		const TerrainRenderBatchPageClass &page,
		const TerrainRenderBatchPageClass::DrawRange &range,
		RenderInfoClass &rinfo,
		TextureClass * const *override_textures,
		VertexMaterialClass *override_material,
		const ShaderClass *override_shader,
		bool receive_shadows);

	std::unordered_map<RenegadeTerrainPatchClass *, TerrainRenderBatchPageClass *> Pages;
};
