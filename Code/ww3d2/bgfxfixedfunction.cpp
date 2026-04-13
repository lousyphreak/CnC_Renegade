#include "bgfxrenderer.h"
#include "shadowmap.h"

#include <cmath>
#include <cstddef>
#include <cstdint>
#include <cstring>

#include <bgfx/bgfx.h>

#include "vertexformat.h"
#include "indexbuffer.h"
#include "vertexbuffer.h"
#include "dx8wrapper.h"
#include "texture.h"
#include "vertmaterial.h"

namespace
{
constexpr unsigned kFixedFunctionTextureStages = 2u;

struct SubmissionVertex
{
    float x;
    float y;
    float z;
    float nx;
    float ny;
    float nz;
    uint32_t diffuse;
    uint32_t specular;
    float u0;
    float v0;
    float u1;
    float v1;
};

bgfx::TextureHandle Resolve_Texture_Handle(TextureClass *texture)
{
    if (texture != nullptr) {
        bgfx::TextureHandle handle = texture->Get_Bgfx_Texture();
        if (bgfx::isValid(handle)) {
            return handle;
        }
    }

    return BgfxRenderer::Get_White_Texture();
}

uint32_t Resolve_Sampler_Flags(TextureClass *texture, unsigned stage)
{
    return texture != nullptr ? texture->Get_Bgfx_Sampler_Flags(stage) : 0u;
}

enum class FillMode
{
    Solid,
    Wireframe,
    Points,
};

FillMode Resolve_Fill_Mode()
{
    switch (DX8Wrapper::Get_DX8_Render_State(D3DRS_FILLMODE)) {
    case D3DFILL_POINT:
        return FillMode::Points;
    case D3DFILL_WIREFRAME:
        return FillMode::Wireframe;
    default:
        return FillMode::Solid;
    }
}

uint64_t Resolve_Primitive_State(FillMode fill_mode)
{
    switch (fill_mode) {
    case FillMode::Wireframe:
        return BGFX_STATE_PT_LINES;
    case FillMode::Points:
        return BGFX_STATE_PT_POINTS | BGFX_STATE_POINT_SIZE(1);
    default:
        return 0u;
    }
}

uint32_t Resolve_Submitted_Index_Count(FillMode fill_mode, bool strip, unsigned short polygon_count)
{
    switch (fill_mode) {
    case FillMode::Wireframe:
        return static_cast<uint32_t>(polygon_count) * 6u;
    case FillMode::Points:
        return strip ? static_cast<uint32_t>(polygon_count) + 2u : static_cast<uint32_t>(polygon_count) * 3u;
    default:
        return static_cast<uint32_t>(polygon_count) * 3u;
    }
}

void Write_Wireframe_Triangle(uint16_t *destination, unsigned short a, unsigned short b, unsigned short c, unsigned short min_vertex_index)
{
    destination[0] = static_cast<uint16_t>(a - min_vertex_index);
    destination[1] = static_cast<uint16_t>(b - min_vertex_index);
    destination[2] = static_cast<uint16_t>(b - min_vertex_index);
    destination[3] = static_cast<uint16_t>(c - min_vertex_index);
    destination[4] = static_cast<uint16_t>(c - min_vertex_index);
    destination[5] = static_cast<uint16_t>(a - min_vertex_index);
}

bool Submit_Cached_Fixed_Function_Draw(
    const VertexBufferClass &vertex_buffer,
    unsigned vertex_buffer_offset,
    const IndexBufferClass &index_buffer,
    unsigned index_buffer_offset,
    unsigned index_base_offset,
    unsigned short start_index,
    unsigned short polygon_count,
    unsigned short min_vertex_index,
    unsigned short vertex_count,
    TextureClass *const *textures,
    const VertexMaterialClass *material,
    const ShaderClass &shader,
    bool receive_shadows,
    bool cast_shadows,
    const Matrix4 &world,
    const Matrix4 &view,
    const Matrix4 &projection,
    bool strip)
{
    if (!BgfxRenderer::Is_Initted() || vertex_count == 0 || polygon_count == 0) {
        return false;
    }

    const auto vertex_buffer_type = vertex_buffer.Type();
    const auto index_buffer_type = index_buffer.Type();
    const bool supported_vertex_buffer =
        vertex_buffer_type == BUFFER_TYPE_RENDER ||
        vertex_buffer_type == BUFFER_TYPE_SORTING ||
        vertex_buffer_type == BUFFER_TYPE_DYNAMIC_RENDER ||
        vertex_buffer_type == BUFFER_TYPE_DYNAMIC_SORTING;
    const bool supported_index_buffer =
        index_buffer_type == BUFFER_TYPE_RENDER ||
        index_buffer_type == BUFFER_TYPE_SORTING ||
        index_buffer_type == BUFFER_TYPE_DYNAMIC_RENDER ||
        index_buffer_type == BUFFER_TYPE_DYNAMIC_SORTING;
    if (!supported_vertex_buffer || !supported_index_buffer) {
        return false;
    }

    const FillMode fill_mode = Resolve_Fill_Mode();
    const uint32_t submitted_index_count = Resolve_Submitted_Index_Count(fill_mode, strip, polygon_count);
    const unsigned short source_index_count = strip
        ? static_cast<unsigned short>(polygon_count + 2)
        : static_cast<unsigned short>(polygon_count * 3u);

    bool use_direct_vertex_buffer = false;
    if (vertex_buffer_type == BUFFER_TYPE_RENDER) {
        if (!static_cast<const RenderVertexBufferClass &>(vertex_buffer).Ensure_Bgfx_Buffer()) {
            return false;
        }
        use_direct_vertex_buffer = true;
    }

    bool use_direct_index_buffer = false;
    if (use_direct_vertex_buffer &&
        index_buffer_type == BUFFER_TYPE_RENDER &&
        fill_mode != FillMode::Wireframe &&
        (!strip || fill_mode == FillMode::Points)) {
        if (!static_cast<const RenderIndexBufferClass &>(index_buffer).Ensure_Bgfx_Buffer()) {
            return false;
        }
        use_direct_index_buffer = true;
    }

    const bgfx::VertexLayout &layout = BgfxRenderer::Get_Fixed_Function_Layout();
    if ((!use_direct_vertex_buffer && bgfx::getAvailTransientVertexBuffer(vertex_count, layout) < vertex_count) ||
        (!use_direct_index_buffer && bgfx::getAvailTransientIndexBuffer(submitted_index_count) < submitted_index_count)) {
        return false;
    }

    bgfx::TransientVertexBuffer transient_vertex_buffer;
    bgfx::TransientIndexBuffer transient_index_buffer;
    SubmissionVertex *submission_vertices = nullptr;
    if (!use_direct_vertex_buffer) {
        bgfx::allocTransientVertexBuffer(&transient_vertex_buffer, vertex_count, layout);
        submission_vertices = reinterpret_cast<SubmissionVertex *>(transient_vertex_buffer.data);

        VertexBufferClass::AppendLockClass vertex_lock(
            const_cast<VertexBufferClass *>(&vertex_buffer),
            vertex_buffer_offset + index_base_offset + min_vertex_index,
            vertex_count);
        const unsigned char *source_vertices = reinterpret_cast<const unsigned char *>(vertex_lock.Get_Vertex_Array());
        const unsigned fvf = vertex_buffer.Vertex_Format_Info().Get_Vertex_Format();
        const unsigned fvf_size = vertex_buffer.Vertex_Format_Info().Get_Vertex_Size();
        const bool has_normals = (fvf & VERTEX_FORMAT_FLAG_NORMAL) != 0u;
        const unsigned texcoord_count = VERTEX_FORMAT_Get_Texcoord_Count(fvf);

        for (unsigned short vertex_index = 0; vertex_index < vertex_count; ++vertex_index) {
            const unsigned char *vertex = source_vertices + vertex_index * fvf_size;
            submission_vertices[vertex_index].x = reinterpret_cast<const float *>(vertex + vertex_buffer.Vertex_Format_Info().Get_Location_Offset())[0];
            submission_vertices[vertex_index].y = reinterpret_cast<const float *>(vertex + vertex_buffer.Vertex_Format_Info().Get_Location_Offset())[1];
            submission_vertices[vertex_index].z = reinterpret_cast<const float *>(vertex + vertex_buffer.Vertex_Format_Info().Get_Location_Offset())[2];

            if (has_normals) {
                submission_vertices[vertex_index].nx = reinterpret_cast<const float *>(vertex + vertex_buffer.Vertex_Format_Info().Get_Normal_Offset())[0];
                submission_vertices[vertex_index].ny = reinterpret_cast<const float *>(vertex + vertex_buffer.Vertex_Format_Info().Get_Normal_Offset())[1];
                submission_vertices[vertex_index].nz = reinterpret_cast<const float *>(vertex + vertex_buffer.Vertex_Format_Info().Get_Normal_Offset())[2];
            } else {
                submission_vertices[vertex_index].nx = 0.0f;
                submission_vertices[vertex_index].ny = 0.0f;
                submission_vertices[vertex_index].nz = 1.0f;
            }

            submission_vertices[vertex_index].diffuse =
                vertex_buffer.Vertex_Format_Info().Get_Diffuse_Offset() < fvf_size
                ? BgfxRenderer::Convert_Packed_Color(*reinterpret_cast<const unsigned *>(vertex + vertex_buffer.Vertex_Format_Info().Get_Diffuse_Offset()))
                : 0xffffffffu;
            submission_vertices[vertex_index].specular =
                vertex_buffer.Vertex_Format_Info().Get_Specular_Offset() < fvf_size
                ? BgfxRenderer::Convert_Packed_Color(*reinterpret_cast<const unsigned *>(vertex + vertex_buffer.Vertex_Format_Info().Get_Specular_Offset()))
                : 0u;

            submission_vertices[vertex_index].u0 = 0.0f;
            submission_vertices[vertex_index].v0 = 0.0f;
            if (texcoord_count > 0u) {
                submission_vertices[vertex_index].u0 = reinterpret_cast<const float *>(vertex + vertex_buffer.Vertex_Format_Info().Get_Tex_Offset(0))[0];
                submission_vertices[vertex_index].v0 = reinterpret_cast<const float *>(vertex + vertex_buffer.Vertex_Format_Info().Get_Tex_Offset(0))[1];
            }

            submission_vertices[vertex_index].u1 = 0.0f;
            submission_vertices[vertex_index].v1 = 0.0f;
            if (texcoord_count > 1u) {
                submission_vertices[vertex_index].u1 = reinterpret_cast<const float *>(vertex + vertex_buffer.Vertex_Format_Info().Get_Tex_Offset(1))[0];
                submission_vertices[vertex_index].v1 = reinterpret_cast<const float *>(vertex + vertex_buffer.Vertex_Format_Info().Get_Tex_Offset(1))[1];
            }
        }
    }

    if (!use_direct_index_buffer) {
        bgfx::allocTransientIndexBuffer(&transient_index_buffer, submitted_index_count);
    }

    uint16_t *submission_indices = !use_direct_index_buffer
        ? reinterpret_cast<uint16_t *>(transient_index_buffer.data)
        : nullptr;

    if (!use_direct_index_buffer) {
        const unsigned short *source_indices = nullptr;
        if (index_buffer_type == BUFFER_TYPE_RENDER) {
            source_indices =
                static_cast<const RenderIndexBufferClass &>(index_buffer).Get_Source_Index_Data()
                + index_buffer_offset
                + start_index;
        } else {
            IndexBufferClass::AppendLockClass index_lock(
                const_cast<IndexBufferClass *>(&index_buffer),
                index_buffer_offset + start_index,
                source_index_count);
            source_indices = index_lock.Get_Index_Array();

            if (fill_mode == FillMode::Wireframe) {
                for (unsigned short triangle = 0; triangle < polygon_count; ++triangle) {
                    unsigned short a = 0;
                    unsigned short b = 0;
                    unsigned short c = 0;
                    if (strip) {
                        const bool odd_triangle = (triangle & 1u) != 0u;
                        a = source_indices[triangle + (odd_triangle ? 1 : 0)];
                        b = source_indices[triangle + (odd_triangle ? 0 : 1)];
                        c = source_indices[triangle + 2];
                    } else {
                        a = source_indices[triangle * 3 + 0];
                        b = source_indices[triangle * 3 + 1];
                        c = source_indices[triangle * 3 + 2];
                    }

                    Write_Wireframe_Triangle(submission_indices + triangle * 6u, a, b, c, min_vertex_index);
                }
            } else if (fill_mode == FillMode::Points) {
                for (uint32_t index = 0; index < submitted_index_count; ++index) {
                    submission_indices[index] = static_cast<uint16_t>(source_indices[index] - min_vertex_index);
                }
            } else if (strip) {
                for (unsigned short triangle = 0; triangle < polygon_count; ++triangle) {
                    const bool odd_triangle = (triangle & 1u) != 0u;
                    const unsigned short a = source_indices[triangle + (odd_triangle ? 1 : 0)];
                    const unsigned short b = source_indices[triangle + (odd_triangle ? 0 : 1)];
                    const unsigned short c = source_indices[triangle + 2];
                    submission_indices[triangle * 3 + 0] = static_cast<uint16_t>(a - min_vertex_index);
                    submission_indices[triangle * 3 + 1] = static_cast<uint16_t>(b - min_vertex_index);
                    submission_indices[triangle * 3 + 2] = static_cast<uint16_t>(c - min_vertex_index);
                }
            } else {
                for (uint32_t index = 0; index < submitted_index_count; ++index) {
                    submission_indices[index] = static_cast<uint16_t>(source_indices[index] - min_vertex_index);
                }
            }
            source_indices = nullptr;
        }

        if (source_indices != nullptr) {
            if (fill_mode == FillMode::Wireframe) {
                for (unsigned short triangle = 0; triangle < polygon_count; ++triangle) {
                    unsigned short a = 0;
                    unsigned short b = 0;
                    unsigned short c = 0;
                    if (strip) {
                        const bool odd_triangle = (triangle & 1u) != 0u;
                        a = source_indices[triangle + (odd_triangle ? 1 : 0)];
                        b = source_indices[triangle + (odd_triangle ? 0 : 1)];
                        c = source_indices[triangle + 2];
                    } else {
                        a = source_indices[triangle * 3 + 0];
                        b = source_indices[triangle * 3 + 1];
                        c = source_indices[triangle * 3 + 2];
                    }

                    Write_Wireframe_Triangle(submission_indices + triangle * 6u, a, b, c, min_vertex_index);
                }
            } else if (fill_mode == FillMode::Points) {
                for (uint32_t index = 0; index < submitted_index_count; ++index) {
                    submission_indices[index] = static_cast<uint16_t>(source_indices[index] - min_vertex_index);
                }
            } else if (strip) {
                for (unsigned short triangle = 0; triangle < polygon_count; ++triangle) {
                    const bool odd_triangle = (triangle & 1u) != 0u;
                    const unsigned short a = source_indices[triangle + (odd_triangle ? 1 : 0)];
                    const unsigned short b = source_indices[triangle + (odd_triangle ? 0 : 1)];
                    const unsigned short c = source_indices[triangle + 2];
                    submission_indices[triangle * 3 + 0] = static_cast<uint16_t>(a - min_vertex_index);
                    submission_indices[triangle * 3 + 1] = static_cast<uint16_t>(b - min_vertex_index);
                    submission_indices[triangle * 3 + 2] = static_cast<uint16_t>(c - min_vertex_index);
                }
            } else {
                for (uint32_t index = 0; index < submitted_index_count; ++index) {
                    submission_indices[index] = static_cast<uint16_t>(source_indices[index] - min_vertex_index);
                }
            }
        }
    }

    const Matrix4 world_transform = world.Transpose();
    bgfx::setTransform(&world_transform[0][0]);
    if (use_direct_index_buffer) {
        // The cached render index buffers keep the original absolute vertex indices.
        // Unlike D3D8's SetIndices(baseVertexIndex), bgfx does not apply a separate
        // base-vertex offset to indexed draws, so direct indexed submission must bind
        // the full cached vertex buffer rather than a min-vertex slice.
        bgfx::setVertexBuffer(
            0,
            static_cast<const RenderVertexBufferClass &>(vertex_buffer).Get_Bgfx_Vertex_Buffer());
    } else if (use_direct_vertex_buffer) {
        bgfx::setVertexBuffer(
            0,
            static_cast<const RenderVertexBufferClass &>(vertex_buffer).Get_Bgfx_Vertex_Buffer(),
            vertex_buffer_offset + index_base_offset + min_vertex_index,
            vertex_count);
    } else {
        bgfx::setVertexBuffer(0, &transient_vertex_buffer);
    }

    if (use_direct_index_buffer) {
        bgfx::setIndexBuffer(
            static_cast<const RenderIndexBufferClass &>(index_buffer).Get_Bgfx_Index_Buffer(),
            index_buffer_offset + start_index,
            fill_mode == FillMode::Points && strip ? source_index_count : submitted_index_count);
    } else {
        bgfx::setIndexBuffer(&transient_index_buffer);
    }

    TextureClass *stage0_texture = textures != nullptr ? textures[0] : nullptr;
    TextureClass *stage1_texture = textures != nullptr ? textures[1] : nullptr;

    bgfx::setTexture(0, BgfxRenderer::Get_Texture0_Uniform(), Resolve_Texture_Handle(stage0_texture), Resolve_Sampler_Flags(stage0_texture, 0));
    bgfx::setTexture(1, BgfxRenderer::Get_Texture1_Uniform(), Resolve_Texture_Handle(stage1_texture), Resolve_Sampler_Flags(stage1_texture, 1));

    MeshShaderProgram selected_program = BgfxRenderer::Select_Mesh_Program(shader, vertex_buffer);
    BgfxRenderer::Apply_Mesh_Shader_Inputs(selected_program, shader, vertex_buffer, material);

    ShadowMapManager::Bind_Shadow_Uniforms(receive_shadows);

    const unsigned cull_mode = DX8Wrapper::Get_DX8_Render_State(D3DRS_CULLMODE);
    uint16_t view_id = BgfxRenderer::Get_View_Id(view, projection);

    BgfxRenderer::Apply_Render_State(
        shader,
        cull_mode != 0x12345678u ? cull_mode : D3DCULL_CW,
        Resolve_Primitive_State(fill_mode));
    bgfx::submit(view_id, BgfxRenderer::Get_Mesh_Program(selected_program));

    const bool alpha_test_enabled = shader.Get_Alpha_Test() == ShaderClass::ALPHATEST_ENABLE;
    const bool blend_blocks_shadow_cast =
        shader.Get_Dst_Blend_Func() != ShaderClass::DSTBLEND_ZERO && !alpha_test_enabled;
    if (cast_shadows && !blend_blocks_shadow_cast) {
        const uint32_t shadow_ib_count = fill_mode == FillMode::Points && strip ? source_index_count : submitted_index_count;
        ShadowMapManager::Submit_Shadow_Draws(
            vertex_buffer, vertex_buffer_offset, index_base_offset,
            min_vertex_index, vertex_count,
            index_buffer, index_buffer_offset, start_index,
            shadow_ib_count,
            use_direct_vertex_buffer, use_direct_index_buffer,
            &transient_vertex_buffer, &transient_index_buffer,
            world,
            Resolve_Texture_Handle(stage0_texture),
            Resolve_Sampler_Flags(stage0_texture, 0),
            Resolve_Texture_Handle(stage1_texture),
            Resolve_Sampler_Flags(stage1_texture, 1),
            alpha_test_enabled,
            cull_mode != 0x12345678u ? cull_mode : D3DCULL_CW);
    }

    return true;
}

bool Submit_Current_Fixed_Function_Draw(
    unsigned short start_index,
    unsigned short polygon_count,
    unsigned short min_vertex_index,
    unsigned short vertex_count,
    bool strip,
    bool use_explicit_shadow_flags,
    bool receive_shadows,
    bool cast_shadows)
{
    if (!BgfxRenderer::Is_Initted() || !DX8Wrapper::_Is_Triangle_Draw_Enabled()) {
        return false;
    }

    DX8Wrapper::Apply_Render_State_Changes();

    RenderStateStruct render_state;
    DX8Wrapper::Get_Render_State(render_state);
    if (render_state.vertex_buffer == nullptr || render_state.index_buffer == nullptr) {
        return false;
    }

    Matrix4 projection;
    DX8Wrapper::Get_Transform(D3DTS_PROJECTION, projection);

    const bool resolved_receive_shadows =
        use_explicit_shadow_flags
            ? receive_shadows
            : render_state.shader.Get_Dst_Blend_Func() == ShaderClass::DSTBLEND_ZERO;
    const bool resolved_cast_shadows = use_explicit_shadow_flags ? cast_shadows : false;

    TextureClass *textures[2] = {render_state.Textures[0], render_state.Textures[1]};
    if (strip) {
        return BgfxRenderer::Submit_Cached_Fixed_Function_Strip(
            *render_state.vertex_buffer,
            render_state.vba_offset,
            *render_state.index_buffer,
            render_state.iba_offset,
            render_state.index_base_offset,
            start_index,
            polygon_count,
            min_vertex_index,
            vertex_count,
            textures,
            render_state.material,
            render_state.shader,
            resolved_receive_shadows,
            resolved_cast_shadows,
            render_state.world,
            render_state.view,
            projection);
    }

    return BgfxRenderer::Submit_Cached_Fixed_Function_Triangles(
        *render_state.vertex_buffer,
        render_state.vba_offset,
        *render_state.index_buffer,
        render_state.iba_offset,
        render_state.index_base_offset,
        start_index,
        polygon_count,
        min_vertex_index,
        vertex_count,
        textures,
        render_state.material,
        render_state.shader,
        resolved_receive_shadows,
        resolved_cast_shadows,
        render_state.world,
        render_state.view,
        projection);
}
}

