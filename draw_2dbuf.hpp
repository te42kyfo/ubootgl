namespace Draw2DBuf {
void init();
void draw(float* buf, int nx, int ny, int screen_width, int screen_height,
          float scale);
void draw_mag(float* buf_vx, float* buf_vy, int nx, int ny, int screen_width,
              int screen_height, float scale);
}
