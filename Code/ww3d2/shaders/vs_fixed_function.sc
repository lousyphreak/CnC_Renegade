$input a_position, a_normal, a_color0, a_color1, a_texcoord0, a_texcoord1
$output v_color0, v_texcoord0, v_texcoord1, v_specular0

#include <bgfx_shader.sh>

uniform vec4 u_ffpMaterialAmbient;
uniform vec4 u_ffpMaterialDiffuse;
uniform vec4 u_ffpMaterialSpecular;
uniform vec4 u_ffpMaterialEmissive;
uniform vec4 u_ffpMaterialParams;
uniform vec4 u_ffpSceneAmbient;
uniform vec4 u_ffpLightingConfig;
uniform vec4 u_ffpMaterialSourceConfig;
uniform vec4 u_ffpLightPositions[4];
uniform vec4 u_ffpLightDirections[4];
uniform vec4 u_ffpLightAmbient[4];
uniform vec4 u_ffpLightDiffuse[4];
uniform vec4 u_ffpLightSpecular[4];
uniform vec4 u_ffpLightAttenuation[4];
uniform vec4 u_ffpLightSpotParams[4];
uniform vec4 u_ffpCameraPosition;

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
        vec4 specular = ResolveColorSource(u_ffpMaterialSourceConfig.y, u_ffpMaterialSpecular, a_color0, a_color1);
        vec4 emissive = ResolveColorSource(u_ffpMaterialSourceConfig.x, u_ffpMaterialEmissive, a_color0, a_color1);
        vec3 litColor = emissive.rgb + (u_ffpSceneAmbient.rgb * ambient.rgb);
        vec3 specularColor = a_color1.rgb;
        vec3 worldPosition = mul(u_model[0], vec4(a_position, 1.0)).xyz;

        vec3 worldNormal = normalize(mul(u_model[0], vec4(a_normal, 0.0)).xyz);
        for (int lightIndex = 0; lightIndex < 4; ++lightIndex) {
            float lightType = u_ffpLightPositions[lightIndex].w;
            if (lightType > 0.5) {
                vec3 lightVector = -normalize(u_ffpLightDirections[lightIndex].xyz);
                float attenuation = 1.0;

                if (lightType < 2.5) {
                    vec3 toLight = u_ffpLightPositions[lightIndex].xyz - worldPosition;
                    float distanceToLight = length(toLight);
                    if (distanceToLight > 1.0e-6) {
                        lightVector = toLight / distanceToLight;
                    }

                    if (u_ffpLightAttenuation[lightIndex].w > 0.0 && distanceToLight > u_ffpLightAttenuation[lightIndex].w) {
                        attenuation = 0.0;
                    } else {
                        float attenuationDenominator =
                            u_ffpLightAttenuation[lightIndex].x +
                            u_ffpLightAttenuation[lightIndex].y * distanceToLight +
                            u_ffpLightAttenuation[lightIndex].z * distanceToLight * distanceToLight;
                        attenuation = 1.0 / max(attenuationDenominator, 1.0e-4);
                    }

                    if (lightType > 1.5) {
                        float spotDot = dot(-lightVector, normalize(u_ffpLightDirections[lightIndex].xyz));
                        float spotCos = u_ffpLightSpotParams[lightIndex].x;
                        float cone = clamp((spotDot - spotCos) / max(1.0 - spotCos, 1.0e-4), 0.0, 1.0);
                        float falloff = max(u_ffpLightSpotParams[lightIndex].y, 0.0);
                        attenuation *= falloff > 0.0 ? pow(cone, falloff) : cone;
                    }
                }

                float ndotl = max(dot(worldNormal, lightVector), 0.0);
                litColor += u_ffpLightAmbient[lightIndex].rgb * ambient.rgb * attenuation;
                litColor += u_ffpLightDiffuse[lightIndex].rgb * diffuse.rgb * ndotl * attenuation;

                if (ndotl > 0.0) {
                    vec3 viewVector = normalize(u_ffpCameraPosition.xyz - worldPosition);
                    vec3 halfVector = normalize(lightVector + viewVector);
                    float specularFactor = pow(max(dot(worldNormal, halfVector), 0.0), max(u_ffpMaterialParams.x, 1.0));
                    specularColor += u_ffpLightSpecular[lightIndex].rgb * specular.rgb * specularFactor * attenuation;
                }
            }
        }

        v_color0 = vec4(clamp(litColor, vec3_splat(0.0), vec3_splat(1.0)), diffuse.a);
        v_specular0 = vec4(clamp(specularColor, vec3_splat(0.0), vec3_splat(1.0)), a_color1.a);
    } else {
        // D3D fixed-function ignores material-source selection when lighting is disabled
        // and forwards the prelit vertex diffuse color instead.
        v_color0 = a_color0;
        v_specular0 = a_color1;
    }
    v_texcoord0 = a_texcoord0;
    v_texcoord1 = a_texcoord1;
}
