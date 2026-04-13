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

#include "shadowmap.h"
#include "bgfxrenderer.h"
#include "camera.h"
#include "indexbuffer.h"
#include "renderer_types.h"
#include "sphere.h"
#include "vertexbuffer.h"
#include "wwdebug.h"
#include <algorithm>
#include <cfloat>
#include <cmath>
#include <cstring>

// Static member initialization
bool ShadowMapManager::Initted = false;
bool ShadowMapManager::Enabled = true;
bool ShadowMapManager::ShadowPassActive = false;
int ShadowMapManager::CurrentShadowCascade = 0;
int ShadowMapManager::CascadeSize = DEFAULT_CASCADE_SIZE;
float ShadowMapManager::ShadowDistance = DEFAULT_SHADOW_DISTANCE;
float ShadowMapManager::ShadowIntensity = DEFAULT_SHADOW_INTENSITY;
float ShadowMapManager::DepthBias = DEFAULT_DEPTH_BIAS;
float ShadowMapManager::NormalBias = DEFAULT_NORMAL_BIAS;

bgfx::TextureHandle ShadowMapManager::ShadowAtlasTexture = BGFX_INVALID_HANDLE;
bgfx::FrameBufferHandle ShadowMapManager::ShadowAtlasFramebuffer = BGFX_INVALID_HANDLE;
bgfx::ProgramHandle ShadowMapManager::ShadowProgram = BGFX_INVALID_HANDLE;
bgfx::UniformHandle ShadowMapManager::ShadowMapSampler = BGFX_INVALID_HANDLE;
bgfx::UniformHandle ShadowMapManager::ShadowLightViewProjUniform = BGFX_INVALID_HANDLE;
bgfx::UniformHandle ShadowMapManager::ShadowCascadeSplitsUniform = BGFX_INVALID_HANDLE;
bgfx::UniformHandle ShadowMapManager::ShadowCascadeTexelSizeUniform = BGFX_INVALID_HANDLE;
bgfx::UniformHandle ShadowMapManager::ShadowConfigUniform = BGFX_INVALID_HANDLE;

Matrix4 ShadowMapManager::LightView[NUM_CASCADES] = {Matrix4(true), Matrix4(true), Matrix4(true)};
Matrix4 ShadowMapManager::LightProjection[NUM_CASCADES] = {Matrix4(true), Matrix4(true), Matrix4(true)};
Matrix4 ShadowMapManager::ShadowTextureMatrix[NUM_CASCADES] = {Matrix4(true), Matrix4(true), Matrix4(true)};
float ShadowMapManager::CascadeSplits[NUM_CASCADES + 1] = {0.0f, 0.0f, 0.0f, 0.0f};
float ShadowMapManager::CascadeWorldTexelSize[NUM_CASCADES] = {0.0f, 0.0f, 0.0f};
OBBoxClass ShadowMapManager::CascadeCullBoxes[NUM_CASCADES];
bool ShadowMapManager::CascadeCullBoxValid[NUM_CASCADES] = {false, false, false};
uint16_t ShadowMapManager::ShadowViewIds[NUM_CASCADES] = {0, 0, 0};

