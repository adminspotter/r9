#version 330 core

in vec2 position;
in vec4 color;
in vec2 texture_uv;

out vec4 vcolor;

void main()
{
    vcolor = color;
    gl_Position = vec4(position, 0.0, 1.0);
}
