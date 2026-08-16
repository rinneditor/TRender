#include "model.h"
#include "our_gl.h"
#include "tgaimage.h"
#include <algorithm>
#include <filesystem>
#include <random>

constexpr int width = 800;
constexpr int height = 800;
constexpr int shadowmapw = 1024;
constexpr int shadowmaph = 1024;
constexpr int n = 1000;

extern mat<4, 4> ModelView, Viewport, Perspective;
extern std::vector<double> ZBuffer;

struct BlankShader : public IShader {
    const Model &model;
    BlankShader(const Model &m) : model(m) {}
    virtual vec4 vertex(const int face, const int vert) {
        vec4 v = model.vert(face, vert);
        return Perspective * ModelView * v;
    }
    virtual std::pair<bool, TGAColor> fragment(const vec3 bar) const { return {false, TGAColor{255, 255, 255, 255}}; }
};

int main(int argc, char **argv) {
    TGAImage framebuffer(width, height, TGAImage::RGB, {177, 195, 209, 255});
    TGAImage Dbuffer(width, height, TGAImage::GRAYSCALE, {0, 0, 0, 0});
    std::vector<std::string> filenames = {"../Object/diablo3_pose/diablo3_pose.obj", "../Object/floor.obj"};
    constexpr vec3 eye{-1, 0, 2};
    constexpr vec3 center{0, 0, 0};
    constexpr vec3 up{0, 1, 0};
    constexpr vec3 l{1, 1, 1};

    std::filesystem::create_directories("Indirect lighting");
    std::vector<Model> models;
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
        framebuffer.write_tga_file("Indirect lighting/diablo3.tga");
    }
    mat<4, 4> M = (Viewport * Perspective * ModelView).invert();
    std::vector<double> eBuffer = ZBuffer;
    std::vector<double> ao(width * height, 0.0);
    std::mt19937 gen(std::random_device{}());
    std::uniform_real_distribution<double> distr(0.0, 1.0);
    auto smoothstep = [](double edge0, double edge1, double x) {
        double t = std::clamp((x - edge0) / (edge1 - edge0), 0., 1.);
        return t * t * (3 - 2 * t);
    };

    {
        TGAImage trash(shadowmapw, shadowmaph, TGAImage::RGB, {177, 195, 209, 255});
        for (int i = 0; i < n; ++i) {
            double y = distr(gen);
            double theta = 2.0 * M_PI * distr(gen);
            double r = std::sqrt(1.0 - y * y);
            vec3 light = vec3{r * std::cos(theta), y, r * std::sin(theta)} * 1.5;
            lookat(light, center, up);
            perspective(norm(light - center));
            viewport(shadowmapw / 16, shadowmaph / 16, shadowmapw * 7 / 8, shadowmaph * 7 / 8);
            zbuffer(shadowmapw, shadowmaph);

            for (const Model &model : models) {
                BlankShader shader(model);
                for (int i = 0; i < model.nfaces(); i++) {
                    Triangle clip;
                    for (int j = 0; j < 3; j++)
                        clip[j] = shader.vertex(i, j);
                    rasterize(clip, shader, trash);
                }
            }
            mat<4, 4> N = Viewport * Perspective * ModelView;
            double bias = 0.03;
            for (int y = 0; y < height; y++) {
                for (int x = 0; x < width; x++) {
                    const int index = x + y * width;
                    bool lit = true;

                    // 相机没有渲染到物体的背景像素直接跳过
                    if (std::isfinite(eBuffer[index])) {
                        vec4 q = M * vec4{double(x), double(y), eBuffer[index], 1.};
                        vec4 pShadow = N * q;
                        vec3 p = pShadow.xyz() / pShadow.w;

                        bool outside = p.x < 0 || p.x >= shadowmapw || p.y < 0 || p.y >= shadowmaph;

                        if (!outside) {
                            const int si = static_cast<int>(p.x) + static_cast<int>(p.y) * shadowmapw;

                            lit = p.z > ZBuffer[si] - bias;
                        }
                    }

                    ao[index] += (double(lit) - ao[index]) / (i + 1.0);
                }
            }
        }
    }

    {
        for (int x = 0; x < width; x++) {
            for (int y = 0; y < height; y++) {
                double m = smoothstep(-1, 1, ao[x + y * width]);
                TGAColor c = framebuffer.get(x, y);
                framebuffer.set(x, y,
                                {static_cast<std::uint8_t>(c[0] * m), static_cast<std::uint8_t>(c[1] * m),
                                 static_cast<std::uint8_t>(c[2] * m), c[3]});
            }
        }
        framebuffer.write_tga_file("Indirect lighting/diablo3_ao.tga");
    }
    {
        constexpr double threshold = .15;
        for (int y = 1; y < framebuffer.height() - 1; ++y) {
            for (int x = 1; x < framebuffer.width() - 1; ++x) {
                vec2 sum;
                for (int j = -1; j <= 1; ++j) {
                    for (int i = -1; i <= 1; ++i) {
                        double depth = eBuffer[(x + i) + (y + j) * width];

                        if (!std::isfinite(depth))
                            depth = -1.0;
                        constexpr int Gx[3][3] = {{-1, 0, 1}, {-2, 0, 2}, {-1, 0, 1}};
                        constexpr int Gy[3][3] = {{-1, -2, -1}, {0, 0, 0}, {1, 2, 1}};
                        sum = sum + vec2{Gx[j + 1][i + 1] * depth,
                                         Gy[j + 1][i + 1] * depth};
                    }
                }
                if (norm(sum) > threshold)
                    framebuffer.set(x, y, TGAColor{0, 0, 0, 255});
            }
        }
        framebuffer.write_tga_file("Indirect lighting/diablo3_edges.tga");
    }
}