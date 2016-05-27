#version 150

in vec4 in_Position;

uniform vec2 aspect_ratio;
uniform vec2 origin;
varying vec2 FragCoord;

void main(void) {
  FragCoord = in_Position.xy;
  gl_Position = vec4(in_Position.xy * aspect_ratio + origin, 0.0, 1.0);
}