bool BgfxRenderer::Submit_Current_Fixed_Function_Triangles(
    unsigned short start_index,
    unsigned short polygon_count,
    unsigned short min_vertex_index,
    unsigned short vertex_count)
{
    return Submit_Current_Fixed_Function_Draw(
        start_index,
        polygon_count,
        min_vertex_index,
        vertex_count,
        false,
        false,
        false,
        false);
}

bool BgfxRenderer::Submit_Current_Fixed_Function_Triangles(
    unsigned short start_index,
    unsigned short polygon_count,
    unsigned short min_vertex_index,
    unsigned short vertex_count,
    bool receive_shadows,
    bool cast_shadows)
{
    return Submit_Current_Fixed_Function_Draw(
        start_index,
        polygon_count,
        min_vertex_index,
        vertex_count,
        false,
        true,
        receive_shadows,
        cast_shadows);
}

bool BgfxRenderer::Submit_Cached_Fixed_Function_Triangles(
    const VertexBufferClass &vertex_buffer,
    unsigned vertex_buffer_offset,
    const IndexBufferClass &index_buffer,
    unsigned index_buffer_offset,
    unsigned index_base_offset,
    unsigned short start_index,
    unsigned short polygon_count,
    unsigned short min_vertex_index,
    unsigned short vertex_count,
    TextureClass *const *textures,
    const VertexMaterialClass *material,
    const ShaderClass &shader,
    bool receive_shadows,
    bool cast_shadows,
    const Matrix4 &world,
    const Matrix4 &view,
    const Matrix4 &projection)
{
    return Submit_Cached_Fixed_Function_Draw(
        vertex_buffer,
        vertex_buffer_offset,
        index_buffer,
        index_buffer_offset,
        index_base_offset,
        start_index,
        polygon_count,
        min_vertex_index,
        vertex_count,
        textures,
        material,
        shader,
        receive_shadows,
        cast_shadows,
        world,
        view,
        projection,
        false);
}

