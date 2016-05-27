#version 150

varying vec2 FragCoord;

uniform sampler2D tex;
uniform vec2 bounds;

void main(void) {
  float tv = texture(tex, (FragCoord + 1.0f) * 0.5f).r;
  tv = (tv - bounds.x) / (bounds.y - bounds.x);
  gl_FragColor = vec4(tv, tv, tv, 1.0);
}
