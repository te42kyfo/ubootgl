#version 150

varying vec2 FragCoord;

uniform sampler2D mask_tex;
uniform sampler2D fill_tex;


void main(void) {
  vec2 texCoord = (FragCoord + 1.0f) * 0.5f;
  float tv = texture(mask_tex, texCoord).r;
  if (tv > 0.5) {
    gl_FragColor = vec4(0.0, 0.0, 0.0, 0.0);
  } else {
    float d = texture(mask_tex, texCoord - vec2(0, 0.01)).r;
    float u = texture(mask_tex, texCoord + vec2(0, 0.01)).r;
    float l = texture(mask_tex, texCoord - vec2(0.005, 0)).r;
    float r = texture(mask_tex, texCoord + vec2(0.005, 0)).r;

    float dx = l - r;
    float dy = u - d;

    gl_FragColor =
      texture(fill_tex, texCoord*-3.0 + vec2(dx, dy)*0.01) * (1- 0.3*(abs(dx) + abs(dy)));
  }
}
