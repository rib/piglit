/*
 * Copyright © 2014 Ilia Mirkin
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
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
 * IN THE SOFTWARE.
 *
 * Author: Ilia Mirkin <imirkin@alum.mit.edu>
 */

/**
 * \file sampling-2d-array-as-cubemap-array.c
 * This tests that you can cast from a 2D Array texture to a Cubemap Array
 * texture and sample from the Cubemap Array view.
 */

#include "piglit-util-gl.h"

PIGLIT_GL_TEST_CONFIG_BEGIN

	config.supports_gl_compat_version = 30;
	config.window_visual = PIGLIT_GL_VISUAL_RGBA | PIGLIT_GL_VISUAL_DOUBLE;

PIGLIT_GL_TEST_CONFIG_END

static float green[] = {0, 1, 0, 1};
static float red[] = {1, 0, 0, 1};

enum piglit_result
piglit_display(void)
{
	GLboolean pass;

	glViewport(0, 0, piglit_width, piglit_height);
	glClearColor(0.5, 0.5, 0.5, 0.5);
	glClear(GL_COLOR_BUFFER_BIT);

	piglit_draw_rect(-1, -1, 2, 2);

	pass = piglit_probe_rect_rgba(0, 0, piglit_width, piglit_height, green);

	piglit_present_results();

	return pass ? PIGLIT_PASS : PIGLIT_FAIL;
}

void
piglit_init(int argc, char **argv)
{
	char *vsCode;
	char *fsCode;
	int tex_loc_cube, prog_cube, l;
	GLuint tex_2DArray, tex_Cube;

	piglit_require_extension("GL_ARB_texture_view");
	piglit_require_extension("GL_ARB_texture_cube_map_array");

	/* setup shaders and program object for Cube rendering */
	(void)!asprintf(&vsCode,
		 "void main()\n"
		 "{\n"
		 "    gl_Position = gl_Vertex;\n"
		 "}\n");
	(void)!asprintf(&fsCode,
		 "#extension GL_ARB_texture_cube_map_array: require\n"
		 "uniform samplerCubeArray tex;\n"
		 "void main()\n"
		 "{\n"
		 "   vec4 color	= texture(tex, vec4(-1, 0, 0, 1));\n"
		 "   gl_FragColor = vec4(color.xyz, 1.0);\n"
		 "}\n");
	prog_cube = piglit_build_simple_program(vsCode, fsCode);
	free(fsCode);
	free(vsCode);
	tex_loc_cube = glGetUniformLocation(prog_cube, "tex");

	glGenTextures(1, &tex_2DArray);
	glBindTexture(GL_TEXTURE_2D_ARRAY, tex_2DArray);

	glTexStorage3D(GL_TEXTURE_2D_ARRAY, 1, GL_RGBA8, 1, 1, 16);

	/* load each array layer with red */
	for (l = 0; l < 16; l++) {
		glTexSubImage3D(GL_TEXTURE_2D_ARRAY, 0, 0, 0, l,
				1, 1, 1, GL_RGBA, GL_FLOAT, red);
	}
	/* make array layer 9 have green */
	glTexSubImage3D(GL_TEXTURE_2D_ARRAY, 0, 0, 0, 9,
			1, 1, 1, GL_RGBA, GL_FLOAT, green);

	glGenTextures(1, &tex_Cube);
	/* the texture view starts at layer 2, so face 1 (-X) of
	 * element 1 will have green */
	glTextureView(tex_Cube, GL_TEXTURE_CUBE_MAP_ARRAY, tex_2DArray, GL_RGBA8,
		      0, 1, 2, 12);
	glTexParameteri(GL_TEXTURE_CUBE_MAP_ARRAY, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_CUBE_MAP_ARRAY, GL_TEXTURE_MIN_FILTER, GL_NEAREST);

	glBindTexture(GL_TEXTURE_CUBE_MAP_ARRAY, tex_Cube);

	glUseProgram(prog_cube);
	glUniform1i(tex_loc_cube, 0);
}
