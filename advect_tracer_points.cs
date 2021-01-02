#version 430


uniform int npoints;
uniform int ntracers;
uniform float dt;
uniform vec2 pdim;
uniform uint rand_seed;
uniform float angle;
uniform sampler2D tex_vxy;
uniform sampler2D tex_flag;

layout( std430, binding=0 )  buffer P {vec2  points[];};
layout( std430, binding=2 )  buffer SP {uint  start_pointers[];};
layout( std430, binding=3 )  buffer EP {uint  end_pointers[];};
layout( std430, binding=4 )  buffer A {float  ages[];};

layout( local_size_x = 256) in;


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
    if (gid >= ntracers) return;

    rng_state = wang_hash(gid + rand_seed);

    
    uint base = gid * npoints;
    uint curr = end_pointers[gid] % npoints;
    uint next = (end_pointers[gid] +1) % npoints;

    vec2 currP = points[base + curr];
    
    // RK2/Midpoint rule integration
    vec2 vel1 = texture(tex_vxy, currP / pdim).xy;
    vec2 midPoint = currP + vel1*dt*0.5;
    vec2 vel2 = texture(tex_vxy, midPoint / pdim).xy;
    vec2 nexP = currP + vel2 * dt; 

    // out of bounds or collision with terragin? Stop advecting and age faster
    if( any( bvec4(lessThan(currP, vec2(0, 0)), greaterThan(currP, pdim))) || texture(tex_flag, currP / pdim).r < 0.6 ) { 
        nexP = currP;
        ages[gid] += 0.1;
    }

    // if age has run out, reset age and pointers and seed new random point 
    if( ages[gid] > 2*3.141  ) { 
        start_pointers[gid] = 0;
        end_pointers[gid] = 0;
        start_pointers[gid] = 0;
        nexP = vec2(  randf(), randf()) * pdim;
        points[base + 0] = nexP;
        ages[gid]  = 0;
    } else { // otherwise advance pointers and age
        end_pointers[gid]  = next;
        if  (start_pointers[gid] == end_pointers[gid])
            start_pointers[gid] = (start_pointers[gid] + 1) % npoints;
        points[base + next] = nexP;
        ages [gid] = ages[gid] + 0.02;
    }
}
