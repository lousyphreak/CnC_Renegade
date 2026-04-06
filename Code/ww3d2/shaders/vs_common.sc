#include <bgfx_shader.sh>

uniform vec4 u_fogState;
uniform vec4 u_materialSource;
uniform vec4 u_materialAmbient;
uniform vec4 u_materialDiffuse;
uniform vec4 u_materialEmissive;
uniform vec4 u_materialState;
uniform vec4 u_renderAmbient;
uniform vec4 u_lightEnvState;
uniform vec4 u_lightEnvAmbient;
uniform vec4 u_lightEnvDir[4];
uniform vec4 u_lightEnvDiffuse[4];
uniform vec4 u_worldViewRow[3];
uniform vec4 u_texGenState;
uniform vec4 u_textureTransformRow[4];
uniform vec4 u_directLightState;
uniform vec4 u_directLightPosition[4];
uniform vec4 u_directLightDirection[4];
uniform vec4 u_directLightAmbient[4];
uniform vec4 u_directLightDiffuse[4];

vec3 transform_world_view_normal(vec3 normal)
{
    return vec3(
        dot(u_worldViewRow[0].xyz, normal),
        dot(u_worldViewRow[1].xyz, normal),
        dot(u_worldViewRow[2].xyz, normal));
}

vec3 transform_world_view_position(vec3 position)
{
    vec4 position4 = vec4(position, 1.0);
    return vec3(
        dot(u_worldViewRow[0], position4),
        dot(u_worldViewRow[1], position4),
        dot(u_worldViewRow[2], position4));
}

vec3 normalize_safe(vec3 value)
{
    float length_squared = dot(value, value);
    if (length_squared <= 1.0e-12)
    {
        return vec3(0.0, 0.0, 1.0);
    }

    return value * inversesqrt(length_squared);
}

float compute_fog_from_camera_position(vec3 camera_position)
{
    if (u_fogState.x < 0.5)
    {
        return 0.0;
    }

    float fog_distance = max(-camera_position.z, 0.0);
    if (u_fogState.z > 0.0)
    {
        return clamp((fog_distance - u_fogState.y) * u_fogState.z, 0.0, 1.0);
    }

    return fog_distance >= u_fogState.w ? 1.0 : 0.0;
}

vec2 compute_stage0_texcoord_from_camera(vec3 camera_position, vec3 normal, vec2 base_texcoord)
{
    vec4 stage0_input = vec4(base_texcoord, 0.0, 1.0);
    if (u_texGenState.x > 0.5)
    {
        if (u_texGenState.x < 1.5)
        {
            stage0_input = vec4(camera_position, 1.0);
        }
        else if (u_texGenState.w > 0.5)
        {
            if (u_texGenState.x < 2.5)
            {
                stage0_input = vec4(normal, 1.0);
            }
            else
            {
                vec3 to_eye = normalize_safe(-camera_position);
                float normal_dot_eye = dot(normal, to_eye);
                vec3 reflection = 2.0 * normal_dot_eye * normal - to_eye;
                stage0_input = vec4(reflection, 1.0);
            }
        }
        else
        {
            stage0_input = vec4(0.0, 0.0, 0.0, 1.0);
        }
    }

    if (u_texGenState.y > 0.5)
    {
        vec4 transformed;
        transformed.x = dot(u_textureTransformRow[0], stage0_input);
        transformed.y = dot(u_textureTransformRow[1], stage0_input);
        transformed.z = dot(u_textureTransformRow[2], stage0_input);
        transformed.w = dot(u_textureTransformRow[3], stage0_input);
        if (u_texGenState.z > 0.5 && abs(transformed.w) > 1.0e-12)
        {
            return transformed.xy / transformed.w;
        }
        return transformed.xy;
    }

    return stage0_input.xy;
}

