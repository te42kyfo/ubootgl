#version 450

in vec2 FragCoord;

uniform sampler2D mask_tex;
uniform sampler2D fill_tex;
uniform sampler2D vel_tex;
uniform float offset;
uniform vec2 bounds;

out vec4 fragColor;

void main(void) {

  float mask_val = texture(mask_tex, FragCoord, 0).r;
  float mask_val2 = texture(mask_tex, FragCoord, 2).r;


  /*if (mask_val == 1.0f) {
      float mag_val = (texture(vel_tex, FragCoord).r - bounds.x) / (bounds.y - bounds.x);
      vec3 vel_color =
          pow(vec3(0.35 * mag_val * mag_val, 0.37 * mag_val * mag_val, 2.0 * mag_val) * 0.9,
              vec3(1.0 / 1.2));

      fragColor = vec4( vel_color, 1.0);//texture(fill_tex, FragCoord * -6 + vec2(offset, 0)).rgb * 1.6, 1.0f);
  }else {*/
      float mag_val = (texture(vel_tex, FragCoord).r - bounds.x) / (bounds.y - bounds.x);

      float aaf = length(fwidth(FragCoord)) * 200.0f;
      
      vec3 vel_color =
          pow(vec3(0.46 * mag_val * mag_val, 0.45 * mag_val * mag_val, 1.0 * mag_val) * 1.0,
              vec3(1.0 / 1.2));

      float opaque = 1.0 - smoothstep(max(0.1, 0.5 -aaf ), min(0.9, 0.5 + aaf ), mask_val);

      vec3 fillColor = texture(fill_tex, FragCoord * vec2(6.0f, 2.0f) - vec2(offset, 0), -1.0 ).rgb;
      fillColor = pow(fillColor, vec3(1.0 / 1.4)) * (0.4 + 2.0*mask_val2);


      fragColor = vec4( opaque * fillColor  * 1.6 +
                        (1.0 - opaque) * vel_color,
                                                1.0);
  //}
}
