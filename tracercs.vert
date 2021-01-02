#version 430

uniform mat4 TM;
uniform int npoints;
varying vec2 FragCoord;
varying vec4 v_color;
varying float v_side;

layout( std430, binding=0 )  buffer SP {uint  start_pointers[];};
layout( std430, binding=1 )  buffer EP {uint  end_pointers[];};
layout( std430, binding=2 )  buffer AA {float  ages[];};
layout( std430, binding=3 )  buffer V {vec2  vertices[];};

vec3 playerColors[5] =
    vec3[](vec3(1.8, 0.0, -10.0), vec3(0.0, 1.0, -10.0), vec3(1.0, 1.0, -10.0),
           vec3(1.0, 0.2, -10.0), vec3(1.4, 0.0, 1.0));

void main(void) {
    float alpha = 0;

    uint tracerID = gl_VertexID / (npoints*2);
    uint pointID = (gl_VertexID - tracerID * (npoints*2))/2;
    uint vertexID = (gl_VertexID % 2) + 2*(( 10*npoints - pointID + end_pointers[tracerID]) % npoints);

    if( pointID >= (end_pointers[tracerID] + npoints - start_pointers[tracerID]) % npoints) {
        gl_Position = vec4(0.0, 0.0, 0.0, 0.0);
        return;
    } 
    
    if (pointID == 0)
        alpha = 0.0;
    else if (pointID == 1)
        alpha = 2.5;
    else if (pointID == 2)
        alpha = 10.0;
    else
        alpha = npoints / (pointID*2.0) - npoints / (npoints*2.0 - 2.0 );

  alpha *= 0.1;
  
  v_color = vec4(1.0, 1.0, 1.0, alpha * 0.5 * (1.0 - cos(ages[tracerID]) ));

  vec2 in_Position = vertices[tracerID * npoints * 2 + vertexID];
  
  v_side = gl_VertexID % 2;
  FragCoord = in_Position.xy;
  gl_Position = TM * vec4(in_Position, 0, 1.0);
}
