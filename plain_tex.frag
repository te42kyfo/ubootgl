#version 150

varying vec2 FragCoord;

uniform sampler2D tex;

uniform ivec2 frameGridSize;

uniform float frame;

void main(void) {

  int frameCount = frameGridSize.x * frameGridSize.y;

  float nowFrame = 0;
  float blendFactor = modf(frame, nowFrame);
  float nextFrame = nowFrame + 1;

  nowFrame = mod(nowFrame, frameCount);
  nextFrame = mod(nextFrame, frameCount);

  ivec2 nowFrameGridIdx =
      ivec2(mod(nowFrame, frameGridSize.x), nowFrame / frameGridSize.x);
  ivec2 nextFrameGridIdx =
      ivec2(mod(nextFrame, frameGridSize.x), nextFrame / frameGridSize.x);

  vec2 nowTexCoord = FragCoord / frameGridSize +
                     nowFrameGridIdx * (vec2(1.0, -1.0) / frameGridSize);

  vec2 nextTexCoord = FragCoord / frameGridSize +
                      nextFrameGridIdx * (vec2(1.0, -1.0) / frameGridSize);

  gl_FragColor =
      mix(texture(tex, nowTexCoord), texture(tex, nextTexCoord), blendFactor);
}
