// Shadow map sampling utilities for mesh fragment shaders.
// Included by fs_mesh_*.sc when shadow mapping is active.

// Shadow map sampler (slot 2)
SAMPLER2D(s_shadowMap, 2);

// u_shadowLightViewProj: 3 cascade light view-projection matrices
uniform mat4 u_shadowLightViewProj[3];

// u_shadowCascadeSplits.xyz = cascade far distances in view space
// u_shadowCascadeSplits.w   = max shadow distance
uniform vec4 u_shadowCascadeSplits;

// u_shadowCascadeTexelSize.xyz = cascade world-space texel size
// u_shadowCascadeTexelSize.w   = normal bias in texels
uniform vec4 u_shadowCascadeTexelSize;

// u_shadowConfig.x = cascade texel size in Y (1.0 / cascade_size)
// u_shadowConfig.y = depth bias
// u_shadowConfig.z = shadow intensity (0..1, how dark shadows are)
// u_shadowConfig.w = receiver enabled (0 or 1)
uniform vec4 u_shadowConfig;

// Select which cascade the fragment belongs to based on view depth.
// Returns cascade index 0..2, or -1 if beyond shadow range.
int SelectCascade(float viewDepth)
{
    if (viewDepth < u_shadowCascadeSplits.x) return 0;
    if (viewDepth < u_shadowCascadeSplits.y) return 1;
    if (viewDepth < u_shadowCascadeSplits.z) return 2;
    return -1;
}

// Transform world position directly into shadow-atlas UV + depth.
// The matrix already includes bgfx clip-space conversion and atlas cropping.
vec3 WorldToShadowUV(vec3 worldPos, int cascade)
{
    vec4 shadowPos = mul(u_shadowLightViewProj[cascade], vec4(worldPos, 1.0));
    shadowPos.xyz /= shadowPos.w;
    return shadowPos.xyz;
}

// Transform a world-space direction into shadow-atlas space without translation.
vec3 WorldDirToShadowOffset(vec3 worldDir, int cascade)
{
    return mul(u_shadowLightViewProj[cascade], vec4(worldDir, 0.0)).xyz;
}

// PCF 3x3 shadow sampling
float SampleShadowPCF(vec3 shadowUV, float refDepth)
{
    const float cascadeCount = 3.0;
    vec2 texelSize = vec2(u_shadowConfig.x / cascadeCount, u_shadowConfig.x);
    float shadow = 0.0;

    // 3x3 PCF kernel
    for (int y = -1; y <= 1; y++) {
        for (int x = -1; x <= 1; x++) {
            vec2 offset = vec2(float(x), float(y)) * texelSize;
            float sampledDepth = texture2D(s_shadowMap, shadowUV.xy + offset).r;
            shadow += step(refDepth, sampledDepth);
        }
    }

    return shadow / 9.0;
}

// Compute shadow factor for a world-space fragment position.
// Returns 1.0 = fully lit, (1 - intensity) = fully shadowed.
float ComputeShadow(vec3 worldPos, vec3 worldNormal, float viewDepth)
{
    if (u_shadowConfig.w < 0.5) {
        return 1.0;
    }

    int cascade = SelectCascade(viewDepth);
    if (cascade < 0) {
        return 1.0; // Beyond shadow range
    }

    vec3 shadowUV = WorldToShadowUV(worldPos, cascade);
    float worldTexelSize = cascade == 0 ? u_shadowCascadeTexelSize.x
                          : cascade == 1 ? u_shadowCascadeTexelSize.y
                                         : u_shadowCascadeTexelSize.z;
    float normalBias = u_shadowCascadeTexelSize.w * worldTexelSize;
    float receiverDepth = shadowUV.z - u_shadowConfig.y;
    if (normalBias > 0.0) {
        // Keep receiver bias in the shadow-depth comparison only. Offsetting
        // the full world position shifts atlas UVs sideways on sloped terrain
        // and makes shadowed shoulders/ruts sample lit texels.
        receiverDepth += WorldDirToShadowOffset(normalize(worldNormal) * normalBias, cascade).z;
    }
    receiverDepth = clamp(receiverDepth, 0.0, 1.0);

    // Clamp to valid atlas region for this cascade
    const float cascadeCount = 3.0;
    vec2 texelSize = vec2(u_shadowConfig.x / cascadeCount, u_shadowConfig.x);
    float cascadeMin = float(cascade) / cascadeCount;
    float cascadeMax = float(cascade + 1) / cascadeCount;
    if (shadowUV.x < cascadeMin + texelSize.x || shadowUV.x > cascadeMax - texelSize.x ||
        shadowUV.y < texelSize.y || shadowUV.y > 1.0 - texelSize.y ||
        shadowUV.z < 0.0 || shadowUV.z > 1.0) {
        return 1.0;
    }

    float shadow = SampleShadowPCF(shadowUV, receiverDepth);
    float intensity = u_shadowConfig.z;
    return 1.0 - intensity * (1.0 - shadow);
}
