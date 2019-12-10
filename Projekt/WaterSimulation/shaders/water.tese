#version 410 core

layout(triangles, equal_spacing, cw) in;
//layout(triangles) in;
in vec3 tcPosition[]; // Original patch vertices
in vec3 tcNormal[];

out vec3 Normal;
out vec3 Position;

uniform mat4 viewMatrix;
uniform mat4 projMatrix;

vec3 interpolate3D(vec3 v0, vec3 v1, vec3 v2)                                                   
{                                                                                               
    return gl_TessCoord.x * v0 + gl_TessCoord.y * v1 + gl_TessCoord.z * v2;   
}

void main()
{
    /*vec3 p0 = gl_TessCoord.x * tcPosition[0]; // Barycentric!
    vec3 p1 = gl_TessCoord.y * tcPosition[1];
    vec3 p2 = gl_TessCoord.z * tcPosition[2];*/
    vec3 A = tcPosition[2] - tcPosition[0];
    vec3 B = tcPosition[1] - tcPosition[0];
    Normal = normalize(cross(A,B));        
    //Normal = normalize(Normal);
    Position = interpolate3D(tcPosition[0], tcPosition[1], tcPosition[2]);

    gl_Position = projMatrix * viewMatrix * vec4(Position, 1); // Sum with weights from the barycentric coords any way we like

// Apply vertex transformation here if we want
}