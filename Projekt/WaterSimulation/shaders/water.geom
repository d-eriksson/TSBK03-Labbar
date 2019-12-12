#version 450

in vec3 teNormal[ ];
in vec3 xyz;
out float gLightIntensity;
const vec3 LIGHTPOS = vec3( 5., 10., 10. );
vec3 V[3];
vec3 CG;
void ProduceVertex( int v , vec3 CG)
{
    gLightIntensity = abs( dot( normalize(LIGHTPOS - V[v]), normalize(teNormal[v]) ) );
    gl_Position = projMatrix * vec4( CG + ( V[v] - CG ), 1. );
    EmitVertex( );
}
void main( ){
    V[0] = gl_PositionIn[0].xyz;
    V[1] = gl_PositionIn[1].xyz;
    V[2] = gl_PositionIn[2].xyz;
    CG = ( V[0] + V[1] + V[2] ) / 3.;
    ProduceVertex( 0 , CG );
    ProduceVertex( 1 , CG);
    ProduceVertex( 2 , CG);
}