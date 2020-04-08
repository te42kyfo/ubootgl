#version 430

layout( std430, binding=0 )  buffer P {vec2  points[];};
layout( std430, binding=1 )  buffer V {vec2  vertices[];};

layout( local_size_x = 256) in;

void main() {
    uint gid = gl_GlobalInvocationID.x;

    vertices[gid] = vec2(100.0+ gid* 10.0, 100. + sin(gid)*10); // (gid * 10.0 + 100.0, 100.0 + 10* sin(gid *0.1));

}
