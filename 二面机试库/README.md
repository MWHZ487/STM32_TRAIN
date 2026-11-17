# 二面机试库 · STM32F103 + HAL

> 用途：二面机试（30~40 分钟，不能联网、不能用 AI）时，把这套文件拷进 CubeMX 工程，
> 5 分钟搭好骨架，剩下的时间全写业务逻辑。
>
> 平台：STM32F103C8T6 + HAL 库 + CubeMX 生成工程

---

## 一、包内容

| 文件 | 功能 | 对应机试要求 |
| --- | --- | --- |
| `uart.c/h` | 串口 **DMA 发送** + **DMA/IDLE 接收** + printf 重定向 | 串口通信（带 DMA） |
| `motor.c/h` | PWM 正反转 + 死区补偿 + 输出限幅 | PWM 电机控制 |
| `pid.c/h` | 位置式 PID + 串级位置环 | PID 定速度/定位置 |
| `key.c/h` | 2~3 个按键 EXTI 中断 + 时间窗消抖 | 2~3 个按键中断 |
| `oled.c/h` + `oledfont.h` | SSD1306 0.96 寸 OLED 全驱动 | OLED 显示 |

---

## 二、CubeMX 配置清单

### 1. 时钟与调试

```text
RCC → HSE = Crystal/Ceramic Resonator
SYS → Debug = Serial Wire
Clock Configuration → HCLK 敲 72，回车
```

### 2. 定时器分配（F103C8 只有 TIM1~TIM4）

| 定时器 | 用途 | 参数 | 引脚 |
| --- | --- | --- | --- |
| TIM3 | PWM 输出 | PSC 71 / ARR 999 → 1 kHz | PA6=CH1, PA7=CH2 |
| TIM2 | 编码器 | PSC 0 / ARR 65535 / TI1 and TI2 | PA0, PA1 |
| TIM1 | 10ms 控制周期 | PSC 7199 / ARR 99，NVIC 勾 update | 不占引脚 |

> **注意**：APB1 分频=2 但定时器时钟自动 ×2，所以 TIM1/2/3 的输入都是 72 MHz。

### 3. 串口 + DMA

```text
USART1 → Asynchronous, 115200, 8N1  → PA9(TX) / PA10(RX)
DMA Settings → Add → USART1_TX → Mode: Normal
DMA Settings → Add → USART1_RX → Mode: Normal   ← 缺这一条，UART_Init 会 HardFault
USART1 → NVIC Settings → 勾 USART1 global interrupt
```

### 4. 编码器引脚上拉（容易漏！）

```text
Pinout → 点 PA0 → GPIO Settings → GPIO Pull-up/Pull-down = Pull-up
        → 点 PA1 → 同上
```

**霍尔编码器多为开漏输出，不上拉会读出一堆假脉冲**（表现为 RPM 突然跳到上千）。

### 5. I2C（OLED）

```text
I2C1 → I2C → PB6(SCL) / PB7(SDA)
```

### 6. 按键

```text
PB0 / PB1 / PB5 → GPIO_EXTIx，下降沿，Pull-up
NVIC → 勾对应的 EXTI line 中断
```

### 7. STBY（TB6612 使能）

```text
PA3 → GPIO_Output，初始电平 **High**
```

### 8. 工程设置

```text
Project Manager → Toolchain/IDE = CMake
Code Generator → Generate peripheral initialization as a pair of .c/.h files（可选）
```

---

## 三、文件放置

```text
你的工程/
├── Core/
│   ├── Inc/
│   │   ├── uart.h   motor.h
│   │   └── key.h    pid.h（含编码器+波形）
│   │   └── oled.h      oledfont.h
│   └── Src/
│       ├── uart.c   motor.c
│       └── key.c    pid.c（含编码器+波形）
│       └── oled.c
```

### CMake 工程要加源文件

在 `CMakeLists.txt` 的 `target_sources(...)` 里加：

```cmake
target_sources(${CMAKE_PROJECT_NAME} PRIVATE
    Core/Src/uart.c
    Core/Src/motor.c
    Core/Src/key.c
    Core/Src/pid.c
    Core/Src/oled.c
)
```

> 如果 CubeMX 用的是 `cmake/stm32cubemx/CMakeLists.txt` 统一管理源文件，
> 则加到那个文件里的 `target_sources` 也行。

---

## 四、main.c 模板（直接抄）

### 引入与变量

```c
/* USER CODE BEGIN Includes */
#include <stdio.h>
#include "uart.h"
#include "motor.h"
#include "key.h"
#include "pid.h"
#include "oled.h"
/* USER CODE END Includes */

/* USER CODE BEGIN PV */
PID_t     g_spd;                 /* 速度环 */
PID_t     g_pos;                 /* 位置环（定位置才用） */
WaveMode  g_wave_mode = WAVE_STEP;
uint16_t  g_tick      = 0;       /* 10ms 一拍，不用 float 累加 */
volatile int32_t g_target = 0;
volatile float   g_rpm    = 0.0f;
/* USER CODE END PV */
```

