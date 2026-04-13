$input v_color0, v_texcoord0, v_texcoord1, v_specular0, v_fogFactor

#include <bgfx_shader.sh>

SAMPLER2D(s_texColor0, 0);
SAMPLER2D(s_texColor1, 1);

// u_meshFragConfig.x = stage0ColorOp (enum StageColorOp)
// u_meshFragConfig.y = stage1ColorOp (enum StageColorOp)
// u_meshFragConfig.z = alphaTestRef (-1 = disabled, otherwise 0..1 ref value)
// u_meshFragConfig.w = stage0AlphaOp (enum StageAlphaOp)
uniform vec4 u_meshFragConfig;

// u_meshFragConfig2.x = stage1AlphaOp
// u_meshFragConfig2.y = fogMode: 0=none, 1=enabled, 2=scale_fragment, 3=white
// u_meshFragConfig2.z = specularEnabled (0 or 1)
// u_meshFragConfig2.w = unused
uniform vec4 u_meshFragConfig2;

uniform vec4 u_meshFogColor;

// Stage color op enum values (must match C++ StageColorOp):
// 0 = DISABLE
// 1 = MODULATE       (tex * current)
// 2 = SELECT_TEXTURE (tex only)
// 3 = SELECT_CURRENT (pass through)
// 4 = ADD            (tex + current)
// 5 = ADDSMOOTH      (current + tex * (1 - current))
// 6 = SUBTRACT       (current - tex)
// 7 = BLEND_TEX_ALPHA (lerp(current, tex, tex.a))
// 8 = BLEND_CUR_ALPHA (lerp(current, tex, current.a))

vec3 ApplyColorOp(float op, vec4 current, vec4 texel)
{
    if (op < 0.5) {
        // DISABLE: pass through
        return current.rgb;
    }
    if (op < 1.5) {
        // MODULATE
        return current.rgb * texel.rgb;
    }
    if (op < 2.5) {
        // SELECT_TEXTURE
        return texel.rgb;
    }
    if (op < 3.5) {
        // SELECT_CURRENT
        return current.rgb;
    }
    if (op < 4.5) {
        // ADD
        return clamp(current.rgb + texel.rgb, vec3_splat(0.0), vec3_splat(1.0));
    }
    if (op < 5.5) {
        // ADDSMOOTH
        return clamp(current.rgb + texel.rgb * (vec3_splat(1.0) - current.rgb), vec3_splat(0.0), vec3_splat(1.0));
    }
    if (op < 6.5) {
        // SUBTRACT
        return clamp(current.rgb - texel.rgb, vec3_splat(0.0), vec3_splat(1.0));
    }
    if (op < 7.5) {
        // BLEND_TEX_ALPHA
        return mix(current.rgb, texel.rgb, texel.a);
    }
    if (op < 8.5) {
        // BLEND_CUR_ALPHA
        return mix(current.rgb, texel.rgb, current.a);
    }
    return current.rgb;
}

float ApplyAlphaOp(float op, float currentAlpha, vec4 texel)
{
    if (op < 0.5) {
        // DISABLE: pass through
        return currentAlpha;
    }
    if (op < 1.5) {
        // MODULATE
        return currentAlpha * texel.a;
    }
    if (op < 2.5) {
        // SELECT_TEXTURE
        return texel.a;
    }
    if (op < 3.5) {
        // SELECT_CURRENT
        return currentAlpha;
    }
    if (op < 4.5) {
        // ADD
        return clamp(currentAlpha + texel.a, 0.0, 1.0);
    }
    if (op < 5.5) {
        // ADDSMOOTH
        return clamp(currentAlpha + texel.a * (1.0 - currentAlpha), 0.0, 1.0);
    }
    return currentAlpha;
}

void main()
{
    vec4 tex0 = texture2D(s_texColor0, v_texcoord0);
    vec4 tex1 = texture2D(s_texColor1, v_texcoord1);
    vec4 current = v_color0;

    // Stage 0
    current = vec4(
        ApplyColorOp(u_meshFragConfig.x, current, tex0),
        ApplyAlphaOp(u_meshFragConfig.w, current.a, tex0));

    // Stage 1
    current = vec4(
        ApplyColorOp(u_meshFragConfig.y, current, tex1),
        ApplyAlphaOp(u_meshFragConfig2.x, current.a, tex1));

    // Specular add
    if (u_meshFragConfig2.z > 0.5) {
        current.rgb = clamp(current.rgb + v_specular0.rgb, vec3_splat(0.0), vec3_splat(1.0));
    }

    // Fog
    float fogMode = u_meshFragConfig2.y;
    if (fogMode > 0.5) {
        float fog_amount = clamp(v_fogFactor, 0.0, 1.0);
        if (fogMode < 1.5) {
            // FOG_ENABLE: blend to fog color
            current.rgb = mix(current.rgb, u_meshFogColor.rgb, fog_amount);
        } else if (fogMode < 2.5) {
            // FOG_SCALE_FRAGMENT: darken by fog
            current.rgb = current.rgb * (1.0 - fog_amount);
        } else {
            // FOG_WHITE: blend to white
            current.rgb = mix(current.rgb, vec3_splat(1.0), fog_amount);
        }
    }

    // Alpha test (greaterequal only, the only mode actually used)
    if (u_meshFragConfig.z >= 0.0) {
        if (current.a < u_meshFragConfig.z) {
            discard;
        }
    }

    gl_FragColor = current;
}
