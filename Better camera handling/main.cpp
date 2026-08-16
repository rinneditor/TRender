#include "model.h"
#include "tgaimage.h"

#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <filesystem>
#include <limits>
#include <vector>

constexpr int width = 800;
constexpr int height = 800;

mat<4, 4> ModelView, Viewport, Perspective;

// 把 [-1,1] 范围映射到画布中指定的视口矩形。
void viewport(const int x, const int y, const int w, const int h)
{
    Viewport = {{{w / 2., 0, 0, x + w / 2.},
                 {0, h / 2., 0, y + h / 2.},
                 {0, 0, 1, 0},
                 {0, 0, 0, 1}}};
}

// 透视矩阵会使齐次坐标 w 变成 1-z/f，随后通过除以 w 产生近大远小。
void perspective(const double f)
{
    Perspective = {{{1, 0, 0, 0},
                    {0, 1, 0, 0},
                    {0, 0, 1, 0},
                    {0, 0, -1 / f, 1}}};
}

// 建立相机坐标系：eye 是相机位置，center 是相机对准的点。
void lookat(const vec3 eye, const vec3 center, const vec3 up)
{
    vec3 n = normalized(eye - center);
    vec3 l = normalized(cross(up, n));
    vec3 m = normalized(cross(n, l));

    ModelView =
        mat<4, 4>{{{l.x, l.y, l.z, 0},
                   {m.x, m.y, m.z, 0},
                   {n.x, n.y, n.z, 0},
                   {0, 0, 0, 1}}} *
        mat<4, 4>{{{1, 0, 0, -center.x},
                   {0, 1, 0, -center.y},
                   {0, 0, 1, -center.z},
                   {0, 0, 0, 1}}};
}

// clip 中保留三个顶点的齐次坐标；透视除法和视口变换在光栅化时完成。
void rasterize(
    const vec4 clip[3],
    std::vector<double> &zbuffer,
    TGAImage &framebuffer,
    const TGAColor color)
{
    if (std::abs(clip[0].w) < 1e-12 ||
        std::abs(clip[1].w) < 1e-12 ||
        std::abs(clip[2].w) < 1e-12)
        return;

    const vec4 ndc[3] = {
        clip[0] / clip[0].w,
        clip[1] / clip[1].w,
        clip[2] / clip[2].w};

    const vec3 screen[3] = {
        (Viewport * ndc[0]).xyz(),
        (Viewport * ndc[1]).xyz(),
        (Viewport * ndc[2]).xyz()};

    const mat<3, 3> ABC = {{{screen[0].x, screen[0].y, 1.0},
                            {screen[1].x, screen[1].y, 1.0},
                            {screen[2].x, screen[2].y, 1.0}}};

    if (ABC.det() < 1)
        return; // 背面剔除，并丢弃面积不足一个像素的三角形

    const auto [bbminx, bbmaxx] = std::minmax(
        {screen[0].x, screen[1].x, screen[2].x});
    const auto [bbminy, bbmaxy] = std::minmax(
        {screen[0].y, screen[1].y, screen[2].y});

    const int minX = std::max(static_cast<int>(bbminx), 0);
    const int maxX = std::min(static_cast<int>(bbmaxx), framebuffer.width() - 1);
    const int minY = std::max(static_cast<int>(bbminy), 0);
    const int maxY = std::min(static_cast<int>(bbmaxy), framebuffer.height() - 1);

    // ABC 对一个三角形不变，所以只求一次逆转置。
    const mat<3, 3> barycentricMatrix = ABC.invert_transpose();

    for (int x = minX; x <= maxX; ++x)
    {
        for (int y = minY; y <= maxY; ++y)
        {
            const vec3 bc = barycentricMatrix *
                vec3{static_cast<double>(x), static_cast<double>(y), 1.0};

            if (bc.x < 0 || bc.y < 0 || bc.z < 0)
                continue;

            const double z = bc * vec3{ndc[0].z, ndc[1].z, ndc[2].z};
            const int index = x + y * framebuffer.width();

            if (z <= zbuffer[index])
                continue;

            zbuffer[index] = z;
            framebuffer.set(x, y, color);
        }
    }
}

TGAImage visualize_depth(const std::vector<double> &zbuffer)
{
    double minDepth = std::numeric_limits<double>::infinity();
    double maxDepth = -std::numeric_limits<double>::infinity();

    for (const double z : zbuffer)
    {
        if (!std::isfinite(z))
            continue;
        minDepth = std::min(minDepth, z);
        maxDepth = std::max(maxDepth, z);
    }

    TGAImage depthImage(width, height, TGAImage::RGB);
    if (minDepth > maxDepth)
        return depthImage;

    const double depthRange = maxDepth - minDepth;
    for (int y = 0; y < height; ++y)
    {
        for (int x = 0; x < width; ++x)
        {
            const double z = zbuffer[x + y * width];
            if (!std::isfinite(z))
                continue;

            const double normalized = depthRange > 0
                ? (z - minDepth) / depthRange
                : 0.0;

            const std::uint8_t gray = static_cast<std::uint8_t>(
                std::clamp(std::round(normalized * 255.0), 0.0, 255.0));

            depthImage.set(x, y, TGAColor{gray, gray, gray, 255});
        }
    }

    return depthImage;
}

int main()
{
    constexpr vec3 eye{-1, 0, 2};
    constexpr vec3 center{0, 0, 0};
    constexpr vec3 up{0, 1, 0};

    lookat(eye, center, up);
    perspective(norm(eye - center));
    // 设置视口为画布的中心区域，留出边框
    viewport(width / 16, height / 16, width * 7 / 8, height * 7 / 8);

    TGAImage framebuffer(width, height, TGAImage::RGB);
    std::vector<double> zbuffer(
        width * height,
        -std::numeric_limits<double>::infinity());

    Model model(
        "../Object/diablo3_pose/diablo3_pose.obj");

    for (int i = 0; i < model.nfaces(); ++i)
    {
        vec4 clip[3];
        for (int d : {0, 1, 2})
        {
            const vec4 v = model.vert(i, d);
            clip[d] = Perspective * ModelView * v;
        }

        TGAColor randomColor{};
        for (int channel = 0; channel < 3; ++channel)
            randomColor[channel] = static_cast<std::uint8_t>(std::rand() % 256);
        randomColor[3] = 255;

        rasterize(clip, zbuffer, framebuffer, randomColor);
    }

    const TGAImage depthImage = visualize_depth(zbuffer);

    std::filesystem::create_directories("Better");
    framebuffer.write_tga_file("Better/diablo3_color.tga");
    depthImage.write_tga_file("Better/diablo3_depth.tga");
    return 0;
}
