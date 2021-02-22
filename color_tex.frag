#version 150

varying vec2 FragCoord;

uniform sampler2D tex;

void main(void) {
  float tv = texture(tex, FragCoord).r;
  gl_FragColor = vec4(tv * 25.0, -tv * 25.0, 0.0, 1.0);
}
