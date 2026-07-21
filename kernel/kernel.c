#include <am.h>
#include <amdev.h>
#include <klib.h>
#include <klib-macros.h>
#include "pic.h"

#define SIDE 16

// 图片信息
#define PIC_W 95
#define PIC_H 91
#define BMP_HEADER_SIZE 138
#define BYTES_PER_PIXEL 3
#define STRIDE (((PIC_W * BYTES_PER_PIXEL + 3) / 4) * 4)  // 每行288字节

static int w, h;  // Screen size

#define KEYNAME(key) \
  [AM_KEY_##key] = #key,
static const char *key_names[] = { AM_KEYS(KEYNAME) };

static inline void puts(const char *s) {
  for (; *s; s++) putch(*s);
}

void print_key() {
  AM_INPUT_KEYBRD_T event = { .keycode = AM_KEY_NONE };
  ioe_read(AM_INPUT_KEYBRD, &event);
  if (event.keycode != AM_KEY_NONE && event.keydown) {
    puts("Key pressed: ");

    const char *key_name = key_names[event.keycode];
    puts(key_name);

    if (strcmp(key_name, "ESCAPE") == 0) {
      halt(0);
    }
    puts("\n");
  }
}

static void draw_tile(int x, int y, int w, int h, uint32_t color) {
  uint32_t pixels[w * h];
  AM_GPU_FBDRAW_T event = {
    .x = x, .y = y, .w = w, .h = h, .sync = 1,
    .pixels = pixels,
  };
  for (int i = 0; i < w * h; i++) {
    pixels[i] = color;
  }
  ioe_write(AM_GPU_FBDRAW, &event);
}

void splash() {
  AM_GPU_CONFIG_T info = {0};
  ioe_read(AM_GPU_CONFIG, &info);
  w = info.width;
  h = info.height;

  for (int x = 0; x * SIDE <= w; x ++) {
    for (int y = 0; y * SIDE <= h; y++) {
      if ((x & 1) ^ (y & 1)) {
        draw_tile(x * SIDE, y * SIDE, SIDE, SIDE, 0xffffff);
      }
    }
  }
}

// 从 BMP 像素数据获取颜色（BGR → RGB）
static uint32_t get_bmp_color(int x, int y) {
    const unsigned char* pixels = fun_bmp + BMP_HEADER_SIZE;
    // BMP 是倒序存储的
    int row = PIC_H - 1 - y;
    int offset = row * STRIDE + x * BYTES_PER_PIXEL;
    
    unsigned char b = pixels[offset];
    unsigned char g = pixels[offset + 1];
    unsigned char r = pixels[offset + 2];
    
    return (r << 16) | (g << 8) | b;
}

void draw_pic() {
    AM_GPU_CONFIG_T info = {0};
    ioe_read(AM_GPU_CONFIG, &info);
    w = info.width;
    h = info.height;

    // 如果屏幕比图片大，就居中显示
    int start_x = (w - PIC_W) / 2;
    int start_y = (h - PIC_H) / 2;
    
    // 如果屏幕比图片小，就只显示左上角部分
    if (start_x < 0) start_x = 0;
    if (start_y < 0) start_y = 0;

    // 绘制方法1：直接绘制原图片（可能会超出屏幕范围）
    
    // int draw_w = (PIC_W < w) ? PIC_W : w;
    // int draw_h = (PIC_H < h) ? PIC_H : h;

    // 使用 draw_tile 逐像素绘制（会慢，但不改动基础函数）
    // for (int y = 0; y < draw_h; y++) {
    //     for (int x = 0; x < draw_w; x++) {
    //         uint32_t color = get_bmp_color(x, y);
    //         draw_tile(start_x + x, start_y + y, 1, 1, color);
    //     }
    // }

    // 绘制方法2：缩放图片以适应屏幕

    for (int y = 0; y < h; y++) {
      for (int x = 0; x < w; x++) {
        // 计算对应的 BMP 像素坐标
        int bmp_x = PIC_W * x / w;
        int bmp_y = PIC_H * y / h;
        uint32_t color = get_bmp_color(bmp_x, bmp_y);
        draw_tile(x, y, 1, 1, color);
      }
    }

}

// Operating system is a C program!
int main(const char *args) {
    ioe_init();

    puts("mainargs = \"");
    puts(args);
    puts("\"\n");

    draw_pic();  // 显示图片

    puts("Press any key to see its key code...\n");
    while (1) {
        print_key();
    }
    return 0;
}