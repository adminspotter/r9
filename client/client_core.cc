/* client_core.cc
 *   by Trinity Quirk <tquirk@ymb.net>
 *   last updated 13 Feb 2016, 10:33:10 tquirk
 *
 * Revision IX game client
 * Copyright (C) 2015  Trinity Annabelle Quirk
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 *
 *
 * This file contains the caches and some general drawing routines.
 *
 * For the time being, we'll have VBOs which contain a single point,
 * and a single color, next to each other:
 *   <position> <color>
 *
 * Things to do
 *   - Act sensibly in draw_objects, instead of brute-forcing it.
 *
 */

#include <config.h>

#include <stdlib.h>
#include <time.h>

#include <stdexcept>
#include <sstream>
#include <functional>

#include "client_core.h"

#include "object.h"

#include <GL/gl.h>
#include <GL/glext.h>

#include <glm/vec3.hpp>
#include <glm/mat4x4.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtx/quaternion.hpp>

#define GLSL(src) "#version 330 core\n" #src

const static GLchar *vert_src = GLSL(
    in vec3 position;
    in vec3 color;

    out vec3 vcolor;

    // This will just be a point
    void main() {
        vcolor = color;
        gl_Position = vec4(position, 1.0);
    }
);

const static GLchar *geom_src = GLSL(
    layout(points) in;
    layout(triangle_strip, max_vertices = 14) out;

    in vec3 vcolor[];
    out vec3 gcolor;

    uniform mat4 model;
    uniform mat4 view;
    uniform mat4 proj;

    // Draw a cube at the input point
    void main() {
        mat4 pvm = proj * view * model;
        vec4 pos = gl_in[0].gl_Position;
        gcolor = vcolor[0];

        // Top
        gl_Position = pvm * (pos + vec4(-0.5, 0.5, -0.5, 1.0));
        EmitVertex();
        gl_Position = pvm * (pos + vec4(0.5, 0.5, -0.5, 1.0));
        EmitVertex();
        gl_Position = pvm * (pos + vec4(-0.5, 0.5, 0.5, 1.0));
        EmitVertex();
        gl_Position = pvm * (pos + vec4(0.5, 0.5, 0.5, 1.0));
        EmitVertex();

        // Right side
        gl_Position = pvm * (pos + vec4(0.5, -0.5, 0.5, 1.0));
        EmitVertex();
        gl_Position = pvm * (pos + vec4(0.5, 0.5, -0.5, 1.0));
        EmitVertex();

        // Back
        gl_Position = pvm * (pos + vec4(0.5, -0.5, -0.5, 1.0));
        EmitVertex();
        gl_Position = pvm * (pos + vec4(-0.5, 0.5, -0.5, 1.0));
        EmitVertex();

        // Left side
        gl_Position = pvm * (pos + vec4(-0.5, -0.5, -0.5, 1.0));
        EmitVertex();
        gl_Position = pvm * (pos + vec4(-0.5, 0.5, 0.5, 1.0));
        EmitVertex();

        // Front
        gl_Position = pvm * (pos + vec4(-0.5, -0.5, 0.5, 1.0));
        EmitVertex();
        gl_Position = pvm * (pos + vec4(0.5, -0.5, 0.5, 1.0));
        EmitVertex();

        // Bottom
        gl_Position = pvm * (pos + vec4(-0.5, -0.5, -0.5, 1.0));
        EmitVertex();
        gl_Position = pvm * (pos + vec4(0.5, -0.5, -0.5, 1.0));
        EmitVertex();

        EndPrimitive();
    }
);

const static GLchar *frag_src = GLSL(
    in vec3 gcolor;

    out vec3 fcolor;

    void main() {
        fcolor = gcolor;
    }
);

static GLuint create_shader(GLenum, const GLchar *);
static GLuint create_program(void);
std::string GLenum_to_string(GLenum);

ObjectCache *obj = NULL;
GLuint vert_shader, geom_shader, frag_shader, shader_pgm;
GLuint model_loc, view_loc, proj_loc;
GLuint vao;
GLint position_attr, color_attr;
glm::mat4 model, view, projection;
unsigned int rand_seed;

