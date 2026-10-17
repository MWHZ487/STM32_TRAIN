# ---------------------------------------------------------------------------
# STM32F103C8T6 —— HAL 库版点灯工程
#
# 与裸机版（01_blink_register）的区别：这里链接 ST 官方 HAL 驱动，
# 驱动源码直接从固件包里取，不需要复制到工程目录。
#
# 构建：cmake --preset debug && cmake --build --preset debug
# ---------------------------------------------------------------------------
cmake_minimum_required(VERSION 3.20)

project(stm32_blink_hal LANGUAGES C ASM)

# ---------------------------------------------------------------------------
# 固件包位置（CubeMX 已把它下载到 E 盘，避免占 C 盘空间）
# ---------------------------------------------------------------------------
set(FW_ROOT "E:/DevBuilds/Caches/STM32Cube/Repository/STM32Cube_FW_F1_V1.8.7"
    CACHE PATH "STM32Cube F1 固件包根目录")
if(NOT EXISTS ${FW_ROOT})
  message(FATAL_ERROR "找不到固件包: ${FW_ROOT}")
endif()

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)

set(MCU_FLAGS -mcpu=cortex-m3 -mthumb)

add_compile_options(
    ${MCU_FLAGS}
    -Wall
    -Og
    -g3
    -ffunction-sections
    -fdata-sections
)

# ---------------------------------------------------------------------------
# 头文件搜索路径
# ---------------------------------------------------------------------------
include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}/Core/Inc
    ${FW_ROOT}/Drivers/STM32F1xx_HAL_Driver/Inc
    ${FW_ROOT}/Drivers/STM32F1xx_HAL_Driver/Inc/Legacy
    ${FW_ROOT}/Drivers/CMSIS/Device/ST/STM32F1xx/Include
    ${FW_ROOT}/Drivers/CMSIS/Include
)

# USE_HAL_DRIVER 让 stm32f1xx.h 去包含 HAL 头文件
# STM32F103xB    告诉 CMSIS 具体是哪颗芯片（决定寄存器定义和中断数量）
add_compile_definitions(USE_HAL_DRIVER STM32F103xB)

add_link_options(
    ${MCU_FLAGS}
    -T${CMAKE_CURRENT_SOURCE_DIR}/STM32F103C8Tx_FLASH.ld
    -Wl,-Map=${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}.map,--cref
    -Wl,--gc-sections
    -specs=nano.specs
    -specs=nosys.specs
)

# 注意：这里【不能】加 -nostartfiles。
# 裸机工程（01_blink_register）用它可以，因为那份代码完全不碰 C 运行时；
# 而 HAL 工程会链接 newlib 的 __libc_init_array，它需要 crti.o 提供的 _init/_fini，
# 加了 -nostartfiles 就会报 "undefined reference to `_init'"。
# 不加的代价只是 crt0 的 _start 被链进来，而它没被向量表引用，会被 --gc-sections 裁掉。

# ---------------------------------------------------------------------------
# HAL 驱动源码：全部纳入，靠链接期 --gc-sections 裁掉没用到的那部分
# （排除 *_template.c，那是给用户抄的样例，编译进来会造成重复定义）
# ---------------------------------------------------------------------------
file(GLOB HAL_SOURCES ${FW_ROOT}/Drivers/STM32F1xx_HAL_Driver/Src/*.c)
list(FILTER HAL_SOURCES EXCLUDE REGEX "template")
list(LENGTH HAL_SOURCES HAL_COUNT)

add_executable(${PROJECT_NAME}
    Core/Src/main.c
    Core/Src/stm32f1xx_it.c
    Core/Src/system_stm32f1xx.c
    Startup/startup_stm32f103xb.s
    ${HAL_SOURCES}
)

# Generic 平台不给可执行文件加扩展名，这里强制指定
set_target_properties(${PROJECT_NAME} PROPERTIES SUFFIX ".elf")

add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
    COMMAND ${CMAKE_OBJCOPY} -O binary $<TARGET_FILE:${PROJECT_NAME}> ${PROJECT_NAME}.bin
    COMMAND ${CMAKE_OBJCOPY} -O ihex   $<TARGET_FILE:${PROJECT_NAME}> ${PROJECT_NAME}.hex
    COMMAND ${CMAKE_SIZE} $<TARGET_FILE:${PROJECT_NAME}>
    COMMENT "生成 ${PROJECT_NAME}.bin / ${PROJECT_NAME}.hex"
)

# 一键烧录（需 ST-Link 已连接）
add_custom_target(flash
    COMMAND STM32_Programmer_CLI -c port=SWD -w ${PROJECT_NAME}.bin 0x08000000 -v -rst
    DEPENDS ${PROJECT_NAME}
    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
    COMMENT "通过 ST-Link 烧录到 0x08000000"
)
