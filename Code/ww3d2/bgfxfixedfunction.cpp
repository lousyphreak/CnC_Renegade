#include "bgfxrenderer.h"

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

float Decode_Float_From_Dword(unsigned value)
{
    float decoded = 0.0f;
    std::memcpy(&decoded, &value, sizeof(decoded));
    return decoded;
}

void Copy_Color_Vector(float *destination, const Vector3 &source, float alpha)
{
    destination[0] = source.X;
    destination[1] = source.Y;
    destination[2] = source.Z;
    destination[3] = alpha;
}

void Copy_Color_Value(float *destination, const D3DCOLORVALUE &source)
{
    destination[0] = source.r;
    destination[1] = source.g;
    destination[2] = source.b;
    destination[3] = source.a;
}

void Copy_Packed_Color(float *destination, unsigned color)
{
    destination[0] = static_cast<float>((color >> 16) & 0xffu) / 255.0f;
    destination[1] = static_cast<float>((color >> 8) & 0xffu) / 255.0f;
    destination[2] = static_cast<float>(color & 0xffu) / 255.0f;
    destination[3] = static_cast<float>((color >> 24) & 0xffu) / 255.0f;
}

unsigned Normalize_Material_Source(unsigned source, bool color_vertex_enabled)
{
    if (!color_vertex_enabled) {
        return D3DMCS_MATERIAL;
    }

    switch (source) {
    case D3DMCS_COLOR1:
    case D3DMCS_COLOR2:
        return source;
    default:
        return D3DMCS_MATERIAL;
    }
}

unsigned Sanitize_Texture_Stage_State(unsigned stage, D3DTEXTURESTAGESTATETYPE state)
{
    const unsigned value = DX8Wrapper::Get_Texture_Stage_State(stage, state);
    if (value != 0x12345678u) {
        return value;
    }

    switch (state) {
    case D3DTSS_COLOROP:
        return stage == 0 ? D3DTOP_MODULATE : D3DTOP_DISABLE;
    case D3DTSS_COLORARG0:
        return D3DTA_CURRENT;
    case D3DTSS_COLORARG1:
        return D3DTA_TEXTURE;
    case D3DTSS_COLORARG2:
        return D3DTA_CURRENT;
    case D3DTSS_ALPHAOP:
        return stage == 0 ? D3DTOP_SELECTARG1 : D3DTOP_DISABLE;
    case D3DTSS_ALPHAARG0:
        return D3DTA_CURRENT;
    case D3DTSS_ALPHAARG1:
        return D3DTA_TEXTURE;
    case D3DTSS_ALPHAARG2:
        return D3DTA_CURRENT;
    default:
        return 0u;
    }
}

unsigned Sanitize_Texcoord_Index(unsigned stage)
{
    const unsigned value = DX8Wrapper::Get_Texture_Stage_State(stage, D3DTSS_TEXCOORDINDEX);
    return value != 0x12345678u ? value : (D3DTSS_TCI_PASSTHRU | stage);
}

