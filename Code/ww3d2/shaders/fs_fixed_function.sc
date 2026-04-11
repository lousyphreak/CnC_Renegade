$input v_color0, v_texcoord0, v_texcoord1, v_specular0, v_fogFactor

#include <bgfx_shader.sh>

SAMPLER2D(s_texColor0, 0);
SAMPLER2D(s_texColor1, 1);

uniform vec4 u_ffpConfig1;
uniform vec4 u_ffpFogColor;
uniform vec4 u_ffpTextureFactor;
uniform vec4 u_ffpStage0Color;
uniform vec4 u_ffpStage0Alpha;
uniform vec4 u_ffpStage1Color;
uniform vec4 u_ffpStage1Alpha;
uniform vec4 u_ffpBumpEnvMatrix;
uniform vec4 u_ffpBumpEnvParams;

vec3 SaturateRgb(vec3 value)
{
    return clamp(value, vec3_splat(0.0), vec3_splat(1.0));
}

float SaturateScalar(float value)
{
    return clamp(value, 0.0, 1.0);
}

bool AlphaTestPasses(float alpha, float func, float reference)
{
    if (func < 0.0) {
        return true;
    }

    float delta = alpha - reference;
    if (func < 0.5) {
        return false;
    }

    if (func < 1.5) {
        return delta < 0.0;
    }

    if (func < 2.5) {
        return abs(delta) <= (0.5 / 255.0);
    }

    if (func < 3.5) {
        return delta <= 0.0;
    }

    if (func < 4.5) {
        return delta > 0.0;
    }

    if (func < 5.5) {
        return abs(delta) > (0.5 / 255.0);
    }

    if (func < 6.5) {
        return delta >= 0.0;
    }

    return true;
}

bool HasFlag(float value, float divisor)
{
    return mod(floor(value / divisor), 2.0) > 0.5;
}

vec4 ResolveArg(float encoded_arg, vec4 diffuse, vec4 current, vec4 texel, vec4 tfactor, vec4 specular, vec4 temp)
{
    float selector = mod(encoded_arg, 16.0);
    vec4 value = current;

    if (selector < 0.5) {
        value = diffuse;
    } else if (selector < 1.5) {
        value = current;
    } else if (selector < 2.5) {
        value = texel;
    } else if (selector < 3.5) {
        value = tfactor;
    } else if (selector < 4.5) {
        value = specular;
    } else if (selector < 5.5) {
        value = temp;
    }

    if (HasFlag(encoded_arg, 32.0)) {
        value = vec4_splat(value.a);
    }

    if (HasFlag(encoded_arg, 16.0)) {
        value = vec4_splat(1.0) - value;
    }

    return value;
}

