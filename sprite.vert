#version 330

in vec2 in_Position;
in float in_Frame;
in vec2 in_UV;

out vec2 out_UV;
out float out_Frame;

void main(void) {
  out_UV = in_UV;
  gl_Position = vec4(in_Position, 0.0, 1.0);
  out_Frame = in_Frame;
}
