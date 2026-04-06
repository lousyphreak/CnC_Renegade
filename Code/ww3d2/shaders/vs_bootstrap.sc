$input a_position, a_normal, a_color0, a_texcoord0
$output v_color0, v_texcoord0, v_fog

#include <bgfx_shader.sh>

uniform vec4 u_fogState;
uniform vec4 u_lightingState;
uniform vec4 u_materialSource;
uniform vec4 u_materialAmbient;
uniform vec4 u_materialDiffuse;
uniform vec4 u_materialEmissive;
uniform vec4 u_renderAmbient;
uniform vec4 u_lightEnvAmbient;
uniform vec4 u_lightEnvDir[4];
uniform vec4 u_lightEnvDiffuse[4];
uniform vec4 u_worldViewRow[3];
uniform vec4 u_texGenState;
uniform vec4 u_textureTransformRow[4];

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

float compute_fog(vec3 position)
{
    if (u_fogState.x < 0.5)
    {
        return 0.0;
    }

    float view_z = transform_world_view_position(position).z;
    float fog_distance = max(-view_z, 0.0);
    if (u_fogState.z > 0.0)
    {
        return clamp((fog_distance - u_fogState.y) * u_fogState.z, 0.0, 1.0);
    }

    return fog_distance >= u_fogState.w ? 1.0 : 0.0;
}

vec2 compute_stage0_texcoord(vec3 position, vec3 normal, vec2 base_texcoord)
{
    vec4 stage0_input = vec4(base_texcoord, 0.0, 1.0);
    if (u_texGenState.x > 0.5)
    {
        vec3 camera_position = transform_world_view_position(position);
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

void main()
{
    vec4 object_position = vec4(a_position, 1.0);
    gl_Position = mul(u_modelViewProj, object_position);
    vec3 normal = normalize_safe(transform_world_view_normal(a_normal));

    vec4 color = a_color0;
    if (u_lightingState.x > 0.5)
    {
        vec3 ambient = u_materialAmbient.rgb + (a_color0.rgb - u_materialAmbient.rgb) * u_materialSource.x;
        vec3 diffuse = u_materialDiffuse.rgb + (a_color0.rgb - u_materialDiffuse.rgb) * u_materialSource.y;
        vec3 emissive = u_materialEmissive.rgb + (a_color0.rgb - u_materialEmissive.rgb) * u_materialSource.z;
        float alpha = u_lightingState.z + (a_color0.a - u_lightingState.z) * u_materialSource.y;
        vec3 lit = emissive;
        lit += ambient * u_renderAmbient.rgb;
        lit += ambient * u_lightEnvAmbient.rgb;
        for (int light_index = 0; light_index < 4; ++light_index)
        {
            if (float(light_index) < u_lightingState.y)
            {
                float ndotl = max(dot(normal, u_lightEnvDir[light_index].xyz), 0.0);
                lit += diffuse * (u_lightEnvDiffuse[light_index].rgb * ndotl);
            }
        }
        color = vec4(clamp(lit, 0.0, 1.0), clamp(alpha, 0.0, 1.0));
    }

    v_color0 = color;
    v_texcoord0 = compute_stage0_texcoord(a_position, normal, a_texcoord0);
    v_fog = compute_fog(a_position);
}
