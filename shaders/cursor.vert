#version 460 core
layout (location = 0) in vec2 vertex;

void main()
{
    gl_Position = vec4(vertex.xy, 0.0, 1.0);
}