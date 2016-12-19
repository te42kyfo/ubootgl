#version 150

varying vec4 v_color;

void main(void) {
  gl_FragColor = v_color;
  gl_FragColor.r = v_color.a*v_color.a;
  gl_FragColor.g = 1-v_color.a*v_color.a;
  gl_FragColor.b = v_color.a;
  gl_FragColor.a = 1.0;
}
