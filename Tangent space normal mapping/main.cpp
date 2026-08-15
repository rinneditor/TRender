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
    vec4 l;      // 光源方向
    vec4 nors[3]; // 三角形的三个顶点法线
    vec4 tris[3]; // 三角形的三个顶点裁剪空间坐标
    vec2 uvs[3]; // 三角形的三个顶点在纹理坐标系下的坐标
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
        // 获取顶点坐标、纹理坐标和法线
        const vec4 v = model.vert(face, vert);
        const vec4 n = M * model.normal(face, vert);
        const vec2 uv = model.uv(face, vert);
        vec4 gl_Position = ModelView * v;
        uvs[vert] = uv;
        tris[vert] = gl_Position;
        nors[vert] = n;
        return Perspective * gl_Position;
    }
    std::pair<bool, TGAColor>
    fragment(const vec3 bar) const override
    {
        mat<2, 4> E = {tris[1] - tris[0], tris[2] - tris[0]};
        mat<2, 2> U = {uvs[1] - uvs[0], uvs[2] - uvs[0]};
        mat<2, 4> T = U.invert() * E;
        // TBN矩阵, T为切线向量，B为副切线向量，N为法线向量
        mat<4, 4> TBN = {
            normalized(T[0]),
            normalized(T[1]),
            normalized(nors[0] * bar.x + nors[1] * bar.y + nors[2] * bar.z),
            {0, 0, 0, 1}};
        constexpr double ambient = 0.4;
        constexpr double spec = 3.0;
        constexpr double shininess = 35.0;

        // 每个点法线贴图
        const vec2 uv = bar.x * uvs[0] + bar.y * uvs[1] + bar.z * uvs[2];
        const vec4 uv_n = normalized(
            TBN.transpose() * model.normal_tangent(uv));

        // 漫反射光照强度
        const double diffuse = 1. * std::max(0.0, uv_n * l);
        // 镜面反射
        const vec4 r = normalized(uv_n * (uv_n * l) * 2 - l);
        const double specular = (spec * sample2D(model.specular(), uv)[0] / 255.) * std::pow(std::max(r.z, 0.), shininess);
        // 纹理采样
        TGAColor color = sample2D(model.diffuse(), uv);

        // 贴图颜色 ×（环境光 + 漫反射 + 白色镜面高光）
        for (int i = 0; i < 3; i++)
        {
            const double value =
                color[i] * (ambient + diffuse + specular);

            color[i] = static_cast<std::uint8_t>(
                std::clamp(value, 0.0, 255.0));
        }

        return {false, color};
    }
};

int main(int argc, char **argv)
{
    TGAImage framebuffer(width, height, TGAImage::RGB);
    // std::vector<std::string> filenames = {
    //     "../Object/african_head/african_head.obj",
    //     "../Object/african_head/african_head_eye_inner.obj"};
    std::vector<std::string> filenames = {
        "../Object/diablo3_pose/diablo3_pose.obj"};
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
        PhongShader shader(model, l);
        for (int i = 0; i < model.nfaces(); i++)
        {
            Triangle clip;
            for (int j = 0; j < 3; j++)
                clip[j] = shader.vertex(i, j);
            rasterize(clip, shader, framebuffer);
        }
    }
    std::filesystem::create_directories("Tangent space normal mapping");
    framebuffer.write_tga_file("Tangent space normal mapping/diablo3.tga");
}
