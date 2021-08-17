#version 430


uniform int npoints;
uniform int ntracers;
uniform float shift;

layout( std430, binding=0 )  buffer P {vec2  points[];};
layout( std430, binding=1 )  buffer V {vec2  vertices[];};

layout( local_size_x = 256) in;




void main() {
    uint gid = gl_GlobalInvocationID.x;
    if (gid >= ntracers*npoints) return;


    points[gid] += vec2(shift, 0.0);
    vertices[gid*2] += vec2(shift, 0.0);
    vertices[gid*2+1] += vec2(shift, 0.0);
}