namespace
{

Matrix4 Build_Shadow_Crop_Matrix(int cascade_index)
{
    const bgfx::Caps *caps = bgfx::getCaps();
    const float cascade_width = 1.0f / static_cast<float>(ShadowMapManager::NUM_CASCADES);
    const float y_scale = (caps != nullptr && caps->originBottomLeft) ? 0.5f : -0.5f;
    const float z_scale = (caps != nullptr && caps->homogeneousDepth) ? 0.5f : 1.0f;
    const float z_offset = (caps != nullptr && caps->homogeneousDepth) ? 0.5f : 0.0f;

    Matrix4 crop(true);
    crop[0][0] = 0.5f * cascade_width;
    crop[0][3] = (0.5f + static_cast<float>(cascade_index)) * cascade_width;
    crop[1][1] = y_scale;
    crop[1][3] = 0.5f;
    crop[2][2] = z_scale;
    crop[2][3] = z_offset;
    return crop;
}

Matrix4 Build_Shadow_Render_Projection(const Matrix4 &projection)
{
    const bgfx::Caps *caps = bgfx::getCaps();
    if (caps == nullptr || caps->homogeneousDepth) {
        return projection;
    }

    Matrix4 adjusted = projection;
    adjusted[2][0] = 0.5f * (projection[2][0] + projection[3][0]);
    adjusted[2][1] = 0.5f * (projection[2][1] + projection[3][1]);
    adjusted[2][2] = 0.5f * (projection[2][2] + projection[3][2]);
    adjusted[2][3] = 0.5f * (projection[2][3] + projection[3][3]);
    return adjusted;
}

// Find a suitable depth format for the shadow map that supports sampling
bgfx::TextureFormat::Enum Find_Shadow_Depth_Format()
{
    // Prefer D16 for shadow maps - sufficient precision and widely supported for sampling
    static constexpr bgfx::TextureFormat::Enum candidates[] = {
        bgfx::TextureFormat::D16,
        bgfx::TextureFormat::D24,
        bgfx::TextureFormat::D32,
        bgfx::TextureFormat::D32F,
        bgfx::TextureFormat::D24S8,
    };

    const bgfx::Caps *caps = bgfx::getCaps();
    if (caps == nullptr) {
        return bgfx::TextureFormat::Count;
    }

    for (auto fmt : candidates) {
        uint32_t flags = caps->formats[fmt];
        // Need both framebuffer and sampling support
        if ((flags & BGFX_CAPS_FORMAT_TEXTURE_FRAMEBUFFER) != 0 &&
            (flags & (BGFX_CAPS_FORMAT_TEXTURE_2D)) != 0) {
            return fmt;
        }
    }

    return bgfx::TextureFormat::Count;
}

} // anonymous namespace

bool ShadowMapManager::Init(int cascade_size)
{
    if (Initted) {
        return true;
    }

    CascadeSize = cascade_size;

    // Find a suitable depth format
    bgfx::TextureFormat::Enum depth_format = Find_Shadow_Depth_Format();
    if (depth_format == bgfx::TextureFormat::Count) {
        WWDEBUG_SAY(("ShadowMapManager: No suitable shadow map depth format found\n"));
        return false;
    }

    // Create shadow atlas: 3 cascades side by side
    const uint16_t atlas_width = static_cast<uint16_t>(CascadeSize * NUM_CASCADES);
    const uint16_t atlas_height = static_cast<uint16_t>(CascadeSize);

    ShadowAtlasTexture = bgfx::createTexture2D(
        atlas_width, atlas_height,
        false, 1,
        depth_format,
        BGFX_TEXTURE_RT |
        BGFX_SAMPLER_U_CLAMP | BGFX_SAMPLER_V_CLAMP |
        BGFX_SAMPLER_MIN_POINT | BGFX_SAMPLER_MAG_POINT);

    if (!bgfx::isValid(ShadowAtlasTexture)) {
        WWDEBUG_SAY(("ShadowMapManager: Failed to create shadow atlas texture\n"));
        return false;
    }

    bgfx::Attachment attachment;
    attachment.init(ShadowAtlasTexture);
    ShadowAtlasFramebuffer = bgfx::createFrameBuffer(1, &attachment, false);

    if (!bgfx::isValid(ShadowAtlasFramebuffer)) {
        WWDEBUG_SAY(("ShadowMapManager: Failed to create shadow atlas framebuffer\n"));
        bgfx::destroy(ShadowAtlasTexture);
        ShadowAtlasTexture = BGFX_INVALID_HANDLE;
        return false;
    }

    // Create uniforms
    ShadowMapSampler = bgfx::createUniform("s_shadowMap", bgfx::UniformType::Sampler);
    ShadowLightViewProjUniform = bgfx::createUniform("u_shadowLightViewProj", bgfx::UniformType::Mat4, NUM_CASCADES);
    ShadowCascadeSplitsUniform = bgfx::createUniform("u_shadowCascadeSplits", bgfx::UniformType::Vec4);
    ShadowCascadeTexelSizeUniform = bgfx::createUniform("u_shadowCascadeTexelSize", bgfx::UniformType::Vec4);
    ShadowConfigUniform = bgfx::createUniform("u_shadowConfig", bgfx::UniformType::Vec4);

    // Load shadow depth shader
    ShadowProgram = BgfxRenderer::Load_Program("vs_shadow", "fs_shadow");
    if (!bgfx::isValid(ShadowProgram)) {
        WWDEBUG_SAY(("ShadowMapManager: Failed to load shadow shader program\n"));
        Shutdown();
        return false;
    }

    Initted = true;
    WWDEBUG_SAY(("ShadowMapManager: Initialized with %dx%d atlas (%d cascades of %d)\n",
                 atlas_width, atlas_height, NUM_CASCADES, CascadeSize));
    return true;
}

