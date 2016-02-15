#include <config.h>

#define GLFW_INCLUDE_GL_3
#include <GLFW/glfw3.h>

#include <vector>

#include "configdata.h"
#include "comm.h"
#include "client_core.h"
#include "l10n.h"

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

    glfwInit();

    /* We need at least an OpenGL 3.3 context, since that's when
     * geometry shaders first appeared.
     */
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    if ((w = glfwCreateWindow(800, 600, "R9", NULL, NULL)) == 0)
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
