/*
 * Copyright © 2015 Intel Corporation
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
 */

/**
 * \file using-transform-feedback-uniform.c
 *
 * Basic example that using transform feedback to get the value passed
 * to the vertex shader as a double uniform.
 *
 * The program and shader define and use vertex attributes, but they
 * are not technically needed for this example.
 */

#include "piglit-util-gl.h"

PIGLIT_GL_TEST_CONFIG_BEGIN

	config.supports_gl_core_version = 33;

	config.window_visual = PIGLIT_GL_VISUAL_RGB | PIGLIT_GL_VISUAL_DOUBLE;

PIGLIT_GL_TEST_CONFIG_END

const char *vs_text =
        "#version 150\n"
        "#extension GL_ARB_gpu_shader_fp64: require\n"
        "uniform double uniformValue;\n"
        "in float inValue;\n"
        "out double outValue;\n"
        "void main()\n"
        "{\n"
        "outValue = uniformValue;\n"
        "gl_Position = vec4(inValue);\n"
        "}\n";

/* General data and buffers*/
GLint vert;
GLint prog;
GLuint vao;
GLuint vbo;
GLint input_location;
GLuint tbo;
GLfloat vertex_data[] = { 1.0f, 2.0f, 3.0f, 4.0f, 5.0f };

/* Transform feedback data */
const char *varyings[] = { "outValue" };
GLdouble feedback[5];

/* Uniform data */
const char *uniform_value_name = "uniformValue";
GLdouble uniform_value = 5.00000000000047073456244106637E0;
GLint uniform_location;

static GLboolean
init_shader_and_locations()
{
        vert = piglit_compile_shader_text(GL_VERTEX_SHADER, vs_text);
        prog = glCreateProgram();
        glAttachShader(prog, vert);
        glTransformFeedbackVaryings(prog, 1, varyings, GL_INTERLEAVED_ATTRIBS);
        glLinkProgram(prog);
        glUseProgram(prog);

        input_location= glGetAttribLocation(prog, "inValue");
        uniform_location = glGetUniformLocation(prog, uniform_value_name);

        return piglit_link_check_status(prog) && (input_location != -1) && (uniform_location != -1);
}

static void
init_buffers_and_data()
{
        glGenVertexArrays(1, &vao);
        glBindVertexArray(vao);

        glGenBuffers(1, &vbo);
        glBindBuffer(GL_ARRAY_BUFFER, vbo);
        glBufferData(GL_ARRAY_BUFFER, sizeof(vertex_data), vertex_data, GL_STATIC_DRAW);

        glEnableVertexAttribArray(input_location);
        glVertexAttribPointer(input_location, 1, GL_FLOAT, GL_FALSE, 0, 0);

        glUniform1d(uniform_location, uniform_value);

        glGenBuffers(1, &tbo);
        glBindBuffer(GL_ARRAY_BUFFER, tbo);
        glBufferData(GL_ARRAY_BUFFER, sizeof(feedback), NULL, GL_STATIC_READ);
        glBindBufferBase(GL_TRANSFORM_FEEDBACK_BUFFER, 0, tbo);
}

static void
render()
{
        glEnable(GL_RASTERIZER_DISCARD);

        glBeginTransformFeedback(GL_POINTS);
        glDrawArrays(GL_POINTS, 0, 5);
        glEndTransformFeedback();

        glDisable(GL_RASTERIZER_DISCARD);

        glFlush();
}

static void
clean()
{
        glDeleteProgram(prog);
        glDeleteShader(vert);

        glDeleteBuffers(1, &tbo);
        glDeleteBuffers(1, &vbo);

        glDeleteVertexArrays(1, &vao);
}

static const char *double_to_hex(double d)
{
        union {
                double d;
                unsigned i[2];
        } b;

        b.d = d;
        char *s = (char *) malloc(100);
        sprintf(s, "0x%08X%08X", b.i[1], b.i[0]);
        return s;
}

static GLboolean
fetch_results()
{
        GLboolean result = GL_TRUE;
        int i;
        GLdouble aux;

        /* Fetching values using transform feedback */
        glGetBufferSubData(GL_TRANSFORM_FEEDBACK_BUFFER, 0, sizeof(feedback), feedback);

        /* Fetching value using glGetUniform*/
        glGetUniformdv(prog, uniform_location, &aux);

        printf("Original value: %.14g[%s]\n", uniform_value, double_to_hex(uniform_value));
        printf("Value fetched using GetUniformdv: %.14g[%s]\n", aux, double_to_hex(aux));
        result = result & (uniform_value == aux);

        for (i = 0; i < 5; i++) {
                printf("Fetched element %i data %.14g[%s]\n", i, feedback[i], double_to_hex(feedback[i]));
                result = result && (uniform_value == feedback[i]);
        }

        return result;
}

enum piglit_result
piglit_display(void)
{
        return PIGLIT_FAIL;
}

void piglit_init(int argc, char **argv)
{
        GLboolean ok = GL_TRUE;

        piglit_require_GLSL_version(150);
        piglit_require_extension("GL_ARB_gpu_shader_fp64");
        piglit_require_extension("GL_ARB_transform_feedback3");

        ok = ok && init_shader_and_locations();
        if (ok) {
                init_buffers_and_data();
                render();
                ok = ok && fetch_results();
        }

        clean();
        piglit_report_result(ok ? PIGLIT_PASS : PIGLIT_FAIL);
}

