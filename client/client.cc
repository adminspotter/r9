/* client.cc
 *   by Trinity Quirk <tquirk@ymb.net>
 *   last updated 08 Sep 2017, 07:36:17 tquirk
 *
 * Revision IX game client
 * Copyright (C) 2017  Trinity Annabelle Quirk
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
#include <map>

#include <glm/vec2.hpp>

#include "client.h"
#include "configdata.h"
#include "comm.h"
#include "client_core.h"
#include "l10n.h"

#include "ui/ui.h"
#include "log_display.h"

void error_callback(int, const char *);
void key_callback(GLFWwindow *, int, int, int, int);
void char_callback(GLFWwindow *, unsigned int, int);
void mouse_position_callback(GLFWwindow *, double, double);
void mouse_button_callback(GLFWwindow *, int, int, int);
void resize_callback(GLFWwindow *, int, int);
void move_key_callback(ui::active *, void *, void *);
void stop_key_callback(ui::active *, void *, void *);

static std::map<int, int> glfw_key_map =
{
    { GLFW_KEY_LEFT, ui::key::l_arrow },
    { GLFW_KEY_RIGHT, ui::key::r_arrow },
    { GLFW_KEY_UP, ui::key::u_arrow },
    { GLFW_KEY_DOWN, ui::key::d_arrow },
    { GLFW_KEY_HOME, ui::key::home },
    { GLFW_KEY_END, ui::key::end },
    { GLFW_KEY_BACKSPACE, ui::key::bkspc },
    { GLFW_KEY_DELETE, ui::key::del },
    { GLFW_KEY_ESCAPE, ui::key::esc },
    { GLFW_PRESS, ui::key::down },
    { GLFW_RELEASE, ui::key::up }
};

static std::map<int, int> glfw_mouse_map =
{
    { GLFW_MOUSE_BUTTON_1, ui::mouse::button0 },
    { GLFW_MOUSE_BUTTON_2, ui::mouse::button1 },
    { GLFW_MOUSE_BUTTON_3, ui::mouse::button2 },
    { GLFW_MOUSE_BUTTON_4, ui::mouse::button3 },
    { GLFW_MOUSE_BUTTON_5, ui::mouse::button4 },
    { GLFW_MOUSE_BUTTON_6, ui::mouse::button5 },
    { GLFW_MOUSE_BUTTON_7, ui::mouse::button6 },
    { GLFW_MOUSE_BUTTON_8, ui::mouse::button7 },
    { GLFW_PRESS, ui::mouse::down },
    { GLFW_RELEASE, ui::mouse::up }
};

std::vector<Comm *> comm;
ConfigData config;
ui::context *ctx;
log_display *log_disp;

int main(int argc, char **argv)
{
    GLFWwindow *w;

#if WANT_LOCALES && HAVE_LIBINTL_H
    setlocale(LC_ALL, "");
    bindtextdomain(PACKAGE, LOCALE_DIR);
    textdomain(PACKAGE);
#endif /* WANT_LOCALES && HAVE_LIBINTL_H */

    config.parse_command_line(argc, argv);

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
    ctx = new ui::context(800, 600);
    log_disp = new log_display(ctx, 0, 0);
    init_client_core();

    glfwSetWindowSizeCallback(w, resize_callback);
    glfwSetKeyCallback(w, key_callback);
    glfwSetCharModsCallback(w, char_callback);
    glfwSetMouseButtonCallback(w, mouse_button_callback);
    glfwSetCursorPosCallback(w, mouse_position_callback);

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
    glfwTerminate();

    return 0;
}

void error_callback(int err, const char *desc)
{
    std::cout << "glfw error: " << desc << " (" << err << ')' << std::endl;
}

int convert_glfw_mods(int mods)
{
    int retval = 0;

    if (mods & GLFW_MOD_SHIFT)
        retval |= ui::key_mod::shift;
    if (mods & GLFW_MOD_CONTROL)
        retval |= ui::key_mod::ctrl;
    if (mods & GLFW_MOD_ALT)
        retval |= ui::key_mod::alt;
    if (mods & GLFW_MOD_SUPER)
        retval |= ui::key_mod::super;
}

void key_callback(GLFWwindow *w, int key, int scan, int action, int mods)
{
    int ui_key = 0, ui_state, ui_mods;

    if (glfw_key_map.find(key) == glfw_key_map.end())
        return;

    ui_key = glfw_key_map[key];
    ui_state = glfw_key_map[action];
    ui_mods = convert_glfw_mods(mods);

    if (ui_key == ui::key::esc && ui_state == ui::key::down)
        glfwSetWindowShouldClose(w, GL_TRUE);

    ctx->key_callback(ui_key, 0, ui_state, ui_mods);
}

void char_callback(GLFWwindow *w, unsigned int c, int mods)
{
    int ui_mods = convert_glfw_mods(mods);

    ctx->key_callback(ui::key::no_key, c, ui::key::down, ui_mods);
}

void mouse_position_callback(GLFWwindow *w, double xpos, double ypos)
{
    ctx->mouse_pos_callback((int)xpos, (int)ypos);
}

void mouse_button_callback(GLFWwindow *w, int button, int action, int mods)
{
    int btn, act;

    btn = glfw_mouse_map[button];
    act = glfw_mouse_map[action];

    ctx->mouse_btn_callback(btn, act);
}

void resize_callback(GLFWwindow *w, int width, int height)
{
    glm::ivec2 sz(width, height);

    ctx->set(ui::element::size, ui::size::all, &sz);
    resize_window(width, height);
}

void setup_comm(struct addrinfo *ai,
                const char *user, const char *pass, const char *charname)
{
    Comm *c = new Comm(ai);
    comm.push_back(c);
    c->start();
    c->send_login(user, pass, charname);
}

void cleanup_comm(void)
{
    while (!comm.empty())
    {
        delete comm.back();
        comm.pop_back();
    }
}
