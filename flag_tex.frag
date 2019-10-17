#version 150

varying vec2 FragCoord;

uniform sampler2D mask_tex;
uniform sampler2D fill_tex;

void main(void) {
  vec2 texCoord = FragCoord;
  // float tdx = dFdx(texCoord.x);
  // float tdy = dFdy(texCoord.y);

  float tv = textureLod(mask_tex, texCoord, 1).r * 0.6 +
             textureLod(mask_tex, texCoord, 0).r * 0.5 +
             textureLod(mask_tex, texCoord, 2).r * -0.1;

  if (tv > 0.5) {
    gl_FragColor = vec4(0, 0, 0, 1.0);
  } else {
    gl_FragColor = texture(fill_tex, texCoord * -6) *
        (0.6 + smoothstep(0.0, 0.4, textureLod(mask_tex, texCoord, 4).r) * 1.0);
  }

  /*float tv = 0.125 * (texture(mask_tex, texCoord + 2 * vec2(tdx, 0)).r +
                      texture(mask_tex, texCoord - 2 * vec2(tdx, 0)).r +
                      texture(mask_tex, texCoord + 2 * vec2(0, tdy)).r +
                      texture(mask_tex, texCoord - 2 * vec2(0, tdy)).r +
                      texture(mask_tex, texCoord).r * 4.0);

  if (tv > 0.7) {
    gl_FragColor = vec4(0, 0, 0, 1.0);
  } else {
    float tv = 0.25 * (texture(mask_tex, texCoord + 20 * vec2(tdx, 0)).r +
                       texture(mask_tex, texCoord - 20 * vec2(tdx, 0)).r +
                       texture(mask_tex, texCoord + 20 * vec2(0, tdy)).r +
                       texture(mask_tex, texCoord - 20  * vec2(0, tdy)).r +
                       texture(mask_tex, texCoord).r * -4.0);

    gl_FragColor = (1 - 1.0 * tv) * texture(fill_tex, texCoord * -6);
    }*/
}
