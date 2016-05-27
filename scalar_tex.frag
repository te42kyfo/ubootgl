#version 150

varying vec2 FragCoord;

uniform sampler2D tex;
uniform vec2 bounds;

void main(void) {
  float tv = texture(tex, (FragCoord + 1.0f) * 0.5f).r;
  tv = (tv - bounds.x) / (bounds.y - bounds.x);

  vec3 cmix = vec3(2*tv-0.5, (0.5-abs(tv-0.5)), 2*(1-tv)-0.5);

  gl_FragColor = vec4(cmix / max(cmix.r, max(cmix.g, cmix.b)), 1.0);
  //gl_FragColor = vec4(0, 0.5-abs(tv-0.5), 0, 1.0);
}
