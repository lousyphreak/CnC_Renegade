$input a_position, a_normal, a_color0, a_color1, a_texcoord0, a_texcoord1
$output v_color0, v_texcoord0, v_texcoord1, v_specular0, v_fogFactor

#include <bgfx_shader.sh>

// u_meshConfig.x = lightingEnabled (0 or 1)
// u_meshConfig.y = hasNormals (0 or 1)
// u_meshConfig.z = fogMode: 0=none, 1=enabled, 2=scale_fragment, 3=white
// u_meshConfig.w = specularEnabled (0 or 1)
uniform vec4 u_meshConfig;

// u_meshMaterialConfig.x = diffuseSource (0=material, 1=color0)
// u_meshMaterialConfig.y = ambientSource (0=material, 1=color0)
// u_meshMaterialConfig.z = emissiveSource (0=material, 1=color0)
// u_meshMaterialConfig.w = unused
uniform vec4 u_meshMaterialConfig;

uniform vec4 u_meshMaterialAmbient;
uniform vec4 u_meshMaterialDiffuse;
uniform vec4 u_meshMaterialEmissive;
uniform vec4 u_meshSceneAmbient;

// u_meshFogParams.x = fogStart
// u_meshFogParams.y = fogEnd
// u_meshFogParams.z = rangeFogSign (1.0 = range, -1.0 = planar)
// u_meshFogParams.w = unused
uniform vec4 u_meshFogParams;

// 4 directional lights: direction.xyz, enabled in w
uniform vec4 u_meshLightDir[4];
// 4 directional lights: color.rgb, unused w
uniform vec4 u_meshLightColor[4];

// u_meshTexgenMode.x = texgen mode stage 0 (0=passthrough, 1=camera_normal, 2=camera_position, 3=reflection)
// u_meshTexgenMode.y = texgen UV source stage 0
// u_meshTexgenMode.z = texgen mode stage 1
// u_meshTexgenMode.w = texgen UV source stage 1
uniform vec4 u_meshTexgenMode;

// u_meshTexTransformFlags.x = stage0 transform flags (0=disabled, 1=count2, 2=count3_projected)
// u_meshTexTransformFlags.y = stage1 transform flags
// u_meshTexTransformFlags.z = unused
// u_meshTexTransformFlags.w = unused
uniform vec4 u_meshTexTransformFlags;

uniform mat4 u_meshTexTransform0;
uniform mat4 u_meshTexTransform1;

vec4 ResolveColorSource(float source, vec4 materialColor, vec4 vertexColor)
{
    return source > 0.5 ? vertexColor : materialColor;
}

vec2 ResolveTexcoord(
    int stage,
    vec2 texcoord0,
    vec2 texcoord1,
    vec3 viewPosition,
    vec3 viewNormal)
{
    float texgenMode = stage == 0 ? u_meshTexgenMode.x : u_meshTexgenMode.z;
    float uvSource = stage == 0 ? u_meshTexgenMode.y : u_meshTexgenMode.w;
    float transformFlags = stage == 0 ? u_meshTexTransformFlags.x : u_meshTexTransformFlags.y;

    vec4 coordinate;
    if (texgenMode < 0.5) {
        // Passthrough
        coordinate = uvSource > 0.5 ? vec4(texcoord1, 0.0, 1.0) : vec4(texcoord0, 0.0, 1.0);
    } else if (texgenMode < 1.5) {
        // Camera-space normal
        coordinate = vec4(viewNormal, 1.0);
    } else if (texgenMode < 2.5) {
        // Camera-space position
        coordinate = vec4(viewPosition, 1.0);
    } else {
        // Reflection
        vec3 eyeVector = normalize(-viewPosition);
        vec3 reflection = viewNormal * (2.0 * dot(viewNormal, eyeVector)) - eyeVector;
        coordinate = vec4(reflection, 1.0);
    }

    if (transformFlags > 0.5) {
        mat4 texMatrix = stage == 0 ? u_meshTexTransform0 : u_meshTexTransform1;
        coordinate = mul(texMatrix, coordinate);

        if (transformFlags > 1.5) {
            // count3|projected: divide xy by z
            float q = coordinate.z;
            if (abs(q) > 1.0e-6) {
                coordinate.xy /= q;
            }
        }
    }

    return coordinate.xy;
}

void main()
{
    gl_Position = mul(u_modelViewProj, vec4(a_position, 1.0));

    vec4 worldPosition4 = mul(u_model[0], vec4(a_position, 1.0));
    vec3 worldPosition = worldPosition4.xyz;
    vec3 viewPosition = mul(u_view, worldPosition4).xyz;
    vec3 worldNormal = normalize(mul(u_model[0], vec4(a_normal, 0.0)).xyz);
    vec3 viewNormal = normalize(mul(u_view, vec4(worldNormal, 0.0)).xyz);

    // Fog: linear vertex fog only
    v_fogFactor = 0.0;
    if (u_meshConfig.z > 0.5) {
        float fogDistance = u_meshFogParams.z > 0.0 ? length(viewPosition) : abs(viewPosition.z);
        float fogFactor = (u_meshFogParams.y - fogDistance) / max(u_meshFogParams.y - u_meshFogParams.x, 1.0e-4);
        v_fogFactor = 1.0 - clamp(fogFactor, 0.0, 1.0);
    }

    // Resolve diffuse color
    vec4 diffuse = ResolveColorSource(u_meshMaterialConfig.x, u_meshMaterialDiffuse, a_color0);

    if (u_meshConfig.x > 0.5) {
        // Lighting enabled
        if (u_meshConfig.y <= 0.5) {
            // No normals: only emissive contributes
            vec4 emissive = ResolveColorSource(u_meshMaterialConfig.z, u_meshMaterialEmissive, a_color0);
            v_color0 = vec4(clamp(emissive.rgb, vec3_splat(0.0), vec3_splat(1.0)), diffuse.a);
            v_specular0 = vec4_splat(0.0);
        } else {
            // Full directional lighting
            vec4 ambient = ResolveColorSource(u_meshMaterialConfig.y, u_meshMaterialAmbient, a_color0);
            vec4 emissive = ResolveColorSource(u_meshMaterialConfig.z, u_meshMaterialEmissive, a_color0);
            vec3 litColor = emissive.rgb + (u_meshSceneAmbient.rgb * ambient.rgb);

            for (int i = 0; i < 4; ++i) {
                if (u_meshLightDir[i].w > 0.5) {
                    vec3 lightVector = -normalize(u_meshLightDir[i].xyz);
                    float ndotl = max(dot(worldNormal, lightVector), 0.0);
                    litColor += u_meshLightColor[i].rgb * diffuse.rgb * ndotl;
                }
            }

            v_color0 = vec4(clamp(litColor, vec3_splat(0.0), vec3_splat(1.0)), diffuse.a);
            v_specular0 = a_color1;
        }
    } else {
        // No lighting: pass through vertex color
        v_color0 = a_color0;
        v_specular0 = a_color1;
    }

    v_texcoord0 = ResolveTexcoord(0, a_texcoord0, a_texcoord1, viewPosition, viewNormal);
    v_texcoord1 = ResolveTexcoord(1, a_texcoord0, a_texcoord1, viewPosition, viewNormal);
}
