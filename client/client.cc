/* client.cc
 *   by Trinity Quirk <tquirk@ymb.net>
 *   last updated 01 Nov 2020, 06:48:14 tquirk
 *
 * Revision IX game client
 * Copyright (C) 2020  Trinity Annabelle Quirk
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
 * This file contains the main event loop for the R9 client.
 *
 * Things to do
 *
 */

#include <config.h>

#define GLFW_INCLUDE_GL_3
#include <GLFW/glfw3.h>

#include <iostream>
#include <vector>

#include <glm/vec2.hpp>

#include "client.h"
#include "configdata.h"
#include "comm.h"
#include "client_core.h"
#include "l10n.h"

#include <cuddly-gl/ui.h>
#include <cuddly-gl/connect_glfw.h>
#include "log_display.h"

void error_callback(int, const char *);
void close_key_callback(ui::active *, void *, void *);
void resize_callback(GLFWwindow *, int, int);
void move_key_callback(ui::active *, void *, void *);

std::vector<Comm *> comm;
ConfigData config;
GLFWwindow *w;
ui::context *ctx;
log_display *log_disp;

int main(int argc, char **argv)
{
#if WANT_LOCALES && HAVE_LIBINTL_H
    setlocale(LC_ALL, "");
    bindtextdomain(PACKAGE, LOCALE_DIR);
    textdomain(PACKAGE);
#endif /* WANT_LOCALES && HAVE_LIBINTL_H */

#if OPENSSL_API_COMPAT < 0x10100000
    /* If our OpenSSL is old enough, it does not automatically
     * load all of the stuff we might need.
     */
    OpenSSL_add_all_algorithms();
    OpenSSL_add_all_ciphers();
    OpenSSL_add_all_digests();
#endif /* OPENSSL_API_COMPAT */

    config.parse_command_line(argc, (const char **)argv);

    if (glfwInit() == GL_FALSE)
    {
        std::cout << _("failed to initialize GLFW") << std::endl;
        return -1;
    }
    glfwSetErrorCallback(error_callback);

    /* We need at least an OpenGL 3.3 context, since that's when
     * geometry shaders first appeared.
     */
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_DOUBLEBUFFER, GL_TRUE);
    /* Pixel subsampling */
    glfwWindowHint(GLFW_SAMPLES, 4);
    /* OSX blows up without this */
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);

    if ((w = glfwCreateWindow(800, 600, "R9", NULL, NULL)) == NULL)
    {
        glfwTerminate();
        return -1;
    }

    glfwMakeContextCurrent(w);
    ctx = new ui::context(ui::element::size,
                          ui::size::all,
                          glm::ivec2(800, 600));
    ui_connect_glfw(ctx, w);
    ctx->add_callback(ui::callback::key_down, close_key_callback, NULL);
    ctx->add_callback(ui::callback::key_down, move_key_callback, NULL);
    ctx->add_callback(ui::callback::key_up, move_key_callback, NULL);

    log_disp = new log_display(ctx,
                               ui::element::position,
                               ui::position::all,
                               glm::ivec2(10, -10));
    init_client_core();

    glfwSetWindowSizeCallback(w, resize_callback);

    /* Set the initial projection matrix */
    resize_callback(w, 800, 600);

    /* Let's try to login immediately */
    create_login_dialog(ctx);

    while (!glfwWindowShouldClose(w))
    {
        draw_objects();
        ctx->draw();
        glfwSwapBuffers(w);
        glfwPollEvents();
    }

    cleanup_comm();
    cleanup_client_core();
    config.write_config_file();
    log_disp->close();
    ui_disconnect_glfw(ctx, w);
    delete ctx;
    glfwTerminate();

    return 0;
}

void error_callback(int err, const char *desc)
{
    std::cout << "glfw error: " << desc << " (" << err << ')' << std::endl;
}

void close_key_callback(ui::active *a, void *call, void *client)
{
    ui::key_call_data *call_data = (ui::key_call_data *)call;

    if (call_data->key == ui::key::esc && call_data->state == ui::key::down)
        glfwSetWindowShouldClose(w, GL_TRUE);
}

void resize_callback(GLFWwindow *w, int width, int height)
{
    glm::ivec2 sz(width, height);

    ctx->set(ui::element::size, ui::size::all, sz);
    resize_window(width, height);
}

void move_key_callback(ui::active *a, void *call, void *client)
{
    ui::key_call_data *call_data = (ui::key_call_data *)call;

    if (call_data->key == ui::key::u_arrow
        || call_data->key == ui::key::d_arrow)
    {
        uint16_t action;

        if (call_data->state == ui::key::down)
            action = 3;
        else if (call_data->state == ui::key::up)
            action = 5;
        else
            return;

        if (comm.size() > 0)
        {
            float val = (call_data->key == ui::key::u_arrow ? 1.0 : -1.0);
            glm::vec3 dir = self_obj->orientation * glm::vec3(0.0, val, 0.0);
            comm[0]->send_action_request(action, dir, 100);
        }
    }
    else if (call_data->key == ui::key::l_arrow
             || call_data->key == ui::key::r_arrow)
    {
        uint16_t action;

        if (call_data->state == ui::key::down)
            action = 4;
        else if (call_data->state == ui::key::up)
            action = 5;
        else
            return;

        if (comm.size() > 0)
        {
            float val = (call_data->key == ui::key::l_arrow ? 1.0 : -1.0);
            glm::vec3 rot = self_obj->orientation * glm::vec3(0.0, 0.0, val);
            comm[0]->send_action_request(action, rot, 100);
        }
    }
}

void setup_comm(struct addrinfo *ai, const char *user, const char *charname)
{
    Comm *c = new Comm(ai);
    comm.push_back(c);
    c->start();
    c->send_login(user, charname);
}

void cleanup_comm(void)
{
    while (!comm.empty())
    {
        comm.back()->send_logout();
        sleep(1);
        delete comm.back();
        comm.pop_back();
    }
}
