/* client_core.cc
 *   by Trinity Quirk <tquirk@ymb.net>
 *   last updated 26 Nov 2017, 10:38:24 tquirk
 *
 * Revision IX game client
 * Copyright (C) 2016  Trinity Annabelle Quirk
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
#include "shader.h"

#include "object.h"

#include <GL/gl.h>
#include <GL/glext.h>

#include <glm/vec3.hpp>
#include <glm/mat4x4.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtx/quaternion.hpp>

ObjectCache *obj = NULL;
GLuint vert_shader, geom_shader, frag_shader, shader_pgm;
GLuint model_loc, view_loc, proj_loc;
GLint position_attr, color_attr;
glm::mat4 model, view, projection;
unsigned int rand_seed;

void init_client_core(void)
{
    rand_seed = time(NULL);

    obj = new ObjectCache("object");

    std::clog << "OpenGL version " << glGetString(GL_VERSION) << std::endl;

    /* Set up the shaders and programs */
    vert_shader = load_shader(GL_VERTEX_SHADER,
                              SHADER_SRC_PATH "/vertex.glsl");
    geom_shader = load_shader(GL_GEOMETRY_SHADER,
                              SHADER_SRC_PATH "/geometry.glsl");
    frag_shader = load_shader(GL_FRAGMENT_SHADER,
                              SHADER_SRC_PATH "/fragment.glsl");
    shader_pgm = create_program(vert_shader,
                                geom_shader,
                                frag_shader,
                                "fcolor");
    glUseProgram(shader_pgm);

    model_loc = glGetUniformLocation(shader_pgm, "model");
    view_loc = glGetUniformLocation(shader_pgm, "view");
    proj_loc = glGetUniformLocation(shader_pgm, "proj");

    position_attr = glGetAttribLocation(shader_pgm, "position");
    color_attr = glGetAttribLocation(shader_pgm, "color");

    view = glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f),
                       glm::vec3(100.0f, 100.0f, 100.0f),
                       glm::vec3(0.0f, 1.0f, 0.0f));
    glUniformMatrix4fv(view_loc, 1, GL_FALSE, glm::value_ptr(view));

    std::clog << "rendering core initialized" << std::endl;
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
            /* We don't get a valid quaternion just yet */
            glm::mat4 rot = /*glm::rotate(*/glm::mat4(1.0f)/*,
                                        glm::angle(o.orientation),
                                        glm::axis(o.orientation))*/;
            model = glm::translate(rot, o.position);
            glUniformMatrix4fv(model_loc, 1, GL_FALSE, glm::value_ptr(model));

            if (o.vbo == -1)
            {
                glGenVertexArrays(1, &o.vao);
                glBindVertexArray(o.vao);
                glGenBuffers(1, &o.vbo);
                glBindBuffer(GL_ARRAY_BUFFER, o.vbo);
                glEnableVertexAttribArray(position_attr);
                glVertexAttribPointer(position_attr, 3, GL_FLOAT, GL_FALSE,
                                      sizeof(float) * 6,
                                      (void *)0);
                glEnableVertexAttribArray(color_attr);
                glVertexAttribPointer(color_attr, 3, GL_FLOAT, GL_FALSE,
                                      sizeof(float) * 6,
                                      (void *)(sizeof(float) * 3));
            }
            else
            {
                glBindVertexArray(o.vao);
                glBindBuffer(GL_ARRAY_BUFFER, o.vbo);
            }
            if (o.dirty == true)
            {
                float buf[6];

                memcpy(buf, glm::value_ptr(o.position), sizeof(float) * 3);
                memcpy(&buf[3], glm::value_ptr(o.color), sizeof(float) * 3);
                glBufferData(GL_ARRAY_BUFFER,
                             sizeof(buf), buf,
                             GL_DYNAMIC_DRAW);
                o.dirty = false;
            }
            glDrawArrays(GL_POINTS, 0, 1);
        }
};

void draw_objects(void)
{
    glUseProgram(shader_pgm);
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);

    std::function<void(object&)> draw = draw_object();
    obj->each(draw);
}

void move_object(uint64_t objectid, uint16_t frame,
                 float xpos, float ypos, float zpos,
                 float xori, float yori, float zori, float wori)
{
    object& oref = (*obj)[objectid];

    /* If it's a new object, let's get things set up for it */
    if (oref.color == glm::vec3(0.0, 0.0, 0.0))
    {
        oref.vbo = -1;
        /* Randomize a new color */
        oref.color = {rand_r(&rand_seed) / (RAND_MAX * 1.0),
                      rand_r(&rand_seed) / (RAND_MAX * 1.0),
                      rand_r(&rand_seed) / (RAND_MAX * 1.0)};
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
    /* A 50mm lens on a 35mm camera has a 39.6° horizontal and a 27.0°
     * vertical FoV.
     */
    glUseProgram(shader_pgm);
    projection = glm::perspective(glm::radians(27.0f),
                                  (float)width / (float)height,
                                  0.1f, 1000.0f);
    glUniformMatrix4fv(proj_loc, 1, GL_FALSE, glm::value_ptr(projection));
}
