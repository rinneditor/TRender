#include "tgaimage.h"
#include "model.h"

const TGAColor white = {255, 255, 255, 255}; // attention, BGRA order
const TGAColor green = {0, 255, 0, 255};
const TGAColor red = {0, 0, 255, 255};
const TGAColor blue = {255, 128, 64, 255};
const TGAColor yellow = {0, 200, 255, 255};

const int width = 128;
const int height = 128;

double edge(
    int ax, int ay,
    int bx, int by,
    int px, int py)
{
    return (bx - ax) * (py - ay) - (by - ay) * (px - ax);
}

void triangle(TGAImage &framebuffer, int x0, int y0, int x1, int y1, int x2, int y2)
{
    // 计算三角形的边界框
    int minX = std::min({x0, x1, x2});
    int maxX = std::max({x0, x1, x2});
    int minY = std::min({y0, y1, y2});
    int maxY = std::max({y0, y1, y2});
    double area = edge(x0, y0, x1, y1, x2, y2);
    // 遍历边界框内的每个像素
    for (int x = minX; x <= maxX; ++x)
    {
        for (int y = minY; y <= maxY; ++y)
        {
            // 使用重心坐标法判断点是否在三角形内
            double alpha = edge(x1, y1, x2, y2, x, y) /
                           area;
            double beta = edge(x2, y2, x0, y0, x, y) /
                          area;
            double gamma = edge(x0, y0, x1, y1, x, y) /
                           area;
            TGAColor color{};

            for (int i = 0; i < 4; ++i)
            {
                double value =
                    red.bgra[i] * alpha +
                    green.bgra[i] * beta +
                    blue.bgra[i] * gamma;

                color.bgra[i] = static_cast<std::uint8_t>(
                    std::clamp(std::round(value), 0.0, 255.0));
            }
            if (gamma < 0 || alpha < 0 || beta < 0)
                continue; // 点在三角形外

            framebuffer.set(x, y, color);
        }
    }
}

int main(int argc, char** argv) {
    TGAImage framebuffer(width, height, TGAImage::RGB);
    triangle(framebuffer,7, 45, 35, 100, 45, 60);
    triangle(framebuffer,120, 35, 90, 5, 45, 110);
    triangle(framebuffer,115, 83, 80, 90, 85, 120);
    framebuffer.write_tga_file("Triangle/triangle.tga");
    return 0;
}