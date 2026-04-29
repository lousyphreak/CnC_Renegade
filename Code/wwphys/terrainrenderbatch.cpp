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

#include "terrainrenderbatch.h"

#include "camera.h"
#include "indexbuffer.h"
#include "matpass.h"
#include "renegadeterrainpatch.h"
#include "rinfo.h"
#include "texture.h"
#include "vertexbuffer.h"
#include "vertmaterial.h"
#include "wwdebug.h"

TerrainRenderBatchPageClass::TerrainRenderBatchPageClass() :
	Patch(NULL),
	VertexBuffer(NULL),
	IndexBuffer(NULL),
	Valid(false)
{
}

TerrainRenderBatchPageClass::~TerrainRenderBatchPageClass()
{
	Reset();
}

void TerrainRenderBatchPageClass::Reset()
{
	REF_PTR_RELEASE(VertexBuffer);
	REF_PTR_RELEASE(IndexBuffer);
	DrawRanges.clear();
	Patch = NULL;
	Valid = false;
}

TerrainRenderBatchManagerClass::TerrainRenderBatchManagerClass()
{
}

TerrainRenderBatchManagerClass::~TerrainRenderBatchManagerClass()
{
	Reset();
}

TerrainRenderBatchPageClass *TerrainRenderBatchManagerClass::Find_Or_Create_Page(RenegadeTerrainPatchClass *patch)
{
	if (patch == NULL) {
		return NULL;
	}

	TerrainRenderBatchPageClass *&page = Pages[patch];
	if (page == NULL) {
		page = new TerrainRenderBatchPageClass;
	}
	return page;
}

TerrainRenderBatchPageClass *TerrainRenderBatchManagerClass::Find_Page(RenegadeTerrainPatchClass *patch)
{
	std::unordered_map<RenegadeTerrainPatchClass *, TerrainRenderBatchPageClass *>::iterator it = Pages.find(patch);
	return it != Pages.end() ? it->second : NULL;
}

bool TerrainRenderBatchManagerClass::Prepare_Patch(RenegadeTerrainPatchClass *patch)
{
	TerrainRenderBatchPageClass *page = Find_Or_Create_Page(patch);
	if (page == NULL) {
		return false;
	}

	if (!page->Is_Valid() || patch->Are_Terrain_Batch_Buffers_Dirty()) {
		return patch->Build_Terrain_Batch_Page(*page);
	}

	return true;
}

void TerrainRenderBatchManagerClass::Unregister_Patch(RenegadeTerrainPatchClass *patch)
{
	std::unordered_map<RenegadeTerrainPatchClass *, TerrainRenderBatchPageClass *>::iterator it = Pages.find(patch);
	if (it != Pages.end()) {
		delete it->second;
		Pages.erase(it);
	}
}

void TerrainRenderBatchManagerClass::Reset()
{
	for (std::unordered_map<RenegadeTerrainPatchClass *, TerrainRenderBatchPageClass *>::iterator it = Pages.begin();
		  it != Pages.end();
		  ++it)
	{
		delete it->second;
	}
	Pages.clear();
}

bool TerrainRenderBatchManagerClass::Submit_Draw(
	const TerrainRenderBatchPageClass &page,
	const TerrainRenderBatchPageClass::DrawRange &range,
	RenderInfoClass &rinfo,
	TextureClass * const *override_textures,
	VertexMaterialClass *override_material,
	const ShaderClass *override_shader,
	bool receive_shadows)
{
	if (!page.Is_Valid() || page.VertexBuffer == NULL || page.IndexBuffer == NULL || range.PolygonCount == 0 || range.VertexCount == 0) {
		return true;
	}

	TextureClass *textures[2] = {
		override_textures != NULL ? override_textures[0] : range.Textures[0],
		override_textures != NULL ? override_textures[1] : range.Textures[1]
	};
	VertexMaterialClass *material = override_material != NULL ? override_material : range.Material;
	ShaderClass shader = override_shader != NULL ? *override_shader : range.Shader;

	WW3D::FixedFunctionStateDesc render_state;
	WW3D::Capture_Current_Fixed_Function_State(render_state, material);

	WW3D::FixedFunctionSubmitDesc submission;
	submission.VertexBuffer = page.VertexBuffer;
	submission.IndexBuffer = page.IndexBuffer;
	submission.StartIndex = range.StartIndex;
	submission.PolygonCount = range.PolygonCount;
	submission.MinVertexIndex = range.MinVertexIndex;
	submission.VertexCount = range.VertexCount;
	submission.Textures[0] = textures[0];
	submission.Textures[1] = textures[1];
	submission.Material = material;
	submission.Shader = shader;
	submission.WorldTransform = Matrix4(page.Patch->Get_Transform());
	WW3D::Get_Transform(WW3D::RENDER_TRANSFORM_VIEW, submission.ViewTransform);
	WW3D::Get_Transform(WW3D::RENDER_TRANSFORM_PROJECTION, submission.ProjectionTransform);
	submission.Lighting = rinfo.lighting_submission;
	submission.RenderState = &render_state;
	submission.ReceiveShadows = receive_shadows;
	submission.CastShadows = range.CastShadows;

	const bool submitted = WW3D::Submit_Fixed_Function_Draw(submission);
	WWASSERT(submitted);
	return submitted;
}

