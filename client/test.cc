#include <stdlib.h>

#define GLFW_INCLUDE_GL_3
#include <GLFW/glfw3.h>

#include <string>
#include <stdexcept>
#include <sstream>
#include <functional>
#include <iostream>
#include <vector>
#include <chrono>

#include <GL/gl.h>
#include <GL/glext.h>

#include <glm/vec3.hpp>
#include <glm/mat4x4.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtx/quaternion.hpp>

#define GLSL(src) "#version 330 core\n" #src

typedef std::chrono::time_point<std::chrono::high_resolution_clock> stp;

void error_callback(int, const char *);
void key_callback(GLFWwindow *, int, int, int, int);
void resize_callback(GLFWwindow *, int, int);
void init_client_core(void);
void cleanup_client_core(void);
void draw_objects(void);
GLuint create_shader(GLenum type, const GLchar *src);
GLuint create_program(GLuint, GLuint, GLuint, const char *);
std::string GLenum_to_string(GLenum);

const char *vert_src = GLSL(
    in vec3 position;
    in vec3 color;

    out vec4 location;
    out vec3 vcolor;

    void main()
    {
        vcolor = color;
        location = vec4(position, 1.0);
        gl_Position = vec4(0.0, 0.0, 0.0, 1.0);
    }
);

const char *geom_src = GLSL(
    layout(points) in;
    layout(triangle_strip, max_vertices = 14) out;

    in vec4 location[];
    in vec3 vcolor[];

    uniform mat4 model;
    uniform mat4 view;
    uniform mat4 proj;

    out vec3 gcolor;

    void main()
    {
        vec4 pos = location[0];
        mat4 pv = proj * view;
        gcolor = vcolor[0];

        // Top
        gl_Position = pv * (pos + (model * vec4(-0.5, 0.5, -0.5, 1.0)));
        EmitVertex();
        gl_Position = pv * (pos + (model * vec4(0.5, 0.5, -0.5, 1.0)));
        EmitVertex();
        gl_Position = pv * (pos + (model * vec4(-0.5, 0.5, 0.5, 1.0)));
        EmitVertex();
        gl_Position = pv * (pos + (model * vec4(0.5, 0.5, 0.5, 1.0)));
        EmitVertex();

        // Right side
        gl_Position = pv * (pos + (model * vec4(0.5, -0.5, 0.5, 1.0)));
        EmitVertex();
        gl_Position = pv * (pos + (model * vec4(0.5, 0.5, -0.5, 1.0)));
        EmitVertex();

        // Back
        gl_Position = pv * (pos + (model * vec4(0.5, -0.5, -0.5, 1.0)));
        EmitVertex();
        gl_Position = pv * (pos + (model * vec4(-0.5, 0.5, -0.5, 1.0)));
        EmitVertex();

        // Left side
        gl_Position = pv * (pos + (model * vec4(-0.5, -0.5, -0.5, 1.0)));
        EmitVertex();
        gl_Position = pv * (pos + (model * vec4(-0.5, 0.5, 0.5, 1.0)));
        EmitVertex();

        // Front
        gl_Position = pv * (pos + (model * vec4(-0.5, -0.5, 0.5, 1.0)));
        EmitVertex();
        gl_Position = pv * (pos + (model * vec4(0.5, -0.5, 0.5, 1.0)));
        EmitVertex();

        // Bottom
        gl_Position = pv * (pos + (model * vec4(-0.5, -0.5, -0.5, 1.0)));
        EmitVertex();
        gl_Position = pv * (pos + (model * vec4(0.5, -0.5, -0.5, 1.0)));
        EmitVertex();

        EndPrimitive();
    }
);

const char *frag_src = GLSL(
    in vec3 gcolor;

    out vec4 fcolor;

    void main()
    {
        fcolor = vec4(gcolor, 1.0);
    }
);

GLfloat vertices[] =
{
    0.0f, 0.0f,     1.0f, 0.0f, 0.0f,
    0.5f, -0.5f,    0.0f, 1.0f, 0.0f,
    -0.5f, -0.5f,   0.0f, 0.0f, 1.0f
};

GLfloat other_vert[] =
{
    0.0f, 0.0f, 0.0f,  1.0f, 0.0f, 0.0f,
    2.0f, 0.0f, 0.0f,  0.0f, 1.0f, 0.0f,
    0.0f, 0.0f, 2.0f,  0.0f, 0.0f, 1.0f
};

GLuint vert_shader, geom_shader, frag_shader, shader_pgm;
GLuint model_loc, view_loc, proj_loc;
GLuint vbo, vao;
GLint position_attr, color_attr;
glm::mat4 model, view, projection;
stp start;

