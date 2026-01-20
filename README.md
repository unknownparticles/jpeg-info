# JPEG Info - JPEG 元数据解析工具

一个用 C++17 编写的 JPEG 图像元数据解析和显示工具。

## 功能特性

该工具可以解析和显示 JPEG 文件中的各种元数据信息：

- **JFIF 信息** (APP0): 版本、密度单位、分辨率、缩略图尺寸
- **SOF 信息** (Start of Frame): 图像尺寸、精度、颜色分量
- **EXIF 信息** (APP1): 相机设置、拍摄参数、GPS 位置等
- **XMP 信息** (APP1): Adobe XMP 元数据
- **ICC Profile** (APP2): 颜色配置文件
- **Adobe 信息** (APP14): Adobe 特定的颜色转换信息
- **COM 注释**: JPEG 注释段

## 项目结构

```
jpeg-info/
├── CMakeLists.txt          # CMake 构建配置
└── src/
    ├── main.cpp            # 主程序入口
    ├── format.h/cpp        # 格式化输出函数
    ├── i18n.h/cpp          # 国际化支持 (中文/英文)
    ├── file_reader.h/cpp   # 文件读取工具
    ├── jpeg_indexer.h/cpp  # JPEG 段索引构建
    ├── jpeg_markers.h      # JPEG 标记定义
    ├── jpeg_types.h        # 数据结构定义
    ├── parse_jfif.h/cpp    # JFIF 解析
    ├── parse_sof.h/cpp     # SOF 解析
    ├── parse_exif.h/cpp    # EXIF 解析
    ├── parse_xmp.h/cpp     # XMP 解析
    ├── parse_icc.h/cpp     # ICC Profile 解析
    ├── parse_adobe.h/cpp   # Adobe APP14 解析
    └── parse_com.h/cpp     # COM 注释解析
```

## 编译

需要 C++17 编译器和 CMake 3.16+。

```bash
mkdir build
cd build
cmake ..
make
```

编译成功后会在 `build` 目录生成 `jpeg_info` 可执行文件。

## 使用方法

```bash
# 使用中文显示 (默认)
./build/jpeg_info image.jpg

# 使用英文显示
./build/jpeg_info image.jpg --lang=en

# 使用中文显示
./build/jpeg_info image.jpg --lang=zh
```

## 输出示例

程序会输出以下信息：

1. **分区列表**: 显示 JPEG 文件中所有的段 (segments)，包括标记类型、偏移量、长度等
2. **JFIF 信息**: 如果存在 JFIF APP0 段
3. **图像基本信息**: 从 SOF 段提取的图像尺寸和颜色信息
4. **EXIF 信息**: 相机和拍摄参数
5. **GPS 信息**: 如果 EXIF 中包含地理位置
6. **XMP 元数据**: Adobe XMP XML 数据
7. **ICC Profile**: 颜色配置文件信息
8. **Adobe 信息**: 颜色转换标志
9. **注释**: COM 段中的文本注释

## 技术细节

### 核心功能

- **段索引**: 快速扫描 JPEG 文件，构建所有段的索引
- **APP 子类型识别**: 自动识别 APP0-APP15 段的具体类型 (JFIF/EXIF/XMP/ICC 等)
- **SOS 数据跳过**: 正确处理 Start of Scan 后的压缩图像数据 (包括 0xFF00 stuffing)
- **ICC Profile 拼接**: 支持多段 ICC Profile 的自动拼接
- **EXIF 解析**: 支持 Big/Little Endian，解析 IFD0/EXIF/GPS 子 IFD
- **国际化**: 支持中英文界面

### 设计特点

- **模块化设计**: 每种元数据类型都有独立的解析模块
- **零拷贝索引**: 先构建索引，按需加载段数据
- **跨平台**: 支持 Windows (MSVC) 和 Unix-like 系统 (macOS/Linux)
- **类型安全**: 使用 C++17 的 `std::optional` 处理可选数据

## 许可证

本项目为开源项目，可自由使用和修改。

## 作者

Alun
