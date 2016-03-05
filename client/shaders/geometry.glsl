#version 330 core

layout(points) in;
layout(triangle_strip, max_vertices = 14) out;

in vec4 location[];
in vec3 vcolor[];

uniform mat4 model;
uniform mat4 view;
uniform mat4 proj;

out vec3 gcolor;

// Draw a cube at the input point
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
