#version 150

in vec2 in_Position;
in float in_Alpha;
in int in_Player;

uniform mat4 TM;

varying vec2 FragCoord;
varying vec4 v_color;
varying float v_side;

vec3 playerColors[5] =
    vec3[](vec3(1.8, 0.0, -10.0), vec3(0.0, 1.0, -10.0), vec3(1.0, 1.0, -10.0),
           vec3(1.0, 0.6, -10.0), vec3(1.4, 0.0, 1.0));

void main(void) {
  if (in_Player >= 0)
    // v_color = vec4(1.0, 0.0, 0.0, in_Alpha);
    v_color = vec4(playerColors[in_Player], in_Alpha * 2.0f);
  else
    v_color = vec4(1.0, 1.0, 1.0, in_Alpha);

  FragCoord = in_Position.xy;
  v_side = gl_VertexID % 2;
  gl_Position = TM * vec4(in_Position, 0, 1.0);
}
