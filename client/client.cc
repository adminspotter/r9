#include <config.h>

#define GLFW_INCLUDE_GL_3
#include <GLFW/glfw3.h>

#include <iostream>
#include <vector>

#include "configdata.h"
#include "comm.h"
#include "client_core.h"
#include "l10n.h"

void error_callback(int, const char *);
void setup_comm(struct addrinfo *, const char *, const char *, const char *);
void cleanup_comm(void);

std::vector<Comm *> comm;
ConfigData config;

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
    /* OSX blows up without this */
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);

    if ((w = glfwCreateWindow(800, 600, "R9", NULL, NULL)) == NULL)
    {
        glfwTerminate();
        return -1;
    }

    glfwMakeContextCurrent(w);
    init_client_core();

    while (!glfwWindowShouldClose(w))
    {
        draw_objects();
        glfwSwapBuffers(w);
        glfwPollEvents();
    }

    cleanup_comm();
    cleanup_client_core();
    config.write_config_file();
    glfwTerminate();

    return 0;
}

void error_callback(int err, const char *desc)
{
    std::cout << "glfw error: " << desc << " (" << err << ')' << std::endl;
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
