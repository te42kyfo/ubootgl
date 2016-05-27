#version 150

varying vec2 FragCoord;

uniform sampler2D tex;

void main(void) {
  vec4 tv = texture(tex, (FragCoord + 1.0f) * 0.5f);
  gl_FragColor = tv;
}
