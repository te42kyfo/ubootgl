#version 150

in vec2 in_Position;
in float in_Alpha;

uniform vec2 aspect_ratio;
uniform vec2 origin;
varying vec2 FragCoord;
varying vec4 v_color;
varying float v_side;

void main(void) {
  v_color = vec4(1.0, 1.0, 1.0, in_Alpha);
  FragCoord = in_Position.xy;
  v_side = gl_VertexID % 2;
  gl_Position = vec4((in_Position.xy + origin) * aspect_ratio, 0.0, 1.0);
}
