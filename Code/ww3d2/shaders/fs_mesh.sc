$input v_color0, v_texcoord0, v_texcoord1, v_fogFactor, v_worldPos, v_viewDepth, v_worldNormal

#include <bgfx_shader.sh>
#include "mesh_common.sh"
#include "shadow_common.sh"

SAMPLER2D(s_texColor0, 0);
SAMPLER2D(s_texColor1, 1);

// u_meshFragConfig.x = stage0ColorOp
// u_meshFragConfig.y = stage1ColorOp
// u_meshFragConfig.z = alphaTestRef (-1 = disabled)
// u_meshFragConfig.w = stage0AlphaOp
uniform vec4 u_meshFragConfig;

// u_meshFragConfig2.x = stage1AlphaOp
// u_meshFragConfig2.y = fogMode (0=none, 1=enable, 2=scale_fragment, 3=white)
uniform vec4 u_meshFragConfig2;

uniform vec4 u_meshFogColor;

// Per-pixel lighting uniforms
// u_meshLitConfig.x = lighting mode: 0=unlit, 1=emissive-only (no normals), 2=per-pixel lit
// u_meshLitConfig.y = diffuseSource (0=material, 1=vertex color)
// u_meshLitConfig.z = ambientSource (0=material, 1=vertex color)
// u_meshLitConfig.w = emissiveSource (0=material, 1=vertex color)
uniform vec4 u_meshLitConfig;
uniform vec4 u_meshMaterialAmbient;
uniform vec4 u_meshMaterialDiffuse;
uniform vec4 u_meshMaterialEmissive;
uniform vec4 u_meshSceneAmbient;
#define MAX_SUBMIT_LIGHTS 16
uniform vec4 u_meshLightPosType[MAX_SUBMIT_LIGHTS];
uniform vec4 u_meshLightDirSpot[MAX_SUBMIT_LIGHTS];
uniform vec4 u_meshLightDiffuseRange[MAX_SUBMIT_LIGHTS];
uniform vec4 u_meshLightAmbientAtten[MAX_SUBMIT_LIGHTS];

// Bump env map uniforms
// u_meshBumpEnvMat = (mat00, mat01, mat10, mat11)
// u_meshBumpEnvLum = (lumScale, lumOffset, 0, 0)
uniform vec4 u_meshBumpEnvMat;
uniform vec4 u_meshBumpEnvLum;

vec4 ResolveColorSource(float source, vec4 materialColor, vec4 vertexColor)
{
    return source > 0.5 ? vertexColor : materialColor;
}

float ComputeLightAttenuation(float lightType, float range, float attenStart, float distance)
{
    if (lightType < 1.5) {
        return 1.0;
    }

    if (range <= 0.0 || distance > range) {
        return 0.0;
    }

    if (attenStart + 1.0e-4 < range) {
        return clamp(1.0 - (distance - attenStart) / max(range - attenStart, 1.0e-4), 0.0, 1.0);
    }

    return 1.0;
}

float ComputeSpotAttenuation(vec3 lightDirection, float spotCos, vec3 fragmentToLight)
{
    if (spotCos < -1.5) {
        return 1.0;
    }

    float cone = (dot(normalize(lightDirection), -fragmentToLight) - spotCos) / max(1.0 - spotCos, 1.0e-5);
    return clamp(cone, 0.0, 1.0);
}

