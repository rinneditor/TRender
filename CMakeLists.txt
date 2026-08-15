cmake_minimum_required(VERSION 3.20)

project(TRender LANGUAGES CXX)
set(RUN_OUTPUT_DIR "${CMAKE_SOURCE_DIR}/output")
file(MAKE_DIRECTORY "${RUN_OUTPUT_DIR}")


# 公共图像库
add_library(tgaimage STATIC
    Home/tgaimage.cpp
)
add_library(model STATIC
    Home/model.cpp
)
add_library(our_gl STATIC 
    Home/our_gl.cpp
)

target_include_directories(tgaimage PUBLIC
    "${CMAKE_CURRENT_SOURCE_DIR}/Home"
)
target_include_directories(model PUBLIC
    "${CMAKE_CURRENT_SOURCE_DIR}/Home"
)
target_include_directories(our_gl PUBLIC
    "${CMAKE_CURRENT_SOURCE_DIR}/Home"
)

target_compile_features(tgaimage PUBLIC cxx_std_17)
target_compile_features(model PUBLIC cxx_std_17)
target_compile_features(our_gl PUBLIC cxx_std_17)

# 每增加一节课程，只需增加一个 add_executable() 和一个 target_link_libraries()
# Bresenham 课程程序
add_executable(bresenham
    "Bresenham’s line drawing algorithm/main.cpp"
)
add_executable(bresenham_homework
    "Bresenham’s line drawing algorithm/homework.cpp"
)
# Triangle 课程程序
add_executable(Triangle
    "Triangle rasterization/main.cpp"
)
add_executable(Triangle_homework 
    "Triangle rasterization/homework.cpp"
)
# Hidden 课程程序
add_executable(Hidden 
    "Hidden faces removal/main.cpp"
)
# Crude 课程程序
add_executable(Crude
    "A crude (but simple) approach to camera handling/main.cpp"
)
add_executable(Crude_homework
    "A crude (but simple) approach to camera handling/homework.cpp"
)
# Better camera handling
add_executable(Better 
    "Better camera handling/main.cpp"
)
# Shading
add_executable(Shading 
    "Shading/main.cpp"
)
add_executable(Shading_homework 
    "Shading/homework.cpp"
)   
# more data
add_executable(MoreData
    "More data!/main.cpp"
)
add_executable(MoreData_homework
    "More data!/homework.cpp"
)

target_link_libraries(bresenham PRIVATE tgaimage model)
target_link_libraries(bresenham_homework PRIVATE tgaimage model)
target_link_libraries(Triangle PRIVATE tgaimage model)
target_link_libraries(Triangle_homework PRIVATE tgaimage model)
target_link_libraries(Crude PRIVATE tgaimage model)
target_link_libraries(Crude_homework PRIVATE tgaimage model)
target_link_libraries(Hidden PRIVATE tgaimage model)
target_link_libraries(Better PRIVATE tgaimage model)
target_link_libraries(Shading PRIVATE tgaimage model our_gl)
target_link_libraries(Shading_homework PRIVATE tgaimage model our_gl)
target_link_libraries(MoreData PRIVATE tgaimage model our_gl)
target_link_libraries(MoreData_homework PRIVATE tgaimage model our_gl)