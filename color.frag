#version 150

varying vec4 v_color;
varying float v_side;

void main(void) {
  float tv = 1.0-2.0*abs(0.5-v_side);
  float aaf = fwidth(tv) + 0.1;
  float edge_blend = smoothstep(max(0, 0.9-aaf*1.0), min(1.0, 0.9 + aaf*1.0), tv);
  gl_FragColor = v_color * vec4(1.0, 1.0, 1.0, edge_blend);
}
