#version 150

in vec2 outTexCoord;
uniform sampler2D texUnit;
float Psize = 0.001953125;
out vec4 out_Color;

void main(void)
{
    vec4 tex = texture(texUnit, outTexCoord  + Psize * vec2(0,-5))+
    texture(texUnit, outTexCoord  + Psize * vec2(0,-4))+
    texture(texUnit, outTexCoord  + Psize * vec2(0,-3))+
    texture(texUnit, outTexCoord  + Psize * vec2(0,-2))+
    texture(texUnit, outTexCoord  + Psize * vec2(0,-1))+
    texture(texUnit, outTexCoord  + Psize * vec2(0,0))+
    texture(texUnit, outTexCoord  + Psize * vec2(0,1))+
    texture(texUnit, outTexCoord  + Psize * vec2(0,2))+
    texture(texUnit, outTexCoord  + Psize * vec2(0,3))+
    texture(texUnit, outTexCoord  + Psize * vec2(0,4))+
    texture(texUnit, outTexCoord  + Psize * vec2(0,5));

    
    out_Color = tex/11;
    
}
