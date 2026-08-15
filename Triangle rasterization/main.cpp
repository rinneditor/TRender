#include "tgaimage.h"
#include "model.h"
#include <cmath>

const TGAColor white = {255, 255, 255, 255}; // attention, BGRA order
const TGAColor green = {0, 255, 0, 255};
const TGAColor red = {0, 0, 255, 255};
const TGAColor blue = {255, 128, 64, 255};
const TGAColor yellow = {0, 200, 255, 255};

const int width = 800;
const int height = 800;

// 水平线截取法（行扫描）
void triangle1(TGAImage &framebuffer, int x0, int y0, int x1, int y1, int x2, int y2, TGAColor color){
    // 先排序
    if (y0 > y1) { std::swap(x0, x1); std::swap(y0, y1); }
    if (y0 > y2) { std::swap(x0, x2); std::swap(y0, y2); }
    if (y1 > y2) { std::swap(x1, x2); std::swap(y1, y2); }
    int total_height = y2 - y0;
    if (y0 != y1)
    {
        int segment_height = y1 - y0;
        for (int y = y0; y <= y1; y++)
        { // y0 to y1
            int a = x0 + ((x2 - x0) * (y - y0)) / total_height;
            int b = x0 + ((x1 - x0) * (y - y0)) / segment_height;
            for (int x = std::min(a, b); x < std::max(a, b); x++)
                framebuffer.set(x, y, color);
        }
    }
    if (y1 != y2)
    {
        int segment_height = y2 - y1;
        for (int y = y1; y <= y2; y++)
        { // y1 to y2
            int a = x0 + ((x2 - x0) * (y - y0)) / total_height;
            int b = x1 + ((x2 - x1) * (y - y1)) / segment_height;
            for (int x = std::min(a, b); x < std::max(a, b); x++)
                framebuffer.set(x, y, color);
        }
    }
}
// 叉积法
double cross(int x0, int y0, int x1, int y1)
{
    return x0 * y1 - y0 * x1;
}
// 面积法（向量叉积）
double edge(
    int ax, int ay,
    int bx, int by,
    int px, int py)
{
    return (bx - ax) * (py - ay) - (by - ay) * (px - ax);
}
// 重心坐标法（公式定义1）
void triangle2(TGAImage &framebuffer, int x0, int y0, int x1, int y1, int x2, int y2, TGAColor color)
{
    // 计算三角形的边界框
    int minX = std::min({x0, x1, x2});
    int maxX = std::max({x0, x1, x2});
    int minY = std::min({y0, y1, y2});
    int maxY = std::max({y0, y1, y2});
    // 遍历边界框内的每个像素
    for (int x = minX; x <= maxX; ++x) {
        for (int y = minY; y <= maxY; ++y) {
            // 使用重心坐标法判断点是否在三角形内
            double gamma = cross(x1 - x0, y1 - y0, x - x0, y - y0) /
                           cross(x1 - x0, y1 - y0, x2 - x0, y2 - y0);
            double alpha = cross(x2 - x1, y2 - y1, x - x1, y - y1) /
                           cross(x2 - x1, y2 - y1, x0 - x1, y0 - y1);
            double beta = 1.0 - alpha - gamma;
            if (gamma < 0 || alpha < 0 || beta < 0) {
                continue; // 点在三角形外
            }
            framebuffer.set(x, y, color);
        }
    }
}
// 重心坐标法（公式定义2）
void triangle3(TGAImage &framebuffer, int x0, int y0, int x1, int y1, int x2, int y2, TGAColor color)
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
            if (gamma < 0 || alpha < 0 || beta < 0) continue; // 点在三角形外
            framebuffer.set(x, y, color);
        }
    }
}

void triangle(TGAImage &framebuffer, int x0, int y0, int x1, int y1, int x2, int y2, TGAColor color)
{
    // 计算三角形的边界框
    int minX = std::min({x0, x1, x2});
    int maxX = std::max({x0, x1, x2});
    int minY = std::min({y0, y1, y2});
    int maxY = std::max({y0, y1, y2});
    double area = edge(x0, y0, x1, y1, x2, y2);
    if (area < 1)
        return;
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
            if (gamma < 0 || alpha < 0 || beta < 0)
                continue; // 点在三角形外
            framebuffer.set(x, y, color);
        }
    }
}
// 缩放
std::tuple<int, int> project(vec4 v)
{
    return {(v.x + 1.) * width / 2,
            (v.y + 1.) * height / 2};
}

int main(int argc, char **argv)
{
    // 创建画布
    TGAImage framebuffer(width, height, TGAImage::RGB);
    // 创建模型
    Model model("../Object/diablo3_pose/diablo3_pose.obj");
    // 遍历模型的每一条边
    for (int i = 0; i < model.nfaces(); i++)
    {
        const std::vector<int> &face = model.face(i);
        int v0 = face[0];
        int v1 = face[1];
        int v2 = face[2];
        // 获取顶点坐标
        vec4 p0 = model.vert(v0);
        vec4 p1 = model.vert(v1);
        vec4 p2 = model.vert(v2);
        TGAColor rnd;
        for (int c = 0; c < 3; c++)
            rnd[c] = std::rand() % 255;
        auto [x0, y0] = project(p0);
        auto [x1, y1] = project(p1);
        auto [x2, y2] = project(p2);
        triangle(framebuffer, x0, y0, x1, y1, x2, y2, rnd);
    }
    framebuffer.write_tga_file("Triangle/diablo3.tga");
    return 0;
}
