#version 150

in vec4 v_color;
in float v_side;

out vec4 fragColor;

void main(void) {
  float tv = 1.0-2.0*abs(0.5-v_side);
  float aaf = fwidth(tv);
  float edge_blend = smoothstep(0, min(0.9, aaf*2.0), tv);
  fragColor = v_color * vec4(1.0, 1.0, 1.0, edge_blend);
}
