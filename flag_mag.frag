#version 450

in vec2 FragCoord;

uniform sampler2D mask_tex;
uniform sampler2D fill_tex;
uniform sampler2D vel_tex;
uniform float offset;
uniform vec2 bounds;

out vec4 fragColor;

void main(void) {

  float mask_val = textureLod(mask_tex, FragCoord, 0).r;
  float mag_val = (texture(vel_tex, FragCoord).r - bounds.x) / (bounds.y - bounds.x);

  vec3 vel_color =
      pow(vec3(0.35 * mag_val * mag_val, 0.37 * mag_val * mag_val, 2.0 * mag_val) * 0.9,
          vec3(1.0 / 1.2));

  float aaf = length(fwidth(FragCoord)) * 64.0f;
  float opaque = 1.0 - smoothstep(max(0.1, 0.5 - aaf), min(0.9, 0.5 + aaf), mask_val);

  fragColor = vec4( opaque * texture(fill_tex, FragCoord * -6 + vec2(offset, 0)).rgb * 1.6 +
          (1.0 - opaque) * vel_color,
      1.0);
}
