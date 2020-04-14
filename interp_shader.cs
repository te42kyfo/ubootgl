
#version 430

uniform uint nx;
uniform uint ny;

uniform sampler2D tex_vx_staggered;
uniform sampler2D tex_vy_staggered;

layout(rg32f, binding = 0)  uniform writeonly restrict image2D img_vxy;
layout(r32f, binding = 1)  uniform writeonly restrict image2D img_mag;

layout( local_size_x = 32, local_size_y = 8) in;

void main() {
    //if (gl_GlobalInvocationId.x >= nx*2-1 || gl_GlobalInvocationID.y > ny*2-1)
    //return;
    vec2 vx_coords = vec2((gl_GlobalInvocationID.x) / (2.0*nx-2),
                          (gl_GlobalInvocationID.y+1) / (2.0*ny));

    vec2 vy_coords = vec2((gl_GlobalInvocationID.x+1) / (2.0*nx),
                          (gl_GlobalInvocationID.y) / (2.0*ny-2 ));

    float vx_val = texture(tex_vx_staggered, vx_coords).r;
    float vy_val = texture(tex_vy_staggered, vy_coords).r;

    float mag = length(vec2(vx_val, vy_val));

    imageStore(img_vxy, ivec2(gl_GlobalInvocationID.xy),
               vec4(vx_val, vy_val, 0.0, 0.0));

    imageStore(img_mag, ivec2(gl_GlobalInvocationID.xy),
               vec4(mag, 0.0, 0.0, 0.0));

}


/*
x > x > x > x
v + v + v + v
x > x > x > x
v + v + v + v
x > x > x > x
v + v + v + v
x > x > x > x

cx / (nx*2-1)

cy / (ny*2-1-1)

x > x > x > x  0
  +   +   +    1
x > x > x > x  2
  +   +   +    3
x > x > x > x  4
  +   +   +    5
x > x > x > x  6
*/
