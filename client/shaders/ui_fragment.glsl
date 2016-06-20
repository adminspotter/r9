#version 330 core

in vec4 vcolor;
in vec2 tex_coord;

out vec4 fcolor;

uniform sampler2D tex;
uniform uint use_text;

void main()
{
    if (tex_coord != vec2(-1.0, -1.0))
    {
        fcolor = texture(tex, tex_coord);
        if (use_text == 1u)
            fcolor = vcolor * fcolor.r;
    }
    else
        fcolor = vcolor;
}
