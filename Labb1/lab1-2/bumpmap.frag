#version 150
// bump mapping should be calculated
// 1) in view coordinates
// 2) in texture coordinates

in vec2 outTexCoord;
in vec3 out_Normal;
in vec3 Ps;
in vec3 Pt;
in vec3 pixPos;  // Needed for specular reflections
uniform sampler2D texUnit;
out vec4 out_Color;

float PSize = 0.001953125;
void main(void)
{
    vec3 light = vec3(0.0, 0.7, 0.7); // Light source in view coordinates
	float BumpStrength = 20;
	// Calculate gradients here
	float offset = 1.0 / 256.0; // texture size, same in both directions
	
    vec3 normal = normalize(out_Normal);
    mat3 Mvt = mat3(Ps,Pt,normal);
    float Bs = vec3(texture(texUnit, outTexCoord + offset*vec2(1,0)) - texture(texUnit, outTexCoord)).x;
	float Bt = vec3(texture(texUnit, outTexCoord + offset*vec2(0,1)) - texture(texUnit, outTexCoord)).x;
    light = Mvt * light;
    normal = normalize(vec3(BumpStrength*Bs, BumpStrength*Bt, 1));
	// Simplified lighting calculation.
	// A full solution would include material, ambient, specular, light sources, multiply by texture.
    out_Color = vec4( dot(normal, light)) * vec4(0.5,0.5,0.9,1.0);
}