int main(int argc, char **argv)
{
    GLFWwindow *w;

    start = std::chrono::high_resolution_clock::now();

    if (glfwInit() == GL_FALSE)
    {
        std::cout << "failed to initialize GLFW" << std::endl;
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

    glfwSetKeyCallback(w, key_callback);
    glfwSetWindowSizeCallback(w, resize_callback);

    /* Set the initial projection matrix */
    resize_callback(w, 800, 600);

    while (!glfwWindowShouldClose(w))
    {
        draw_objects();
        glfwSwapBuffers(w);
        glfwPollEvents();
    }

    cleanup_client_core();
    glfwTerminate();

    return 0;
}

void error_callback(int err, const char *desc)
{
    std::cout << "glfw error: " << desc << " (" << err << ')' << std::endl;
}

void key_callback(GLFWwindow *w, int key, int scan, int action, int mods)
{
    if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS)
        glfwSetWindowShouldClose(w, GL_TRUE);
}

void resize_callback(GLFWwindow *w, int width, int height)
{
    projection = glm::perspective(glm::radians(27.0f),
                                  (float)width / (float)height,
                                  0.1f, 1000.0f);
    glUniformMatrix4fv(proj_loc, 1, GL_FALSE, glm::value_ptr(projection));
}

void init_client_core(void)
{
    std::cout << "OpenGL version " << glGetString(GL_VERSION) << std::endl;

    glGenVertexArrays(1, &vao);
    glBindVertexArray(vao);

    glGenBuffers(1, &vbo);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(other_vert), other_vert, GL_STATIC_DRAW);

    /* Set up the shaders, programs, and vbos */
    vert_shader = create_shader(GL_VERTEX_SHADER, vert_src);
    geom_shader = create_shader(GL_GEOMETRY_SHADER, geom_src);
    frag_shader = create_shader(GL_FRAGMENT_SHADER, frag_src);
    shader_pgm = create_program(vert_shader,
                                geom_shader,
                                frag_shader,
                                "fcolor");
    glUseProgram(shader_pgm);

    model_loc = glGetUniformLocation(shader_pgm, "model");
    view_loc = glGetUniformLocation(shader_pgm, "view");
    proj_loc = glGetUniformLocation(shader_pgm, "proj");

    position_attr = glGetAttribLocation(shader_pgm, "position");
    glEnableVertexAttribArray(position_attr);
    glVertexAttribPointer(position_attr, 3, GL_FLOAT, GL_FALSE,
                          sizeof(float) * 6,
                          0);
    color_attr = glGetAttribLocation(shader_pgm, "color");
    glEnableVertexAttribArray(color_attr);
    glVertexAttribPointer(color_attr, 3, GL_FLOAT, GL_FALSE,
                          sizeof(float) * 6,
                          (void *)(sizeof(float) * 3));

    view = glm::lookAt(glm::vec3(5.0f, 2.0f, 1.2f),
                       glm::vec3(0.0f, 0.0f, 0.0f),
                       glm::vec3(0.0f, 1.0f, 0.0f));
    glUniformMatrix4fv(view_loc, 1, GL_FALSE, glm::value_ptr(view));

    std::cout << "rendering core initialized" << std::endl;
}

void cleanup_client_core(void)
{
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

void draw_objects(void)
{
    stp now = std::chrono::high_resolution_clock::now();
    float interval = std::chrono::duration_cast<std::chrono::duration<float>>(now - start).count();

    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);

    model = glm::rotate(model,
                        interval * glm::radians(180.0f),
                        glm::vec3(0.0f, 1.0f, 0.0f));
    glUniformMatrix4fv(model_loc, 1, GL_FALSE, glm::value_ptr(model));
    start = now;

    glDrawArrays(GL_POINTS, 0, 1);
    glDrawArrays(GL_POINTS, 1, 1);
    glDrawArrays(GL_POINTS, 2, 1);
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

GLuint create_program(GLuint vert, GLuint geom, GLuint frag, const char *out)
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
    if (vert)
        glAttachShader(pgm, vert);
    if (geom)
        glAttachShader(pgm, geom);
    if (frag)
        glAttachShader(pgm, frag);
    if ((err = glGetError()) != GL_NO_ERROR)
    {
        s << "Could not attach shaders to the shader program: "
          << GLenum_to_string(err);
        glDeleteProgram(pgm);
        throw std::runtime_error(s.str());
    }

    glBindFragDataLocation(pgm, 0, out);
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
