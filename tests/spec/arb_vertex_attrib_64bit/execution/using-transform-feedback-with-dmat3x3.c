/*
 * Copyright © 2010 Intel Corporation
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
 * \file using-transform-feedback.c
 *
 * Basic example that using transform feedback to get the value passed
 * to the vertex shader.
 *
 * Transform feedback can't get the input attribute, but varying
 * output. So we copy the value to a out variable.
 */

#include "piglit-util-gl.h"

PIGLIT_GL_TEST_CONFIG_BEGIN

	config.supports_gl_core_version = 33;

	config.window_visual = PIGLIT_GL_VISUAL_RGB | PIGLIT_GL_VISUAL_DOUBLE;

PIGLIT_GL_TEST_CONFIG_END

const char *vs_text =
        "#version 150\n"
        "#extension GL_ARB_vertex_attrib_64bit: require\n"
        "#extension GL_ARB_gpu_shader_fp64: require\n"
        "in dmat3x3 inValue;\n"
        "out dmat3x3 outValue;\n"
        "void main()\n"
        "{\n"
        "outValue = inValue;\n"
        "}\n";



GLint vert;
GLint prog;
GLuint vao;
GLuint vbo;
GLint inputAttrib;
GLuint tbo;
const char *varyings[] = { "outValue" };

#define NUM_ROWS 3
#define NUM_COLUMNS 3
#define NUM_ELEMENTS NUM_ROWS*NUM_COLUMNS
#define NUM_SAMPLES 1

unsigned DATA_SIZE = 0;
unsigned ATTRIBUTE_SIZE = 0;

GLdouble LSB = 0.00000000010000111022302462516E0;
GLdouble data[9] = {0, 1, 2, 10, 11, 12, 100, 101, 102};

GLdouble feedback[9];

static void
init_shader()
{
        vert = piglit_compile_shader_text(GL_VERTEX_SHADER, vs_text);
        prog = glCreateProgram();
        glAttachShader(prog, vert);
        glTransformFeedbackVaryings(prog, 1, varyings, GL_INTERLEAVED_ATTRIBS);
        glLinkProgram(prog);
        glUseProgram(prog);
        inputAttrib= glGetAttribLocation(prog, "inValue");
}

static void
init_buffers(size_t *offset)
{
        unsigned row;
        unsigned column;

        ATTRIBUTE_SIZE = sizeof(GLdouble);
        DATA_SIZE = ATTRIBUTE_SIZE * NUM_SAMPLES * NUM_ROWS * NUM_COLUMNS;
        size_t stride = ATTRIBUTE_SIZE * NUM_ROWS * NUM_COLUMNS;

        for (row = 0; row < NUM_ROWS; row++)
                for (column = 0; column < NUM_COLUMNS; column++)
                        data[column*NUM_ROWS + row] += 1 + LSB;

        glGenVertexArrays(1, &vao);
        glBindVertexArray(vao);

        glGenBuffers(1, &vbo);
        glBindBuffer(GL_ARRAY_BUFFER, vbo);
        glBufferData(GL_ARRAY_BUFFER, DATA_SIZE, data, GL_STATIC_DRAW);

        for (column = 0; column < NUM_COLUMNS; column++) {
                glEnableVertexAttribArray(inputAttrib + column);
                glVertexAttribLPointer(inputAttrib + column, NUM_ROWS, GL_DOUBLE, stride, (void*) *offset);
                *offset += NUM_COLUMNS * ATTRIBUTE_SIZE;
        }

        glGenBuffers(1, &tbo);
        glBindBuffer(GL_ARRAY_BUFFER, tbo);
        glBufferData(GL_ARRAY_BUFFER, DATA_SIZE, NULL, GL_STATIC_READ);
}

static void
render()
{
        glEnable(GL_RASTERIZER_DISCARD);

        glBindBufferBase(GL_TRANSFORM_FEEDBACK_BUFFER, 0, tbo);

        glBeginTransformFeedback(GL_POINTS);
        glDrawArrays(GL_POINTS, 0, NUM_SAMPLES);
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

        glGetBufferSubData(GL_TRANSFORM_FEEDBACK_BUFFER, 0, DATA_SIZE, feedback);

        for (i = 0; i < NUM_ELEMENTS; i++) {
                printf("%i - Original = %.14g[%s] Fetched = %.14g[%s]", i + 1,
                       data[i], double_to_hex(data[i]),
                       feedback[i], double_to_hex(feedback[i]));
                if (data[i] == feedback[i])
                        printf("\tequal\n");
                else
                        printf("\tWRONG\n");
                result = result && (data[i] == feedback[i]);
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
        piglit_require_extension("GL_ARB_transform_feedback3");
        piglit_require_extension("GL_ARB_vertex_attrib_64bit");
        piglit_require_extension("GL_ARB_gpu_shader_fp64");

        init_shader();
        ok = ok && piglit_link_check_status(prog);

        if (ok) {
                size_t offset = 0;

                init_buffers(&offset);
                render();

                ok = ok && fetch_results();
        }

        clean();
        piglit_report_result(ok ? PIGLIT_PASS : PIGLIT_FAIL);
}