### 初始化（USER CODE 2，必须在所有 MX_xxx_Init 之后）

```c
/* USER CODE BEGIN 2 */
UART_Init(&huart1);
MOTOR_Init();
ENCODER_Init();
KEY_Init();
OLED_Init();
OLED_Clear();
OLED_ShowString(0, 0, "READY", 16);

/* Kp / Ki / Kd / out_max / i_max */
PID_Init(&g_spd, 0.5f, 0.03f, 0.0f, 450.0f, 3000.0f);
PID_Init(&g_pos, 2.0f, 0.00f, 0.0f, 300.0f, 0.0f);

HAL_TIM_Base_Start_IT(&htim1);   /* 10ms 控制周期 */
/* USER CODE END 2 */
```

### 主循环

```c
/* USER CODE BEGIN WHILE */
while (1)
{
    /* 1. 按键切模式 */
    if (KEY_GetEvent(0)) {
        g_wave_mode = (g_wave_mode == WAVE_STEP) ? WAVE_SINE_FIX :
                                            (g_wave_mode == WAVE_SINE_FIX) ? WAVE_SINE_SWEEP : WAVE_STEP;
        g_tick = 0;
        PID_Reset(&g_spd);          /* 切波形必须清积分！ */
    }

    /* 2. OLED 显示。转速可能为负，不能直接转 uint32_t */
    OLED_ShowString(0,  0, (char *)WAVE_Name(g_wave_mode), 16);
    OLED_ShowString(0,  2, "T:", 16);
    OLED_ShowNum   (16, 2, (uint32_t)g_target, 4, 16);
    OLED_ShowString(64, 2, "A:", 16);
    {
        int rpm_i = (int)g_rpm;
        OLED_ShowNum(88, 2, (uint32_t)(rpm_i < 0 ? -rpm_i : rpm_i), 4, 16);
    }

    /* 3. 串口上报（DMA，不阻塞） */
    printf("%d,%d\n", (int)g_target, (int)g_rpm);

    /* 4. 串口命令（可选） */
    uint8_t cmd[32];
    if (UART_ReadFrame(cmd, sizeof cmd) > 0) {
        /* 解析 cmd，比如 "100" 直接设目标转速 */
    }
    /* USER CODE END WHILE */
}
```

### 中断回调（USER CODE 4）

```c
/* USER CODE BEGIN 4 */
void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim)
{
    if (htim->Instance != TIM1) return;

    /* ① 测速（同时更新 position） */
    g_rpm = ENCODER_ReadRpm();

    /* ② 生成目标波形。整数节拍，避免 float 每次 +0.01 的累积误差 */
    g_tick++;
    if (g_tick >= 6000) g_tick = 0;          /* 60 秒一轮 */
    g_target = WAVE_Get(g_wave_mode, g_tick * ENC_PERIOD_S);

    /* ③ PID */
    float u = PID_Calc(&g_spd, (float)g_target, g_rpm);

    /* ④ 输出 */
    MOTOR_Set((int16_t)u);
}
/* USER CODE END 4 */
```

---

## 五、API 速查

### 串口

```c
UART_Init(&huart1);                        /* 初始化，启动 DMA 接收 */
UART_Send(buf, len);                       /* DMA 发送 */
UART_Printf("%d,%d\n", a, b);              /* 格式化发送 */
uint8_t buf[64];
uint16_t n = UART_ReadFrame(buf, sizeof buf);   /* 取一帧，0 = 无新数据 */
```

### 电机

```c
MOTOR_Set(u);      /* u: -450 ~ +450，正负表示方向，0 = 停 */
MOTOR_Stop();
```

### 编码器

```c
float   rpm = ENCODER_ReadRpm();     /* 同时更新 delta / position */
int16_t d   = ENCODER_GetDelta();
int32_t pos = ENCODER_GetPosition();
ENCODER_ResetPosition();
```

### PID

```c
PID_Init(&pid, Kp, Ki, Kd, out_max, i_max);
float u = PID_Calc(&pid, target, actual);
PID_Reset(&pid);                                    /* 切工况时清积分 */

/* 串级位置环 */
float u = PID_PositionCalc(&g_pos, &g_spd, target_pulse, now_pulse, now_rpm, 300.0f);
```

### 按键

```c
if (KEY_GetEvent(0)) { ... }    /* 0/1/2，读后自动清除 */
if (!KEY_Raw(0))     { ... }    /* 0 = 按下 */
```

### OLED

