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

#include <algorithm>
#include <cstdint>

namespace
{
uintptr_t Build_Terrain_Texture_Key(TextureClass * const *textures)
{
	uintptr_t key = 0;
	for (unsigned stage = 0; stage < 2; ++stage) {
		const uintptr_t pointer_value = reinterpret_cast<uintptr_t>(textures[stage]);
		key ^= pointer_value + 0x9e3779b9u + (key << 6) + (key >> 2);
	}
	return key;
}

bool Terrain_Texture_Slots_Match(
	const TerrainRenderBatchPageClass::DrawRange &lhs,
	const TerrainRenderBatchPageClass::DrawRange &rhs)
{
	return lhs.Textures[0] == rhs.Textures[0] && lhs.Textures[1] == rhs.Textures[1];
}

bool Can_Merge_Terrain_Draw_Ranges(
	const TerrainRenderBatchPageClass::DrawRange &lhs,
	const TerrainRenderBatchPageClass::DrawRange &rhs)
{
	if (lhs.PassType != rhs.PassType ||
		lhs.Material != rhs.Material ||
		lhs.Shader.Get_Bits() != rhs.Shader.Get_Bits() ||
		!Terrain_Texture_Slots_Match(lhs, rhs) ||
		lhs.ReceiveShadows != rhs.ReceiveShadows ||
		lhs.CastShadows != rhs.CastShadows)
	{
		return false;
	}

	const unsigned lhs_end_index = static_cast<unsigned>(lhs.StartIndex) + (static_cast<unsigned>(lhs.PolygonCount) * 3u);
	const unsigned lhs_end_vertex = static_cast<unsigned>(lhs.MinVertexIndex) + static_cast<unsigned>(lhs.VertexCount);
	return lhs_end_index == rhs.StartIndex && lhs_end_vertex == rhs.MinVertexIndex;
}

TerrainRenderBatchPageClass::DrawRange Merge_Terrain_Draw_Ranges(
	const TerrainRenderBatchPageClass::DrawRange &lhs,
	const TerrainRenderBatchPageClass::DrawRange &rhs)
{
	TerrainRenderBatchPageClass::DrawRange merged = lhs;
	merged.PolygonCount = static_cast<unsigned short>(lhs.PolygonCount + rhs.PolygonCount);
	merged.VertexCount = static_cast<unsigned short>((rhs.MinVertexIndex + rhs.VertexCount) - lhs.MinVertexIndex);
	return merged;
}

bool Sort_Queued_Terrain_Draw_Task(
	const TerrainRenderBatchManagerClass::QueuedDrawTask &lhs,
	const TerrainRenderBatchManagerClass::QueuedDrawTask &rhs)
{
	const TerrainRenderBatchPageClass::DrawRange &lhs_range = lhs.Range;
	const TerrainRenderBatchPageClass::DrawRange &rhs_range = rhs.Range;

	if (lhs_range.PassType != rhs_range.PassType) {
		return lhs_range.PassType < rhs_range.PassType;
	}
	if (lhs_range.Shader.Get_Bits() != rhs_range.Shader.Get_Bits()) {
		return lhs_range.Shader.Get_Bits() < rhs_range.Shader.Get_Bits();
	}

	const uintptr_t lhs_texture_key = Build_Terrain_Texture_Key(lhs_range.Textures);
	const uintptr_t rhs_texture_key = Build_Terrain_Texture_Key(rhs_range.Textures);
	if (lhs_texture_key != rhs_texture_key) {
		return lhs_texture_key < rhs_texture_key;
	}
	if (lhs_range.Material != rhs_range.Material) {
		return lhs_range.Material < rhs_range.Material;
	}
	if (lhs.Page != rhs.Page) {
		return lhs.Page < rhs.Page;
	}
	return lhs_range.StartIndex < rhs_range.StartIndex;
}
}

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
	QueuedDraws.clear();
}

