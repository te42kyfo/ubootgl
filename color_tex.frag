#version 150

varying vec2 FragCoord;

uniform sampler2D tex;

void main(void) {
  float tv = texture(tex, FragCoord).r - 0.001;
  tv = sign(tv)*pow(abs(tv)*25.0, (1.3));
  gl_FragColor = vec4(tv, -tv, 0.0, 1.0);
}
