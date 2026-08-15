#include "tgaimage.h"

// 自定义的颜色类
constexpr TGAColor white = {255, 255, 255, 255}; // attention, BGRA order
constexpr TGAColor green = {0, 255, 0, 255};
constexpr TGAColor red = {0, 0, 255, 255};
constexpr TGAColor blue = {255, 128, 64, 255};
constexpr TGAColor yellow = {0, 200, 255, 255};

// 会出现断连的线段，蓝色段消失了
// 原因是因为斜率大导致
void line1(TGAImage& framebuffer, int x0, int y0, int x1, int y1, TGAColor color){
    // y = kx + b
    // k = (y1 - y0) / (x1 - x0)
    // b = y0 - k * x0
    for (int x = x0; x < x1; x++) {
        int y = (y1 - y0) * (x - x0) / (x1 - x0) + y0;
        framebuffer.set(x, y, color);
    }
}

// 解决蓝色险段消失问题
void line2(TGAImage& framebuffer, int x0, int y0, int x1, int y1, TGAColor color){
    if (x0 > x1) {
        std::swap(x0, x1);
        std::swap(y0, y1);
    }
    for (int x = x0; x < x1; x++)
    {
        int y = (y1 - y0) * (x - x0) / (x1 - x0) + y0;
        framebuffer.set(x, y, color);
    }
}

// 解决断连问题
void line3(TGAImage& framebuffer, int x0, int y0, int x1, int y1, TGAColor color){
    // 斜率大于1的情况，交换x和y
    bool steep = false;
    if (std::abs(x0 - x1) < std::abs(y0 - y1)) {
        std::swap(x0, y0);
        std::swap(x1, y1);
        steep = true;
    }
    if (x0 > x1) {
        std::swap(x0, x1);
        std::swap(y0, y1);
    }
    for (int x = x0; x < x1; x++)
    {
        float t = (float)(x - x0) / (float)(x1 - x0);
        // int y = y0 * (1. - t) + y1 * t;
        int y = std::round( y0 + (y1-y0)*t );
        if (steep) {
            framebuffer.set(y, x, color);
        } else {
            framebuffer.set(x, y, color);
        }
    }
}

// 优化问题
void line(TGAImage &framebuffer, int ax, int ay, int bx, int by,  TGAColor color)
{
    bool steep = std::abs(ax - bx) < std::abs(ay - by);
    if (steep)
    { // if the line is steep, we transpose the image
        std::swap(ax, ay);
        std::swap(bx, by);
    }
    if (ax > bx)
    { // make it left−to−right
        std::swap(ax, bx);
        std::swap(ay, by);
    }
    int y = ay;
    int ierror = 0;
    for (int x = ax; x <= bx; x++)
    {
        if (steep) // if transposed, de−transpose
            framebuffer.set(y, x, color);
        else
            framebuffer.set(x, y, color);
        // 误差累计2*dy
        ierror += 2 * std::abs(by - ay);
        if (ierror > bx - ax)
        {
            y += by > ay ? 1 : -1;
            ierror -= 2 * (bx - ax);
        }
    }
}

int main(int argc, char **argv)
{
    // 画布大小
    constexpr int width = 64;
    constexpr int height = 64;
    // 创建画布
    TGAImage framebuffer(width, height, TGAImage::RGB);
    // 3个点的坐标
    int ax = 7, ay = 3;
    int bx = 12, by = 37;
    int cx = 62, cy = 53;

    // 画出3条线段
    line(framebuffer, ax, ay, bx, by, red);
    line(framebuffer, bx, by, cx, cy, green);
    line(framebuffer, cx, cy, ax, ay, blue);

    // 画出3个点
    framebuffer.set(ax, ay, white);
    framebuffer.set(bx, by, white);
    framebuffer.set(cx, cy, white);

    framebuffer.write_tga_file("Bresenham/line.tga");
    return 0;
}