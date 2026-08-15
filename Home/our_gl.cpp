#include "our_gl.h"
#include <algorithm>

mat<4, 4> ModelView, Viewport, Perspective;
std::vector<double> ZBuffer;

void viewport(const int x, const int y, const int w, const int h)
{
    Viewport = {{{w / 2., 0, 0, x + w / 2.},
                 {0, h / 2., 0, y + h / 2.},
                 {0, 0, 1, 0},
                 {0, 0, 0, 1}}};
}
void perspective(const double f)
{
    Perspective = {{{1, 0, 0, 0},
                    {0, 1, 0, 0},
                    {0, 0, 1, 0},
                    {0, 0, -1 / f, 1}}};
}
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
void zbuffer(const int width, const int height)
{
    ZBuffer = std::vector<double>(width * height, -std::numeric_limits<double>::infinity());
}
void rasterize(const Triangle &clip, const IShader &shader, TGAImage &framebuffer)
{
    // 透视除法
    vec4 ndc[3] = {
        clip[0] / clip[0].w,
        clip[1] / clip[1].w,
        clip[2] / clip[2].w};
    // 视口变换
    vec2 screen[3] = {
        (Viewport * ndc[0]).xy(),
        (Viewport * ndc[1]).xy(),
        (Viewport * ndc[2]).xy()};
    mat<3,3> ABC = {{
        {screen[0].x, screen[0].y, 1.0},
        {screen[1].x, screen[1].y, 1.0},
        {screen[2].x, screen[2].y, 1.0}}};
    if (ABC.det() < 1)
        return;
    // 计算三角形的边界框
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
    // 光栅化
    for (int x = minX; x <= maxX; ++x)
    {
        for (int y = minY; y <= maxY; ++y)
        {
            // 矩阵运算求重心坐标
            const vec3 bc = barycentricMatrix *
                vec3{static_cast<double>(x), static_cast<double>(y), 1.0};

            if (bc.x < 0 || bc.y < 0 || bc.z < 0)
                continue;
            
            const double z = bc * vec3{ndc[0].z, ndc[1].z, ndc[2].z};
            const int index = x + y * framebuffer.width();

            if (z <= ZBuffer[index])
                continue;

            ZBuffer[index] = z;
            // 调用片段着色器
            vec3 bar = {
                bc.x / clip[0].w,
                bc.y / clip[1].w,
                bc.z / clip[2].w};
            bar = bar / (bar.x + bar.y + bar.z);
            auto [discard, color] = shader.fragment(bar);
            if (!discard)
                framebuffer.set(x, y, color);
        }
    }
}