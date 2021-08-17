#version 450

in vec2 FragCoord;

uniform sampler2D mask_tex;

out vec4 fragColor;

void main(void) {

  float mask_val = textureLod(mask_tex, FragCoord, 0).r;

  float aaf = length(fwidth(FragCoord)) * 64.0f;
  float opaque = 1.0 - smoothstep(max(0.1, 0.5 - aaf), min(0.9, 0.5 + aaf), mask_val);

  fragColor = vec4( vec3(opaque), 1.0);
}