void ShadowMapManager::Shutdown()
{
    auto destroy_uniform = [](bgfx::UniformHandle &h) {
        if (bgfx::isValid(h)) { bgfx::destroy(h); h = BGFX_INVALID_HANDLE; }
    };

    if (bgfx::isValid(ShadowProgram)) {
        bgfx::destroy(ShadowProgram);
        ShadowProgram = BGFX_INVALID_HANDLE;
    }

    destroy_uniform(ShadowConfigUniform);
    destroy_uniform(ShadowCascadeTexelSizeUniform);
    destroy_uniform(ShadowCascadeSplitsUniform);
    destroy_uniform(ShadowLightViewProjUniform);
    destroy_uniform(ShadowMapSampler);

    if (bgfx::isValid(ShadowAtlasFramebuffer)) {
        bgfx::destroy(ShadowAtlasFramebuffer);
        ShadowAtlasFramebuffer = BGFX_INVALID_HANDLE;
    }

    if (bgfx::isValid(ShadowAtlasTexture)) {
        bgfx::destroy(ShadowAtlasTexture);
        ShadowAtlasTexture = BGFX_INVALID_HANDLE;
    }

    for (int cascade = 0; cascade < NUM_CASCADES; ++cascade) {
        CascadeCullBoxValid[cascade] = false;
    }

    Initted = false;
}

void ShadowMapManager::Compute_Cascade_Splits(float near_clip, float far_clip)
{
    // Clamp far to shadow distance
    float shadow_far = std::min(far_clip, ShadowDistance);

    // Practical split scheme: blend between logarithmic and uniform
    CascadeSplits[0] = near_clip;
    for (int i = 1; i <= NUM_CASCADES; ++i) {
        float p = static_cast<float>(i) / static_cast<float>(NUM_CASCADES);
        float log_split = near_clip * std::pow(shadow_far / near_clip, p);
        float uniform_split = near_clip + (shadow_far - near_clip) * p;
        CascadeSplits[i] = CASCADE_SPLIT_LAMBDA * log_split + (1.0f - CASCADE_SPLIT_LAMBDA) * uniform_split;
    }
}

void ShadowMapManager::Compute_Frustum_Corners(const CameraClass &camera, float near_frac, float far_frac,
                                                  Vector3 corners[8])
{
    // Use the camera's own world-space frustum corners.
    // Layout: near plane 0-3 (UL, UR, LL, LR), far plane 4-7 (UL, UR, LL, LR)
    const Vector3 *cam_corners = camera.Get_Frustum_Corners();

    // Interpolate from near plane to far plane at the requested fractions.
    for (int i = 0; i < 4; ++i) {
        Vector3 dir = cam_corners[i + 4] - cam_corners[i];
        corners[i]     = cam_corners[i] + dir * near_frac;
        corners[i + 4] = cam_corners[i] + dir * far_frac;
    }
}

