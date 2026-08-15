#pragma once
#include "tgaimage.h"
#include <string>
#include <vector>
#include "geometry.h"

struct Model {
    // 点
    std::vector<vec<4>> verts_;
    // 法线
    std::vector<vec<4>> norms_;
    // 纹理
    std::vector<vec<2>> uvs_;
    // 每个面的顶点索引
    std::vector<std::vector<int>> faces_;
    // 每个面的法线索引
    std::vector<std::vector<int>> normfaces_;
    // 每个面的纹理索引
    std::vector<std::vector<int>> uvfaces_;
    TGAImage normalmap = {};
    TGAImage diffusemap = {};
    TGAImage specularmap = {};
    Model() = default;
    explicit Model(const std::string &filename);
    // 获取顶点数和面数
    int nverts() const;
    int nnormals() const;
    int nfaces() const;
    // 获取第i个顶点
    const vec<4>& vert(int i) const;
    // 获取第i个法线
    const vec<4>& normal(int i) const;
    const vec2 &uv(int face, int vert) const;
    // 获取第idx个面的顶点索引
    const std::vector<int>& face(int idx) const;
    // 获取第idx个面的第nthvert个顶点
    const vec4& vert(const int iface, const int nthvert) const;
    // 获取第idx个面的第nthvert个顶点法线
    const vec4& normal(const int iface, const int nthvert) const;
    // 根据纹理坐标获取法线
    const vec4 normal(const vec2 &uv) const;
    const TGAImage &diffuse() const;
    const TGAImage &specular() const;
};
