/* client.cc
 *   by Trinity Quirk <tquirk@ymb.net>
 *   last updated 24 Jan 2017, 08:34:29 tquirk
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

#include "ui/ui.h"
#include "log_display.h"

void error_callback(int, const char *);
void key_callback(GLFWwindow *, int, int, int, int);
void char_callback(GLFWwindow *, unsigned int, int);
void mouse_position_callback(GLFWwindow *, double, double);
void mouse_button_callback(GLFWwindow *, int, int, int);
void resize_callback(GLFWwindow *, int, int);

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

void key_callback(GLFWwindow *w, int key, int scan, int action, int mods)
{
    int ui_key = 0, ui_state, ui_mods = 0;

    if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS)
        glfwSetWindowShouldClose(w, GL_TRUE);
    switch (key)
    {
      case GLFW_KEY_LEFT:       ui_key = ui::key::l_arrow;  break;
      case GLFW_KEY_RIGHT:      ui_key = ui::key::r_arrow;  break;
      case GLFW_KEY_UP:         ui_key = ui::key::u_arrow;  break;
      case GLFW_KEY_DOWN:       ui_key = ui::key::d_arrow;  break;
      case GLFW_KEY_HOME:       ui_key = ui::key::home;     break;
      case GLFW_KEY_END:        ui_key = ui::key::end;      break;
      case GLFW_KEY_BACKSPACE:  ui_key = ui::key::bkspc;    break;
      case GLFW_KEY_DELETE:     ui_key = ui::key::del;      break;
      default:              return;
    }
    ui_state = (action == GLFW_PRESS ? ui::key::down : ui::key::up);
    if (mods & GLFW_MOD_SHIFT)
        ui_mods |= ui::key_mod::shift;
    if (mods & GLFW_MOD_CONTROL)
        ui_mods |= ui::key_mod::ctrl;
    if (mods & GLFW_MOD_ALT)
        ui_mods |= ui::key_mod::alt;
    if (mods & GLFW_MOD_SUPER)
        ui_mods |= ui::key_mod::super;
    ctx->key_callback(ui_key, 0, ui_state, ui_mods);
}

void char_callback(GLFWwindow *w, unsigned int c, int mods)
{
    int ui_mods = 0;

    if (mods & GLFW_MOD_SHIFT)
        ui_mods |= ui::key_mod::shift;
    if (mods & GLFW_MOD_CONTROL)
        ui_mods |= ui::key_mod::ctrl;
    if (mods & GLFW_MOD_ALT)
        ui_mods |= ui::key_mod::alt;
    if (mods & GLFW_MOD_SUPER)
        ui_mods |= ui::key_mod::super;
    ctx->key_callback(ui::key::no_key, c, ui::key::down, mods);
}

void mouse_position_callback(GLFWwindow *w, double xpos, double ypos)
{
    ctx->mouse_pos_callback((int)xpos, (int)ypos);
}

void mouse_button_callback(GLFWwindow *w, int button, int action, int mods)
{
    int btn, act;

    switch (button)
    {
      default:
      case GLFW_MOUSE_BUTTON_1:  btn = ui::mouse::button0;  break;
      case GLFW_MOUSE_BUTTON_2:  btn = ui::mouse::button1;  break;
      case GLFW_MOUSE_BUTTON_3:  btn = ui::mouse::button2;  break;
      case GLFW_MOUSE_BUTTON_4:  btn = ui::mouse::button3;  break;
      case GLFW_MOUSE_BUTTON_5:  btn = ui::mouse::button4;  break;
      case GLFW_MOUSE_BUTTON_6:  btn = ui::mouse::button5;  break;
      case GLFW_MOUSE_BUTTON_7:  btn = ui::mouse::button6;  break;
      case GLFW_MOUSE_BUTTON_8:  btn = ui::mouse::button7;  break;
    }

    switch (action)
    {
      default:
      case GLFW_PRESS:    act = ui::mouse::down;  break;
      case GLFW_RELEASE:  act = ui::mouse::up;    break;
    }

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
