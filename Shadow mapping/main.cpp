#include "tgaimage.h"
#include "model.h"
#include "our_gl.h"

constexpr int width = 800;
constexpr int height = 800;
constexpr int shadowmapw = 1024;
constexpr int shadowmaph = 1024;

extern mat<4, 4> ModelView, Viewport, Perspective;
extern std::vector<double> ZBuffer;

struct BlankShader : public IShader
{
    const Model &model;
    BlankShader(const Model &m) : model(m) {}
    virtual vec4 vertex(const int face, const int vert)
    {
        vec4 v = model.vert(face, vert);
        return Perspective * ModelView * v;
    }
    virtual std::pair<bool, TGAColor> fragment(const vec3 bar) const
    {
        return {false, TGAColor{255, 255, 255, 255}};
    }
};

struct PhongShader : public IShader
{
    const Model &model;
    vec4 l;
    vec4 nors[3];
    vec4 tris[3];
    vec2 uvs[3];
    mat<4, 4> M;

    PhongShader(const Model &m, const vec3 &light)
        : model(m),
          M(ModelView.invert_transpose())
    {
        l = normalized(
            ModelView *
            vec4{light.x, light.y, light.z, 0.});
    }
    virtual vec4 vertex(const int face, const int vert)
    {
        const vec4 v = model.vert(face, vert);
        const vec4 n = M * model.normal(face, vert);
        const vec2 uv = model.uv(face, vert);
        vec4 gl_Position = ModelView * v;
        uvs[vert] = uv;
        tris[vert] = gl_Position;
        nors[vert] = n;
        return Perspective * gl_Position;
    }
    virtual std::pair<bool, TGAColor> fragment(const vec3 bar) const{
        mat<2,4> E = {tris[1] - tris[0], tris[2] - tris[0]};
        mat<2,2> U = {uvs[1] - uvs[0], uvs[2] - uvs[0]};
        mat<2,4> T = U.invert() * E;
        mat<4,4> TBN = {
            normalized(T[0]),
            normalized(T[1]),
            normalized(nors[0] * bar.x + nors[1] * bar.y + nors[2] * bar.z),
            {0, 0, 0, 1}};
        constexpr double ambient = 0.4;
        constexpr double spec = 3.0;
        constexpr double shininess = 35.0;
        vec2 uv = bar.x * uvs[0] + bar.y * uvs[1] + bar.z * uvs[2];
        vec4 uv_n = normalized(TBN.transpose() * model.normal_tangent(uv));
        double diffuse = 1. * std::max(0.0, uv_n * l);
        vec4 r = normalized(uv_n * (uv_n * l) * 2 - l);
        double specular = (spec * sample2D(model.specular(), uv)[0] / 255.) * std::pow(std::max(r.z, 0.), shininess);
        TGAColor color = sample2D(model.diffuse(), uv);
        for (int i = 0; i < 3; i++)
        {
            color[i] = std::clamp(static_cast<int>(ambient * color[i] + diffuse * color[i] + specular * 255.), 0, 255);
        }
        return {false, color};
    }
};

TGAImage drop_zbuffer(const std::vector<double> &zbuffer, const int width, const int height)
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

int main(int argc, char **argv)
{
    TGAImage framebuffer(width, height, TGAImage::RGB, {177, 195, 209, 255});
    TGAImage Dbuffer(width, height, TGAImage::GRAYSCALE, {0, 0, 0, 0});
    // std::vector<std::string> filenames = {
    //     "../Object/african_head/african_head.obj",
    //     "../Object/african_head/african_head_eye_inner.obj"};
    std::vector<std::string> filenames = {
        "../Object/diablo3_pose/diablo3_pose.obj",
        "../Object/floor.obj"
    };
    constexpr vec3 eye{-1, 0, 2};
    constexpr vec3 center{0, 0, 0};
    constexpr vec3 up{0, 1, 0};
    constexpr vec3 l{1, 1, 1};

    std::filesystem::create_directories("Shadow mapping");

    { // 阴影贴图pass
        lookat(l, center, up);
        perspective(norm(l - center));
        viewport(shadowmapw / 16, shadowmapw / 16, shadowmapw * 7 / 8, shadowmaph * 7 / 8);
        zbuffer(shadowmapw, shadowmapw);
        TGAImage trash(shadowmapw, shadowmaph, TGAImage::RGB, {177, 195, 209, 255});
        
        for (const auto &fn : filenames)
        {
            Model model(fn);
            BlankShader shader(model);
            for (int i = 0; i < model.nfaces(); i++)
            {
                Triangle clip;
                for (int j = 0; j < 3; j++)
                    clip[j] = shader.vertex(i, j);
                rasterize(clip, shader, trash);
            }
        }
        trash.write_tga_file("Shadow mapping/shadowmap.tga");
        drop_zbuffer(ZBuffer, shadowmapw, shadowmaph).write_tga_file("Shadow mapping/shadowmap_depth.tga");
        
    }
    std::vector<double> shadowBuffer = ZBuffer;
    mat<4, 4> N = Viewport * Perspective * ModelView;

    {// 法线贴图pass
        lookat(eye, center, up);
        perspective(norm(eye - center));
        viewport(width / 16, height / 16, width * 7 / 8, height * 7 / 8);
        zbuffer(width, height);
        for (const auto &fn : filenames)
        {
            Model model(fn);
            PhongShader shader(model, l);
            for (int i = 0; i < model.nfaces(); i++)
            {
                Triangle clip;
                for (int j = 0; j < 3; j++)
                    clip[j] = shader.vertex(i, j);
                rasterize(clip, shader, framebuffer);
            }
        }
        
        framebuffer.write_tga_file("Shadow mapping/diablo3.tga");
        drop_zbuffer(ZBuffer, width, height).write_tga_file("Shadow mapping/diablo3_depth.tga");
    }
    mat<4, 4> M = (Viewport * Perspective * ModelView).invert();
    std::vector<bool> mask(width * height, false);
    TGAImage maskimg(width, height, TGAImage::GRAYSCALE);
    {// 后处理pass
        double bias = 0.03;
        for (int x = 0; x < width; x++)
        {
            for (int y = 0; y < height; y++)
            {
                vec4 q = M * vec4{x * 1., y * 1., ZBuffer[x + y * width], 1.};
                vec4 p_shadow = N * q;
                vec3 p = p_shadow.xyz() / p_shadow.w;

                bool msk = (p_shadow.z < -100 ||
                            (p.x < 0 ||
                            p.x >= shadowmapw ||
                            p.y < 0 ||
                            p.y >= shadowmaph) ||
                            (p.z > shadowBuffer[static_cast<int>(p.x) + static_cast<int>(p.y) * shadowmapw] - bias));

                mask[x + y * width] = msk;
                if (mask[x + y * width])
                    maskimg.set(x, y, TGAColor{255, 255, 255, 255});
            }
        }
        maskimg.write_tga_file("Shadow mapping/mask.tga");
    }

    {
        for (int x = 0; x < width; x++)
        {
            for (int y = 0; y < height; y++)
            {
                if (mask[x + y * width])
                    continue;
                TGAColor color = framebuffer.get(x, y);

                constexpr double shadowStrength = 0.3;

                for (int c = 0; c < 3; ++c)
                {
                    color[c] = static_cast<std::uint8_t>(
                        color[c] * shadowStrength);
                }

                framebuffer.set(x, y, color);
            }
        }
        framebuffer.write_tga_file("Shadow mapping/diablo3_shadow.tga");
    }
}