$input a_position, a_normal, a_color0, a_color1, a_texcoord0, a_texcoord1
$output v_color0, v_texcoord0, v_texcoord1, v_specular0, v_fogFactor

#include <bgfx_shader.sh>

uniform vec4 u_ffpMaterialAmbient;
uniform vec4 u_ffpMaterialDiffuse;
uniform vec4 u_ffpMaterialSpecular;
uniform vec4 u_ffpMaterialEmissive;
uniform vec4 u_ffpMaterialParams;
uniform vec4 u_ffpSceneAmbient;
uniform vec4 u_ffpLightingConfig;
uniform vec4 u_ffpMaterialSourceConfig;
uniform vec4 u_ffpTexcoordConfig[2];
uniform vec4 u_ffpTextureMatrix[8];
uniform vec4 u_ffpLightPositions[4];
uniform vec4 u_ffpLightDirections[4];
uniform vec4 u_ffpLightAmbient[4];
uniform vec4 u_ffpLightDiffuse[4];
uniform vec4 u_ffpLightSpecular[4];
uniform vec4 u_ffpLightAttenuation[4];
uniform vec4 u_ffpLightSpotParams[4];
uniform vec4 u_ffpViewer;
uniform vec4 u_ffpFogParams;

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

vec4 ResolvePassthroughTexcoord(float sourceSet, vec2 texcoord0, vec2 texcoord1)
{
    return sourceSet > 0.5 ? vec4(texcoord1, 0.0, 1.0) : vec4(texcoord0, 0.0, 1.0);
}

vec4 MultiplyTextureMatrix(int stage, vec4 coordinate)
{
    int rowOffset = stage * 4;
    return vec4(
        dot(u_ffpTextureMatrix[rowOffset + 0], coordinate),
        dot(u_ffpTextureMatrix[rowOffset + 1], coordinate),
        dot(u_ffpTextureMatrix[rowOffset + 2], coordinate),
        dot(u_ffpTextureMatrix[rowOffset + 3], coordinate));
}

float ResolveTextureCoordCount(float transformFlags)
{
    float count = mod(transformFlags, 256.0);
    if (count >= 1.0 && count <= 4.0) {
        return count;
    }

    return 2.0;
}

vec2 ResolveStageTexcoord(
    int stage,
    vec2 texcoord0,
    vec2 texcoord1,
    vec3 viewPosition,
    vec3 viewNormal)
{
    float texcoordIndex = u_ffpTexcoordConfig[stage].x;
    float transformFlags = u_ffpTexcoordConfig[stage].y;
    float texcoordMode = floor(texcoordIndex / 65536.0 + 0.5);
    float sourceSet = texcoordIndex - texcoordMode * 65536.0;
    vec4 coordinate = ResolvePassthroughTexcoord(sourceSet, texcoord0, texcoord1);

    if (texcoordMode > 0.5) {
        if (texcoordMode < 1.5) {
            coordinate = vec4(viewNormal, 1.0);
        } else if (texcoordMode < 2.5) {
            coordinate = vec4(viewPosition, 1.0);
        } else {
            vec3 eyeVector = normalize(-viewPosition);
            vec3 reflection = viewNormal * (2.0 * dot(viewNormal, eyeVector)) - eyeVector;
            coordinate = vec4(reflection, 1.0);
        }
    }

    if (transformFlags > 0.5) {
        coordinate = MultiplyTextureMatrix(stage, coordinate);

        if (mod(floor(transformFlags / 256.0), 2.0) > 0.5) {
            float coordCount = ResolveTextureCoordCount(transformFlags);
            float q = coordCount < 2.5
                ? coordinate.y
                : (coordCount < 3.5 ? coordinate.z : coordinate.w);
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
    float fogMode = abs(u_ffpFogParams.w);
    v_fogFactor = 0.0;

    if (fogMode > 0.5) {
        float fogDistance = u_ffpFogParams.w < 0.0 ? length(viewPosition) : abs(viewPosition.z);
        float fogFactor = 1.0;

        if (fogMode < 1.5) {
            fogFactor = exp(-max(u_ffpFogParams.z, 0.0) * fogDistance);
        } else if (fogMode < 2.5) {
            float fogDensityDistance = max(u_ffpFogParams.z, 0.0) * fogDistance;
            fogFactor = exp(-(fogDensityDistance * fogDensityDistance));
        } else {
            fogFactor = (u_ffpFogParams.y - fogDistance) / max(u_ffpFogParams.y - u_ffpFogParams.x, 1.0e-4);
        }

        v_fogFactor = 1.0 - clamp(fogFactor, 0.0, 1.0);
    }

    vec4 diffuse = ResolveColorSource(u_ffpLightingConfig.z, u_ffpMaterialDiffuse, a_color0, a_color1);

    if (u_ffpLightingConfig.x > 0.5) {
        if (u_ffpLightingConfig.y <= 0.5) {
            // Prelit meshes can request lighting while omitting normals. D3D fixed-function
            // uses the vertex diffuse color directly in that case instead of material sources.
            v_color0 = a_color0;
            v_texcoord0 = ResolveStageTexcoord(0, a_texcoord0, a_texcoord1, viewPosition, viewNormal);
            v_texcoord1 = ResolveStageTexcoord(1, a_texcoord0, a_texcoord1, viewPosition, viewNormal);
            v_specular0 = a_color1;
            return;
        }

        vec4 ambient = ResolveColorSource(u_ffpLightingConfig.w, u_ffpMaterialAmbient, a_color0, a_color1);
        vec4 specular = ResolveColorSource(u_ffpMaterialSourceConfig.y, u_ffpMaterialSpecular, a_color0, a_color1);
        vec4 emissive = ResolveColorSource(u_ffpMaterialSourceConfig.x, u_ffpMaterialEmissive, a_color0, a_color1);
        vec3 litColor = emissive.rgb + (u_ffpSceneAmbient.rgb * ambient.rgb);
        vec3 specularColor = a_color1.rgb;

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
                    vec3 viewVector = u_ffpViewer.w > 0.5
                        ? normalize(u_ffpViewer.xyz - worldPosition)
                        : normalize(u_ffpViewer.xyz);
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
    v_texcoord0 = ResolveStageTexcoord(0, a_texcoord0, a_texcoord1, viewPosition, viewNormal);
    v_texcoord1 = ResolveStageTexcoord(1, a_texcoord0, a_texcoord1, viewPosition, viewNormal);
}