```c
OLED_Init();  OLED_Clear();
OLED_ShowString(x, page, "ABC", 16);         /* page = 0~7，16 号字占 2 页 */
OLED_ShowNum(x, page, 123, 4, 16);           /* 4 位数字 */
OLED_ShowChar(x, page, 'A', 16);
OLED_ShowCHinese(x, page, 0);
OLED_DrawBMP(0, 0, 128, 8, BMP);
```

### 波形

```c
g_target = WAVE_Get(WAVE_STEP, t);        /* 阶跃 */
g_target = WAVE_Get(WAVE_SINE_FIX, t);    /* 固定正弦 */
g_target = WAVE_Get(WAVE_SINE_SWEEP, t);  /* 变频正弦 */
```

---

## 六、常见坑（每一条都是踩过的）

### 1. 启动代码放错位置 → HardFault

```c
HAL_TIM_Base_Start_IT(&htim1) 等
必须放在 USER CODE BEGIN 2，不能放 USER CODE BEGIN Init
```

`Init` 段在 `SystemClock_Config()` 和 `MX_xxx_Init()` **之前**执行，此时句柄的 `Instance` 还是 NULL。

### 2. 定时器对错

```text
TIM1 = 10ms 时基   → HAL_TIM_Base_Start_IT(&htim1)
TIM2 = 编码器      → HAL_TIM_Encoder_Start(&htim2, TIM_CHANNEL_ALL)   ← 必须 ALL
TIM3 = PWM         → HAL_TIM_PWM_Start(&htim3, TIM_CHANNEL_1/2)
```

### 3. `#include <stdio.h>` 不能放函数体内

会报 `invalid storage class for function '__sputc_r'`。放文件顶部 `USER CODE BEGIN Includes`。

### 4. VOFA 看不到波形

| 原因 | 修法 |
| --- | --- |
| `printf` 没 `\n` | 必须 `"%d,%d\n"` |
| 用空格分隔 | 多通道必须**逗号** |
| 数据格式选成 JustFloat | 文本要选 **FireWater** |

### 5. 编码器读数异常（突然上千 RPM）

```text
① PA0/PA1 没上拉 → 配置成立刻 Pull-up
② 编码器线和电机线捆在一起 → 分开走，A/B 与 GND 双绞
③ 忘写 HAL_TIM_Encoder_Start → 计数不动
```

### 6. 电机不转

```text
① STBY 没拉高（PA3 必须 High）
② TB6612 的 PWMA 悬空 → 接 3.3V/5V
③ 12V 没接 → 只靠 ST-Link 供电带不动
④ motor_set 传了负数进 CCR → 变成 65036 → 全速正转
⑤ 只写了 CH1 没清 CH2 → H 桥两输入同时有效 → 刹车
```

### 7. PID 越推越快（正反馈）

```text
现象：target=0 时手拨一下电机，它自己越转越快
原因：PWM 正转方向 和 编码器计数正方向 相反
修法：对调 E1A/E1B 两根线，或 rpm 取负
必须先开环验方向再上闭环！
```

### 8. 参数调不出来

```text
① 目标 0 那一档会触发反接制动 → 速度环别用 0 当目标
② 切波形/切模式不清积分 → 前几秒必定跑偏
③ 死区补偿不准 → ccr = DEAD + |u|，DEAD 要接近实测死区
```

---

## 七、机试 30 分钟时间分配

```text
0~5   min   CubeMX 按上面清单配好 → GENERATE
5~8   min   拷 5 个 .c + 6 个 .h，CMake 加源文件
8~12  min   main.c 抄模板（初始化 + 主循环 + 中断）
12~15 min   编译烧录，确认 OLED 亮、串口有输出、电机能动
15~35 min   写题目要求的业务逻辑
35~40 min   录屏 / 演示
```

**关键：前 15 分钟不要纠结代码美观，先让四条链路都活着**（串口 / OLED / 电机 / 按键），后面全是写逻辑。

---

## 八、面试可能问的点（这套库里的设计都对应得上）

| 会被问 | 答什么 |
| --- | --- |
| 串口为什么用 DMA | 阻塞发送会占 CPU；DMA 让 CPU 只负责填缓冲 |
| 怎么收不定长帧 | DMA 一直收 + IDLE 中断判断帧结束 |
| PID 为什么要传指针 | integral / e_last 要跨调用保留，传值就丢 |
| 为什么要两个限幅 | i_max 防积分饱和，out_max 防 PWM 越界 |
| 为什么用 int16_t 存 delta | 16 位计数器溢出后按补码解释，正反转都对 |
| 死区怎么补偿 | ccr = DEAD + \|u\|，把输出原点挪到起转点 |
| 位置环为什么外面套速度环 | 位置环给目标速度，速度环负责动态响应，串级更稳 |
