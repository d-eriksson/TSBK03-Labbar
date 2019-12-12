#version 450 core

layout(triangles, equal_spacing, ccw) in;

in vec3 WorldPos_ES_in[];
in vec3 Normal_ES_in[];

out vec3 WorldPos_CS_in;
out vec3 Normal_CS_in;

uniform mat4 viewMatrix;
uniform mat4 projMatrix;
vec3 interpolate3D(vec3 v0, vec3 v1, vec3 v2)
{
    return vec3(gl_TessCoord.x) * v0 + vec3(gl_TessCoord.y) * v1 + vec3(gl_TessCoord.z) * v2;
}

void main()
{
    // Interpolate the attributes of the output vertex using the barycentric coordinates
    Normal_CS_in = interpolate3D(Normal_ES_in[0], Normal_ES_in[1], Normal_ES_in[2]);
    Normal_CS_in = normalize(Normal_CS_in);
    WorldPos_CS_in = interpolate3D(WorldPos_ES_in[0], WorldPos_ES_in[1], WorldPos_ES_in[2]);
    // Displace the vertex along the normal
    WorldPos_CS_in += Normal_CS_in * 0.01;

    vec3 p1 = WorldPos_ES_in[0];
    vec3 p2 = WorldPos_ES_in[1];
    vec3 p3 = WorldPos_ES_in[2];

    vec3 n1 = Normal_ES_in[0];
    vec3 n2 = Normal_ES_in[1];
    vec3 n3 = Normal_ES_in[2];

    float u = gl_TessCoord.x;
    float v = gl_TessCoord.y;
    float w = gl_TessCoord.z;

    vec3 b300 = p1;
    vec3 b030 = p2;
    vec3 b003 = p3;

    float w12 = dot( p2 - p1, n1 );
    float w21 = dot( p1 - p2, n2 );
    float w13 = dot( p3 - p1, n1 );
    float w31 = dot( p1 - p3, n3 );
    float w23 = dot( p3 - p2, n2 );
    float w32 = dot( p2 - p3, n3 );

    vec3 b210 = ( 2.*p1 + p2 - w12*n1 ) / 3.;
    vec3 b120 = ( 2.*p2 + p1 - w21*n2 ) / 3.;
    vec3 b021 = ( 2.*p2 + p3 - w23*n2 ) / 3.;
    vec3 b012 = ( 2.*p3 + p2 - w32*n3 ) / 3.;
    vec3 b102 = ( 2.*p3 + p1 - w31*n3 ) / 3.;
    vec3 b201 = ( 2.*p1 + p3 - w13*n1 ) / 3.;

    vec3 ee = ( b210 + b120 + b021 + b012 + b102 + b201 ) / 6.;
    vec3 vv = ( p1 + p2 + p3 ) / 3.;
    vec3 b111 = ee + ( ee - vv ) / 2.;
    WorldPos_CS_in = 1.*b300*w*w*w + 1.*b030*u*u*u + 1.*b003*v*v*v +
    3.*b210*u*w*w + 3.*b120*u*u*w + 3.*b201*v*w*w +
    3.*b021*u*u*v + 3.*b102*v*v*w + 3.*b012*u*v*v +
    6.*b111*u*v*w;
    float v12 = 2. * dot( p2-p1, n1+n2 ) / dot( p2-p1, p2-p1 );
    float v23 = 2. * dot( p3-p2, n2+n3 ) / dot( p3-p2, p3-p2 );
    float v31 = 2. * dot( p1-p3, n3+n1 ) / dot( p1-p3, p1-p3 );
    vec3 n200 = n1;
    vec3 n020 = n2;
    vec3 n002 = n3;
    vec3 n110 = normalize( n1 + n2 - v12*(p2-p1) );
    vec3 n011 = normalize( n2 + n3 - v23*(p3-p2) );
    vec3 n101 = normalize( n3 + n1 - v31*(p1-p3) );

    Normal_CS_in = n200*w*w + n020*u*u + n002*v*v + n110*w*u + n011*u*v + n101*w*v;

    gl_Position = projMatrix * viewMatrix * vec4(WorldPos_CS_in, 1.0);
}