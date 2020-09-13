#version 150

in vec2 FragCoord;

uniform sampler2D tex;
uniform vec2 bounds;

out vec4 fragColor;

void main(void) {
  float tv = texture(tex, FragCoord).r;
  tv = (tv - bounds.x) / (bounds.y - bounds.x);

  fragColor = vec4(0.35*tv*tv, 0.37*tv*tv, 2.0*tv, 1.0);
}