vec3 ExecuteColorOp(float op, vec4 arg0, vec4 arg1, vec4 arg2, vec4 diffuse, vec4 current, vec4 texel, vec4 tfactor)
{
    if (op < 0.5) {
        return current.rgb;
    }

    if (op < 1.5) {
        return arg1.rgb;
    }

    if (op < 2.5) {
        return arg2.rgb;
    }

    if (op < 3.5) {
        return arg1.rgb * arg2.rgb;
    }

    if (op < 4.5) {
        return SaturateRgb(arg1.rgb * arg2.rgb * 2.0);
    }

    if (op < 5.5) {
        return SaturateRgb(arg1.rgb * arg2.rgb * 4.0);
    }

    if (op < 6.5) {
        return SaturateRgb(arg1.rgb + arg2.rgb);
    }

    if (op < 7.5) {
        return SaturateRgb(arg1.rgb + arg2.rgb - vec3_splat(0.5));
    }

    if (op < 8.5) {
        return SaturateRgb((arg1.rgb + arg2.rgb - vec3_splat(0.5)) * 2.0);
    }

    if (op < 9.5) {
        return SaturateRgb(arg1.rgb - arg2.rgb);
    }

    if (op < 10.5) {
        return SaturateRgb(arg1.rgb + arg2.rgb * (vec3_splat(1.0) - arg1.rgb));
    }

    if (op < 11.5) {
        return SaturateRgb(arg1.rgb * diffuse.a + arg2.rgb * (1.0 - diffuse.a));
    }

    if (op < 12.5) {
        return SaturateRgb(arg1.rgb * texel.a + arg2.rgb * (1.0 - texel.a));
    }

    if (op < 13.5) {
        return SaturateRgb(arg1.rgb * tfactor.a + arg2.rgb * (1.0 - tfactor.a));
    }

    if (op < 14.5) {
        return SaturateRgb(arg1.rgb + arg2.rgb * (1.0 - texel.a));
    }

    if (op < 15.5) {
        return SaturateRgb(arg1.rgb * current.a + arg2.rgb * (1.0 - current.a));
    }

    if (op < 16.5) {
        return SaturateRgb(arg1.rgb * arg2.rgb);
    }

    if (op < 17.5) {
        return SaturateRgb(arg1.rgb * arg1.a + arg2.rgb);
    }

    if (op < 18.5) {
        return SaturateRgb(arg1.rgb + arg2.rgb * arg2.a);
    }

    if (op < 19.5) {
        return SaturateRgb(arg1.rgb * (1.0 - arg1.a) + arg2.rgb);
    }

    if (op < 20.5) {
        return SaturateRgb(arg1.rgb * (vec3_splat(1.0) - arg1.rgb) + arg2.rgb * arg2.a);
    }

    if (op < 23.5) {
        vec3 signed_arg1 = arg1.rgb * 2.0 - 1.0;
        vec3 signed_arg2 = arg2.rgb * 2.0 - 1.0;
        return vec3_splat(SaturateScalar(dot(signed_arg1, signed_arg2)));
    }

    if (op < 24.5) {
        return SaturateRgb(arg0.rgb + arg1.rgb * arg2.rgb);
    }

    if (op < 25.5) {
        return SaturateRgb(arg0.rgb * arg1.rgb + (vec3_splat(1.0) - arg0.rgb) * arg2.rgb);
    }

    return current.rgb;
}

float ExecuteAlphaOp(float op, vec4 arg0, vec4 arg1, vec4 arg2, vec4 diffuse, vec4 current, vec4 texel, vec4 tfactor)
{
    if (op < 0.5) {
        return current.a;
    }

    if (op < 1.5) {
        return arg1.a;
    }

    if (op < 2.5) {
        return arg2.a;
    }

    if (op < 3.5) {
        return SaturateScalar(arg1.a * arg2.a);
    }

    if (op < 4.5) {
        return SaturateScalar(arg1.a * arg2.a * 2.0);
    }

    if (op < 5.5) {
        return SaturateScalar(arg1.a * arg2.a * 4.0);
    }

    if (op < 6.5) {
        return SaturateScalar(arg1.a + arg2.a);
    }

    if (op < 7.5) {
        return SaturateScalar(arg1.a + arg2.a - 0.5);
    }

    if (op < 8.5) {
        return SaturateScalar((arg1.a + arg2.a - 0.5) * 2.0);
    }

    if (op < 9.5) {
        return SaturateScalar(arg1.a - arg2.a);
    }

    if (op < 10.5) {
        return SaturateScalar(arg1.a + arg2.a * (1.0 - arg1.a));
    }

    if (op < 11.5) {
        return SaturateScalar(arg1.a * diffuse.a + arg2.a * (1.0 - diffuse.a));
    }

    if (op < 12.5) {
        return SaturateScalar(arg1.a * texel.a + arg2.a * (1.0 - texel.a));
    }

    if (op < 13.5) {
        return SaturateScalar(arg1.a * tfactor.a + arg2.a * (1.0 - tfactor.a));
    }

    if (op < 14.5) {
        return SaturateScalar(arg1.a + arg2.a * (1.0 - texel.a));
    }

    if (op < 15.5) {
        return SaturateScalar(arg1.a * current.a + arg2.a * (1.0 - current.a));
    }

    if (op < 16.5) {
        return SaturateScalar(arg1.a * arg2.a);
    }

    if (op < 24.5) {
        return SaturateScalar(arg0.a + arg1.a * arg2.a);
    }

    if (op < 25.5) {
        return SaturateScalar(arg0.a * arg1.a + (1.0 - arg0.a) * arg2.a);
    }

    return current.a;
}

