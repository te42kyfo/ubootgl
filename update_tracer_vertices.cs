#version 430


uniform int npoints;
uniform int ntracers;
uniform vec2 pdim;

layout( std430, binding=0 )  buffer P {vec2  points[];};
layout( std430, binding=1 )  buffer V {vec2  vertices[];};
layout( std430, binding=2 )  buffer SP {uint  start_pointers[];};
layout( std430, binding=3 )  buffer EP {uint  end_pointers[];};
layout( std430, binding=4 )  buffer A {float  ages[];};
layout( std430, binding=5 )  buffer I {uint  indices [];};

layout( local_size_x = 256) in;




void main() {
    uint gid = gl_GlobalInvocationID.x;
    if (gid >= ntracers) return;

    
    uint base = gid * npoints;
    uint curr = (end_pointers[gid]+npoints-1) % npoints;
    uint next = (end_pointers[gid] ) % npoints;
    uint prev = (end_pointers[gid] + npoints - 2) % npoints;

    vec2 currP = points[base + curr];
    
    vec2 v1 = normalize(points[base + curr]- points[base + prev]);
    vec2 v2 =  normalize(points[base + next]- points[base + curr ]);

    float width = 0.0002 + length(points[base + next] - points[base + curr]) * 0.1;
    
    
    vec2 dir;
    vec2 perp;


    // set the foremost front vertices perp to first line segment with half width
    perp = vec2(v2.y, -v2.x);
    vertices[(base + next)*2 ] = points[base + next] + perp* width * 0.5;
    vertices[(base + next)*2 + 1]  = points[base + next] - perp* width * 0.5;


    // set vertices one behind to nicely connect first two line segments and full width
    dir = 0.5* (v1 + v2);
    perp = vec2(dir.y, -dir.x);
    perp /=   0.1 + 0.9* dot(perp, vec2(v2.y, -v2.x));
    vertices[(base+curr)*2 ] = points[base + curr] + perp* width;
    vertices[(base+curr)*2 + 1] = points[base + curr] -  perp* width;

}
