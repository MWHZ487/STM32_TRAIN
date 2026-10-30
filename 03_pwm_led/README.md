# 01 · PWM 控制 LED 呼吸灯

**任务**：用 TIM2 的 PWM 输出让 LED 亮度往复变化，形成呼吸效果；一个完整周期约 1 s。
**芯片**：STM32F103C8T6（Blue Pill 类开发板）

## 一、接线

| 引脚 | 接到哪 | 说明 |
| --- | --- | --- |
| **PA1** | LED 的**阴极**（通过 LED 后） | TIM2_CH2 的 PWM 输出，低电平点亮 |
| 3V3 | 限流电阻（1 kΩ 左右）→ LED 阳极 | 板载 PC13 的 LED **不参与**本实验 |
| GND | 开发板 GND | 与外部电源共地 |

> 为什么是低电平点亮：CubeMX 里把 TIM2 CH2 的 `OCPolarity` 配成了 **LOW**，
> 所以占空比越大，低电平时间越长，LED 越亮。代码注释里的「低电平灌电为亮」就是这个意思。

## 二、参数

| 项 | 值 | 计算 |
| --- | --- | --- |
| 定时器时钟 | 72 MHz | APB1 定时器时钟（PSC 之前） |
| PWM 频率 | **1 kHz** | 72 MHz ÷ (71+1) ÷ (999+1) = 1 kHz |
| 分辨率 | 1000 级 | ARR = 999，CCR 取 0…1000 |
| 呼吸周期 | ≈ 0.9 s | 上行 101 步 × 5 ms + 下行 100 步 × 4 ms |
| 输出极性 | 低电平有效 | `TIM_OCPOLARITY_LOW` |

> 如需**严格 1.000 s**：把下降段 `HAL_Delay(4)` 改成 `HAL_Delay(5)`（100 步 × 5 ms = 500 ms，
> 上行 505 ms + 下行 500 ms ≈ 1.005 s）。

## 三、编译与烧录

```powershell
cd 01_LED呼吸灯
cmake --preset Debug
cmake --build --preset Debug

# 烧录（ST-Link V2，SWD）
STM32_Programmer_CLI -c port=SWD -w build\Debug\PWM_1.elf -rst
```

- 工具链：STM32CubeCLT 1.22（arm-none-eabi-gcc 14.3）、CMake 4.3、Ninja
- HAL：STM32Cube FW_F1 V1.8.7

## 四、代码结构

```text
Core/Src/main.c
├── MX_GPIO_Init()        // 本工程无普通 GPIO 需求
├── MX_TIM2_Init()        // PSC=71, ARR=999；CH2 PWM1 模式，极性 LOW
├── HAL_TIM_PWM_Start()   // 启动 CH2 输出
└── while (1)
    ├── for (ccr = 0 → 1000, +10)   __HAL_TIM_SET_COMPARE(..., ccr); HAL_Delay(5);
    └── for (ccr = 1000 → 10, -10)  __HAL_TIM_SET_COMPARE(..., ccr); HAL_Delay(4);
```

## 五、现象（验收）

上电后 PA1 上的 LED 应该**渐亮 → 渐暗 → 渐亮**，循环不停，肉眼看起来像呼吸；
周期约 0.9 s。若用示波器看 PA1：1 kHz 方波，占空比在 0% ↔ 100% 之间来回扫。

## 六、排错

| 现象 | 先查什么 |
| --- | --- |
| 完全不亮 | LED 极性接反 / 没接限流电阻 / 用错引脚（本工程是 **PA1**，不是 PC13） |
| 常亮不呼吸 | 程序没跑起来（先确认烧录成功、复位）；或用了板载 PC13 的 LED |
| 亮度变化很生硬 | PWM 频率被改过；确认 PSC=71、ARR=999 |
| 周期明显不是 1 s | 检查两个 `HAL_Delay` 的值与循环步数 |
