# Smart Water Tank Controller — VS Code / GCC

这是从最终可运行 V4.5 固件整理出的 VS Code 工程，目标芯片为 STM32F103xB，采用裸机分层架构，不使用 FreeRTOS。

本版本已经进行第一轮低风险Flash优化，但仍属于V4.5优化基线。V5.0最终架构和分阶段编写计划见：

- `docs/ARCHITECTURE_V5.md`
- `docs/DEVELOPMENT_PLAN_V5.md`
- `docs/FLASH_OPTIMIZATION.md`

## 目录职责

- `Core/`：CubeMX 生成的启动入口、中断和外设初始化。
- `Drivers/`：STM32 HAL 与 CMSIS，编译必需，不能删除。
- `User/Inc`、`User/Src`：水箱业务代码，头文件和源文件必须成套更新。
- `cmake/`、`CMakeLists.txt`、`CMakePresets.json`：GNU Arm 编译配置。
- `startup_stm32f103xb.s`：GCC 启动文件，编译必需。
- `STM32F103XX_FLASH.ld`：GCC 链接脚本，编译必需。
- `smart_water_tank_f103.ioc`：CubeMX 硬件配置源文件。

## VS Code 编译

1. 用 VS Code 打开本目录，而不是只打开 `User` 文件夹。
2. 安装 STM32Cube for Visual Studio Code 扩展及其 GNU Tools、CMake、Ninja 组件。
3. 选择 `CMake: Configure`，配置选择 `Debug`。
4. 选择 `CMake: Build`。
5. 成功后输出位于 `build/Debug/`：
   - `smart_water_tank_f103.elf`
   - `smart_water_tank_f103.hex`
   - `smart_water_tank_f103.bin`

如果替换过 `Core` 或 `User` 文件，先执行 `CMake: Delete Cache and Reconfigure`，不要继续使用旧的 `build` 缓存。

工程通过扩展提供的 `CUBE_BUNDLE_PATH` 查找编译器，不再写死某个 Windows 用户目录。若从普通终端编译，请先把 `arm-none-eabi-gcc`、CMake 和 Ninja 加入 `PATH`。

## 烧录

打开 STM32CubeProgrammer：

1. ST-LINK 连接 SWDIO、SWCLK、GND 和目标板供电。
2. 连接方式选择 ST-LINK / SWD。
3. 选择 `build/Debug/smart_water_tank_f103.hex`。
4. 点击 `Download`，地址由 HEX 文件携带，无需手工填写。
5. 烧录完成后复位开发板。

## 本次修复

- 将旧版 `app_command.h` 与 V4.5 源文件统一：补回 `APP_COMMAND_PRESSURE_TARE`、`AppCommand_TakeStopLatch()`，并把 `AppCommand_Get()` 返回类型修正为 `bool`。
- 在 `main.c` 接入 `App_Init()`、`App_Run()`、超声波 EXTI 回调和网络 UART 回调。
- 恢复 EXTI1 与 USART1 的中断优先级关系。
- 删除旧构建缓存、Keil 工程文件、重复启动文件和未使用的旧 Air780E 头文件。
- 编译后自动生成 HEX 和 BIN。
- Debug改用 `-Og`，Release启用 `-Os + LTO`。
- 使用轻量整数解析器替换AT、MQTT和JSON接收路径中的 `sscanf()`。

## 为什么直接复制 Keil 的 User 文件会报错

`User` 并不是一个完全独立的程序。它还依赖 `Core/main.c` 中的初始化与回调，也要求 `User/Inc` 和 `User/Src` 来自同一版本。本次复制后，`.c` 使用 V4.5 接口，`app_command.h` 仍声明旧接口，所以 GCC 正确地报告“枚举未声明”和“函数类型冲突”。问题不是 Keil 与 VS Code 的 C 语法不同，而是工程文件版本没有成套同步。

以后更新业务代码时，至少同时比较并同步：

- `User/Inc/`
- `User/Src/`
- `Core/Src/main.c` 的 `USER CODE` 区域
- 与中断优先级有关的 `Core/Src/gpio.c`、`Core/Src/usart.c`

不要复制 `MDK-ARM` 输出目录和旧 `build` 目录。
