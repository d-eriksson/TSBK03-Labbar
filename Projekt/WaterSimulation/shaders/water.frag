#version 330 core
out vec4 out_color;


in vec3 Normal;
in vec3 Position;
in vec3 Camera;

uniform samplerCube skybox;

vec4 calculateLighting()
{
	const vec3 light = vec3(2, 1, -1); // Given in VIEW coordinates! You usually specify light sources in world coordinates.
	
    vec3 normal = normalize(Normal);

    vec3 lightDir, refl, color = vec3(0.0, 0.0, 0.0);
    float ambient = 0.8, diffuse, specular = 0.0;

    vec3 camDir = normalize(Position);

    lightDir = normalize(light);
    

    refl = normalize(-reflect(lightDir, normal));

    diffuse = max(dot(lightDir, normal), 0.0);

    specular = pow( max(dot(refl, camDir), 0.0), 20);
    
    color = vec3(ambient + 0.3*diffuse + 1.0*specular);

    return vec4(color, 1.0);
}

void main()
{   
    const vec3 cameraPos = vec3(0, 4, 4); 
    float eta = 1.00 / 1.33;
    vec3 I = normalize(Position - Camera);
    vec3 Reflect = reflect(I, normalize(Normal));
    vec3 Refract = refract(I, normalize(Normal), eta);
    float fresnel = 0.02037 + (1.0 - 0.02037) * pow(max(0.0,1.0 - dot(-I,Normal)),5.0);
    //float fresnel = eta + (1.0 - eta) * pow(max(0.0,1.0 - dot(-I,Normal)),5.0);

    //vec4 blue = vec4(0.84,0.96,0.96,1);
    vec4 blue = vec4(0.78,0.86,0.94,1);
    
    
    out_color = mix(texture(skybox, Refract),texture(skybox, Reflect), fresnel) * blue * calculateLighting();
   
}