vec4 ExecuteStage(vec4 current, vec4 diffuse, vec4 specular, vec4 tfactor, vec4 texel, vec4 other_texel, vec2 other_texcoord, vec4 stage_color, vec4 stage_alpha)
{
    vec4 temp = current;
    vec4 arg0_color = ResolveArg(stage_color.y, diffuse, current, texel, tfactor, specular, temp);
    vec4 arg1_color = ResolveArg(stage_color.z, diffuse, current, texel, tfactor, specular, temp);
    vec4 arg2_color = ResolveArg(stage_color.w, diffuse, current, texel, tfactor, specular, temp);
    vec4 arg0_alpha = ResolveArg(stage_alpha.y, diffuse, current, texel, tfactor, specular, temp);
    vec4 arg1_alpha = ResolveArg(stage_alpha.z, diffuse, current, texel, tfactor, specular, temp);
    vec4 arg2_alpha = ResolveArg(stage_alpha.w, diffuse, current, texel, tfactor, specular, temp);

    vec3 color_result = ExecuteColorOp(stage_color.x, arg0_color, arg1_color, arg2_color, diffuse, current, texel, tfactor);
    if (stage_color.x > 20.5 && stage_color.x < 22.5) {
        vec2 signed_bump = texel.rg * 2.0 - 1.0;
        vec2 bump_offset = vec2(
            dot(u_ffpBumpEnvMatrix.xy, signed_bump),
            dot(u_ffpBumpEnvMatrix.zw, signed_bump));
        vec4 env_texel = texture2D(s_texColor1, other_texcoord + bump_offset);
        color_result = env_texel.rgb;
        if (stage_color.x > 21.5) {
            float luminance = SaturateScalar(texel.a * u_ffpBumpEnvParams.x + u_ffpBumpEnvParams.y);
            color_result *= luminance;
        }
    }

    float alpha_result = ExecuteAlphaOp(stage_alpha.x, arg0_alpha, arg1_alpha, arg2_alpha, diffuse, current, texel, tfactor);
    return vec4(color_result, alpha_result);
}

vec4 ApplyFog(vec4 color, float fogFactor)
{
    if (u_ffpConfig1.z < 0.5) {
        return color;
    }

    float fog_amount = SaturateScalar(fogFactor);
    if (u_ffpConfig1.z < 1.5) {
        return vec4(mix(color.rgb, u_ffpFogColor.rgb, fog_amount), color.a);
    }

    if (u_ffpConfig1.z < 2.5) {
        return vec4(color.rgb * (1.0 - fog_amount), color.a);
    }

    return vec4(mix(color.rgb, vec3_splat(1.0), fog_amount), color.a);
}

void main()
{
    vec4 tex0 = texture2D(s_texColor0, v_texcoord0);
    vec4 tex1 = texture2D(s_texColor1, v_texcoord1);
    vec4 current = v_color0;

    current = ExecuteStage(current, v_color0, v_specular0, u_ffpTextureFactor, tex0, tex1, v_texcoord1, u_ffpStage0Color, u_ffpStage0Alpha);
    current = ExecuteStage(current, v_color0, v_specular0, u_ffpTextureFactor, tex1, tex0, v_texcoord0, u_ffpStage1Color, u_ffpStage1Alpha);

    if (u_ffpConfig1.w > 0.5) {
        current.rgb = SaturateRgb(current.rgb + v_specular0.rgb);
    }

    current = ApplyFog(current, v_fogFactor);

    if (!AlphaTestPasses(current.a, u_ffpConfig1.x, u_ffpConfig1.y)) {
        discard;
    }

    gl_FragColor = current;
}
