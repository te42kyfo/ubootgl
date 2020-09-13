#version 430

layout(std430, binding = 0) buffer pos { vec2 data_pos[]; };
layout(std430, binding = 1) buffer size { vec2 data_size[]; };
layout(std430, binding = 2) buffer rot { float data_rot[]; };

smooth out vec2 out_UV;

uniform mat4 PVM;

void main(void) {
  int quadID = gl_VertexID / 4;


  switch (gl_VertexID % 4) {
  case 0:
    out_UV = vec2(0.0, 0.0);
    break;
  case 1:
    out_UV = vec2(0.0, 1.0);
    break;
  case 2:
    out_UV = vec2(1.0, 1.0);
    break;
  case 3:
  default:
    out_UV = vec2(1.0, 0.0);
    break;
  }

  vec2 edge = (out_UV - 0.5) * data_size[quadID];

  float s = sin(data_rot[quadID]);
  float c = cos(data_rot[quadID]);
  mat2 m = mat2(c, -s, s, c);
  edge = edge *m;

  gl_Position = PVM * vec4(edge + data_pos[quadID], 0.0, 1.0);
}
