#version 150

in vec2 outTexCoord;
uniform sampler2D texUnit;
out vec4 out_Color;

void main(void)
{
    vec4 tex = texture(texUnit, outTexCoord);
    out_Color = tex -1;
    out_Color.x = max(out_Color.x, 0.0);
    out_Color.y = max(out_Color.y, 0.0);
    out_Color.z = max(out_Color.z, 0.0);
    out_Color.a = 1.0;

    
}
