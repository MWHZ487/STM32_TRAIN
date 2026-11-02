# STM32_TRAIN
大一RMer基于STM32CUBE IDE以及CUBE MX的STM32 操作实现

## 目录与本地工程对应关系

| 仓库目录 | 本地工程 | 对应任务 | 内容 |
| --- | --- | --- | --- |
| `00_env/` | — | 任务 1 | 环境体检记录 |
| `01_blink_register/` | — | 任务 1 | 寄存器点灯 |
| `02_blink_hal/` | — | 任务 1 | HAL 点灯 |
| `05_gpio_out/` | `E:\MCU\STM\01light` | 任务 2.1 | GPIO 输出点灯 |
| `06_gpio_in/` | `E:\MCU\STM\02switch` | 任务 2.2 | 轮询读按键 + 消抖 |
| `07_gpio_exti/` | `E:\MCU\STM\03switch2` | 任务 2.3 | EXTI 中断 + 消抖 |
| `03_pwm_led/` | `E:\MCU\PWM\PWM_1` | 任务 3.1 | PWM 呼吸灯 |
| `04_pwm_motor/` | `E:\MCU\PWM\PWM_2` | 任务 3.2 | PWM 电机正反转调速 |
| `docs/` | `E:\MCU\` | — | 提交前核对表、Blue Pill 原理图 |

## 同步约定

- **每次改完代码，把本地工程同步到这里并 push**。
- 只同步源码与工程配置：`Core/`、`*.ioc`、`CMakeLists.txt`、`CMakePresets.json`、`*.ld`、`startup_*.s`、`cmake/`。
- **不同步**：`build/`、`cmake-build-*/`、`.idea/`（已在 `.gitignore` 里）、`Drivers/`（ST 官方库，可由 CubeMX + FW_F1 包重新生成）。
