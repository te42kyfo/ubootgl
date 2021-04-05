#version 450

varying vec2 FragCoord;

uniform sampler2D mask_tex;
uniform sampler2D fill_tex;
uniform float offset;
out vec4 fragColor;

void main(void) {
  vec2 texCoord = FragCoord;
  // float tdy = dFdy(texCoord.y);

  float tv = textureLod(mask_tex, texCoord, 2).r * 0.6 +
             textureLod(mask_tex, texCoord, 1).r * 0.4 +
             textureLod(mask_tex, texCoord, 0).r * 0.3 +
             textureLod(mask_tex, texCoord, 3).r * -0.2;

  float aaf = length(fwidth(texCoord)) * 64.0f;
  float opaque = 1.0 - smoothstep( max(0.1, 0.5 - aaf), min(0.9, 0.5 + aaf), tv);

  fragColor = vec4(
      opaque * texture(fill_tex, texCoord * -6 + vec2(offset, 0)).rgb *
          (0.6 +
           smoothstep(0.0, 0.4, textureLod(mask_tex, texCoord, 4).r) * 1.0),
      1.0);
}
