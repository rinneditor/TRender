#include "model.h"
#include "our_gl.h"
#include "tgaimage.h"
#include <algorithm>
#include <filesystem>
#include <random>
using namespace std;

constexpr int width = 800;
constexpr int height = 800;
constexpr int samples = 128;
constexpr double ao_radius = 0.05;

extern mat<4, 4> ModelView, Viewport, Perspective;
extern vector<double> ZBuffer;

struct ToonShader : public IShader {
    vec4 color;
    const Model &model;
    vec4 l;
    vec4 nors[3];
    ToonShader(const Model &m, const vec3 &light, vec4 color) : model(m),color(color) {
        l = normalized(ModelView * vec4{light.x, light.y, light.z, 0.});
    }
    virtual vec4 vertex(const int face, const int vert) {
        vec4 v = model.vert(face, vert);

        nors[vert] = ModelView.invert_transpose() * model.normal(face, vert);
        return Perspective * ModelView * v;
    }
    virtual pair<bool, TGAColor> fragment(const vec3 bar) const {
        vec4 n = normalized(nors[0] * bar.x + nors[1] * bar.y + nors[2] * bar.z);
        const double diffuse = std::max(0.0, n * l);
        double intensity = .15 + diffuse;
        if (intensity > .66)
            intensity = 1;
        else if (intensity > .33)
            intensity = .66;
        else
            intensity = .33;

        TGAColor g_c{};
        for (int i = 0; i < 3; i++) {
            const double value = color[i] * intensity;
            g_c[i] = static_cast<std::uint8_t>(std::clamp(value, 0.0, 255.0));
        }
        return {false, g_c};
    }
};

int main(int argc, char **argv) {
    TGAImage framebuffer(width, height, TGAImage::RGB, {177, 195, 209, 255});
    vector<string> filenames = {"../Object/diablo3_pose/diablo3_pose.obj", "../Object/floor.obj"};
    constexpr vec3 eye{-1, 0, 2};
    constexpr vec3 center{0, 0, 0};
    constexpr vec3 light{1, 1, 1};
    constexpr vec3 up{0, 1, 0};
    constexpr vec4 colors[] = {{22 * 4, 56 * 4, 147 * 4, 255}, {123, 98, 88, 255}};
    filesystem::create_directories("Bonustoonshading");
    vector<Model> models;
    for (const auto &fn : filenames)
        models.emplace_back(fn);
    { // 相机渲染
        lookat(eye, center, up);
        perspective(norm(eye - center));
        viewport(width / 16, height / 16, width * 7 / 8, height * 7 / 8);
        zbuffer(width, height);
        for (int i = 0; i < models.size(); ++i) {
            ToonShader shader(models[i], light, colors[i % 2]);
            for (int j = 0; j < models[i].nfaces(); j++) {
                Triangle clip;
                for (int k = 0; k < 3; k++)
                    clip[k] = shader.vertex(j, k);
                rasterize(clip, shader, framebuffer);
            }
        }
    }
    framebuffer.write_tga_file("Bonustoonshading/diablo3.tga");

    {
        constexpr double threshold = .15;
        for (int y = 1; y < framebuffer.height() - 1; ++y) {
            for (int x = 1; x < framebuffer.width() - 1; ++x) {
                vec2 sum;
                for (int j = -1; j <= 1; ++j) {
                    for (int i = -1; i <= 1; ++i) {
                        double depth = ZBuffer[(x + i) + (y + j) * width];

                        if (!std::isfinite(depth))
                            depth = -1.0;
                        constexpr int Gx[3][3] = {{-1, 0, 1}, {-2, 0, 2}, {-1, 0, 1}};
                        constexpr int Gy[3][3] = {{-1, -2, -1}, {0, 0, 0}, {1, 2, 1}};
                        sum = sum + vec2{Gx[j + 1][i + 1] * depth, Gy[j + 1][i + 1] * depth};
                    }
                }
                if (norm(sum) > threshold)
                    framebuffer.set(x, y, TGAColor{0, 0, 0, 255});
            }
        }
        framebuffer.write_tga_file("Bonustoonshading/diablo3_edges.tga");
    }
    return 0;
}
