#include "tgaimage.h"
#include "geometry.h"

void lookat(const vec3 eye, const vec3 center, const vec3 up);
void perspective(const double f);
void viewport(const int x, const int y, const int w, const int h);
void zbuffer(const int width, const int height);

struct IShader
{
    static TGAColor sample2D(const TGAImage &img, const vec2 &uv)
    {
        const int x = std::clamp(
            static_cast<int>(uv[0] * img.width()),
            0,
            img.width() - 1);

        const int y = std::clamp(
            static_cast<int>(uv[1] * img.height()),
            0,
            img.height() - 1);

        return img.get(x, y);
    }
    virtual std::pair<bool, TGAColor> fragment(const vec3 bar) const = 0;
};

typedef vec4 Triangle[3]; // a triangle primitive is made of three ordered points
void rasterize(const Triangle &clip, const IShader &shader, TGAImage &framebuffer);