#include <bgfx_shader.sh>

SAMPLER2D(s_texColor, 0);

uniform vec4 u_fogState;
uniform vec4 u_fogColor;
uniform vec4 u_colorAdjust;

vec4 sample_base_color(vec2 texcoord, vec4 vertex_color)
{
    return texture2D(s_texColor, texcoord) * vertex_color;
}

vec4 apply_scene_fog(vec4 color, float fog)
{
    if (u_fogState.x > 0.5)
    {
        if (u_fogState.x < 1.5)
        {
            color.rgb = mix(color.rgb, u_fogColor.rgb, fog);
        }
        else if (u_fogState.x < 2.5)
        {
            color *= (1.0 - fog);
        }
        else
        {
            color.rgb = mix(color.rgb, vec3(1.0, 1.0, 1.0), fog);
        }
    }

    return color;
}

vec4 finalize_color(vec4 color)
{
    if (color.a < u_alphaRef)
    {
        discard;
    }

    color.rgb = (color.rgb - 0.5) * u_colorAdjust.z + 0.5;
    color.rgb += vec3(u_colorAdjust.y, u_colorAdjust.y, u_colorAdjust.y);
    color.rgb = clamp(color.rgb, 0.0, 1.0);
    color.rgb = pow(max(color.rgb, vec3(0.0, 0.0, 0.0)), vec3(u_colorAdjust.x, u_colorAdjust.x, u_colorAdjust.x));
    return color;
}