unsigned Sanitize_Texture_Transform_Flags(unsigned stage)
{
    const unsigned value = DX8Wrapper::Get_Texture_Stage_State(stage, D3DTSS_TEXTURETRANSFORMFLAGS);
    return value != 0x12345678u ? value : D3DTTFF_DISABLE;
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

void Populate_Fixed_Function_Stage_Inputs(BgfxRenderer::FixedFunctionShaderInputs &shader_inputs)
{
    const unsigned texture_factor = DX8Wrapper::Get_DX8_Render_State(D3DRS_TEXTUREFACTOR);
    shader_inputs.TextureFactor = texture_factor != 0x12345678u ? texture_factor : 0xffffffffu;

    shader_inputs.Stage0Color[0] = static_cast<float>(Sanitize_Texture_Stage_State(0, D3DTSS_COLOROP));
    shader_inputs.Stage0Color[1] = static_cast<float>(Sanitize_Texture_Stage_State(0, D3DTSS_COLORARG0));
    shader_inputs.Stage0Color[2] = static_cast<float>(Sanitize_Texture_Stage_State(0, D3DTSS_COLORARG1));
    shader_inputs.Stage0Color[3] = static_cast<float>(Sanitize_Texture_Stage_State(0, D3DTSS_COLORARG2));
    shader_inputs.Stage0Alpha[0] = static_cast<float>(Sanitize_Texture_Stage_State(0, D3DTSS_ALPHAOP));
    shader_inputs.Stage0Alpha[1] = static_cast<float>(Sanitize_Texture_Stage_State(0, D3DTSS_ALPHAARG0));
    shader_inputs.Stage0Alpha[2] = static_cast<float>(Sanitize_Texture_Stage_State(0, D3DTSS_ALPHAARG1));
    shader_inputs.Stage0Alpha[3] = static_cast<float>(Sanitize_Texture_Stage_State(0, D3DTSS_ALPHAARG2));
    shader_inputs.Stage1Color[0] = static_cast<float>(Sanitize_Texture_Stage_State(1, D3DTSS_COLOROP));
    shader_inputs.Stage1Color[1] = static_cast<float>(Sanitize_Texture_Stage_State(1, D3DTSS_COLORARG0));
    shader_inputs.Stage1Color[2] = static_cast<float>(Sanitize_Texture_Stage_State(1, D3DTSS_COLORARG1));
    shader_inputs.Stage1Color[3] = static_cast<float>(Sanitize_Texture_Stage_State(1, D3DTSS_COLORARG2));
    shader_inputs.Stage1Alpha[0] = static_cast<float>(Sanitize_Texture_Stage_State(1, D3DTSS_ALPHAOP));
    shader_inputs.Stage1Alpha[1] = static_cast<float>(Sanitize_Texture_Stage_State(1, D3DTSS_ALPHAARG0));
    shader_inputs.Stage1Alpha[2] = static_cast<float>(Sanitize_Texture_Stage_State(1, D3DTSS_ALPHAARG1));
    shader_inputs.Stage1Alpha[3] = static_cast<float>(Sanitize_Texture_Stage_State(1, D3DTSS_ALPHAARG2));
}

void Populate_Fixed_Function_Texture_Inputs(BgfxRenderer::FixedFunctionShaderInputs &shader_inputs)
{
    for (unsigned stage = 0; stage < kFixedFunctionTextureStages; ++stage) {
        shader_inputs.TextureStageConfig[stage * 4 + 0] = static_cast<float>(Sanitize_Texcoord_Index(stage));
        shader_inputs.TextureStageConfig[stage * 4 + 1] = static_cast<float>(Sanitize_Texture_Transform_Flags(stage));
        shader_inputs.TextureStageConfig[stage * 4 + 2] = 0.0f;
        shader_inputs.TextureStageConfig[stage * 4 + 3] = 0.0f;

        Matrix4 texture_transform(true);
        DX8Wrapper::Get_Transform(static_cast<D3DTRANSFORMSTATETYPE>(D3DTS_TEXTURE0 + stage), texture_transform);
        for (unsigned row = 0; row < 4u; ++row) {
            const size_t offset = static_cast<size_t>(stage) * 16u + row * 4u;
            shader_inputs.TextureTransformRows[offset + 0] = texture_transform[static_cast<int>(row)][0];
            shader_inputs.TextureTransformRows[offset + 1] = texture_transform[static_cast<int>(row)][1];
            shader_inputs.TextureTransformRows[offset + 2] = texture_transform[static_cast<int>(row)][2];
            shader_inputs.TextureTransformRows[offset + 3] = texture_transform[static_cast<int>(row)][3];
        }
    }
}

void Populate_Fixed_Function_Lighting_Inputs(
    BgfxRenderer::FixedFunctionShaderInputs &shader_inputs,
    const VertexBufferClass &vertex_buffer,
    const VertexMaterialClass *material)
{
    RenderStateStruct render_state;
    DX8Wrapper::Get_Render_State(render_state);

    Vector3 ambient(1.0f, 1.0f, 1.0f);
    Vector3 diffuse(1.0f, 1.0f, 1.0f);
    Vector3 specular(0.0f, 0.0f, 0.0f);
    Vector3 emissive(0.0f, 0.0f, 0.0f);
    float diffuse_alpha = 1.0f;
    float specular_power = render_state.material_state.Power;

    if (material != nullptr) {
        material->Get_Ambient(&ambient);
        material->Get_Diffuse(&diffuse);
        material->Get_Specular(&specular);
        material->Get_Emissive(&emissive);
        diffuse_alpha = material->Get_Opacity();
        specular_power = material->Get_Shininess();
    } else {
        ambient = Vector3(
            render_state.material_state.Ambient.r,
            render_state.material_state.Ambient.g,
            render_state.material_state.Ambient.b);
        diffuse = Vector3(
            render_state.material_state.Diffuse.r,
            render_state.material_state.Diffuse.g,
            render_state.material_state.Diffuse.b);
        specular = Vector3(
            render_state.material_state.Specular.r,
            render_state.material_state.Specular.g,
            render_state.material_state.Specular.b);
        emissive = Vector3(
            render_state.material_state.Emissive.r,
            render_state.material_state.Emissive.g,
            render_state.material_state.Emissive.b);
        diffuse_alpha = render_state.material_state.Diffuse.a;
    }

    Copy_Color_Vector(shader_inputs.MaterialAmbient, ambient, 1.0f);
    Copy_Color_Vector(shader_inputs.MaterialDiffuse, diffuse, diffuse_alpha);
    Copy_Color_Vector(shader_inputs.MaterialSpecular, specular, 1.0f);
    Copy_Color_Vector(shader_inputs.MaterialEmissive, emissive, 1.0f);
    shader_inputs.MaterialParams[0] = std::max(specular_power, 1.0f);

    const unsigned ambient_color = DX8Wrapper::Get_DX8_Render_State(D3DRS_AMBIENT);
    Copy_Packed_Color(shader_inputs.SceneAmbient, ambient_color != 0x12345678u ? ambient_color : 0u);

    const unsigned fvf = vertex_buffer.Vertex_Format_Info().Get_Vertex_Format();
    const unsigned color_vertex_state = DX8Wrapper::Get_DX8_Render_State(D3DRS_COLORVERTEX);
    const bool color_vertex_enabled = color_vertex_state != 0x12345678u ? color_vertex_state != FALSE : true;
    shader_inputs.LightingConfig[0] = DX8Wrapper::Get_DX8_Render_State(D3DRS_LIGHTING) != 0u ? 1.0f : 0.0f;
    shader_inputs.LightingConfig[1] = (fvf & VERTEX_FORMAT_FLAG_NORMAL) != 0u ? 1.0f : 0.0f;
    shader_inputs.LightingConfig[2] = static_cast<float>(
        Normalize_Material_Source(DX8Wrapper::Get_DX8_Render_State(D3DRS_DIFFUSEMATERIALSOURCE), color_vertex_enabled));
    shader_inputs.LightingConfig[3] = static_cast<float>(
        Normalize_Material_Source(DX8Wrapper::Get_DX8_Render_State(D3DRS_AMBIENTMATERIALSOURCE), color_vertex_enabled));
    shader_inputs.MaterialSourceConfig[0] = static_cast<float>(
        Normalize_Material_Source(DX8Wrapper::Get_DX8_Render_State(D3DRS_EMISSIVEMATERIALSOURCE), color_vertex_enabled));
    shader_inputs.MaterialSourceConfig[1] = static_cast<float>(
        Normalize_Material_Source(DX8Wrapper::Get_DX8_Render_State(D3DRS_SPECULARMATERIALSOURCE), color_vertex_enabled));

    for (unsigned light_index = 0; light_index < 4u; ++light_index) {
        const D3DLIGHT8 &light = DX8Wrapper::Peek_Light(light_index);
        const size_t offset = static_cast<size_t>(light_index) * 4u;
        shader_inputs.LightPositions[offset + 0] = light.Position.x;
        shader_inputs.LightPositions[offset + 1] = light.Position.y;
        shader_inputs.LightPositions[offset + 2] = light.Position.z;
        shader_inputs.LightPositions[offset + 3] = DX8Wrapper::Is_Light_Enabled(light_index) ? static_cast<float>(light.Type) : 0.0f;
        shader_inputs.LightDirections[offset + 0] = light.Direction.x;
        shader_inputs.LightDirections[offset + 1] = light.Direction.y;
        shader_inputs.LightDirections[offset + 2] = light.Direction.z;
        shader_inputs.LightDirections[offset + 3] = 0.0f;
        Copy_Color_Value(shader_inputs.LightAmbient + offset, light.Ambient);
        Copy_Color_Value(shader_inputs.LightDiffuse + offset, light.Diffuse);
        Copy_Color_Value(shader_inputs.LightSpecular + offset, light.Specular);
        shader_inputs.LightAttenuation[offset + 0] = light.Attenuation0;
        shader_inputs.LightAttenuation[offset + 1] = light.Attenuation1;
        shader_inputs.LightAttenuation[offset + 2] = light.Attenuation2;
        shader_inputs.LightAttenuation[offset + 3] = light.Range;
        shader_inputs.LightSpotParams[offset + 0] = std::cos(light.Phi);
        shader_inputs.LightSpotParams[offset + 1] = light.Falloff;
        shader_inputs.LightSpotParams[offset + 2] = light.Theta;
        shader_inputs.LightSpotParams[offset + 3] = light.Phi;
    }
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

    BgfxRenderer::FixedFunctionShaderInputs shader_inputs;
    shader_inputs.FogEnabled = DX8Wrapper::Get_Fog_Enable();
    shader_inputs.FogColor = DX8Wrapper::Get_Fog_Color();
    shader_inputs.BumpEnvMatrix[0] = Decode_Float_From_Dword(DX8Wrapper::Get_Texture_Stage_State(0, D3DTSS_BUMPENVMAT00));
    shader_inputs.BumpEnvMatrix[1] = Decode_Float_From_Dword(DX8Wrapper::Get_Texture_Stage_State(0, D3DTSS_BUMPENVMAT01));
    shader_inputs.BumpEnvMatrix[2] = Decode_Float_From_Dword(DX8Wrapper::Get_Texture_Stage_State(0, D3DTSS_BUMPENVMAT10));
    shader_inputs.BumpEnvMatrix[3] = Decode_Float_From_Dword(DX8Wrapper::Get_Texture_Stage_State(0, D3DTSS_BUMPENVMAT11));
    shader_inputs.BumpEnvLuminanceScale = Decode_Float_From_Dword(DX8Wrapper::Get_Texture_Stage_State(1, D3DTSS_BUMPENVLSCALE));
    shader_inputs.BumpEnvLuminanceOffset = Decode_Float_From_Dword(DX8Wrapper::Get_Texture_Stage_State(1, D3DTSS_BUMPENVLOFFSET));
    Populate_Fixed_Function_Stage_Inputs(shader_inputs);
    Populate_Fixed_Function_Texture_Inputs(shader_inputs);
    Populate_Fixed_Function_Lighting_Inputs(shader_inputs, vertex_buffer, material);

    TextureClass *stage0_texture = textures != nullptr ? textures[0] : nullptr;
    TextureClass *stage1_texture = textures != nullptr ? textures[1] : nullptr;

    bgfx::setTexture(0, BgfxRenderer::Get_Texture0_Uniform(), Resolve_Texture_Handle(stage0_texture), Resolve_Sampler_Flags(stage0_texture, 0));
    bgfx::setTexture(1, BgfxRenderer::Get_Texture1_Uniform(), Resolve_Texture_Handle(stage1_texture), Resolve_Sampler_Flags(stage1_texture, 1));
    BgfxRenderer::Apply_Fixed_Function_Shader_Inputs(shader, shader_inputs, view);

    const unsigned cull_mode = DX8Wrapper::Get_DX8_Render_State(D3DRS_CULLMODE);
    uint16_t view_id = BgfxRenderer::Get_View_Id(view, projection);

    BgfxRenderer::Apply_Render_State(
        shader,
        cull_mode != 0x12345678u ? cull_mode : D3DCULL_CW,
        Resolve_Primitive_State(fill_mode));
    bgfx::submit(view_id, BgfxRenderer::Get_Fixed_Function_Program());
    return true;
}

bool Submit_Current_Fixed_Function_Draw(
    unsigned short start_index,
    unsigned short polygon_count,
    unsigned short min_vertex_index,
    unsigned short vertex_count,
    bool strip)
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
        false);
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
        true);
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
        world,
        view,
        projection,
        true);
}
