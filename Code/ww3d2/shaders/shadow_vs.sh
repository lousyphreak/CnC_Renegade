// Vertex-side cascaded-shadow projection.
//
// For each cascade we transform the world-space vertex position into the
// shadow atlas (xy = atlas UV, z = normalized depth). Doing this multiplication
// in the vertex stage means the rasterizer interpolates the already-tiny
// 0..1-range result across the triangle rather than the large world-space
// position. This is critical on WebGL/OpenGLES: per-fragment interpolators run
// at lower effective precision than full IEEE-32, and large terrain triangles
// would otherwise lose the bits needed to recover a meaningful shadow depth in
// the fragment shader.

#ifndef SHADOW_VS_SH
#define SHADOW_VS_SH

#include "shadow_uniforms.sh"

void ComputeShadowProjections(highp vec3 worldPos,
    out highp vec3 shadowProj0,
    out highp vec3 shadowProj1,
    out highp vec3 shadowProj2)
{
    highp vec4 wp = vec4(worldPos, 1.0);
    highp vec4 p0 = mul(u_shadowLightViewProj[0], wp);
    highp vec4 p1 = mul(u_shadowLightViewProj[1], wp);
    highp vec4 p2 = mul(u_shadowLightViewProj[2], wp);
    shadowProj0 = p0.xyz / max(p0.w, 1.0e-6);
    shadowProj1 = p1.xyz / max(p1.w, 1.0e-6);
    shadowProj2 = p2.xyz / max(p2.w, 1.0e-6);
}

#endif // SHADOW_VS_SH
