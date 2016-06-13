#version 330 core

in vec4 vcolor;

out vec4 fcolor;

uniform uint use_text;

void main()
{
    fcolor = vcolor;
}
