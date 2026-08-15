#include <iostream>
#include "tgaimage.h"
#include "model.h"
#include "our_gl.h"

extern mat<4, 4> ModelView, Perspective;
extern std::vector<double> ZBuffer;
constexpr int width = 800;
constexpr int height = 800;
// 随机颜色渲染器
struct RandomShader : public IShader
{
    const Model &model;
    TGAColor color{};
    vec3 light_dir;
    vec3 tri[3];

    RandomShader(const Model &m) : model(m) {}

    virtual vec4 vertex(const int face, const int vert)
    {
        const vec4 v = model.vert(face, vert); // current vertex in object coordinates
        vec4 gl_Position = ModelView * vec4{v.x, v.y, v.z, 1.};
        tri[vert] = gl_Position.xyz();    // in eye coordinates
        return Perspective * gl_Position; // in clip coordinates
    }

    std::pair<bool, TGAColor> fragment(const vec3 bar) const override
    {
        return {false, color};
    }
};
struct DepthShader : public IShader
{
    const Model &model;
    vec3 tri[3];
    double depth[3];

    DepthShader(const Model &m) : model(m) {}

    vec4 vertex(const int face, const int vert)
    {
        vec4 v = model.vert(face, vert);

        vec4 eye_position =
            ModelView * v;

        tri[vert] = eye_position.xyz();

        return Perspective * eye_position;
    }

    std::pair<bool, TGAColor>
    fragment(const vec3 bar) const override
    {
        double minDepth = std::numeric_limits<double>::infinity();
        double maxDepth = -std::numeric_limits<double>::infinity();
        for (const double z : ZBuffer)
        {
            if (!std::isfinite(z))
                continue;
            minDepth = std::min(minDepth, z);
            maxDepth = std::max(maxDepth, z);
        }
        if (minDepth > maxDepth)
            return {true, TGAColor{0, 0, 0, 255}};

        const double depthRange = maxDepth - minDepth;
        const double z = ZBuffer[bar.x + bar.y * width];
        if (!std::isfinite(z))
            return {true, TGAColor{0, 0, 0, 255}};

        const double normalized = depthRange > 0
                                      ? (z - minDepth) / depthRange
                                      : 0.0;
        const std::uint8_t gray = static_cast<std::uint8_t>(
            std::clamp(std::round(normalized * 255.0), 0.0, 255.0));
        return {false, TGAColor{gray, gray, gray, 255}};
    }
};

int main(int argc, char **argv)
{
    TGAImage framebuffer(width, height, TGAImage::RGB);
    Model model("../Object/diablo3_pose/diablo3_pose.obj");
    constexpr vec3 eye{-1, 0, 2};   // camera position
    constexpr vec3 center{0, 0, 0}; // camera direction
    constexpr vec3 up{0, 1, 0};     // camera up vector
    lookat(eye, center, up);
    perspective(norm(eye - center));
    viewport(width / 16, height / 16, width * 7 / 8, height * 7 / 8);
    zbuffer(width, height);

    RandomShader shader(model);
    shader.light_dir = {1, 1, 1};

    for (int i = 0; i < model.nfaces(); i++)
    {
        Triangle clip;
        for (int j = 0; j < 3; j++)
            clip[j] = shader.vertex(i, j);
        // 当前三角面使用的颜色
        shader.color = {
            static_cast<std::uint8_t>(std::rand() % 256),
            static_cast<std::uint8_t>(std::rand() % 256),
            static_cast<std::uint8_t>(std::rand() % 256),
            255};
        rasterize(clip, shader, framebuffer);
    }
    std::filesystem::create_directories("Shading");
    framebuffer.write_tga_file("Shading/diablo3_color.tga");
}