void main()
{
    vec4 current;
    float litMode = u_meshLitConfig.x;

    if (litMode < 0.5) {
        // Unlit: vertex color passthrough
        current = v_color0;
    } else {
        vec4 diffuse = ResolveColorSource(u_meshLitConfig.y, u_meshMaterialDiffuse, v_color0);
        vec4 emissive = ResolveColorSource(u_meshLitConfig.w, u_meshMaterialEmissive, v_color0);

        if (litMode < 1.5) {
            // Lit, no normals: emissive only (D3D8 behavior)
            current = vec4(clamp(emissive.rgb, vec3_splat(0.0), vec3_splat(1.0)), diffuse.a);
        } else {
            // Full per-pixel lighting
            vec3 N = normalize(v_worldNormal);
            vec4 ambient = ResolveColorSource(u_meshLitConfig.z, u_meshMaterialAmbient, v_color0);
            vec3 litColor = emissive.rgb + (u_meshSceneAmbient.rgb * ambient.rgb);

            for (int i = 0; i < MAX_SUBMIT_LIGHTS; ++i) {
                float lightType = u_meshLightPosType[i].w;
                if (lightType > 0.5) {
                    vec3 L = vec3_splat(0.0);
                    float distanceToLight = 0.0;

                    if (lightType < 1.5) {
                        L = -normalize(u_meshLightDirSpot[i].xyz);
                    } else {
                        vec3 toLight = u_meshLightPosType[i].xyz - v_worldPos;
                        distanceToLight = length(toLight);
                        if (distanceToLight > 1.0e-6) {
                            L = toLight / distanceToLight;
                        }
                    }

                    float attenuation = ComputeLightAttenuation(
                        lightType,
                        u_meshLightDiffuseRange[i].w,
                        u_meshLightAmbientAtten[i].w,
                        distanceToLight);

                    if (lightType > 2.5) {
                        attenuation *= ComputeSpotAttenuation(u_meshLightDirSpot[i].xyz, u_meshLightDirSpot[i].w, L);
                    }

                    litColor += u_meshLightAmbientAtten[i].rgb * ambient.rgb * attenuation;

                    if (lightType < 1.5 || distanceToLight > 1.0e-6) {
                        float ndotl = max(dot(N, L), 0.0);
                        litColor += u_meshLightDiffuseRange[i].rgb * diffuse.rgb * (attenuation * ndotl);
                    }
                }
            }

            current = vec4(clamp(litColor, vec3_splat(0.0), vec3_splat(1.0)), diffuse.a);
        }
    }

    float stage0Op = u_meshFragConfig.x;
    vec4 tex0 = texture2D(s_texColor0, v_texcoord0);
    vec2 stage1_uv = v_texcoord1;

    if (stage0Op > 8.5 && stage0Op < 10.5) {
        // BUMPENVMAP or BUMPENVMAPLUMINANCE
        // Bump texture stores du/dv in RG (biased unsigned, decode to signed)
        vec2 bump = tex0.rg * 2.0 - 1.0;
        // Apply 2x2 bump env rotation/scale matrix
        vec2 perturbation;
        perturbation.x = bump.x * u_meshBumpEnvMat.x + bump.y * u_meshBumpEnvMat.y;
        perturbation.y = bump.x * u_meshBumpEnvMat.z + bump.y * u_meshBumpEnvMat.w;
        stage1_uv = v_texcoord1 + perturbation;

        if (stage0Op > 9.5) {
            // BUMPENVMAPLUMINANCE: modulate by luminance from alpha channel
            float lum = tex0.a * u_meshBumpEnvLum.x + u_meshBumpEnvLum.y;
            current.rgb *= clamp(lum, 0.0, 1.0);
        }
        // Stage 0 passes through current unchanged (bump doesn't produce color)
    } else if (stage0Op > 10.5 && stage0Op < 11.5) {
        // DOTPRODUCT3: per-pixel dot product between texture normal and current color
        vec3 t = tex0.rgb * 2.0 - 1.0;
        vec3 c = current.rgb * 2.0 - 1.0;
        float dp3 = clamp(dot(t, c), 0.0, 1.0);
        current = vec4(dp3, dp3, dp3, current.a);
    } else {
        // Standard stage 0 color op
        current = vec4(
            ApplyColorOp(stage0Op, current, tex0),
            ApplyAlphaOp(u_meshFragConfig.w, current.a, tex0));
    }

    vec4 tex1 = texture2D(s_texColor1, stage1_uv);

    current = vec4(
        ApplyColorOp(u_meshFragConfig.y, current, tex1),
        ApplyAlphaOp(u_meshFragConfig2.x, current.a, tex1));

    // Apply shadow
    float shadow = ComputeShadow(v_worldPos, v_worldNormal, v_viewDepth);
    current.rgb *= shadow;

    ApplyFog(current.rgb, u_meshFragConfig2.y, v_fogFactor, u_meshFogColor.rgb);

    if (u_meshFragConfig.z >= 0.0) {
        if (current.a < u_meshFragConfig.z) {
            discard;
        }
    }

    gl_FragColor = current;
}
