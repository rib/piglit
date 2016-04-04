/*
 * Copyright © 2016 Intel Corporation
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice (including the next
 * paragraph) shall be included in all copies or substantial portions of the
 * Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 */

/** @file layout-std140-fp64-shader.c
 *
 * Tests that shader storage block reads/writes in GLSL works correctly (offsets
 * and values) when interface packing qualifier is std140 and row_major, using
 * doubles.
 */

#include "piglit-util-gl.h"

PIGLIT_GL_TEST_CONFIG_BEGIN
	config.window_width = 100;
	config.window_height = 100;
	config.supports_gl_compat_version = 32;
	config.supports_gl_core_version = 32;
	config.window_visual = PIGLIT_GL_VISUAL_DOUBLE | PIGLIT_GL_VISUAL_RGBA;

PIGLIT_GL_TEST_CONFIG_END


#define XSTR(S) STR(S)
#define STR(S) #S

#define SSBO_SIZE 64
#define TOLERANCE 1e-5
#define DIFFER(a,b) ((a > b ? a - b : b - a) > TOLERANCE)

static const char vs_code[] =
	"#version 150\n"
	"#extension GL_ARB_shader_storage_buffer_object : require\n"
        "#extension GL_ARB_gpu_shader_fp64 : require\n"
        "#extension GL_ARB_vertex_attrib_64bit : require\n"
	"\n"
	"layout(std430, row_major, binding=2) buffer ssbo {\n"
        "       double u[" XSTR(SSBO_SIZE) "/4];\n"
	"};\n"
        "in vec4 vertex;\n"
	"in dmat4 value;\n"
	"void main() {\n"
	"	gl_Position = vec4(vertex);\n"
	"       u[0] = value[0].x;\n"
        "       u[1] = value[0].y;\n"
        "       u[2] = value[0].z;\n"
        "       u[3] = value[0].w;\n"
        "       u[4] = value[1].x;\n"
        "       u[5] = value[1].y;\n"
        "       u[6] = value[1].z;\n"
        "       u[7] = value[1].w;\n"
        "       u[8] = value[2].x;\n"
        "       u[9] = value[2].y;\n"
        "       u[10] = value[2].z;\n"
        "       u[11] = value[2].w;\n"
        "       u[12] = value[3].x;\n"
        "       u[13] = value[3].y;\n"
        "       u[14] = value[3].z;\n"
        "       u[15] = value[3].w;\n"
        "}\n";

static const char fs_source[] =
	"#version 150\n"
	"#extension GL_ARB_shader_storage_buffer_object : require\n"
        "#extension GL_ARB_gpu_shader_fp64 : require\n"
        "#extension GL_ARB_vertex_attrib_64bit : require\n"
	"out vec4 color;\n"
	"void main() {\n"
	"       color = vec4(0.0, 1.0, 0.0, 1.0);\n"
	"}\n";

GLuint prog;

double ssbo_values[SSBO_SIZE] = { 0.0 };

static const float verts[] = {
  6.776277770192E-21, 0.0, 0.0, 0.0,
  0.0, 0.0, 0.0, 0.0,
};

void
piglit_init(int argc, char **argv)
{
	GLuint buffer;

	piglit_require_extension("GL_ARB_shader_storage_buffer_object");
	piglit_require_extension("GL_ARB_gpu_shader_fp64");
	piglit_require_extension("GL_ARB_vertex_attrib_64bit");
        piglit_require_GLSL_version(150);

	prog = piglit_build_simple_program_multiple_shaders(GL_VERTEX_SHADER, vs_code,
                                                            GL_FRAGMENT_SHADER, fs_source,
                                                            NULL);

	glUseProgram(prog);

	glClearColor(0, 0, 0, 0);

	glGenBuffers(1, &buffer);
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 2, buffer);

	glBufferData(GL_SHADER_STORAGE_BUFFER, SSBO_SIZE*sizeof(double),
				&ssbo_values[0], GL_DYNAMIC_DRAW);

        glViewport(0, 0, piglit_width, piglit_height);


	piglit_check_gl_error(GL_NO_ERROR);
}

static const char *float_to_hex(float f)
{
  union {
    float f;
    unsigned int i;
  } b;

  b.f = f;
  char *s = (char *) malloc(100);
  sprintf(s, "0x%08X", b.i);
  return s;
}

static const char *double_to_hex(double d)
{
  union {
    double d;
    unsigned int i[2];
  } b;

  b.d = d;
  char *s = (char *) malloc(100);
  sprintf(s, "0x%08X%08x", b.i[1], b.i[0]);
  return s;
}

enum piglit_result piglit_display(void)
{
  GLuint vao, vbo;

        glClear(GL_COLOR_BUFFER_BIT);
        glGenVertexArrays(1, &vao);
        glBindVertexArray(vao);
        glGenBuffers(1, &vbo);
        glBindBuffer(GL_ARRAY_BUFFER, vbo);
        glBufferData(GL_ARRAY_BUFFER, sizeof(verts), verts, GL_STATIC_DRAW);

  GLint attrib_location = glGetAttribLocation(prog, "value");
  double v1=3.50000000000727684579260312603E02; //0x4075E00000003202
  double v2=5.00000000000047073456244106637E00; //0x4014000000000212
  double v3=9.32200000000000045474735088646E02; //0x408D21999999999A
  double v4=3.23200000000000006381006834033E-3; //0x3F6A79FEC99F1AE3
  double v5=8.28100000000000058264504332328E0; //0x40208FDF3B645A1D
  double v6=1.02230000000000007531752999057E1; //0x4024722D0E560419
  double v7=1.20223110000000005470610631164E2; //0x405E0E476F2A5A47
  double v8=1.02221000000000003637978807092E2; //0x40598E24DD2F1AA0
  double v9=2.02200899999999990086507750675E2; //0x4069466DC5D63886
  double v10=2.12205900000000013960743672214E2; //0x406A8696BB98C7E3
  double v11=1.21999999999999992894572642399E1; //0x4028666666666666
  double v12=1.92520000000000010231815394945E2; //0x406810A3D70A3D71
  double v13=1.93252099999999995816324371845E3; //0x409E3215810624DD
  double v14=1.92125210000000006402842700481E4; //0x40D2C3215810624E
  double v15=1.33125210000000006402842700481E4; //0x40CA0042B020C49C
  double v16=3.31259000000000014551915228367E3; //0x40A9E12E147AE148

  glVertexAttribL4d(attrib_location + 0, v1, v2, v3, v4);
  glVertexAttribL4d(attrib_location + 1, v5, v6, v7, v8);
  glVertexAttribL4d(attrib_location + 2, v9, v10, v11, v12);
  glVertexAttribL4d(attrib_location + 3, v13, v14, v15, v16);


        attrib_location = glGetAttribLocation(prog, "vertex");

        glVertexAttribPointer(attrib_location, 4, GL_FLOAT, GL_FALSE, 0, (const GLvoid  *)0);
        glEnableVertexAttribArray(attrib_location);
        glDrawArrays(GL_LINES, 0, 2);

        double *map = (double *) glMapBuffer(GL_SHADER_STORAGE_BUFFER,  GL_READ_ONLY);
        int i;
	for (i = 0; i < SSBO_SIZE; i+=4) {
          printf("read[%d] = (%.14g [%s],  %.14g [%s],  %.14g [%s],  %.14g [%s])\n",
                 i,
                 map[i], double_to_hex(map[i]),
                 map[i+1], double_to_hex(map[i+1]),
                 map[i+2], double_to_hex(map[i+2]),
                 map[i+3], double_to_hex(map[i+3]));
        }

  return PIGLIT_PASS;
}
