$input a_position, a_normal, a_color0, a_color1, a_texcoord0, a_texcoord1
$output v_color0, v_texcoord0, v_texcoord1, v_fogFactor, v_worldPos, v_viewDepth, v_worldNormal

#include <bgfx_shader.sh>

// u_meshFogConfig.x = fogEnabled (0 or 1)
// u_meshFogConfig.y = fogStart
// u_meshFogConfig.z = fogEnd
// u_meshFogConfig.w = rangeFogSign (1.0 = range, -1.0 = planar)
uniform vec4 u_meshFogConfig;

// u_meshLitConfig.x = hasNormals (0 or 1)
// u_meshLitConfig.y = diffuseSource (0=material, 1=color0)
// u_meshLitConfig.z = ambientSource (0=material, 1=color0)
// u_meshLitConfig.w = emissiveSource (0=material, 1=color0)
uniform vec4 u_meshLitConfig;

uniform vec4 u_meshMaterialAmbient;
uniform vec4 u_meshMaterialDiffuse;
uniform vec4 u_meshMaterialEmissive;
uniform vec4 u_meshSceneAmbient;

// 4 directional lights: direction.xyz, enabled in w
uniform vec4 u_meshLightDir[4];
// 4 directional lights: color.rgb, unused w
uniform vec4 u_meshLightColor[4];

vec4 ResolveColorSource(float source, vec4 materialColor, vec4 vertexColor)
{
    return source > 0.5 ? vertexColor : materialColor;
}

void main()
{
    gl_Position = mul(u_modelViewProj, vec4(a_position, 1.0));

    vec3 worldNormal = normalize(mul(u_model[0], vec4(a_normal, 0.0)).xyz);

    // Resolve material colors
    vec4 diffuse = ResolveColorSource(u_meshLitConfig.y, u_meshMaterialDiffuse, a_color0);

    if (u_meshLitConfig.x <= 0.5) {
        // No normals: only emissive contributes (D3D8 behavior)
        vec4 emissive = ResolveColorSource(u_meshLitConfig.w, u_meshMaterialEmissive, a_color0);
        v_color0 = vec4(clamp(emissive.rgb, vec3_splat(0.0), vec3_splat(1.0)), diffuse.a);
    } else {
        // Full directional lighting
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

    v_texcoord0 = a_texcoord0;
    v_texcoord1 = a_texcoord1;

    v_worldPos = mul(u_model[0], vec4(a_position, 1.0)).xyz;
    v_worldNormal = worldNormal;
    vec3 viewPos = mul(u_modelView, vec4(a_position, 1.0)).xyz;
    v_viewDepth = -viewPos.z;

    // Fog
    v_fogFactor = 0.0;
    if (u_meshFogConfig.x > 0.5) {
        float dist = u_meshFogConfig.w > 0.0 ? length(viewPos) : abs(viewPos.z);
        float factor = (u_meshFogConfig.z - dist) / max(u_meshFogConfig.z - u_meshFogConfig.y, 1.0e-4);
        v_fogFactor = 1.0 - clamp(factor, 0.0, 1.0);
    }
}
