#include "tgaimage.h"
#include "model.h"
#include <cmath>
#include <algorithm>
#include <limits>
#include <vector>

// 画布大小
constexpr int width = 800;
constexpr int height = 800;

double edge(
    int ax, int ay,
    int bx, int by,
    int px, int py)
{
    return (bx - ax) * (py - ay) - (by - ay) * (px - ax);
}
// 按Y轴旋转
vec3 roty(vec3 v)
{
    // 旋转角度
    const double a = M_PI / 6;
    // 旋转矩阵
    const mat<3, 3> Ry = {{{std::cos(a), 0, std::sin(a)},
                           {0, 1, 0},
                           {-std::sin(a), 0, std::cos(a)}}};
    return Ry * v;
}
// 透视投影
vec3 persp(vec3 v)
{
    constexpr double c = 3.;
    // 截距定理
    return v / (1 - v.z / c);
}

void triangle(
    TGAImage &framebuffer,
    std::vector<double> &zbuffer,
    int x0, int y0, double z0,
    int x1, int y1, double z1,
    int x2, int y2, double z2,
    TGAColor color)
{
    // 计算三角形的边界框
    int minX = std::min({x0, x1, x2});
    int maxX = std::max({x0, x1, x2});
    int minY = std::min({y0, y1, y2});
    int maxY = std::max({y0, y1, y2});

    minX = std::max(minX, 0);
    maxX = std::min(maxX, width - 1);
    minY = std::max(minY, 0);
    maxY = std::min(maxY, height - 1);

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

            // 使用浮点数保存真实深度，不再限制到 0～255
            double z = alpha * z0 + beta * z1 + gamma * z2;
            int index = x + y * width;

            if (z < zbuffer[index])
                continue; // 深度小于 z-buffer 中的深度，说明该点被遮挡

            zbuffer[index] = z;
            framebuffer.set(x, y, color);
        }
    }
}
// 缩放
std::tuple<int, int, double> project(vec3 v)
{
    return {static_cast<int>((v.x + 1.) * width / 2),
            static_cast<int>((v.y + 1.) * height / 2),
            (v.z + 1.) * 255. / 2};
}

// 隐藏面消除
int main(int argc, char **argv)
{
    // 创建画布
    TGAImage framebuffer(width, height, TGAImage::RGB);
    // vector(元素数量, 每个元素的初始值);
    std::vector<double> zbuffer(
        width * height,
        // 负无穷大
        -std::numeric_limits<double>::infinity());
    // 创建模型
    Model model("/Users/meryrinne/Documents/TRender/Object/diablo3_pose/diablo3_pose.obj");
    // 遍历模型的每一条边
    for (int i = 0; i < model.nfaces(); i++)
    {
        const std::vector<int> &face = model.face(i);
        int v0 = face[0];
        int v1 = face[1];
        int v2 = face[2];
        // 获取顶点坐标
        vec3 p0 = model.vert(v0);
        vec3 p1 = model.vert(v1);
        vec3 p2 = model.vert(v2);
        TGAColor rnd;
        for (int c = 0; c < 3; c++)
            rnd[c] = std::rand() % 255;
        // 旋转顶点坐标
        auto [x0, y0, z0] = project(persp(roty(p0)));
        auto [x1, y1, z1] = project(persp(roty(p1)));
        auto [x2, y2, z2] = project(persp(roty(p2)));
        triangle(framebuffer, zbuffer, x0, y0, z0, x1, y1, z1, x2, y2, z2, rnd);
    }

    // 彩色图使用 framebuffer；深度数据另外归一化为可视化图片
    double minDepth = std::numeric_limits<double>::infinity();
    double maxDepth = -std::numeric_limits<double>::infinity();

    for (double z : zbuffer)
    {
        if (!std::isfinite(z))
            continue;
        minDepth = std::min(minDepth, z);
        maxDepth = std::max(maxDepth, z);
    }
    // 归一化深度数据到 0～255
    TGAImage depthImage(width, height, TGAImage::RGB);
    if (minDepth <= maxDepth)
    {
        double depthRange = maxDepth - minDepth;
        for (int y = 0; y < height; ++y)
        {
            for (int x = 0; x < width; ++x)
            {
                double z = zbuffer[x + y * width];
                // 如果深度值不是有限数，则跳过该像素
                if (!std::isfinite(z))
                    continue;
                // 归一化深度值到 [0, 1] 范围
                double normalized = depthRange > 0
                                        ? (z - minDepth) / depthRange
                                        : 0.0;

                std::uint8_t gray = static_cast<std::uint8_t>(
                    std::clamp(std::round(normalized * 255.0), 0.0, 255.0));

                depthImage.set(x, y, TGAColor{gray, gray, gray, 255});
            }
        }
    }

    framebuffer.write_tga_file("Crude/diablo3_pr.tga");
    depthImage.write_tga_file("Crude/diablo3_pr_zbuffer.tga");
    return 0;
}