void ShadowMapManager::Compute_Light_Matrices(const CameraClass &camera, const Vector3 &sun_direction)
{
    float near_clip, camera_far_clip;
    camera.Get_Clip_Planes(near_clip, camera_far_clip);

        Vector3 light_forward = sun_direction;
        light_forward.Normalize();

        // Build light coordinate axes (constant for all cascades)
        Vector3 light_up(0.0f, 0.0f, 1.0f);
        if (std::fabs(Vector3::Dot_Product(light_forward, light_up)) > 0.99f) {
            light_up = Vector3(0.0f, 1.0f, 0.0f);
        }
        Vector3 light_right;
        Vector3::Cross_Product(light_up, light_forward, &light_right);
        light_right.Normalize();
        Vector3::Cross_Product(light_forward, light_right, &light_up);
        light_up.Normalize();

    for (int cascade = 0; cascade < NUM_CASCADES; ++cascade) {
        float split_near = CascadeSplits[cascade];
        float split_far = CascadeSplits[cascade + 1];

        const float frustum_depth = camera_far_clip - near_clip;
        float near_frac = frustum_depth > 1.0e-6f ? (split_near - near_clip) / frustum_depth : 0.0f;
        float far_frac = frustum_depth > 1.0e-6f ? (split_far - near_clip) / frustum_depth : 0.0f;

        Vector3 corners[8];
        Compute_Frustum_Corners(camera, near_frac, far_frac, corners);

        // Compute frustum center and bounding sphere radius.
        // The radius is rotation-invariant (depends only on frustum shape),
        // which makes the ortho projection stable regardless of camera rotation.
        Vector3 center(0.0f, 0.0f, 0.0f);
        for (int i = 0; i < 8; ++i) {
            center += corners[i];
        }
        center *= (1.0f / 8.0f);

        float radius = 0.0f;
        for (int i = 0; i < 8; ++i) {
            float dist = (corners[i] - center).Length();
            radius = std::max(radius, dist);
        }
        // Round up to avoid sub-texel size changes
        radius = std::ceil(radius * 16.0f) / 16.0f;

        // Texel size is FIXED for a given cascade (2*radius / CascadeSize).
        // This prevents the snap grid from changing when the camera rotates.
        float texel_size = (2.0f * radius) / static_cast<float>(CascadeSize);

        // Project sphere center onto light-space XY and snap to texel grid.
        // This stabilizes the shadow map against camera translation swimming.
        float cx = Vector3::Dot_Product(center, light_right);
        float cy = Vector3::Dot_Product(center, light_up);
        float cz = Vector3::Dot_Product(center, light_forward);
        cx = std::floor(cx / texel_size) * texel_size;
        cy = std::floor(cy / texel_size) * texel_size;

        // Reconstruct snapped center in world space
        Vector3 snapped_center = light_right * cx + light_up * cy + light_forward * cz;

        // Position light camera behind the scene
        Vector3 light_pos = snapped_center - light_forward * radius * 4.0f;

        // Build view matrix (row-major)
        Matrix4 light_view(true);
        light_view[0][0] = light_right.X;
        light_view[0][1] = light_right.Y;
        light_view[0][2] = light_right.Z;
        light_view[0][3] = -Vector3::Dot_Product(light_right, light_pos);
        light_view[1][0] = light_up.X;
        light_view[1][1] = light_up.Y;
        light_view[1][2] = light_up.Z;
        light_view[1][3] = -Vector3::Dot_Product(light_up, light_pos);
        light_view[2][0] = -light_forward.X;
        light_view[2][1] = -light_forward.Y;
        light_view[2][2] = -light_forward.Z;
        light_view[2][3] = Vector3::Dot_Product(light_forward, light_pos);
        light_view[3][0] = 0.0f;
        light_view[3][1] = 0.0f;
        light_view[3][2] = 0.0f;
        light_view[3][3] = 1.0f;

        // Transform corners to light space for Z bounds only
        float min_z = FLT_MAX, max_z = -FLT_MAX;
        for (int i = 0; i < 8; ++i) {
            float lz = light_view[2][0] * corners[i].X + light_view[2][1] * corners[i].Y +
                        light_view[2][2] * corners[i].Z + light_view[2][3];
            min_z = std::min(min_z, lz);
            max_z = std::max(max_z, lz);
        }
        // Extend near to catch shadow casters behind the frustum
        float z_margin = (max_z - min_z) * 2.0f;
        min_z -= z_margin;

        const float z_near = std::max(0.1f, -max_z);
        const float z_far = std::max(z_near + 0.1f, -min_z);
        if ((z_far - z_near) < 1e-6f || radius < 1e-6f) {
            LightView[cascade] = Matrix4(true);
            LightProjection[cascade] = Matrix4(true);
            ShadowTextureMatrix[cascade] = Matrix4(true);
            CascadeWorldTexelSize[cascade] = 0.0f;
            CascadeCullBoxValid[cascade] = false;
            continue;
        }

        Matrix4 light_proj(true);
        light_proj.Init_Ortho(-radius, radius, -radius, radius, z_near, z_far);
        Matrix4 render_projection = Build_Shadow_Render_Projection(light_proj);

        LightView[cascade] = light_view;
        LightProjection[cascade] = render_projection;
        ShadowTextureMatrix[cascade] = Build_Shadow_Crop_Matrix(cascade) * render_projection * light_view;
        CascadeWorldTexelSize[cascade] = texel_size;

        const Vector3 light_depth_axis = -light_forward;
        const float depth_center = 0.5f * (min_z + max_z);
        const float depth_extent = 0.5f * (max_z - min_z);
        CascadeCullBoxes[cascade].Basis = Matrix3(
            light_right.X, light_up.X, light_depth_axis.X,
            light_right.Y, light_up.Y, light_depth_axis.Y,
            light_right.Z, light_up.Z, light_depth_axis.Z);
        CascadeCullBoxes[cascade].Center = light_pos + light_depth_axis * depth_center;
        CascadeCullBoxes[cascade].Extent.Set(radius, radius, depth_extent);
        CascadeCullBoxValid[cascade] = true;
    }
}

