#version 330 core

in vec3 position;
in vec3 color;

out vec4 location;
out vec3 vcolor;

// This will just be a point
void main()
{
    vcolor = color;
    location = vec4(position, 1.0);
    gl_Position = vec4(0.0, 0.0, 0.0, 1.0);
}
