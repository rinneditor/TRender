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

struct BlankShader : public IShader {
    const Model &model;
    BlankShader(const Model &m) : model(m) {}
    virtual vec4 vertex(const int face, const int vert) {
        vec4 v = model.vert(face, vert);
        return Perspective * ModelView * v;
    }
    virtual pair<bool, TGAColor> fragment(const vec3 bar) const { return {false, TGAColor{255, 255, 255, 255}}; }
};

int main(int argc, char **argv) {
    TGAImage framebuffer(width, height, TGAImage::RGB, {177, 195, 209, 255});
    vector<string> filenames = {"../Object/diablo3_pose/diablo3_pose.obj", "../Object/floor.obj"};
    constexpr vec3 eye{-1, 0, 2};
    constexpr vec3 center{0, 0, 0};
    constexpr vec3 up{0, 1, 0};

    filesystem::create_directories("Indirect lighting");
    vector<Model> models;
    for (const auto &fn : filenames)
        models.emplace_back(fn);
    { // 相机渲染
        lookat(eye, center, up);
        perspective(norm(eye - center));
        viewport(width / 16, height / 16, width * 7 / 8, height * 7 / 8);
        zbuffer(width, height);
        for (const Model &model : models) {
            BlankShader shader(model);
            for (int i = 0; i < model.nfaces(); i++) {
                Triangle clip;
                for (int j = 0; j < 3; j++)
                    clip[j] = shader.vertex(i, j);
                rasterize(clip, shader, framebuffer);
            }
        }
    }
    const mat<4, 4> invViewport = Viewport.invert();
    mt19937 gen(random_device{}());
    uniform_real_distribution<double> distr(-ao_radius, ao_radius);
    auto smoothstep = [](double edge0, double edge1, double x) {
        double t = clamp((x - edge0) / (edge1 - edge0), 0., 1.);
        return t * t * (3 - 2 * t);
    };

    for (int y = 0; y < height; ++y) {
        for (int x = 0; x < width; ++x) {
            const int index = x + y * width;
            const double z = ZBuffer[index];
            if (!isfinite(z))
                continue;

            const vec4 fragment = invViewport * vec4{double(x), double(y), z, 1.};
            double vote = 0.0;
            int voters = 0;

            for (int sample = 0; sample < samples; ++sample) {
                const vec4 projected = Viewport * (fragment + vec4{distr(gen), distr(gen), distr(gen), 0.});
                if (abs(projected.w) < 1e-8)
                    continue;

                const vec3 p = projected.xyz() / projected.w;
                if (p.x < 0 || p.x >= width || p.y < 0 || p.y >= height)
                    continue;

                const int si = static_cast<int>(p.x) + static_cast<int>(p.y) * width;
                const double d = ZBuffer[si];

                if (z + 5.0 * ao_radius < d)
                    continue;

                ++voters;
                vote += d > p.z;
            }

            const double vb = voters > 0 ? 1.0 - vote / voters * 0.4 : 1.0;
            const double ssao = smoothstep(0.0, 1.0, vb);
            TGAColor color = framebuffer.get(x, y);
            for (int channel = 0; channel < 3; ++channel)
                color[channel] = static_cast<uint8_t>(clamp(color[channel] * ssao, 0.0, 255.0));
            framebuffer.set(x, y, color);
        }
    }

    framebuffer.write_tga_file("Indirect lighting/diablo3_ssao.tga");
    return 0;
}
