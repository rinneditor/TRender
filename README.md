# TRender

TRender 是一个使用 C++17 编写的教学型软件光栅化渲染器，学习过程参考
[ssloy/tinyrenderer](https://github.com/ssloy/tinyrenderer)。项目从像素和直线绘制开始，逐步实现三角形光栅化、深度缓冲、相机变换、纹理与法线贴图、阴影、环境光遮蔽以及卡通渲染。

整个渲染流程由 CPU 完成，不依赖 OpenGL、DirectX 等图形 API。渲染结果保存为 TGA 图片。

## 已实现内容

| CMake 目标 | 内容 |
| --- | --- |
| `bresenham` | Bresenham 直线绘制 |
| `bresenham_homework` | 直线绘制作业 |
| `Triangle` | 三角形光栅化 |
| `Triangle_homework` | 三角形光栅化作业 |
| `Hidden` | 背面剔除与深度处理 |
| `Crude` | 基础相机与投影 |
| `Crude_homework` | 基础相机作业 |
| `Better` | LookAt、透视与视口变换 |
| `Shading` | 基础光照与着色 |
| `Shading_homework` | 着色作业 |
| `MoreData` | OBJ 属性、纹理和法线贴图 |
| `MoreData_homework` | Phong 光照作业 |
| `TangentSpaceNormalMapping` | 切线空间法线贴图与 TBN 矩阵 |
| `ShadowMapping` | Shadow Mapping 阴影 |
| `Indirect` | 多方向采样的环境光遮蔽与边缘检测 |
| `Indirect_homework` | 屏幕空间环境光遮蔽（SSAO） |
| `BonusToonShading` | 分段光照与深度描边卡通渲染 |

## 项目结构

```text
TRender/
├── Home/                         # 公共数学、模型、图像和光栅化代码
│   ├── geometry.h                # 向量与矩阵
│   ├── model.h / model.cpp       # Wavefront OBJ 与纹理加载
│   ├── tgaimage.h / tgaimage.cpp # TGA 图像读写
│   └── our_gl.h / our_gl.cpp     # 相机、视口、ZBuffer 和光栅化
├── Object/                       # OBJ 模型及配套纹理
├── */main.cpp                    # 各章节示例
├── */homework.cpp                # 各章节作业
├── CMakeLists.txt
└── output/                       # 运行时生成的图片（Git 忽略）
```

## 环境要求

- CMake 3.20 或更高版本
- 支持 C++17 的编译器，例如 Apple Clang、Clang 或 GCC
- 可选：VS Code、C/C++ 扩展和 CMake Tools 扩展

macOS 可以确认工具是否安装成功：

```bash
cmake --version
clang++ --version
```

## 编译

在项目根目录执行：

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build -j 10
```

只编译某个示例，例如卡通渲染：

```bash
cmake --build build --target BonusToonShading -j 10
```

如果移动了项目目录并遇到 `CMakeCache.txt directory is different`，请删除旧缓存后重新配置：

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release --fresh
```

在 VS Code 中也可以执行 `CMake: Delete Cache and Reconfigure`。

## 运行

示例代码使用类似 `../Object/...` 的相对模型路径，因此应从 `output/` 目录启动程序：

```bash
cd output
../build/BonusToonShading
```

运行其他章节时替换可执行文件名即可，例如：

```bash
../build/ShadowMapping
../build/Indirect
../build/Indirect_homework
```

程序生成的 `.tga` 图片会保存在 `output/` 下对应的章节目录中。

## 渲染流程

```text
OBJ 模型顶点
    ↓ ModelView
相机空间
    ↓ Perspective
裁剪空间
    ↓ 透视除法
NDC 坐标
    ↓ Viewport
屏幕坐标
    ↓ 三角形光栅化 + ZBuffer
片段着色
    ↓
TGA 图片
```

## 说明

- `build/` 保存 CMake 构建文件，可以安全删除并重新生成。
- `output/` 保存程序输出，不参与版本控制。
- TGA 像素通道在代码中采用 BGRA 顺序。
- 法线、光线和顶点参与运算前应变换到同一个坐标空间。
- 项目主要用于理解渲染管线和相关数学原理，不以实时性能为目标。

## 致谢

- [ssloy/tinyrenderer](https://github.com/ssloy/tinyrenderer)
- [Tiny Renderer 教程](https://haqr.eu/tinyrenderer/)
