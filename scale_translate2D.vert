#version 150

in vec2 in_Position;

uniform mat4 TM;

out vec2 FragCoord;
out vec4 v_color;
out float v_side;

void main(void) {
  v_color = vec4(1.0, 1.0, 1.0, 1.0);
  FragCoord = in_Position.xy;
  v_side = gl_VertexID % 2;
  gl_Position = TM * vec4(in_Position, 0, 1.0);
}
