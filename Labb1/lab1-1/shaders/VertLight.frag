#version 150

in vec2 outTexCoord;
uniform sampler2D texUnit;
float Psize = 0.001953125;
out vec4 out_Color;

void main(void)
{
    vec4 tex = texture(texUnit, outTexCoord  + Psize * vec2(-5,0))+
    texture(texUnit, outTexCoord  + Psize * vec2(-4,0))+
    texture(texUnit, outTexCoord  + Psize * vec2(-3,0))+
    texture(texUnit, outTexCoord  + Psize * vec2(-2,0))+
    texture(texUnit, outTexCoord  + Psize * vec2(-1,0))+
    texture(texUnit, outTexCoord  + Psize * vec2(0,0))+
    texture(texUnit, outTexCoord  + Psize * vec2(1,0))+
    texture(texUnit, outTexCoord  + Psize * vec2(2,0))+
    texture(texUnit, outTexCoord  + Psize * vec2(3,0))+
    texture(texUnit, outTexCoord  + Psize * vec2(4,0))+
    texture(texUnit, outTexCoord  + Psize * vec2(5,0));

    
    out_Color = tex/11;
    
}
