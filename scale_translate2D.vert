#version 150

in vec2 in_Position;
in float in_Alpha;

uniform mat4 TM;

varying vec2 FragCoord;
varying vec4 v_color;
varying float v_side;

void main(void) {
  v_color = vec4(1.0, 1.0, 1.0, in_Alpha);
  FragCoord = in_Position.xy;
  v_side = gl_VertexID % 2;
  gl_Position = TM * vec4(in_Position, 0, 1.0);
}