void init_client_core(void)
{
    std::clog << "initializing the client core" << std::endl;
    rand_seed = time(NULL);

    obj = new ObjectCache("object");

    std::clog << "OpenGL version " << glGetString(GL_VERSION) << std::endl;

    /* Set up the shaders, programs, and vbos */
    vert_shader = create_shader(GL_VERTEX_SHADER, vert_src);
    geom_shader = create_shader(GL_GEOMETRY_SHADER, geom_src);
    frag_shader = create_shader(GL_FRAGMENT_SHADER, frag_src);
    shader_pgm = create_program();

    model_loc = glGetUniformLocation(shader_pgm, "model");
    view_loc = glGetUniformLocation(shader_pgm, "view");
    proj_loc = glGetUniformLocation(shader_pgm, "proj");

    glGenVertexArrays(1, &vao);
    glBindVertexArray(vao);
    position_attr = glGetAttribLocation(shader_pgm, "position");
    glEnableVertexAttribArray(position_attr);
    glVertexAttribPointer(position_attr, 3, GL_FLOAT, GL_FALSE,
                          sizeof(float) * 6,
                          (void *)0);
    color_attr = glGetAttribLocation(shader_pgm, "color");
    glEnableVertexAttribArray(color_attr);
    glVertexAttribPointer(color_attr, 3, GL_FLOAT, GL_FALSE,
                          sizeof(float) * 6,
                          (void *)(sizeof(float) * 3));

    std::clog << "client core initialized" << std::endl;
}

void cleanup_client_core(void)
{
    if (obj != NULL)
    {
        delete obj;
        obj = NULL;
    }

    /* Delete the programs, shaders, and vbos */
    if (frag_shader != 0)
        glDeleteShader(frag_shader);
    if (geom_shader != 0)
        glDeleteShader(geom_shader);
    if (vert_shader != 0)
        glDeleteShader(vert_shader);
    if (shader_pgm != 0)
    {
        glUseProgram(0);
        glDeleteProgram(shader_pgm);
    }
}

struct draw_object
{
    void operator()(object& o)
        {
            glm::mat4 trans = glm::translate(glm::mat4(1.0f), o.position);
            model = glm::rotate(trans,
                                glm::angle(o.orientation),
                                glm::axis(o.orientation));

            glUniformMatrix4fv(model_loc, 1, GL_FALSE, glm::value_ptr(model));

            glBindBuffer(GL_ARRAY_BUFFER, o.vbo);
            if (o.dirty == true)
            {
                float buf[6];

                memcpy(buf, glm::value_ptr(o.position), sizeof(float) * 3);
                memcpy(&buf[3], glm::value_ptr(o.color), sizeof(float) * 3);
                glBufferData(GL_ARRAY_BUFFER, sizeof(buf), buf, GL_DYNAMIC_DRAW);
                o.dirty = false;
            }
            glDrawElements(GL_POINTS, 1, GL_UNSIGNED_INT, &o.vbo);
        }
};

void draw_objects(void)
{
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);

    glUniformMatrix4fv(view_loc, 1, GL_FALSE, glm::value_ptr(view));

    /* Brute force, baby - ya just can't beat it */
    glUseProgram(shader_pgm);
    std::function<void(object&)> draw = draw_object();
    obj->each(draw);
}

void move_object(uint64_t objectid, uint16_t frame,
                 float xpos, float ypos, float zpos,
                 float xori, float yori, float zori, float wori)
{
    object& oref = (*obj)[objectid];

    /* If it's a new object, let's get things set up for it */
    if (oref.color[0] == 0.0 && oref.color[1] == 0.0 && oref.color[2] == 0.0)
    {
        glGenBuffers(1, &oref.vbo);
        /* Randomize a new color */
        oref.color = {rand_r(&rand_seed),
                      rand_r(&rand_seed),
                      rand_r(&rand_seed)};
    }

    /* Update the object's position */
    oref.position[0] = xpos;
    oref.position[1] = ypos;
    oref.position[2] = zpos;
    oref.orientation[0] = xori;
    oref.orientation[1] = yori;
    oref.orientation[2] = zori;
    oref.orientation[3] = wori;

    oref.dirty = true;
}

