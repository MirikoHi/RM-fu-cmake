# F1yepian

这是用于 RoboMaster 能量机关单个灯板控制的 STM32F103RC 工程（基于 CMake 构建，MinGW Makefiles 生成器）。主要代码维护在 `module/` 目录，已弃用的实现保存在 `Application/` 目录。

## 目录结构

- `module/` — 主实现（C++ 代码），主要维护区。
- `Application/` — 旧实现
- `Core/`
	- `Core/Inc`
	- `Core/Src`
- `Hardware/`

## 常见问题

- 构建失败：
	- 使用 `cmake` 时指定 `-DCMAKE_TOOLCHAIN_FILE=cmake/gcc-arm-none-eabi.cmake`。
	- 在 Windows 上使用 `-G "MinGW Makefiles"` 以生成 `mingw32-make` 项目文件。
- 链接失败或找不到库：
	- 检查 `CMakePresets.json` 或 `CMakeLists.txt` 中的编译选项
- WS2812 灯不亮或闪烁异常：
	- 检查供电电压与时序

---

维护：荼め

爱来自HFUT

