$input a_position, a_normal, a_color0, a_color1, a_texcoord0, a_texcoord1
$output v_color0, v_texcoord0, v_texcoord1, v_fogFactor

#include <bgfx_shader.sh>

// Fog
uniform vec4 u_meshFogConfig;

// Lighting config
uniform vec4 u_meshLitConfig;

// Material colors
uniform vec4 u_meshMaterialAmbient;
uniform vec4 u_meshMaterialDiffuse;
uniform vec4 u_meshMaterialEmissive;
uniform vec4 u_meshSceneAmbient;

// Lights
uniform vec4 u_meshLightDir[4];
uniform vec4 u_meshLightColor[4];

// Texgen
uniform vec4 u_meshTexgenMode;
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
        coordinate = uvSource > 0.5 ? vec4(texcoord1, 0.0, 1.0) : vec4(texcoord0, 0.0, 1.0);
    } else if (texgenMode < 1.5) {
        coordinate = vec4(viewNormal, 1.0);
    } else if (texgenMode < 2.5) {
        coordinate = vec4(viewPosition, 1.0);
    } else {
        vec3 eyeVector = normalize(-viewPosition);
        vec3 reflection = viewNormal * (2.0 * dot(viewNormal, eyeVector)) - eyeVector;
        coordinate = vec4(reflection, 1.0);
    }

    if (transformFlags > 0.5) {
        mat4 texMatrix = stage == 0 ? u_meshTexTransform0 : u_meshTexTransform1;
        coordinate = mul(texMatrix, coordinate);
        if (transformFlags > 1.5) {
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

    vec3 worldNormal = normalize(mul(u_model[0], vec4(a_normal, 0.0)).xyz);

    vec4 diffuse = ResolveColorSource(u_meshLitConfig.y, u_meshMaterialDiffuse, a_color0);

    if (u_meshLitConfig.x <= 0.5) {
        vec4 emissive = ResolveColorSource(u_meshLitConfig.w, u_meshMaterialEmissive, a_color0);
        v_color0 = vec4(clamp(emissive.rgb, vec3_splat(0.0), vec3_splat(1.0)), diffuse.a);
    } else {
        vec4 ambient = ResolveColorSource(u_meshLitConfig.z, u_meshMaterialAmbient, a_color0);
        vec4 emissive = ResolveColorSource(u_meshLitConfig.w, u_meshMaterialEmissive, a_color0);
        vec3 litColor = emissive.rgb + (u_meshSceneAmbient.rgb * ambient.rgb);

        for (int i = 0; i < 4; ++i) {
            if (u_meshLightDir[i].w > 0.5) {
                float ndotl = max(dot(worldNormal, -normalize(u_meshLightDir[i].xyz)), 0.0);
                litColor += u_meshLightColor[i].rgb * diffuse.rgb * ndotl;
            }
        }

        v_color0 = vec4(clamp(litColor, vec3_splat(0.0), vec3_splat(1.0)), diffuse.a);
    }

    // Texgen
    vec3 viewPos = mul(u_modelView, vec4(a_position, 1.0)).xyz;
    vec3 viewNormal = normalize(mul(u_modelView, vec4(a_normal, 0.0)).xyz);

    v_texcoord0 = ResolveTexcoord(0, a_texcoord0, a_texcoord1, viewPos, viewNormal);
    v_texcoord1 = ResolveTexcoord(1, a_texcoord0, a_texcoord1, viewPos, viewNormal);

    // Fog
    v_fogFactor = 0.0;
    if (u_meshFogConfig.x > 0.5) {
        float dist = u_meshFogConfig.w > 0.0 ? length(viewPos) : abs(viewPos.z);
        float factor = (u_meshFogConfig.z - dist) / max(u_meshFogConfig.z - u_meshFogConfig.y, 1.0e-4);
        v_fogFactor = 1.0 - clamp(factor, 0.0, 1.0);
    }
}
