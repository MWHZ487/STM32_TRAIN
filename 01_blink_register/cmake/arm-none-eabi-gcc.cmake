# ---------------------------------------------------------------------------
# ARM Cortex-M 交叉编译工具链配置（CMake Toolchain File）
#
# 这个文件在"配置阶段"最早被读，作用是告诉 CMake：
#   不要用本机的 gcc（那是给 Windows 编 x86 程序的），
#   要用 arm-none-eabi-gcc（给 ARM 芯片编程序的）。
#
# 工具来源：STM32CubeCLT
#   D:\Software\Dev\STM32CubeCLT\GNU-tools-for-STM32\bin
# ---------------------------------------------------------------------------

set(CMAKE_SYSTEM_NAME       Generic)
set(CMAKE_SYSTEM_PROCESSOR  arm)

# Generic 平台默认不给可执行文件加扩展名，生成的产物会叫 stm32_blink 而不是 stm32_blink.elf，
# 后续 objcopy 和调试器都按 .elf 找文件，所以这里显式补上
set(CMAKE_EXECUTABLE_SUFFIX ".elf")

set(TOOLCHAIN_PREFIX arm-none-eabi-)

# 优先从 PATH 查找（CubeCLT 的 bin 目录已加入用户级 PATH）
find_program(CMAKE_C_COMPILER   NAMES ${TOOLCHAIN_PREFIX}gcc)
find_program(CMAKE_ASM_COMPILER NAMES ${TOOLCHAIN_PREFIX}gcc)
find_program(CMAKE_CXX_COMPILER NAMES ${TOOLCHAIN_PREFIX}g++)
find_program(CMAKE_OBJCOPY      NAMES ${TOOLCHAIN_PREFIX}objcopy)
find_program(CMAKE_OBJDUMP      NAMES ${TOOLCHAIN_PREFIX}objdump)
find_program(CMAKE_SIZE         NAMES ${TOOLCHAIN_PREFIX}size)
find_program(CMAKE_GDB          NAMES ${TOOLCHAIN_PREFIX}gdb)

# PATH 里找不到就回退到 CubeCLT 的固定安装路径
if(NOT CMAKE_C_COMPILER)
  set(CUBECLT_ROOT "D:/Software/Dev/STM32CubeCLT")
  set(_BIN "${CUBECLT_ROOT}/GNU-tools-for-STM32/bin")
  set(CMAKE_C_COMPILER   "${_BIN}/${TOOLCHAIN_PREFIX}gcc.exe")
  set(CMAKE_ASM_COMPILER "${_BIN}/${TOOLCHAIN_PREFIX}gcc.exe")
  set(CMAKE_CXX_COMPILER "${_BIN}/${TOOLCHAIN_PREFIX}g++.exe")
  set(CMAKE_OBJCOPY      "${_BIN}/${TOOLCHAIN_PREFIX}objcopy.exe")
  set(CMAKE_OBJDUMP      "${_BIN}/${TOOLCHAIN_PREFIX}objdump.exe")
  set(CMAKE_SIZE         "${_BIN}/${TOOLCHAIN_PREFIX}size.exe")
  set(CMAKE_GDB          "${_BIN}/${TOOLCHAIN_PREFIX}gdb.exe")
  message(STATUS "使用 CubeCLT 固定路径工具链: ${_BIN}")
endif()

# 交叉编译产生的可执行文件在 Windows 上跑不了，
# 所以告诉 CMake 别做"能运行吗"的测试，只做静态库测试
set(CMAKE_TRY_COMPILE_TARGET_TYPE STATIC_LIBRARY)

set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_PACKAGE ONLY)
