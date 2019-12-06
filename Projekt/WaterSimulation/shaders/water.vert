#version 330

in vec3 in_Position;
in vec3 in_Normal;

uniform mat4 viewMatrix, mdlMatrix;
uniform mat4 projMatrix;

out vec2 outTexCoord;
out vec3 pixPos;
out vec3 out_Normal;

void main(void)
{
    outTexCoord.x = in_Position.x*5.5+.5; // Hard coded adjustment for small model
    outTexCoord.y = in_Position.z*5.5+.5;
    pixPos = vec3(viewMatrix /* * mdlMatrix*/ * vec4(in_Position, 1.0));
    out_Normal = mat3(viewMatrix) * in_Normal;

	gl_Position = projMatrix * viewMatrix * vec4(in_Position, 1.0);
}

/**#version 330 core
layout (location = 0) in vec3 in_Position;
layout (location = 1) in vec3 in_Normal;

out vec3 Normal;
out vec3 Position;

uniform mat4 viewMatrix, mdlMatrix;
uniform mat4 projMatrix;

void main()
{
    Normal = mat3(transpose(inverse(mdlMatrix))) * in_Normal;
    Position = vec3(mdlMatrix * vec4(in_Position, 1.0));
    gl_Position = projMatrix * viewMatrix * mdlMatrix * vec4(in_Position, 1.0);
}  **/