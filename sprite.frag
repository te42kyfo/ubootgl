#version 430

layout(std430, binding = 3) buffer frame { float data_frame[]; };
layout(std430, binding = 4) buffer playerColor { int data_playerColor[]; };

smooth in vec2 out_UV;

uniform sampler2D tex;
uniform ivec2 frameGridSize;

layout(location = 0) out vec4 outColor;

vec3 playerColors[5] =

    //    vec3[](vec3(1.8, 0.0, 0.0), vec3(0.0, 1.8, 0.0), vec3(1.8,  1.8, 0.0),
    //           vec3(1.8, 0.2, 0.0), vec3(1.4, 0.0, 1.0));

    vec3[](vec3(1.0, 0.0, 0.0), vec3(0.0, 1.0, 0.1), vec3(1.0, 1.0, 0.0),
           vec3(0.8, 0.0, 1.0), vec3(1.0, 0.0, 1.0));

void main(void) {

  int frameCount = frameGridSize.x * frameGridSize.y;

  float frame = data_frame[gl_PrimitiveID / 2];

  float nowFrame = 0;
  float blendFactor = modf(frame, nowFrame);
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

  int out_PlayerColor = data_playerColor[gl_PrimitiveID/2];
  if (out_PlayerColor < 0) {
    outColor =
        mix(texture(tex, nowTexCoord), texture(tex, nextTexCoord), blendFactor);
  } else {
    vec4 texColor =
        mix(textureLod(tex, nowTexCoord, 1), texture(tex, nextTexCoord, 1), blendFactor);

    outColor = vec4(playerColors[out_PlayerColor],
                    smoothstep(0.0, 0.2, texColor.a)) ;
  }
}
