#version 430


uniform int npoints;
uniform int ntracers;
uniform float dt;
uniform vec2 pdim;
uniform uint rand_seed;

layout( std430, binding=0 )  buffer P {vec2  points[];};
layout( std430, binding=1 )  buffer V {vec2  vertices[];};
layout( std430, binding=2 )  buffer PP {uint  pointers[];};
layout( std430, binding=3 )  buffer A {float  ages[];};
layout( std430, binding=4 )  buffer I {uint  indices [];};

layout( local_size_x = 256) in;

uniform sampler2D tex_vxy;
uniform sampler2D tex_flag;

uint wang_hash(uint seed) {
    seed = uint(seed ^ uint(61)) ^ uint(seed >> 16);
    seed *= 9;
    seed = seed ^ (seed >> 4);
    seed *= 0x27d4eb2d;
    seed = seed ^ (seed >> 15);
    return seed;
}


uint rng_state = rand_seed;
uint rand_lcg() {
    // LCG values from Numerical Recipes
    rng_state = 1664525 * rng_state + 1013904223;
    return rng_state;
}


float randf() {
    return (rand_lcg() % 100000) / 100000.0;
}


void main() {
    uint gid = gl_GlobalInvocationID.x;
    rng_state = wang_hash(gid + rand_seed);

    float width = 0.001;

    if (gid >= ntracers) return;

    uint base = gid * npoints;
    uint curr = pointers[gid] % npoints;
    uint next = (pointers[gid] +1) % npoints;
    uint prev = (pointers[gid] + npoints - 1) % npoints;


    vec2 currP = points[base + curr];

    bool brokenLink = false;
    if( any( bvec4(lessThan(currP, vec2(0, 0)), greaterThan(currP, pdim))) || texture(tex_flag, currP / pdim).r < 0.6  || ages[gid] > 2*3.141) {
        ages[gid] -= 2*3.141;
        currP = vec2(  randf(), randf()) * pdim;
        brokenLink = true;
        // uint ibase = gid * (npoints -1 )  * 6;
        // indices[ibase + ((pointers[gid])% (npoints-1))*6 +  0] = 0;
        // indices[ibase + ((pointers[gid])% (npoints-1))*6 +  1] = 0;
        // indices[ibase + ((pointers[gid])% (npoints-1))*6 +  2] = 0;
        // indices[ibase + ((pointers[gid])% (npoints-1))*6 +  3] = 0;
        // indices[ibase + ((pointers[gid])% (npoints-1))*6 +  4] = 0;
        // indices[ibase + ((pointers[gid])% (npoints-1))*6 +  5] = 0;
    }

    // RK2/Midpoint rule integration
    vec2 vel1 = texture(tex_vxy, currP / pdim).xy;
    vec2 midPoint = currP + vel1*dt*0.5;
    vec2 vel2 = texture(tex_vxy, midPoint / pdim).xy;
    points[base + next] = currP +   vel2 * dt;
    width = 0.0001 + length(vel2) * 0.0004;

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


    if (!brokenLink) {
        indices[ibase + ((pointers[gid] + 1)% (npoints-1))*6 +  0] = vbase + (curr * 2 + 0) % (npoints*2);
        indices[ibase + ((pointers[gid] + 1)% (npoints-1))*6 +  1] = vbase + (curr * 2 + 1) % (npoints*2);
        indices[ibase + ((pointers[gid] + 1)% (npoints-1))*6 +  2] = vbase + (curr * 2 + 2) % (npoints*2);
        indices[ibase + ((pointers[gid] + 1)% (npoints-1))*6 +  3] = vbase + (curr * 2 + 1) % (npoints*2);
        indices[ibase + ((pointers[gid] + 1)% (npoints-1))*6 +  4] = vbase + (curr * 2 + 2) % (npoints*2);
        indices[ibase + ((pointers[gid] + 1)% (npoints-1))*6 +  5] = vbase + (curr * 2 + 3) % (npoints*2);
    } else {
        indices[ibase + ((pointers[gid] + 1)% (npoints-1))*6 +  0] =0;
        indices[ibase + ((pointers[gid] + 1)% (npoints-1))*6 +  1] =0;
        indices[ibase + ((pointers[gid] + 1)% (npoints-1))*6 +  2] =0;
        indices[ibase + ((pointers[gid] + 1)% (npoints-1))*6 +  3] =0;
        indices[ibase + ((pointers[gid] + 1)% (npoints-1))*6 +  4] =0;
        indices[ibase + ((pointers[gid] + 1)% (npoints-1))*6 +  5] =0;

    }

    ages [gid] = ages[gid] + 0.02;
    pointers[gid] = pointers[gid] + 1;
}
