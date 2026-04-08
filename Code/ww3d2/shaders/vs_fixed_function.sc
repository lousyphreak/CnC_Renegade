$input a_position, a_normal, a_color0, a_color1, a_texcoord0, a_texcoord1
$output v_color0, v_texcoord0, v_texcoord1, v_specular0

#include <bgfx_shader.sh>

uniform vec4 u_ffpMaterialAmbient;
uniform vec4 u_ffpMaterialDiffuse;
uniform vec4 u_ffpMaterialEmissive;
uniform vec4 u_ffpSceneAmbient;
uniform vec4 u_ffpLightingConfig;
uniform vec4 u_ffpMaterialSourceConfig;
uniform vec4 u_ffpLightDirections[4];
uniform vec4 u_ffpLightDiffuse[4];

vec4 ResolveColorSource(float source, vec4 materialColor, vec4 color0, vec4 color1)
{
    if (source < 0.5) {
        return materialColor;
    }

    if (source < 1.5) {
        return color0;
    }

    return color1;
}

void main()
{
    gl_Position = mul(u_modelViewProj, vec4(a_position, 1.0));
    vec4 diffuse = ResolveColorSource(u_ffpLightingConfig.z, u_ffpMaterialDiffuse, a_color0, a_color1);

    if (u_ffpLightingConfig.x > 0.5) {
        if (u_ffpLightingConfig.y <= 0.5) {
            // Prelit meshes can request lighting while omitting normals. D3D fixed-function
            // uses the vertex diffuse color directly in that case instead of material sources.
            v_color0 = a_color0;
            v_texcoord0 = a_texcoord0;
            v_texcoord1 = a_texcoord1;
            v_specular0 = a_color1;
            return;
        }

        vec4 ambient = ResolveColorSource(u_ffpLightingConfig.w, u_ffpMaterialAmbient, a_color0, a_color1);
        vec4 emissive = ResolveColorSource(u_ffpMaterialSourceConfig.x, u_ffpMaterialEmissive, a_color0, a_color1);
        vec3 litColor = emissive.rgb + (u_ffpSceneAmbient.rgb * ambient.rgb);

        vec3 worldNormal = normalize(mul(u_model[0], vec4(a_normal, 0.0)).xyz);
        for (int lightIndex = 0; lightIndex < 4; ++lightIndex) {
            if (u_ffpLightDiffuse[lightIndex].a > 0.0) {
                float ndotl = max(dot(worldNormal, -u_ffpLightDirections[lightIndex].xyz), 0.0);
                litColor += u_ffpLightDiffuse[lightIndex].rgb * diffuse.rgb * ndotl;
            }
        }

        v_color0 = vec4(clamp(litColor, vec3_splat(0.0), vec3_splat(1.0)), diffuse.a);
    } else {
        // D3D fixed-function ignores material-source selection when lighting is disabled
        // and forwards the prelit vertex diffuse color instead.
        v_color0 = a_color0;
    }
    v_texcoord0 = a_texcoord0;
    v_texcoord1 = a_texcoord1;
    v_specular0 = a_color1;
}
