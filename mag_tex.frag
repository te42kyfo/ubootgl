#version 150

varying vec2 FragCoord;

uniform sampler2D tex;
uniform vec2 bounds;

void main(void) {
  float tv = texture(tex, FragCoord).r;
  tv = (tv - bounds.x) / (bounds.y - bounds.x);


  //  gl_FragColor = vec4(tv, tv-0.2, tv-0.5, 1.0);
  //gl_FragColor = vec4(2.0*tv-1.2, tv-0.2, 2.0*tv, 1.0);
  //
  gl_FragColor = vec4(0.35*tv*tv, 0.37*tv*tv, 2.0*tv, 1.0);
  gl_FragColor.rgb = pow(gl_FragColor.rgb*0.9, vec3(1.0/1.2));
  //gl_FragColor = vec4(0, 0.5-abs(tv-0.5), 0, 1.0);
}
