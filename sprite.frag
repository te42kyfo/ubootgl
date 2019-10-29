#version 330

smooth in vec2 out_UV;
flat in float out_Frame;
flat in int out_PlayerColor;

uniform sampler2D tex;
uniform ivec2 frameGridSize;

vec3 playerColors[5] =
    vec3[](vec3(1.8, 0.0, 0.0), vec3(0.0, 1.8, 0.0), vec3(1.8,  1.8, 0.0),
           vec3(1.8, 0.2, 0.0), vec3(1.4, 0.0, 1.0));

void main(void) {

  int frameCount = frameGridSize.x * frameGridSize.y;

  float nowFrame = 0;
  float blendFactor = modf(out_Frame, nowFrame);
  float nextFrame = nowFrame + 1;

  nowFrame = mod(nowFrame, frameCount);
  nextFrame = mod(nextFrame, frameCount);

  ivec2 nowFrameGridIdx =
      ivec2(mod(nowFrame, frameGridSize.x), nowFrame / frameGridSize.x);
  ivec2 nextFrameGridIdx =
      ivec2(mod(nextFrame, frameGridSize.x), nextFrame / frameGridSize.x);

  vec2 nowTexCoord = out_UV / frameGridSize +
                     nowFrameGridIdx * (vec2(1.0, -1.0) / frameGridSize);

  vec2 nextTexCoord = out_UV / frameGridSize +
                      nextFrameGridIdx * (vec2(1.0, -1.0) / frameGridSize);

  if (out_PlayerColor < 0) {
    gl_FragColor =
        mix(texture(tex, nowTexCoord), texture(tex, nextTexCoord), blendFactor);
  } else {
    vec4 texColor =
        mix(texture(tex, nowTexCoord), texture(tex, nextTexCoord), blendFactor);

    gl_FragColor = vec4(texColor.r * playerColors[out_PlayerColor] +
                            texColor.g * vec3(1.0, 1.0, 1.0),
                        texColor.a);
  }
}
