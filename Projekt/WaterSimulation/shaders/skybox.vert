
#version 330 core
layout (location = 0) in vec3 in_Position;

out vec3 TexCoords;

uniform mat4 viewMatrix;
uniform mat4 projMatrix;



void main()
{
    TexCoords = in_Position;
    gl_Position = projMatrix * viewMatrix * vec4(in_Position, 1.0);
}  