#include <iostream>
#include "tgaimage.h"
#include "model.h"
#include "our_gl.h"

extern mat<4, 4> ModelView, Perspective;
extern std::vector<double> ZBuffer;
constexpr int width = 800;
constexpr int height = 800;
// Phong着色器
struct PhongShader : public IShader
{
    const Model &model;
    vec3 l; // 光源方向
    // 三角形的三个顶点在眼坐标系下的坐标
    vec3 tri[3];

    PhongShader(const Model &m, const vec3 &light) : model(m), l(light) {
        l = normalized(ModelView*vec4{light.x, light.y, light.z, 0.}).xyz();
    }

    virtual vec4 vertex(const int face, const int vert)
    {
        const vec4 v = model.vert(face, vert); // current vertex in object coordinates
        vec4 gl_Position = ModelView * vec4{v.x, v.y, v.z, 1.};
        tri[vert] = gl_Position.xyz();    // in eye coordinates
        return Perspective * gl_Position; // in clip coordinates
    }

    std::pair<bool, TGAColor>
    fragment(const vec3 bar) const override
    {
        constexpr double ambientStrength = 0.3;
        constexpr double diffuseStrength = 0.4;
        constexpr double specularStrength = 0.9;
        constexpr double shininess = 35.0;

        // 一个三角形面的法线
        const vec3 n = normalized(
            cross(
                tri[1] - tri[0],
                tri[2] - tri[0]));

        // 漫反射
        const double diffuse =
            std::max(0.0, n * l);

        // 镜面反射
        double specular = 0.0;

        if (diffuse > 0.0)
        {
            const vec3 r = normalized(
                2.0 * n * (n * l) - l);

            // 相机空间中的观察方向为 +z
            specular = std::pow(
                std::max(0.0, r.z),
                shininess);
        }

        const double intensity = std::clamp(
            ambientStrength +
            diffuseStrength * diffuse +
            specularStrength * specular,
            0.0,
            1.0);

        const std::uint8_t gray =
            static_cast<std::uint8_t>(
                std::round(intensity * 255.0));

        return {
            false,
            TGAColor{gray, gray, gray, 255}};
    }
};

int main(int argc, char **argv)
{
    TGAImage framebuffer(width, height, TGAImage::RGB);
    std::vector<std::string> filenames = {
        "../Object/african_head/african_head.obj",
        "../Object/african_head/african_head_eye_inner.obj",
        "../Object/african_head/african_head_eye_outer.obj"};
    constexpr vec3 eye{-1, 0, 2};   // camera position
    constexpr vec3 center{0, 0, 0}; // camera direction
    constexpr vec3 up{0, 1, 0};     // camera up vector
    constexpr vec3 l{1, 1, 1};      // light direction
    lookat(eye, center, up);
    perspective(norm(eye - center));
    viewport(width / 16, height / 16, width * 7 / 8, height * 7 / 8);
    zbuffer(width, height);
    for (const auto &fn : filenames)
    {
        Model model(fn);
        PhongShader shader(model,l);
        for (int i = 0; i < model.nfaces(); i++)
        {
            Triangle clip;
            for (int j = 0; j < 3; j++)
                clip[j] = shader.vertex(i, j);
            rasterize(clip, shader, framebuffer);
        }
    }
    std::filesystem::create_directories("Shading");
    framebuffer.write_tga_file("Shading/head.tga");
}
