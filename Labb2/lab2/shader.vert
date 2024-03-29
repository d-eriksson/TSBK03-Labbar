#version 150

in vec3 in_Color;
in vec3 in_Position;
in vec3 in_Normal;
in vec2 in_TexCoord;
uniform mat4 matrix;
uniform mat4 Rot0;
uniform mat4 Rot1;
uniform vec3 Pos0;
uniform vec3 Pos1;

out vec4 g_color;
const vec3 lightDir = normalize(vec3(0.3, 0.5, 1.0));

// Uppgift 3: Soft-skinning p� GPU
//
// Flytta �ver din implementation av soft skinning fr�n CPU-sidan
// till vertexshadern. Mer info finns p� hemsidan.

void main(void)
{
	// transformera resultatet med ModelView- och Projection-matriserna
	//gl_Position = matrix * vec4(in_Position, 1.0);

	// s�tt r�d+gr�n f�rgkanal till vertex Weights
	vec4 color = vec4(in_TexCoord.x, in_TexCoord.y, 0.0, 1.0);
	vec4 P0 = vec4(Pos0,1);
	vec4 P1 = vec4(Pos1,1);

	vec4 b0Contribution = color.x* ((Rot0 * (vec4(in_Position, 1.0)-P0))+P0);
	vec4 b1Contribution = color.y* ((Rot1 * (vec4(in_Position, 1.0)-P1))+P1);
	gl_Position = b0Contribution + b1Contribution;
	gl_Position = matrix * gl_Position;

	// L�gg p� en enkel ljuss�ttning p� vertexarna 	
	float intensity = dot(in_Normal, lightDir);
	color.xyz *= intensity;

	g_color = color;
}

