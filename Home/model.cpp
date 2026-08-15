#include <fstream>
#include <sstream>
#include <iostream>
#include <algorithm>
#include "model.h"

Model::Model(const std::string &filename)
    : verts_(), norms_(), faces_(), normfaces_(), uvfaces_()
{
    // 打开文件
    std::ifstream in(filename, std::ifstream::in);
    if (in.fail()) return;
    // 读取文件内容接收
    std::string line;
    while (!in.eof()) {
        std::getline(in, line);
        std::istringstream iss(line);
        char trash;
        // 字符串.compare(起始位置, 比较长度, 目标字符串)
        // 返回 0     两部分相等
        // 返回负数 左边按字典序小于右边
        // 返回正数 左边按字典序大于右边 
        if (!line.compare(0, 2, "v "))
        {
            // >> 是流提取运算符 以空格为分隔符将数据从流中提取出来
            iss >> trash;
            vec<4> v = {0, 0, 0, 1};
            for (int i=0;i<4;i++) iss >> v[i];
            verts_.push_back(v);
        }
        else if (!line.compare(0, 3, "vn "))
        {
            // OBJ 法线格式：vn x y z
            iss >> trash >> trash;
            vec<4> n;
            for (int i=0; i<3; i++) iss >> n[i];
            norms_.push_back(normalized(n));
        }
        else if (!line.compare(0, 3, "vt "))
        {
            // OBJ 纹理坐标格式：vt u v
            iss >> trash >> trash;

            vec2 uv;
            iss >> uv.x >> uv.y;

            uvs_.push_back({uv.x, 1.0 - uv.y}); // 翻转y坐标
        }
        else if (!line.compare(0, 2, "f "))
        {
            std::vector<int> f;
            std::vector<int> uvf;
            std::vector<int> nf;
            int idx, uvidx, nidx;
            iss >> trash;
            while (iss >> idx >> trash >> uvidx >> trash >> nidx)
            {
                idx--; // in wavefront obj all indices start at 1, not zero
                uvidx--;
                nidx--; // in wavefront obj all indices start at 1, not zero
                f.push_back(idx);
                uvf.push_back(uvidx);
                nf.push_back(nidx);
            }
            faces_.push_back(f);
            uvfaces_.push_back(uvf);
            normfaces_.push_back(nf);
        }
    }
    std::cerr << "# v# " << verts_.size()
              << " vn# " << norms_.size()
              << " f# " << faces_.size() << "\n";
    // 隐藏面消除（画家算法）
    // std::vector<int> idx(nfaces());
    // for (int i = 0; i < nfaces(); i++)
    //     idx[i] = i; // 每一个i表示一个面

    // std::sort(idx.begin(), idx.end(),
    //           [&](const int &a, const int &b) { // given two triangles, compare their min z coordinate
    //               float aminz = std::min(vert(a, 0).z,
    //                                      std::min(vert(a, 1).z,
    //                                               vert(a, 2).z));
    //               float bminz = std::min(vert(b, 0).z,
    //                                      std::min(vert(b, 1).z,
    //                                               vert(b, 2).z));
    //               return aminz < bminz;
    //           });

    // std::vector<std::vector<int>> facet_2(nfaces()); // allocate an array to store permutated facets
    // for (int i = 0; i < nfaces(); i++)         // for each (new) facet
    //     facet_2[i] = faces_[idx[i]];

    // faces_ = facet_2;
    auto load_texture = [&filename](const std::string suffix, TGAImage &img)
    {
        size_t dot = filename.find_last_of(".");
        if (dot == std::string::npos)
            return;
        std::string texfile = filename.substr(0, dot) + suffix;
        std::cerr << "texture file " << texfile << " loading " << (img.read_tga_file(texfile.c_str()) ? "ok" : "failed") << std::endl;
    };
    load_texture("_nm.tga", normalmap);
    load_texture("_diffuse.tga", diffusemap);
    load_texture("_spec.tga", specularmap);
}


int Model::nverts() const
{
    return (int)verts_.size();
}
int Model::nnormals() const
{
    return (int)norms_.size();
}
const vec<4>& Model::vert(int i) const
{
    return verts_[i];
}
const vec<4>& Model::normal(int i) const
{
    return norms_[i];
}
const vec4& Model::vert(const int iface, const int nthvert) const
{
    return verts_[faces_[iface][nthvert]];
}
const vec4& Model::normal(const int iface, const int nthvert) const
{
    return norms_[normfaces_[iface][nthvert]];
}
const vec4 Model::normal(const vec2 &uv) const
{
    TGAColor c = normalmap.get(uv[0] * normalmap.width(), uv[1] * normalmap.height());
    // BGRA
    return vec4{(double)c[2], (double)c[1], (double)c[0], 0} * 2. / 255. - vec4{1, 1, 1, 0};
}
const TGAImage &Model::diffuse() const
{
    return diffusemap;
}

const TGAImage &Model::specular() const
{
    return specularmap;
}
int Model::nfaces() const
{
    return (int)faces_.size();
}
const std::vector<int>& Model::face(int idx) const
{
    return faces_[idx];
}
const vec2 &Model::uv(
    const int face,
    const int vert) const
{
    return uvs_[uvfaces_[face][vert]];
}