#version 410 core

layout(vertices = 3) out;
in vec3 Position[]; // From vertex shader
in vec3 Normal[];

out vec3 tcPosition[]; // Output of TC
out vec3 tcNormal[];

uniform int TessLevelInner; // Sent from main program
uniform int TessLevelOuter;

void main()
{
    tcPosition[gl_InvocationID] = Position[gl_InvocationID]; // Pass through the vertex at hand
    tcNormal[gl_InvocationID] = Normal[gl_InvocationID];
    gl_TessLevelInner[0] = TessLevelInner; // Decide tesselation level
    gl_TessLevelOuter[0] = TessLevelOuter;
    gl_TessLevelOuter[1] = TessLevelOuter;
    gl_TessLevelOuter[2] = TessLevelOuter;
}