vec4 resolve_material_color(vec4 vertex_color)
{
    vec3 diffuse = mix(u_materialDiffuse.rgb, vertex_color.rgb, u_materialSource.y);
    vec3 emissive = mix(u_materialEmissive.rgb, vertex_color.rgb, u_materialSource.z);
    float alpha = mix(u_materialState.x, vertex_color.a, u_materialSource.y);
    return vec4(clamp(diffuse + emissive, 0.0, 1.0), clamp(alpha, 0.0, 1.0));
}

vec4 compute_light_environment_color(vec3 normal, vec4 vertex_color)
{
    vec3 ambient = mix(u_materialAmbient.rgb, vertex_color.rgb, u_materialSource.x);
    vec3 diffuse = mix(u_materialDiffuse.rgb, vertex_color.rgb, u_materialSource.y);
    vec3 emissive = mix(u_materialEmissive.rgb, vertex_color.rgb, u_materialSource.z);
    float alpha = mix(u_materialState.x, vertex_color.a, u_materialSource.y);
    vec3 lit = emissive;
    lit += ambient * u_renderAmbient.rgb;
    lit += ambient * u_lightEnvAmbient.rgb;
    for (int light_index = 0; light_index < 4; ++light_index)
    {
        if (float(light_index) >= u_lightEnvState.x)
        {
            break;
        }
        float ndotl = max(dot(normal, u_lightEnvDir[light_index].xyz), 0.0);
        lit += diffuse * (u_lightEnvDiffuse[light_index].rgb * ndotl);
    }
    return vec4(clamp(lit, 0.0, 1.0), clamp(alpha, 0.0, 1.0));
}

vec4 compute_direct_light_color(vec3 camera_position, vec3 normal, vec4 vertex_color)
{
    vec3 ambient = mix(u_materialAmbient.rgb, vertex_color.rgb, u_materialSource.x);
    vec3 diffuse = mix(u_materialDiffuse.rgb, vertex_color.rgb, u_materialSource.y);
    vec3 emissive = mix(u_materialEmissive.rgb, vertex_color.rgb, u_materialSource.z);
    float alpha = mix(u_materialState.x, vertex_color.a, u_materialSource.y);
    vec3 lit = emissive + ambient * u_renderAmbient.rgb;

    for (int light_index = 0; light_index < 4; ++light_index)
    {
        if (float(light_index) >= u_directLightState.x)
        {
            break;
        }

        float light_type = u_directLightAmbient[light_index].w;
        vec3 light_direction = normalize_safe(u_directLightDirection[light_index].xyz);
        float attenuation = 1.0;

        if (light_type > 0.5)
        {
            vec3 to_light = u_directLightPosition[light_index].xyz - camera_position;
            float distance = length(to_light);
            light_direction = distance > 1.0e-6 ? to_light / distance : vec3(0.0, 0.0, 1.0);

            float atten_start = u_directLightPosition[light_index].w;
            float atten_end = u_directLightDirection[light_index].w;
            float atten_range = atten_end - atten_start;
            if (atten_range > 1.0e-6)
            {
                attenuation = clamp(1.0 - (distance - atten_start) / atten_range, 0.0, 1.0);
            }

            if (light_type > 1.5 && attenuation > 0.0)
            {
                vec3 to_object = distance > 1.0e-6 ? -light_direction : vec3(0.0, 0.0, 1.0);
                float spot_angle_cos = u_directLightDiffuse[light_index].w;
                float cone = dot(normalize_safe(u_directLightDirection[light_index].xyz), to_object);
                if (cone <= spot_angle_cos)
                {
                    attenuation = 0.0;
                }
                else
                {
                    attenuation *= clamp((cone - spot_angle_cos) / max(1.0 - spot_angle_cos, 1.0e-6), 0.0, 1.0);
                }
            }
        }

        float ndotl = max(dot(normal, light_direction), 0.0);
        lit += ambient * (u_directLightAmbient[light_index].rgb * attenuation);
        lit += diffuse * (u_directLightDiffuse[light_index].rgb * attenuation * ndotl);
    }

    return vec4(clamp(lit, 0.0, 1.0), clamp(alpha, 0.0, 1.0));
}
