#pragma once

#if defined(SIMP_GLES)
#define SIMP_GLSL_VERT "#version 300 es\nprecision highp float;\nprecision highp int;\n"
#define SIMP_GLSL_FRAG "#version 300 es\nprecision highp float;\nprecision highp int;\nprecision highp sampler2D;\n"
#else
#define SIMP_GLSL_VERT "#version 400 core\n"
#define SIMP_GLSL_FRAG "#version 400 core\n"
#endif
