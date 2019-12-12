#version 450 core

// define the number of CPs in the output patch
layout (vertices = 3) out;

// attributes of the input CPs
in vec3 WorldPos_CS_in[];
in vec3 Normal_CS_in[];

// attributes of the output CPs
out vec3 WorldPos_ES_in[];
out vec3 Normal_ES_in[];

void main()
{
    // Set the control points of the output patch
    Normal_ES_in[gl_InvocationID] = Normal_CS_in[gl_InvocationID];
    WorldPos_ES_in[gl_InvocationID] = WorldPos_CS_in[gl_InvocationID];
        // Calculate the distance from the camera to the three control point

    // Calculate the tessellation levels
    gl_TessLevelOuter[0] = 5.0;
    gl_TessLevelOuter[1] = 5.0;
    gl_TessLevelOuter[2] = 5.0;
    gl_TessLevelInner[0] = 5.0;
}