void ShadowMapManager::Update(const CameraClass &camera, const Vector3 &sun_direction)
{
    if (!Is_Enabled()) {
        return;
    }

    float near_clip, far_clip;
    camera.Get_Clip_Planes(near_clip, far_clip);

    Compute_Cascade_Splits(near_clip, far_clip);
    Compute_Light_Matrices(camera, sun_direction);
}

bool ShadowMapManager::Get_Cascade_Cull_Box(int cascade, OBBoxClass *set_box)
{
    if (cascade < 0 || cascade >= NUM_CASCADES || !CascadeCullBoxValid[cascade]) {
        return false;
    }

    if (set_box != NULL) {
        *set_box = CascadeCullBoxes[cascade];
    }

    return true;
}

bool ShadowMapManager::Intersects_Shadow_Cascades(const SphereClass &sphere)
{
    if (!Is_Enabled()) {
        return false;
    }

    for (int cascade = 0; cascade < NUM_CASCADES; ++cascade) {
        if (!CascadeCullBoxValid[cascade]) {
            continue;
        }

        const OBBoxClass &box = CascadeCullBoxes[cascade];
        const Vector3 offset = sphere.Center - box.Center;
        const Vector3 axis_x(box.Basis[0][0], box.Basis[1][0], box.Basis[2][0]);
        const Vector3 axis_y(box.Basis[0][1], box.Basis[1][1], box.Basis[2][1]);
        const Vector3 axis_z(box.Basis[0][2], box.Basis[1][2], box.Basis[2][2]);

        const float local_x = Vector3::Dot_Product(offset, axis_x);
        const float local_y = Vector3::Dot_Product(offset, axis_y);
        const float local_z = Vector3::Dot_Product(offset, axis_z);

        if (std::fabs(local_x) <= box.Extent.X + sphere.Radius &&
            std::fabs(local_y) <= box.Extent.Y + sphere.Radius &&
            std::fabs(local_z) <= box.Extent.Z + sphere.Radius) {
            return true;
        }
    }

    return false;
}

void ShadowMapManager::Setup_Shadow_Views()
{
    if (!Is_Enabled()) {
        return;
    }

    for (int cascade = 0; cascade < NUM_CASCADES; ++cascade) {
        uint16_t view_id = static_cast<uint16_t>(cascade); // Views 0-2 for shadow cascades
        ShadowViewIds[cascade] = view_id;

        bgfx::setViewFrameBuffer(view_id, ShadowAtlasFramebuffer);
        bgfx::setViewRect(view_id,
                          static_cast<uint16_t>(cascade * CascadeSize), 0,
                          static_cast<uint16_t>(CascadeSize),
                          static_cast<uint16_t>(CascadeSize));

        bgfx::setViewClear(view_id, BGFX_CLEAR_DEPTH, 0, 1.0f, 0);
        bgfx::setViewMode(view_id, bgfx::ViewMode::Sequential);

        // Touch the view to ensure it's processed even if no geometry is submitted
        bgfx::touch(view_id);

        Matrix4 view = LightView[cascade].Transpose();
        Matrix4 projection = LightProjection[cascade].Transpose();
        bgfx::setViewTransform(view_id, &view[0][0], &projection[0][0]);
    }
}

uint16_t ShadowMapManager::Begin_Shadow_Pass(int cascade_index)
{
    if (!Is_Enabled() || cascade_index < 0 || cascade_index >= NUM_CASCADES) {
        return 0;
    }

    ShadowPassActive = true;
    CurrentShadowCascade = cascade_index;

    return ShadowViewIds[cascade_index];
}

void ShadowMapManager::End_Shadow_Pass()
{
    ShadowPassActive = false;
    CurrentShadowCascade = 0;
}

