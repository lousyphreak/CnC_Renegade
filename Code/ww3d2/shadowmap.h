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

#pragma once

#ifndef SHADOWMAP_H
#define SHADOWMAP_H

#include "always.h"
#include "matrix3d.h"
#include "matrix4.h"
#include "vector3.h"
#include "aabox.h"
#include "obbox.h"
#include <bgfx/bgfx.h>

class CameraClass;
class VertexBufferClass;
class IndexBufferClass;
class SphereClass;

/**
** ShadowMapManager
** Manages Cascaded Shadow Maps (CSM) with PCF filtering.
** Renders the scene from the sun's perspective into a depth atlas,
** which mesh fragment shaders sample to compute shadow.
**
** Atlas layout: 3 cascades side-by-side in a single depth texture.
** Each cascade covers a frustum slice at increasing distance from the camera.
*/
class ShadowMapManager
{
public:
    static constexpr int NUM_CASCADES = 3;
    static constexpr int DEFAULT_CASCADE_SIZE = 2048;
    static constexpr float DEFAULT_SHADOW_DISTANCE = 200.0f;
    static constexpr float DEFAULT_SHADOW_INTENSITY = 0.6f;
    static constexpr float DEFAULT_DEPTH_BIAS = 0.0003f;
    static constexpr float DEFAULT_NORMAL_BIAS = 0.5f;
    static constexpr float CASCADE_SPLIT_LAMBDA = 0.95f;

    static bool Init(int cascade_size = DEFAULT_CASCADE_SIZE);
    static void Shutdown();
    static bool Is_Initted() { return Initted; }

    static void Set_Shadow_Distance(float distance) { ShadowDistance = distance; }
    static float Get_Shadow_Distance() { return ShadowDistance; }

    static void Set_Shadow_Intensity(float intensity) { ShadowIntensity = intensity; }
    static float Get_Shadow_Intensity() { return ShadowIntensity; }

    static void Set_Depth_Bias(float bias) { DepthBias = bias; }
    static float Get_Depth_Bias() { return DepthBias; }

    static void Set_Normal_Bias(float bias) { NormalBias = bias; }
    static float Get_Normal_Bias() { return NormalBias; }

    static void Set_Enabled(bool enabled) { Enabled = enabled; }
    static bool Is_Enabled() { return Enabled && Initted; }

    /**
    ** Compute cascade view-projection matrices for the current frame.
    ** Must be called before rendering shadow passes.
    ** @param camera The main game camera
    ** @param sun_direction Normalized direction FROM the sun (points toward ground)
    */
    static void Update(const CameraClass &camera, const Vector3 &sun_direction);

    /**
    ** Get the bgfx view ID for a given cascade's shadow render pass.
    ** These views are set up by Begin_Shadow_Pass.
    */
    static uint16_t Begin_Shadow_Pass(int cascade_index);
    static void End_Shadow_Pass();

    /**
    ** Bind shadow map texture and uniforms for mesh rendering.
    ** Call before submitting mesh draws.
    */
    static void Bind_Shadow_Uniforms(bool receive_shadows);

    /**
    ** Set up bgfx views for all shadow cascades (framebuffer, viewport, clear, transform).
    ** Call once per frame after Update(), before any mesh rendering.
    */
    static void Setup_Shadow_Views();

    /**
    ** Submit the current geometry to all shadow cascade views.
    ** Call after the normal bgfx submit to also render into shadow maps.
    ** Re-binds vertex/index buffers since bgfx consumes them on submit.
    */
    static void Submit_Shadow_Draws(
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
        unsigned cull_mode);

    // Shadow pass state management
    static bool Is_Shadow_Pass_Active() { return ShadowPassActive; }
    static uint16_t Get_Current_Shadow_View_Id() { return ShadowPassActive ? ShadowViewIds[CurrentShadowCascade] : 0; }

    // Accessors for shader data
    static bgfx::TextureHandle Get_Shadow_Texture() { return ShadowAtlasTexture; }
    static bgfx::FrameBufferHandle Get_Shadow_Framebuffer() { return ShadowAtlasFramebuffer; }
    static bgfx::UniformHandle Get_Shadow_Sampler() { return ShadowMapSampler; }
    static bgfx::ProgramHandle Get_Shadow_Program() { return ShadowProgram; }
    static const Matrix4 &Get_Light_View_Proj(int cascade) { return ShadowTextureMatrix[cascade]; }
    static float Get_Cascade_Split(int cascade) { return CascadeSplits[cascade]; }
    static int Get_Cascade_Size() { return CascadeSize; }
    static bool Get_Cascade_Cull_Box(int cascade, OBBoxClass *set_box);
    static bool Intersects_Shadow_Cascades(const SphereClass &sphere);

private:
    static void Compute_Cascade_Splits(float near_clip, float far_clip);
    static void Compute_Light_Matrices(const CameraClass &camera, const Vector3 &sun_direction);
    static void Compute_Frustum_Corners(const CameraClass &camera, float near_frac, float far_frac,
                                         Vector3 corners[8]);

    static bool Initted;
    static bool Enabled;
    static bool ShadowPassActive;
    static int CurrentShadowCascade;
    static int CascadeSize;
    static float ShadowDistance;
    static float ShadowIntensity;
    static float DepthBias;
    static float NormalBias;

    // Atlas: single depth texture with 3 cascades side-by-side
    static bgfx::TextureHandle ShadowAtlasTexture;
    static bgfx::FrameBufferHandle ShadowAtlasFramebuffer;

    // Shadow depth shader
    static bgfx::ProgramHandle ShadowProgram;

    // Uniforms
    static bgfx::UniformHandle ShadowMapSampler;
    static bgfx::UniformHandle ShadowLightViewProjUniform;
    static bgfx::UniformHandle ShadowCascadeSplitsUniform;
    static bgfx::UniformHandle ShadowCascadeTexelSizeUniform;
    static bgfx::UniformHandle ShadowConfigUniform;

    // Per-frame computed data
    static Matrix4 LightView[NUM_CASCADES];
    static Matrix4 LightProjection[NUM_CASCADES];
    static Matrix4 ShadowTextureMatrix[NUM_CASCADES];
    static float CascadeSplits[NUM_CASCADES + 1];
    static float CascadeWorldTexelSize[NUM_CASCADES];
    static OBBoxClass CascadeCullBoxes[NUM_CASCADES];
    static bool CascadeCullBoxValid[NUM_CASCADES];

    // bgfx view IDs reserved for shadow passes
    static uint16_t ShadowViewIds[NUM_CASCADES];
};

#endif // SHADOWMAP_H
