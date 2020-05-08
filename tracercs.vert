#version 430

in vec2 in_Position;

uniform mat4 TM;

varying vec2 FragCoord;
varying vec4 v_color;
varying float v_side;

layout( std430, binding=0 )  buffer PP {uint  pointers[];};
layout( std430, binding=1 )  buffer AA {float  ages[];};

vec3 playerColors[5] =
    vec3[](vec3(1.8, 0.0, -10.0), vec3(0.0, 1.0, -10.0), vec3(1.0, 1.0, -10.0),
           vec3(1.0, 0.2, -10.0), vec3(1.4, 0.0, 1.0));

void main(void) {
    float alpha = 0;
    uint position = ((pointers[gl_VertexID/100/2] - ((gl_VertexID/2) % 100)) % 100);
    if (position == 0)
        alpha = 0.0;
    else if (position == 1)
        alpha = 0.5;
    else if (position == 2)
        alpha = 1.0;
    else
        alpha = 3.4 / ( position-2) - 0.01;

  v_color = vec4(1.0, 1.0, 1.0, alpha * 0.5 * (1.0 - cos(ages[gl_VertexID/100/2]) ));


  FragCoord = in_Position.xy;
  v_side = gl_VertexID % 2;
  gl_Position = TM * vec4(in_Position, 0, 1.0);
}
