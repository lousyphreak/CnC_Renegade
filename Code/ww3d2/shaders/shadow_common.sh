// Shadow map sampling utilities for mesh fragment shaders.
// Included by fs_mesh_*.sc when shadow mapping is active.

#include "shadow_uniforms.sh"

// Shadow map sampler (slot 2)
SAMPLER2D(s_shadowMap, 2);

// u_shadowCascadeSplits.xyz = cascade far distances in view space
// u_shadowCascadeSplits.w   = max shadow distance
uniform highp vec4 u_shadowCascadeSplits;

// u_shadowCascadeTexelSize.xyz = cascade world-space texel size
// u_shadowCascadeTexelSize.w   = normal bias in texels
uniform highp vec4 u_shadowCascadeTexelSize;

// u_shadowConfig.x = cascade texel size in Y (1.0 / cascade_size)
// u_shadowConfig.y = explicit normalized depth bias
// u_shadowConfig.z = shadow intensity (0..1, how dark shadows are)
// u_shadowConfig.w = receiver enabled (0 or 1)
uniform highp vec4 u_shadowConfig;

// u_shadowLightDirection.xyz = normalized world-space direction from the
// light toward receivers. Using this directly keeps the receiver bias
// independent of backend depth conventions.
uniform highp vec4 u_shadowLightDirection;

// u_shadowReceiverDebug.xyz = cascade light-depth ranges in world units
// u_shadowReceiverDebug.w   = receiver debug draw enabled (> 0 enables debug draw)
uniform highp vec4 u_shadowReceiverDebug;

// Select which cascade the fragment belongs to based on view depth.
// Returns cascade index 0..2, or -1 if beyond shadow range.
int SelectCascade(highp float viewDepth)
{
    if (viewDepth < u_shadowCascadeSplits.x) return 0;
    if (viewDepth < u_shadowCascadeSplits.y) return 1;
    if (viewDepth < u_shadowCascadeSplits.z) return 2;
    return -1;
}

// GLSL ES 1.00 (WebGL 1.0) Appendix A forbids dynamic indexing of uniform
// arrays in fragment shaders outside of constant-index-expressions and
// loop-indices. The constant-indexed cascade selector lives in
// shadow_uniforms.sh now so both the VS and FS use the same helper.

// Pick the per-cascade pre-projected shadow position (computed in the vertex
// shader and interpolated). xy = atlas UV, z = normalized depth.
highp vec3 GetCascadeShadowPos(int cascade,
    highp vec3 shadowProj0,
    highp vec3 shadowProj1,
    highp vec3 shadowProj2)
{
    if (cascade <= 0) {
        return shadowProj0;
    }
    if (cascade == 1) {
        return shadowProj1;
    }
    return shadowProj2;
}

// PCF 3x3 shadow sampling
highp float SampleShadowPCF(highp vec3 shadowUV, highp float refDepth)
{
    const highp float cascadeCount = 3.0;
    highp vec2 texelSize = vec2(u_shadowConfig.x / cascadeCount, u_shadowConfig.x);
    highp float shadow = 0.0;

    // 3x3 PCF kernel
    for (int y = -1; y <= 1; y++) {
        for (int x = -1; x <= 1; x++) {
            highp vec2 offset = vec2(float(x), float(y)) * texelSize;
            highp float sampledDepth = texture2D(s_shadowMap, shadowUV.xy + offset).r;
            shadow += step(refDepth, sampledDepth);
        }
    }

    return shadow / 9.0;
}

highp float GetCascadeWorldTexelSize(int cascade)
{
    return cascade == 0 ? u_shadowCascadeTexelSize.x
         : cascade == 1 ? u_shadowCascadeTexelSize.y
                        : u_shadowCascadeTexelSize.z;
}

highp float GetCascadeDepthRange(int cascade)
{
    return cascade == 0 ? u_shadowReceiverDebug.x
         : cascade == 1 ? u_shadowReceiverDebug.y
                        : u_shadowReceiverDebug.z;
}

highp vec4 EvaluateShadowReceiver(highp vec3 shadowProj0, highp vec3 shadowProj1, highp vec3 shadowProj2,
    highp vec3 worldNormal, highp float viewDepth)
{
    if (u_shadowConfig.w < 0.5) {
        return vec4(1.0, 0.0, 0.0, 0.0);
    }

    int cascade = SelectCascade(viewDepth);
    if (cascade < 0) {
        return vec4(1.0, 0.0, 0.0, 1.0); // Beyond shadow range
    }

    highp vec3 shadowUV = GetCascadeShadowPos(cascade, shadowProj0, shadowProj1, shadowProj2);
    highp float worldTexelSize = GetCascadeWorldTexelSize(cascade);
    highp float cascadeDepthRange = max(GetCascadeDepthRange(cascade), 1.0e-4);
    const highp float kReceiverBiasFloorTexels = 0.35;
    highp vec3 surfaceToLight = -normalize(u_shadowLightDirection.xyz);
    highp float ndotl = clamp(dot(normalize(worldNormal), surfaceToLight), 0.0, 1.0);
    // Use a small texel-derived floor for flat surfaces, then add extra bias as
    // the receiver becomes more grazing to the light. This fixes desktop acne
    // without depending on the projected normal-bias sign that regressed WebGL.
    highp float receiverDepthBiasWorld = min(
        max(u_shadowConfig.y * cascadeDepthRange, worldTexelSize * kReceiverBiasFloorTexels),
        worldTexelSize * 2.0);
    highp float normalBiasOffset = (u_shadowCascadeTexelSize.w * worldTexelSize *
        sqrt(max(1.0 - ndotl * ndotl, 0.0))) / cascadeDepthRange;
    highp float receiverDepth = shadowUV.z - (receiverDepthBiasWorld / cascadeDepthRange) - normalBiasOffset;
    receiverDepth = clamp(receiverDepth, 0.0, 1.0);

    // Clamp to valid atlas region for this cascade
    const highp float cascadeCount = 3.0;
    highp vec2 texelSize = vec2(u_shadowConfig.x / cascadeCount, u_shadowConfig.x);
    highp float cascadeMin = float(cascade) / cascadeCount;
    highp float cascadeMax = float(cascade + 1) / cascadeCount;
    if (shadowUV.x < cascadeMin + texelSize.x || shadowUV.x > cascadeMax - texelSize.x ||
        shadowUV.y < texelSize.y || shadowUV.y > 1.0 - texelSize.y ||
        shadowUV.z < 0.0 || shadowUV.z > 1.0) {
        return vec4(1.0, 0.0, 0.0, 2.0);
    }

    highp float shadow = SampleShadowPCF(shadowUV, receiverDepth);
    highp float intensity = u_shadowConfig.z;
    if (u_shadowReceiverDebug.w > 0.0) {
        // Diagnostic mode: expose the raw shadow comparison so we can see
        // exactly where the receiver-depth test fails. Encode the
        // unbiased and biased comparisons into shadowInfo.x / .y so fs_mesh can paint
        // them. Layout:
        //   x = sampledDepth (raw atlas value, 0=closest to light, 1=far/empty)
        //   y = receiverDepth (after bias)
        //   z = unbiasedReceiver (just shadowUV.z, no bias applied)
        // shadowInfo.w stays at 3.0 to mark the success path.
        highp float sampledDepth = texture2D(s_shadowMap, shadowUV.xy).r;
        return vec4(sampledDepth, receiverDepth, shadowUV.z, 3.0);
    }

    return vec4(1.0 - intensity * (1.0 - shadow), 0.0, 1.0, 3.0);
}
