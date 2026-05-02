// Shared shadow uniform declarations. Included by both the vertex-side
// projection helper (shadow_vs.sh) and the fragment-side sampling helper
// (shadow_common.sh) so the cascade matrices and config map to the same
// bgfx uniform handles regardless of stage.

#ifndef SHADOW_UNIFORMS_SH
#define SHADOW_UNIFORMS_SH

// Force highp on WebGL: large-magnitude world coordinates would otherwise be
// folded down into mediump in the fragment stage and break terrain-sized
// receivers.
uniform highp mat4 u_shadowLightViewProj[3];

// GLSL ES 1.00 (WebGL 1.0) Appendix A forbids dynamic indexing of uniform
// arrays in fragment shaders outside of constant-index-expressions and
// loop-indices. Even on WebGL 2 some drivers downgrade dynamic-indexed
// uniform reads, so fan out into a constant-indexed selector.
highp mat4 GetCascadeViewProj(int cascade)
{
    if (cascade <= 0) {
        return u_shadowLightViewProj[0];
    }
    if (cascade == 1) {
        return u_shadowLightViewProj[1];
    }
    return u_shadowLightViewProj[2];
}

#endif // SHADOW_UNIFORMS_SH