void resize_window(int width, int height)
{
    /* A 50mm lens on a 35mm camera has a 39.6 degree horizontal FoV */
    projection = glm::perspective(glm::radians(39.6f),
                                  (float)width / (float)height,
                                  0.1f, 1000.0f);
    glUniformMatrix4fv(proj_loc, 1, GL_FALSE, glm::value_ptr(projection));
}

GLuint create_shader(GLenum type, const GLchar *src)
{
    GLint res = GL_FALSE;
    int len = 0;
    std::ostringstream s;
    GLuint shader = glCreateShader(type);

    s << GLenum_to_string(type) << ": ";

    if (shader == 0)
    {
        s << "could not create shader: " << GLenum_to_string(glGetError());
        throw std::runtime_error(s.str());
    }

    glShaderSource(shader, 1, &src, NULL);
    glCompileShader(shader);

    glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &len);
    if (len > 0)
    {
        GLchar message[len + 1];

        memset(message, 0, len + 1);
        glGetShaderInfoLog(shader, len, NULL, message);

        s << message;
    }

    /* Delete the shader and throw an exception on failure */
    glGetShaderiv(shader, GL_COMPILE_STATUS, &res);
    if (res == GL_FALSE)
    {
        glDeleteShader(shader);
        throw std::runtime_error(s.str());
    }

    if (len > 0)
        std::clog << s.str() << std::endl;
    return shader;
}

GLuint create_program(void)
{
    GLenum err;
    GLint res = GL_FALSE;
    int len = 0;
    std::ostringstream s;
    GLuint pgm = glCreateProgram();

    s << "GL program: ";

    if (pgm == 0)
    {
        s << "could not create GL program: " << GLenum_to_string(glGetError());
        throw std::runtime_error(s.str());
    }

    glGetError();
    glAttachShader(pgm, vert_shader);
    glAttachShader(pgm, geom_shader);
    glAttachShader(pgm, frag_shader);
    if ((err = glGetError()) != GL_NO_ERROR)
    {
        s << "Could not attach shaders to the shader program: "
          << GLenum_to_string(err);
        glDeleteProgram(pgm);
        throw std::runtime_error(s.str());
    }

    glLinkProgram(pgm);

    glGetProgramiv(pgm, GL_INFO_LOG_LENGTH, &len);
    if (len > 0)
    {
        GLchar message[len + 1];

        memset(message, 0, len + 1);
        glGetProgramInfoLog(pgm, len, NULL, message);
        s << message;
    }

    glGetProgramiv(pgm, GL_LINK_STATUS, &res);
    if (res == GL_FALSE)
    {
        glDeleteProgram(pgm);
        throw std::runtime_error(s.str());
    }

    if (len > 0)
        std::clog << s.str() << std::endl;
    return pgm;
}

std::string GLenum_to_string(GLenum e)
{
    std::string s;

    switch (e)
    {
      case GL_NO_ERROR:          s = "GL_NO_ERROR"; break;
      case GL_INVALID_ENUM:      s = "GL_INVALID_ENUM"; break;
      case GL_INVALID_VALUE:     s = "GL_INVALID_VALUE"; break;
      case GL_INVALID_OPERATION: s = "GL_INVALID_OPERATION"; break;
      case GL_STACK_OVERFLOW:    s = "GL_STACK_OVERFLOW"; break;
      case GL_STACK_UNDERFLOW:   s = "GL_STACK_UNDERFLOW"; break;
      case GL_OUT_OF_MEMORY:     s = "GL_OUT_OF_MEMORY"; break;
      case GL_TABLE_TOO_LARGE:   s = "GL_TABLE_TOO_LARGE"; break;
      case GL_VERTEX_SHADER:     s = "GL_VERTEX_SHADER"; break;
      case GL_GEOMETRY_SHADER:   s = "GL_GEOMETRY_SHADER"; break;
      case GL_FRAGMENT_SHADER:   s = "GL_FRAGMENT_SHADER"; break;

      default:
        break;
    }
    return s;
}
