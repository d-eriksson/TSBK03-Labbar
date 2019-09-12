#version 150

in vec2 outTexCoord;
uniform sampler2D texUnit;
float Psize = 0.001953125;
out vec4 out_Color;

void main(void)
{
    vec4 tex = 1*texture(texUnit, outTexCoord  + Psize * vec2(-2,2))+
    4*texture(texUnit, outTexCoord  + Psize * vec2(-1,2))+
    6*texture(texUnit, outTexCoord  + Psize * vec2(0,2))+
    4*texture(texUnit, outTexCoord  + Psize * vec2(1,2))+
    1*texture(texUnit, outTexCoord  + Psize * vec2(2,2))+

    4*texture(texUnit, outTexCoord  + Psize * vec2(-2,1))+
    16*texture(texUnit, outTexCoord  + Psize * vec2(-1,1))+
    24*texture(texUnit, outTexCoord  + Psize * vec2(0,1))+
    16*texture(texUnit, outTexCoord  + Psize * vec2(1,1))+
    4*texture(texUnit, outTexCoord  + Psize * vec2(2,1))+
    
    6*texture(texUnit, outTexCoord  + Psize * vec2(-2,0))+
    24*texture(texUnit, outTexCoord  + Psize * vec2(-1,0))+
    36*texture(texUnit, outTexCoord  + Psize * vec2(0,0))+
    24*texture(texUnit, outTexCoord  + Psize * vec2(1,0))+
    6*texture(texUnit, outTexCoord  + Psize * vec2(2,0))+
    
    4*texture(texUnit, outTexCoord  + Psize * vec2(-2,-1))+
    16*texture(texUnit, outTexCoord  + Psize * vec2(-1,-1))+
    24*texture(texUnit, outTexCoord  + Psize * vec2(0,-1))+
    16*texture(texUnit, outTexCoord  + Psize * vec2(1,-1))+
    4*texture(texUnit, outTexCoord  + Psize * vec2(2,-1))+

    1*texture(texUnit, outTexCoord  + Psize * vec2(-2,-2))+
    4*texture(texUnit, outTexCoord  + Psize * vec2(-1,-2))+
    6*texture(texUnit, outTexCoord  + Psize * vec2(0,-2))+
    4*texture(texUnit, outTexCoord  + Psize * vec2(1,-2))+
    1*texture(texUnit, outTexCoord  + Psize * vec2(2,-2));
    out_Color = tex /256;
    
}
