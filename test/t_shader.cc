#include <tap++.h>

using namespace TAP;

#include <string.h>

#include "../client/shader.h"

bool shader_fail = false, shaderiv_result = false, delete_called = false;
bool create_program_fail = false, get_error_fail = false;
bool programiv_result = false;
int attach_shader_count = 0;

void glShaderSource(GLuint a, GLsizei b, const GLchar **c, const GLint *d) {}
void glCompileShader(GLuint a) {}
void glDeleteShader(GLuint a) { delete_called = true;}
void glDeleteProgram(GLuint a) {}
void glBindFragDataLocation(GLuint a, GLuint b, const char *c) {}
void glLinkProgram(GLuint a) {}

GLuint glCreateShader(GLenum a)
{
    return (shader_fail ? 0 : 1);
}

GLenum glGetError(void)
{
    if (get_error_fail)
        return GL_INVALID_ENUM;
    return GL_NO_ERROR;
}

void glGetShaderiv(GLuint a, GLenum b, GLint *c)
{
    if (shaderiv_result)
        *c = 10;
    else
        *c = 0;
}

void glGetShaderInfoLog(GLuint a, GLsizei b, GLsizei *c, GLchar *d)
{
    strcpy(d, "abc");
}

GLuint glCreateProgram(void) {
    return (create_program_fail ? 0 : 1);
}

void glAttachShader(GLuint a, GLuint b)
{
    ++attach_shader_count;
}
void glGetProgramInfoLog(GLuint a, GLint b, GLsizei *c, GLchar *d)
{
    strcpy(d, "abc");
}
void glGetProgramiv(GLuint a, GLenum b, GLint *c)
{
    if (programiv_result)
        *c = 10;
    else
        *c = 0;
}

void test_load_shader(void)
{
    std::string test = "load shader: ", st;
    bool got_exception;

    st = "file error: ";
    try
    {
        load_shader(GL_VERTEX_SHADER, "asdfasdfasdfasdf");
    }
    catch (std::runtime_error& e)
    {
        std::string err(e.what());

        got_exception = true;
        isnt(err.find("opening file"), std::string::npos,
             test + st + "correct error contents");
    }
    catch (...)
    {
        got_exception = true;
        fail(test + st + "wrong error type");
    }
    if (!got_exception)
        fail(test + st + "no exception");

    st = "ok: ";
    shader_fail = false;
    shaderiv_result = true;
    GLuint result = load_shader(GL_VERTEX_SHADER, "t_shader.cc");
    is(result, 1, test + st + "expected result");
}

void test_create_shader(void)
{
    std::string test = "create_shader: ", st;
    bool got_exception;

    st = "create failure: ";
    shader_fail = true;
    got_exception = false;
    try
    {
        create_shader(GL_VERTEX_SHADER, "");
    }
    catch (std::runtime_error& e)
    {
        std::string err(e.what());

        got_exception = true;
        isnt(err.find("creating shader"), std::string::npos,
             test + st + "correct error contents");
    }
    catch (...)
    {
        got_exception = true;
        fail(test + st + "wrong error type");
    }
    if (!got_exception)
        fail(test + st + "no exception");

    st = "bad compile: ";
    shader_fail = false;
    shaderiv_result = false;
    got_exception = false;
    try
    {
        create_shader(GL_VERTEX_SHADER, "");
    }
    catch (std::runtime_error& e)
    {
        std::string err(e.what());

        got_exception = true;
        isnt(err.find("GL_VERTEX_SHADER"), std::string::npos,
             test + st + "correct error contents");
    }
    catch (...)
    {
        got_exception = true;
        fail(test + st + "wrong error type");
    }
    if (!got_exception)
        fail(test + st + "no exception");
    is(delete_called, true, test + st + "delete called");

    st = "message: ";
    shader_fail = false;
    shaderiv_result = true;
    create_shader(GL_VERTEX_SHADER, "");
}

void test_create_program(void)
{
    std::string test = "create_program: ", st;
    bool got_exception;

    st = "create failure: ";
    create_program_fail = true;
    got_exception = false;
    try
    {
        create_program(1, 2, 3, "abc");
    }
    catch (std::runtime_error& e)
    {
        std::string err(e.what());

        got_exception = true;
        isnt(err.find("creating GL program"), std::string::npos,
             test + st + "correct error contents");
    }
    catch (...)
    {
        got_exception = true;
        fail(test + st + "wrong error type");
    }
    if (!got_exception)
        fail(test + st + "no exception");

    st = "attach shader fail: ";
    create_program_fail = false;
    get_error_fail = true;
    got_exception = false;
    try
    {
        create_program(1, 2, 3, "abc");
    }
    catch (std::runtime_error& e)
    {
        std::string err(e.what());

        got_exception = true;
        isnt(err.find("attaching shaders"), std::string::npos,
             test + st + "correct error contents");
    }
    catch (...)
    {
        got_exception = true;
        fail(test + st + "wrong error type");
    }
    if (!got_exception)
        fail(test + st + "no exception");
    is(attach_shader_count, 3, test + st + "expected shaders attached");

    st = "bad link status: ";
    get_error_fail = false;
    got_exception = false;
    programiv_result = false;
    try
    {
        create_program(1, 2, 3, "abc");
    }
    catch (std::runtime_error& e)
    {
        std::string err(e.what());

        got_exception = true;
        pass(test + st + "correct error type");
    }
    catch (...)
    {
        got_exception = true;
        fail(test + st + "wrong error type");
    }
    if (!got_exception)
        fail(test + st + "no exception");

    st = "message: ";
    programiv_result = true;
    create_program(1, 2, 3, "abc");
}

void test_enum_to_string(void)
{
    std::string test = "enum_to_string: ";

    is(GLenum_to_string(GL_NO_ERROR),
       "GL_NO_ERROR",
       test + "expected no_error string");
    is(GLenum_to_string(GL_INVALID_ENUM),
       "GL_INVALID_ENUM",
       test + "expected invalid_enum string");
    is(GLenum_to_string(GL_INVALID_VALUE),
       "GL_INVALID_VALUE",
       test + "expected invalid_value string");
    is(GLenum_to_string(GL_INVALID_OPERATION),
       "GL_INVALID_OPERATION",
       test + "expected invalid_operation string");
    is(GLenum_to_string(GL_STACK_OVERFLOW),
       "GL_STACK_OVERFLOW",
       test + "expected stack_overflow string");
    is(GLenum_to_string(GL_STACK_UNDERFLOW),
       "GL_STACK_UNDERFLOW",
       test + "expected stack_underflow string");
    is(GLenum_to_string(GL_OUT_OF_MEMORY),
       "GL_OUT_OF_MEMORY",
       test + "expected out_of_memory string");
    is(GLenum_to_string(GL_TABLE_TOO_LARGE),
       "GL_TABLE_TOO_LARGE",
       test + "expected table_too_large string");
    is(GLenum_to_string(GL_VERTEX_SHADER),
       "GL_VERTEX_SHADER",
       test + "expected vertex_shader string");
    is(GLenum_to_string(GL_GEOMETRY_SHADER),
       "GL_GEOMETRY_SHADER",
       test + "expected geometry_shader string");
    is(GLenum_to_string(GL_FRAGMENT_SHADER),
       "GL_FRAGMENT_SHADER",
       test + "expected fragment_shader string");
    is(GLenum_to_string(999999),
       "",
       test + "expected default string");
}

int main(int argc, char **argv)
{
    plan(21);

    test_load_shader();
    test_create_shader();
    test_create_program();
    test_enum_to_string();

    return exit_status();
}
