# ---------------------------------------------------------------------------
# STM32F103C8T6 (Blue Pill) 最小工程
#
# 命令行构建方式：
#   cmake --preset debug          配置
#   cmake --build --preset debug  编译
#
# 在 VS Code 里：装好 CMake Tools 扩展后直接点底部的 Build 按钮
# ---------------------------------------------------------------------------
cmake_minimum_required(VERSION 3.20)

project(stm32_blink LANGUAGES C ASM)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)

# ---------------------------------------------------------------------------
# 目标芯片参数
#   cortex-m3  = STM32F103 的内核
#   thumb      = Cortex-M 只支持 Thumb 指令集
# ---------------------------------------------------------------------------
set(MCU_FLAGS -mcpu=cortex-m3 -mthumb)

# ---------------------------------------------------------------------------
# 编译选项
#   -Wall -Wextra            打开常见警告，学 C 阶段非常重要
#   -Og                      优化但保留调试信息，嵌入式调试推荐用 -Og
#   -g3                      生成最详细调试信息
#   -ffunction-sections      每个函数单独成段
#   -fdata-sections          每个变量单独成段
#   （后两项配合链接时的 --gc-sections，可以把没用到的代码裁掉省 Flash）
# ---------------------------------------------------------------------------
add_compile_options(
    ${MCU_FLAGS}
    -Wall
    -Wextra
    -Og
    -g3
    -ffunction-sections
    -fdata-sections
)

# ---------------------------------------------------------------------------
# 链接选项
#   -T<脚本>                 指定链接脚本，决定内存布局
#   -Wl,-Map=...             生成 map 文件，可以查每个函数占了多少 Flash
#   -Wl,--gc-sections        删除没被引用的段
#   -nostartfiles            不用 GCC 默认启动代码（我们自己写了启动文件）
#   -specs=nano.specs        用精简版 C 库，省空间
#   -specs=nosys.specs       提供系统调用的空实现（裸机没有文件系统）
# ---------------------------------------------------------------------------
add_link_options(
    ${MCU_FLAGS}
    -T${CMAKE_CURRENT_SOURCE_DIR}/STM32F103C8Tx_FLASH.ld
    -Wl,-Map=${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}.map,--cref
    -Wl,--gc-sections
    -nostartfiles
    -specs=nano.specs
    -specs=nosys.specs
)

# ---------------------------------------------------------------------------
# 源文件
#   main.c      —— 业务代码
#   startup.s   —— 启动文件，必须参与编译，否则没有向量表
# ---------------------------------------------------------------------------
add_executable(${PROJECT_NAME}
    src/main.c
    startup/startup_stm32f103xb.s
)

# Generic 平台默认不给可执行文件加扩展名，产物会叫 stm32_blink。
# 但 objcopy 和 Cortex-Debug 都按 .elf 找文件，所以这里强制指定后缀
set_target_properties(${PROJECT_NAME} PROPERTIES SUFFIX ".elf")

# ---------------------------------------------------------------------------
# 编译完成后自动生成 .bin 和 .hex，并打印占用空间
#   .elf  带调试信息，用于 GDB 调试
#   .bin  纯二进制，烧录用
#   .hex  Intel HEX 格式，Keil / ST-Link Utility 都认
# ---------------------------------------------------------------------------
add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
    COMMAND ${CMAKE_OBJCOPY} -O binary $<TARGET_FILE:${PROJECT_NAME}> ${PROJECT_NAME}.bin
    COMMAND ${CMAKE_OBJCOPY} -O ihex   $<TARGET_FILE:${PROJECT_NAME}> ${PROJECT_NAME}.hex
    COMMAND ${CMAKE_SIZE} $<TARGET_FILE:${PROJECT_NAME}>
    COMMENT "生成 ${PROJECT_NAME}.bin / ${PROJECT_NAME}.hex"
)

# ---------------------------------------------------------------------------
# 一键烧录（需要 ST-Link 已连接并装好驱动）
#   VS Code: 终端里执行  cmake --build --preset debug --target flash
# ---------------------------------------------------------------------------
add_custom_target(flash
    COMMAND STM32_Programmer_CLI -c port=SWD -w ${PROJECT_NAME}.bin 0x08000000 -v -rst
    DEPENDS ${PROJECT_NAME}
    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
    COMMENT "通过 ST-Link 烧录到 0x08000000"
)