void ShadowMapManager::Bind_Shadow_Uniforms(bool receive_shadows)
{
    if (!bgfx::isValid(ShadowConfigUniform)) {
        return;
    }

    const bool shadows_enabled = Is_Enabled() && receive_shadows;
    const float texel_size = CascadeSize > 0 ? 1.0f / static_cast<float>(CascadeSize) : 0.0f;
    float config[4] = {texel_size, DepthBias, ShadowIntensity, shadows_enabled ? 1.0f : 0.0f};
    bgfx::setUniform(ShadowConfigUniform, config);

    if (!shadows_enabled) {
        return;
    }

    float vp_data[NUM_CASCADES * 16];
    for (int i = 0; i < NUM_CASCADES; ++i) {
        Matrix4 transposed = ShadowTextureMatrix[i].Transpose();
        std::memcpy(&vp_data[i * 16], &transposed[0][0], sizeof(float) * 16);
    }
    bgfx::setUniform(ShadowLightViewProjUniform, vp_data, NUM_CASCADES);

    float splits[4] = {CascadeSplits[1], CascadeSplits[2], CascadeSplits[3], ShadowDistance};
    bgfx::setUniform(ShadowCascadeSplitsUniform, splits);

    float cascade_texel_size[4] = {
        CascadeWorldTexelSize[0],
        CascadeWorldTexelSize[1],
        CascadeWorldTexelSize[2],
        NormalBias
    };
    bgfx::setUniform(ShadowCascadeTexelSizeUniform, cascade_texel_size);

    bgfx::setTexture(2, ShadowMapSampler, ShadowAtlasTexture,
                     BGFX_SAMPLER_U_CLAMP | BGFX_SAMPLER_V_CLAMP |
                     BGFX_SAMPLER_MIN_POINT | BGFX_SAMPLER_MAG_POINT);
}

void ShadowMapManager::Submit_Shadow_Draws(
    const VertexBufferClass &vertex_buffer,
    unsigned vertex_buffer_offset,
    unsigned index_base_offset,
    unsigned short min_vertex_index,
    unsigned short vertex_count,
    const IndexBufferClass &index_buffer,
    unsigned index_buffer_offset,
    unsigned short start_index,
    uint32_t submitted_index_count,
    bool use_direct_vertex_buffer,
    bool use_direct_index_buffer,
    const bgfx::TransientVertexBuffer *transient_vb,
    const bgfx::TransientIndexBuffer *transient_ib,
    const Matrix4 &world,
    bgfx::TextureHandle stage0_texture,
    uint32_t stage0_flags,
    bgfx::TextureHandle stage1_texture,
    uint32_t stage1_flags,
    bool alpha_test_enabled,
    unsigned cull_mode)
{
    if (!Is_Enabled()) {
        return;
    }

    const Matrix4 world_transform = world.Transpose();
    uint64_t shadow_state = BGFX_STATE_WRITE_Z | BGFX_STATE_DEPTH_TEST_LESS | BGFX_STATE_MSAA;
    if (cull_mode == D3DCULL_CW) {
        shadow_state |= BGFX_STATE_CULL_CW;
    } else if (cull_mode == D3DCULL_CCW) {
        shadow_state |= BGFX_STATE_CULL_CCW;
    }

    for (int cascade = 0; cascade < NUM_CASCADES; ++cascade) {
        bgfx::setTransform(&world_transform[0][0]);

        // Re-bind vertex buffer
        if (use_direct_index_buffer) {
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
            bgfx::setVertexBuffer(0, transient_vb);
        }

        // Re-bind index buffer
        if (use_direct_index_buffer) {
            bgfx::setIndexBuffer(
                static_cast<const RenderIndexBufferClass &>(index_buffer).Get_Bgfx_Index_Buffer(),
                index_buffer_offset + start_index,
                submitted_index_count);
        } else {
            bgfx::setIndexBuffer(transient_ib);
        }

        if (alpha_test_enabled) {
            bgfx::setTexture(0, BgfxRenderer::Get_Texture0_Uniform(), stage0_texture, stage0_flags);
            bgfx::setTexture(1, BgfxRenderer::Get_Texture1_Uniform(), stage1_texture, stage1_flags);
        }

        bgfx::setState(shadow_state);
        bgfx::submit(ShadowViewIds[cascade], ShadowProgram);
    }
}
