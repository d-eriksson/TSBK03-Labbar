#version 450
out vec4 out_color;


in vec3 WorldPos_FS_in;
in vec3 Normal_FS_in;


uniform samplerCube skybox;
uniform vec3 camera;

vec4 calculateLighting()
{
	const vec3 light = vec3(2, 1, -1); // Given in VIEW coordinates! You usually specify light sources in world coordinates.
	
    vec3 normal = normalize(Normal_FS_in);

    vec3 lightDir, refl, color = vec3(0.0, 0.0, 0.0);
    float ambient = 0.8, diffuse, specular = 0.0;

    vec3 camDir = normalize(WorldPos_FS_in);

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
    vec3 I = normalize(WorldPos_FS_in - camera);
    vec3 Reflect = reflect(I, normalize(Normal_FS_in));
    vec3 Refract = refract(I, normalize(Normal_FS_in), eta);
    float fresnel = 0.02037 + (1.0 - 0.02037) * pow(max(0.0,1.0 - dot(-I,Normal_FS_in)),5.0);
    //float fresnel = eta + (1.0 - eta) * pow(max(0.0,1.0 - dot(-I,Normal_FS_in)),5.0);

    //vec4 blue = vec4(0.84,0.96,0.96,1);
    vec4 blue = vec4(0.78,0.86,0.94,1);
    
    
    out_color = mix(texture(skybox, Refract),texture(skybox, Reflect), fresnel) * blue * calculateLighting();
   
}