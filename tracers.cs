#version 430


uniform int npoints;
uniform int ntracers;

layout( std430, binding=0 )  buffer P {vec2  points[];};
layout( std430, binding=1 )  buffer V {vec2  vertices[];};
layout( std430, binding=2 )  buffer PP {uint  pointers[];};
layout( std430, binding=3 )  buffer A {float  ages[];};
layout( std430, binding=4 )  buffer I {uint  indices [];};

layout( local_size_x = 256) in;

void main() {
    const float width = 1.0;
    uint gid = gl_GlobalInvocationID.x;

    if (gid >= ntracers) return;

    uint base = gid * npoints;
    uint curr = pointers[gid] % npoints;
    uint next = (pointers[gid] +1) % npoints;
    uint prev = (pointers[gid] + npoints - 1) % npoints;

    vec2 currP = points[base + curr];
    points[base + next] = vec2(100+gid*350, 200) + vec2(sin(ages[gid]*(20.4 + gid*1.1)), cos(ages[gid]*1.2)) * (gid + 150.0);


    vec2 v1 = normalize(points[base + curr]- points[base + prev]);
    vec2 v2 =  normalize(points[base + next]- points[base + curr ]);

    vec2 dir;
    vec2 perp;


    perp = vec2(v2.y, -v2.x);
    vertices[base*2 + next*2] = points[base + next] + perp* width * 0.5;
    vertices[base*2 + next*2 + 1]  = points[base + next] - perp* width * 0.5;


    dir = 0.5* (v1 + v2);
    perp = vec2(dir.y, -dir.x);
    perp /=   0.1 + 0.9* dot(perp, vec2(v2.y, -v2.x));

    vertices[base*2 + curr*2] = points[base + curr] + perp* width;
    vertices[base*2 + curr*2 + 1] = points[base + curr] - perp* width;

    // dir = normalize(points[(next + 2) % npoints] - points[(next + 1) % npoints]);
    // perp = vec2(dir.y, -dir.x);
    // vertices[base*2 + ((next+1)%npoints)*2] = points[base + (next+1) % npoints] + perp* width * 0.5;
    // vertices[base*2 + ((next+1)%npoints)*2 + 1]  = points[base + (next+1) % npoints] - perp* width * 0.5;

    uint ibase = gid * (npoints -1 )  * 6;
    uint vbase = (gid * npoints) * 2;



    indices[ibase + ((pointers[gid] + 1)% (npoints-1))*6 +  0] = vbase + (curr * 2 + 0) % (npoints*2);
    indices[ibase + ((pointers[gid] + 1)% (npoints-1))*6 +  1] = vbase + (curr * 2 + 1) % (npoints*2);
    indices[ibase + ((pointers[gid] + 1)% (npoints-1))*6 +  2] = vbase + (curr * 2 + 2) % (npoints*2);
    indices[ibase + ((pointers[gid] + 1)% (npoints-1))*6 +  3] = vbase + (curr * 2 + 1) % (npoints*2);
    indices[ibase + ((pointers[gid] + 1)% (npoints-1))*6 +  4] = vbase + (curr * 2 + 2) % (npoints*2);
    indices[ibase + ((pointers[gid] + 1)% (npoints-1))*6 +  5] = vbase + (curr * 2 + 3) % (npoints*2);


    ages [gid] = ages[gid] + 0.002;
    pointers[gid] = pointers[gid] + 1;
}