bool TerrainRenderBatchManagerClass::Render_Patch(RenegadeTerrainPatchClass *patch, RenderInfoClass &rinfo)
{
	if (patch == NULL) {
		return false;
	}

	TerrainRenderBatchPageClass *page = Find_Page(patch);
	if (page == NULL || !page->Is_Valid() || patch->Are_Terrain_Batch_Buffers_Dirty()) {
		WWASSERT_PRINT(0, "Terrain batch page was not prepared before render; rebuilding through compatibility path.\n");
		if (!Prepare_Patch(patch)) {
			return false;
		}
		page = Find_Page(patch);
	}

	if (page == NULL) {
		return false;
	}

	bool ok = true;
	for (std::vector<TerrainRenderBatchPageClass::DrawRange>::const_iterator it = page->DrawRanges.begin();
		  it != page->DrawRanges.end();
		  ++it)
	{
		ok = Submit_Draw(*page, *it, rinfo, NULL, NULL, NULL, it->ReceiveShadows) && ok;
	}
	return ok;
}

bool TerrainRenderBatchManagerClass::Render_Patch_Material_Passes(
	RenegadeTerrainPatchClass *patch,
	RenderInfoClass &rinfo,
	MaterialPassClass * const *passes,
	int pass_count)
{
	if (patch == NULL || passes == NULL || pass_count <= 0) {
		return false;
	}

	TerrainRenderBatchPageClass *page = Find_Page(patch);
	if (page == NULL || !page->Is_Valid() || patch->Are_Terrain_Batch_Buffers_Dirty()) {
		WWASSERT_PRINT(0, "Terrain batch page was not prepared before material-pass render; rebuilding through compatibility path.\n");
		if (!Prepare_Patch(patch)) {
			return false;
		}
		page = Find_Page(patch);
	}

	if (page == NULL) {
		return false;
	}

	bool ok = true;
	for (int pass_index = 0; pass_index < pass_count; ++pass_index) {
		MaterialPassClass *matpass = passes[pass_index];
		if (matpass == NULL) {
			continue;
		}

		TextureClass *textures[2] = {
			matpass->Peek_Texture(0),
			matpass->Peek_Texture(1)
		};
		ShaderClass shader = matpass->Peek_Shader();
		const bool receive_shadows = shader.Get_Dst_Blend_Func() == ShaderClass::DSTBLEND_ZERO;

		for (std::vector<TerrainRenderBatchPageClass::DrawRange>::const_iterator it = page->DrawRanges.begin();
			  it != page->DrawRanges.end();
			  ++it)
		{
			ok = Submit_Draw(*page, *it, rinfo, textures, matpass->Peek_Material(), &shader, receive_shadows) && ok;
		}
	}

	return ok;
}

bool TerrainRenderBatchManagerClass::Render_Immediate_Patch(RenegadeTerrainPatchClass *patch, RenderInfoClass &rinfo)
{
	TerrainRenderBatchManagerClass manager;
	return manager.Prepare_Patch(patch) && manager.Render_Patch(patch, rinfo);
}

bool TerrainRenderBatchManagerClass::Render_Immediate_Patch_Material_Passes(
	RenegadeTerrainPatchClass *patch,
	RenderInfoClass &rinfo,
	MaterialPassClass * const *passes,
	int pass_count)
{
	TerrainRenderBatchManagerClass manager;
	return manager.Prepare_Patch(patch) && manager.Render_Patch_Material_Passes(patch, rinfo, passes, pass_count);
}
