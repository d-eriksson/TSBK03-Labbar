#version 330 core
out vec4 out_color;


in vec3 Normal;
in vec3 Position;
in vec3 Camera;

uniform samplerCube skybox;

void main()
{   
    const vec3 cameraPos = vec3(0, 4, 4); 
    float eta = 1.00 / 1.33;
    vec3 I = normalize(Position - Camera);
    vec3 Reflect = reflect(I, normalize(Normal));
    vec3 Refract = refract(I, normalize(Normal), eta);
    //float fresnel = 0.02037 + (1.0 - 0.02037) * pow(max(0.0,1.0 - dot(-I,Normal)),5.0);
    float fresnel = eta + (1.0 - eta) * pow(max(0.0,1.0 - dot(-I,Normal)),5.0);
    
    
    out_color = mix(texture(skybox, Refract),texture(skybox, Reflect), fresnel) ;
   
}