bool BgfxRenderer::Submit_Current_Fixed_Function_Strip(
    unsigned short start_index,
    unsigned short polygon_count,
    unsigned short min_vertex_index,
    unsigned short vertex_count)
{
    return Submit_Current_Fixed_Function_Draw(
        start_index,
        polygon_count,
        min_vertex_index,
        vertex_count,
        true,
        false,
        false,
        false);
}

bool BgfxRenderer::Submit_Current_Fixed_Function_Strip(
    unsigned short start_index,
    unsigned short polygon_count,
    unsigned short min_vertex_index,
    unsigned short vertex_count,
    bool receive_shadows,
    bool cast_shadows)
{
    return Submit_Current_Fixed_Function_Draw(
        start_index,
        polygon_count,
        min_vertex_index,
        vertex_count,
        true,
        true,
        receive_shadows,
        cast_shadows);
}

bool BgfxRenderer::Submit_Cached_Fixed_Function_Strip(
    const VertexBufferClass &vertex_buffer,
    unsigned vertex_buffer_offset,
    const IndexBufferClass &index_buffer,
    unsigned index_buffer_offset,
    unsigned index_base_offset,
    unsigned short start_index,
    unsigned short polygon_count,
    unsigned short min_vertex_index,
    unsigned short vertex_count,
    TextureClass *const *textures,
    const VertexMaterialClass *material,
    const ShaderClass &shader,
    bool receive_shadows,
    bool cast_shadows,
    const Matrix4 &world,
    const Matrix4 &view,
    const Matrix4 &projection)
{
    return Submit_Cached_Fixed_Function_Draw(
        vertex_buffer,
        vertex_buffer_offset,
        index_buffer,
        index_buffer_offset,
        index_base_offset,
        start_index,
        polygon_count,
        min_vertex_index,
        vertex_count,
        textures,
        material,
        shader,
        receive_shadows,
        cast_shadows,
        world,
        view,
        projection,
        true);
}
