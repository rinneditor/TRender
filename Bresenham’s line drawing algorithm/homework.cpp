#include "tgaimage.h"
#include "model.h"

// 自定义的颜色类
constexpr TGAColor white = {255, 255, 255, 255}; // attention, BGRA order
constexpr TGAColor green = {0, 255, 0, 255};
constexpr TGAColor red = {0, 0, 255, 255};
constexpr TGAColor blue = {255, 128, 64, 255};
constexpr TGAColor yellow = {0, 200, 255, 255};

// 解决断连问题
void line(TGAImage &framebuffer, int x0, int y0, int x1, int y1, TGAColor color)
{
    // 斜率大于1的情况，交换x和y
    bool steep = false;
    if (std::abs(x0 - x1) < std::abs(y0 - y1))
    {
        std::swap(x0, y0);
        std::swap(x1, y1);
        steep = true;
    }
    if (x0 > x1)
    {
        std::swap(x0, x1);
        std::swap(y0, y1);
    }
    for (int x = x0; x < x1; x++)
    {
        float t = (float)(x - x0) / (float)(x1 - x0);
        int y = y0 * (1. - t) + y1 * t;
        if (steep)
        {
            framebuffer.set(y, x, color);
        }
        else
        {
            framebuffer.set(x, y, color);
        }
    }
}

int main(int argc, char **argv)
{
    // 画布大小
    constexpr int width = 800;
    constexpr int height = 800;
    // 创建画布
    TGAImage framebuffer(width, height, TGAImage::RGB);
    // 创建模型
    Model model("../Object/african_head/african_head_eye_outer.obj");
    // 遍历模型的每一条边
    for (int i = 0; i < model.nfaces(); i++){
        const std::vector<int>& face = model.face(i);
        for (int j = 0; j < face.size(); j++){
            // 获取顶点索引
            int v0 = face[j];
            int v1 = face[(j + 1) % face.size()];
            // 获取顶点坐标
            vec<3> p0 = model.vert(v0);
            vec<3> p1 = model.vert(v1);
            // 将顶点坐标映射到画布坐标系
            int x0 = (p0.x + 1.) * width / 2.;
            int y0 = (p0.y + 1.) * height / 2.;
            int x1 = (p1.x + 1.) * width / 2.;
            int y1 = (p1.y + 1.) * height / 2.;
            // 绘制线段
            line(framebuffer, x0, y0, x1, y1, red);
        }
    }
    // 绘制顶点
    for (int i = 0;i < model.nverts();i++){
        vec<3> p = model.vert(i);
        int x = (p.x + 1.) * width / 2.;
        int y = (p.y + 1.) * height / 2.;
        framebuffer.set(x, y, white);
    }
    framebuffer.write_tga_file("Bresenham/eyes_outer.tga");
    return 0;
}
