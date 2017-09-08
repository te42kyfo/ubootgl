#version 150

varying vec2 FragCoord;

uniform sampler2D tex;

void main(void) {
  gl_FragColor = texture(tex, FragCoord);
}
