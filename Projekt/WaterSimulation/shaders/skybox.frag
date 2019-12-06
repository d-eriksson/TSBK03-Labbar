#version 330 core
out vec4 FragColor;

in vec3 TexCoords;

uniform samplerCube skybox;

void main()
{   
    const vec4 light = vec4(0.58, 0.58, 0.58, 0.58);
    FragColor = texture(skybox, TexCoords);
}