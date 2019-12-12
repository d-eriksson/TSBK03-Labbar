#version 450 core

layout (location = 0) in vec3 in_Position;
layout (location = 1) in vec3 in_Normal;

uniform mat4 viewMatrix;
uniform mat4 projMatrix;

out vec3 WorldPos_CS_in;
out vec3 Normal_CS_in;

void main()
{
    WorldPos_CS_in = (vec4(in_Position, 1.0)).xyz;
    Normal_CS_in = (vec4(in_Normal, 0.0)).xyz;
}
