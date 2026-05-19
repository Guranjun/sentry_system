cmake_minimum_required(VERSION 3.10)
project(Monitor)

set(CMAKE_C_STANDARD 99)
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/bin)


# 强制指定头文件搜索路径（将板子的 include 压在最前面）
include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}/inc
    ${CMAKE_FIND_ROOT_PATH}/usr/include
)

# 2. 收集源文件
file(GLOB_RECURSE SRCS "src/*.c" "src/*.cpp")

# 3. 生成可执行程序
add_executable(video_detect_test ${SRCS})

# 4. 手动强行链接目标库

target_link_directories(video_detect_test PRIVATE ${CMAKE_FIND_ROOT_PATH}/usr/lib)

target_link_libraries(video_detect_test 
    PRIVATE 
    # OpenCV 的核心组件库名
    opencv_core opencv_imgproc opencv_imgcodecs
    # FFmpeg 的四大核心组件库名
    avcodec avformat avutil swscale
    # 基础系统依赖库
    pthread m dl rt
)

# 5. 添加必要的宏定义，防止 FFmpeg 的 C/C++ 混编内联报错
target_compile_definitions(video_detect_test PRIVATE __STDC_CONSTANT_MACROS)