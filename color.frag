#version 150

varying vec4 v_color;
varying float v_side;

void main(void) {
  float edge_blend = max(0.0, 1.0 - abs(1.0 - 2.5 * v_side));
  gl_FragColor = v_color * vec4(1.0, 1.0, 1.0, edge_blend);
}
