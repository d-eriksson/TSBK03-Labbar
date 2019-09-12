#version 150

in vec2 outTexCoord;
uniform sampler2D texUnit;
float Psize = 0.001953125;
out vec4 out_Color;

void main(void)
{
    vec4 tex = 
    texture(texUnit, outTexCoord  + Psize * vec2(-2,2))+
    texture(texUnit, outTexCoord  + Psize * vec2(2,2))+
    texture(texUnit, outTexCoord  + Psize * vec2(-1,1))+
    texture(texUnit, outTexCoord  + Psize * vec2(1,1))+
    2*texture(texUnit, outTexCoord  + Psize * vec2(0,0))+
    texture(texUnit, outTexCoord  + Psize * vec2(-1,-1))+
    texture(texUnit, outTexCoord  + Psize * vec2(1,-1))+
    texture(texUnit, outTexCoord  + Psize * vec2(-2,-2))+
    texture(texUnit, outTexCoord  + Psize * vec2(2,-2));


    
    out_Color = tex/10;
    
}
