
#version 330 core
layout (location = 0) in vec3 in_Position;
layout (location = 1) in vec3 in_Normal;


out vec3 Normal;
out vec3 Position;
out vec3 Camera;

uniform mat4 viewMatrix;
uniform mat4 projMatrix;
uniform mat4 mdlMatrix;
uniform vec3 camera;



void main()
{
    
   // Normal = mat3(transpose(inverse(mdlMatrix))) * in_Normal;
   // Position = vec3(mdlMatrix * vec4(in_Position, 1.0));
    Normal = in_Normal;
    Position = in_Position;
    Camera = camera;
    gl_Position = projMatrix * viewMatrix * vec4(in_Position, 1.0);
    //gl_Position = projMatrix * viewMatrix * mdlMatrix * vec4(in_Position, 1.0);
} 