bool TerrainRenderBatchManagerClass::Submit_Draw_Task(const QueuedDrawTask &task)
{
	if (task.Page == NULL) {
		return false;
	}

	const TerrainRenderBatchPageClass &page = *task.Page;
	const TerrainRenderBatchPageClass::DrawRange &range = task.Range;
	if (!page.Is_Valid() || page.VertexBuffer == NULL || page.IndexBuffer == NULL || range.PolygonCount == 0 || range.VertexCount == 0) {
		return true;
	}

	WW3D::FixedFunctionSubmitDesc submission;
	submission.VertexBuffer = page.VertexBuffer;
	submission.IndexBuffer = page.IndexBuffer;
	submission.StartIndex = range.StartIndex;
	submission.PolygonCount = range.PolygonCount;
	submission.MinVertexIndex = range.MinVertexIndex;
	submission.VertexCount = range.VertexCount;
	submission.Textures[0] = range.Textures[0];
	submission.Textures[1] = range.Textures[1];
	submission.Material = range.Material;
	submission.Shader = range.Shader;
	submission.WorldTransform = task.WorldTransform;
	submission.ViewTransform = task.ViewTransform;
	submission.ProjectionTransform = task.ProjectionTransform;
	submission.Lighting = task.Lighting;
	submission.RenderState = &task.RenderState;
	submission.ReceiveShadows = range.ReceiveShadows;
	submission.CastShadows = range.CastShadows;

	const bool submitted = WW3D::Submit_Fixed_Function_Draw(submission);
	WWASSERT(submitted);
	return submitted;
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

bool TerrainRenderBatchManagerClass::Queue_Patch(RenegadeTerrainPatchClass *patch, RenderInfoClass &rinfo)
{
	if (patch == NULL) {
		return false;
	}

	TerrainRenderBatchPageClass *page = Find_Page(patch);
	if (page == NULL || !page->Is_Valid() || patch->Are_Terrain_Batch_Buffers_Dirty()) {
		if (!Prepare_Patch(patch)) {
			return false;
		}
		page = Find_Page(patch);
	}

	if (page == NULL) {
		return false;
	}

	const Matrix4 world_transform(page->Patch->Get_Transform());
	Matrix4 view_transform(true);
	Matrix4 projection_transform(true);
	WW3D::Get_Transform(WW3D::RENDER_TRANSFORM_VIEW, view_transform);
	WW3D::Get_Transform(WW3D::RENDER_TRANSFORM_PROJECTION, projection_transform);

	QueuedDraws.reserve(QueuedDraws.size() + page->DrawRanges.size());
	for (std::vector<TerrainRenderBatchPageClass::DrawRange>::const_iterator it = page->DrawRanges.begin();
		  it != page->DrawRanges.end();)
	{
		if (it->PolygonCount == 0 || it->VertexCount == 0) {
			++it;
			continue;
		}

		TerrainRenderBatchPageClass::DrawRange merged_range = *it;
		++it;
		while (it != page->DrawRanges.end()) {
			if (it->PolygonCount == 0 || it->VertexCount == 0) {
				++it;
				continue;
			}
			if (!Can_Merge_Terrain_Draw_Ranges(merged_range, *it)) {
				break;
			}
			merged_range = Merge_Terrain_Draw_Ranges(merged_range, *it);
			++it;
		}

		QueuedDrawTask task;
		task.Page = page;
		task.Range = merged_range;
		task.Lighting = rinfo.lighting_submission;
		task.WorldTransform = world_transform;
		task.ViewTransform = view_transform;
		task.ProjectionTransform = projection_transform;
		WW3D::Capture_Current_Fixed_Function_State(task.RenderState, merged_range.Material);
		QueuedDraws.push_back(task);
	}

	return true;
}

bool TerrainRenderBatchManagerClass::Flush_Queued_Patches()
{
	if (QueuedDraws.empty()) {
		return true;
	}

	std::sort(QueuedDraws.begin(), QueuedDraws.end(), Sort_Queued_Terrain_Draw_Task);

	bool ok = true;
	for (std::vector<QueuedDrawTask>::const_iterator it = QueuedDraws.begin();
		  it != QueuedDraws.end();
		  ++it)
	{
		ok = Submit_Draw_Task(*it) && ok;
	}

	QueuedDraws.clear();
	return ok;
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
		  it != page->DrawRanges.end();)
	{
		if (it->PolygonCount == 0 || it->VertexCount == 0) {
			++it;
			continue;
		}

		TerrainRenderBatchPageClass::DrawRange merged_range = *it;
		++it;
		while (it != page->DrawRanges.end()) {
			if (it->PolygonCount == 0 || it->VertexCount == 0) {
				++it;
				continue;
			}
			if (!Can_Merge_Terrain_Draw_Ranges(merged_range, *it)) {
				break;
			}
			merged_range = Merge_Terrain_Draw_Ranges(merged_range, *it);
			++it;
		}

		ok = Submit_Draw(*page, merged_range, rinfo, NULL, NULL, NULL, merged_range.ReceiveShadows) && ok;
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

	if (!page->Has_Draws()) {
		return true;
	}

	TerrainRenderBatchPageClass::DrawRange full_page_range;
	full_page_range.StartIndex = 0;
	full_page_range.PolygonCount = static_cast<unsigned short>(page->IndexBuffer != NULL ? page->IndexBuffer->Get_Index_Count() / 3u : 0u);
	full_page_range.MinVertexIndex = 0;
	full_page_range.VertexCount = page->VertexBuffer != NULL ? page->VertexBuffer->Get_Vertex_Count() : 0;
	full_page_range.ReceiveShadows = true;
	full_page_range.CastShadows = false;
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
		ok = Submit_Draw(*page, full_page_range, rinfo, textures, matpass->Peek_Material(), &shader, receive_shadows) && ok;
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
