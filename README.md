# STM32 FreeRTOS 学习项目 —— P4.1：接入厂家 LCD 库，绘制界面面板

> 基于 STM32F103 + FreeRTOS 的练习项目。
> P3.3 是自己用 SPI+DMA 写了一套 ST7789 驱动；本版（P4.1）改用**厂家提供的预编译 LCD 库**
> （`Drivers/lcd.a` + `Drivers/lcd.h`），通过 4 个回调把底层收发接进来；
> 同时把屏幕初始化挪到任务里执行，修掉了中断回调的写法问题，
> 最后用库函数在屏幕上画出「Run Mode / One-Shot」面板。

---

## 一、这一版做了什么（和 P3.3 对比）

| 项目 | P3.3 | P4.1 |
|---|---|---|
| LCD 驱动 | 自己写：发命令 / 发数据 / 设窗口 / 刷图 | **厂家预编译库** `lcd.a`，调用 `lcd_init` / `lcd_clear` / `lcd_set_cursor` / `lcd_fill_rect` / `lcd_print_string` 等 |
| 显示内容 | 交替显示 `image1` / `image2` 两帧图片 | 右侧粉色标题条 **"Run Mode"** + 灰色面板 **"One-Shot"** |
| LCD 初始化位置 | `main()` 里，调度器启动**前** | **`vLcdTask` 里，调度器启动后**（避免 HardFault） |
| SPI 分频 | 256（SCLK ≈ 281 kHz） | **4（SCLK = 18 MHz）** |
| SPI 完成回调 | `pxHigherPriorityTaskWoken` 用法有误 | 改成正确的「标志变量 + 取地址」写法 |
| 图片数据 | 参与编译，约占 75 KB Flash | `image1.h` / `image2.h` 保留但**不再参与链接**，FLASH 反而大减 |
| FLASH / RAM | 102.7 KB / 10.9 KB | **55.6 KB / 17.7 KB** |
| 工程名 | p3.3 | **p4.1**（`p4.1.ioc`，产物 `p4.1.elf`） |

一句话：**P3.3 练的是“自己实现驱动”，P4.1 练的是“把厂家库接进 RTOS 工程”——重点变成适配接口、安排初始化时机、复盘 HardFault。**

---

## 二、运行现象

1. 上电后 `main()` 只初始化外设并创建 4 个任务，然后启动调度器；
2. `vLcdTask` 启动后**在任务里**初始化 LCD（背光、复位、厂家库初始化），清屏；
3. 屏幕右侧绘制粉色标题条并显示 `Run Mode`，下方绘制灰色面板并显示 `One-Shot`；
4. 任意按键**松手瞬间**，蜂鸣器响 100 ms；
5. LED1（PC3）以 100 ms 亮 / 100 ms 灭的节奏闪烁；
6. 串口 USART1 已配置为 115200，本版没有往串口发数据。

---

## 三、硬件与软件环境

| 项目 | 说明 |
|---|---|
| 主控 | STM32F103RCTx（256 KB Flash / 48 KB RAM，Cortex-M3） |
| 主频 | 72 MHz（HSE 8 MHz × PLL9） |
| 屏幕 | SPI 接口 TFT 彩屏；**厂家库按 320×240 横向坐标使用**（x 0~319，y 0~239），命令集由库封装 |
| 按键 | 6 个，输入上拉，按下接 GND（松手触发） |
| 蜂鸣器 | PC8，高电平响（有源蜂鸣器） |
| LED | LED1 = PC3（本版使用）；LED3 = PC1（未使用，注意见「已知局限」第 1 条） |
| RTOS | FreeRTOS **V11.1.0**，内存管理 `heap_4`，堆 8 KB，tick 100 Hz |
| HAL 时基 | **TIM6 1 ms**（`stm32f1xx_hal_timebase_tim.c`，避免和 FreeRTOS 抢 SysTick） |
| HAL | STM32F1xx HAL Driver（启用 GPIO / USART / SPI / DMA / TIM） |
| LCD 库 | 厂家预编译静态库 `Drivers/lcd.a`（无源码），配套 `Drivers/lcd.h` |
| 构建 | CMake ≥ 3.22 + Ninja |
| 工具链 | GNU Tools for STM32 `arm-none-eabi-gcc` |
| 开发环境 | VS Code + STM32Cube for VS Code 扩展 |

### 按键引脚（与 1.3 相同）

引脚宏定义在 `Core/Inc/main.h`（由 CubeMX 生成）：

| 宏 | 引脚 | 驱动模块 | 按下动作 |
|---|---|---|---|
| `Key1_Pin` | PC9 | `User/keys/key1.c` | `Buzzer_beep()` |
| `Key2_Pin` | PA8 | `User/keys/key2.c` | `Buzzer_beep()` |
| `Key3_Pin` | PC15 | `User/keys/key3.c` | `Buzzer_beep()` |
| `Key4_Pin` | PC0 | `User/keys/key4.c` | `Buzzer_beep()` |
| `Key_UP_Pin` | PC13 | `User/keys/key_up.c` | `Buzzer_beep()` |
| `Key_DOWN_Pin` | PC14 | `User/keys/key_down.c` | `Buzzer_beep()` |

> GPIO 均为**输入上拉**：松开读到高电平，按下接 GND 读到低电平。
> 因此驱动检测的是「低 → 高」，即**松手瞬间**触发回调。

### LCD 引脚（与 P3.3 相同）

| 宏 | 引脚 | 作用 | 说明 |
|---|---|---|---|
| `Lcd_SCK_Pin` | PB3 | SPI1_SCK | 使用了 **SPI1 重映射** |
| `Lcd_MOSI_Pin` | PB5 | SPI1_MOSI | 同上 |
| `Lcd_NSS_Pin` | PD2 | 片选 | **软件片选**（`SPI_NSS_SOFT`） |
| `Lcd_RS_Pin` | PC12 | 命令/数据选择 | 0 = 命令，1 = 数据 |
| `Lcd_RST_Pin` | PB4 | 复位 | 低电平复位，初始化时拉低 1 ms |
| `Lcd_BL_Pin` | PB6 | 背光 | 高电平点亮 |

> ⚠️ PB3 / PB4 默认是 JTAG 引脚（JTDO / NJTRST）。工程的 `HAL_MspInit()`
> 调用了 `__HAL_AFIO_REMAP_SWJ_NOJTAG()`，关闭 JTAG、保留 SWD，
> 把 PB3 / PB4 释放为普通 GPIO；SPI1 通过 `__HAL_AFIO_REMAP_SPI1_ENABLE()` 映射到 PB3 / PB5。
> 调试时请使用 **SWD**（SWDIO / SWCLK），JTAG 已不可用。

### 其他外设

| 外设 | 引脚 | 配置 |
|---|---|---|
| 蜂鸣器 | PC8 | 推挽输出，初始低电平 |
| LED1 | PC3 | 推挽输出，初始低电平 |
| USART1 | PA9 / PA10 | 115200，8N1（本版未使用） |
| SPI1 | PB3 / PB5 | 主机、8 位、MSB、模式 0、**分频 4** |
| DMA1_CH3 | — | SPI1_TX，内存→外设，字节对齐，普通模式，中断优先级 5 |

> SPI1 位于 APB2（72 MHz），4 分频后 SCLK = **18 MHz**。
> 厂家库刷屏时大量使用 DMA，多数时间 CPU 都在等信号量，不会空转。

---

## 四、目录结构

```
p4.1/
├── Core/                         # CubeMX 生成的启动、时钟、中断、主程序
│   ├── Inc/
│   │   ├── main.h                #   ★ 按键 / LCD / 蜂鸣器 / LED 引脚宏
│   │   └── FreeRTOSConfig.h      #   FreeRTOS 配置（堆 8 KB、tick 100 Hz）
│   └── Src/
│       ├── main.c                #   外设初始化 + 创建 4 个任务
│       ├── stm32f1xx_hal_msp.c   #   SPI1 重映射、DMA、JTAG 关闭
│       ├── stm32f1xx_it.c        #   DMA1_CH3 / TIM6 中断
│       └── stm32f1xx_hal_timebase_tim.c  # HAL 时基改用 TIM6
├── Drivers/
│   ├── lcd.a                     #   ★ 厂家预编译 LCD 库（无源码）
│   ├── lcd.h                     #   ★ 厂家库头文件（LCD_InitTypeDef + 绘图 API）
│   ├── lcd_tool.exe              #   ★ 厂家工具：生成 fp.h
│   ├── fp.h                      #   ★ 本机硬件指纹（由 lcd_tool.exe 生成）
│   ├── key/                      #   【基础驱动层】与具体按键无关
│   │   ├── key.h                 #   KeyHandle_Typedef + 接口声明
│   │   └── key.c                 #   key_init / key_scan 实现
│   ├── STM32F1xx_HAL_Driver/     #   ST HAL 库
│   └── CMSIS/
├── FreeRTOS/                     # FreeRTOS 内核 V11.1.0（Src / Inc）
├── User/
│   ├── keys/                     # 【按键配置层】每个按键一个模块
│   │   ├── key1.c  / key1.h      #   PC9
│   │   ├── key2.c  / key2.h      #   PA8
│   │   ├── key3.c  / key3.h      #   PC15
│   │   ├── key4.c  / key4.h      #   PC0
│   │   ├── key_up.c   / key_up.h   # PC13
│   │   └── key_down.c / key_down.h # PC14
│   └── task/
│       ├── key_task.c / key_task.h     # 按键统一扫描任务
│       ├── Lcd_task.c / Lcd_task.h     # LCD 回调 + SPI 收发 + 面板绘制
│       ├── buzzer_task.c / buzzer_task.h # 蜂鸣器任务
│       ├── image1.h                    # 【保留未用】P3.3 的第一帧图片
│       └── image2.h                    # 【保留未用】P3.3 的第二帧图片
├── cmake/stm32cubemx/            # CubeMX 自动生成的 CMake 子工程
├── CMakeLists.txt                # 顶层构建脚本（工程名 p4.1，IMPORTED lcd 库）
├── CMakePresets.json
├── .vscode/launch.json           # ST-Link 调试配置
├── p4.1.ioc                      # CubeMX 工程文件
└── README.md
```

---

## 五、任务划分与调度

`main.c` 在启动调度器前依次创建 4 个任务（栈大小单位为字，1 字 = 4 字节）：

| 任务 | 函数 | 栈（字 / 字节） | 优先级 | 运行方式 | 职责 |
|---|---|---|---|---|---|
| 按键 | `keytask` | 128 / 512 B | **3**（最高） | 每 10 ms 扫描一轮 | 扫描 6 个按键，松手 → `Buzzer_beep()` |
| LCD | `vLcdTask` | 256 / 1024 B | 2 | 初始化并绘制一次，之后空循环 | 初始化厂家库、清屏、画面板和文字 |
| 蜂鸣器 | `vBuzzerTask` | 64 / 256 B | 1 | 等信号量 | 收到信号 → PC8 拉高 100 ms 再拉低 |
| LED | `vLedTask` | 128 / 512 B | 1 | 每 100 ms 翻转 | LED1 闪烁 |
| 空闲 | Idle | 128 / 512 B | 0 | 空闲时运行 | FreeRTOS 内核任务 |

```c
/* main.c —— 注意：这里不再调用 Lcd_Init() */
xTaskCreate(keytask,    "Key",    128, NULL, 3, NULL);     // 按键扫描
xTaskCreate(vBuzzerTask,"Buzzer",  64, NULL, 1, NULL);     // 蜂鸣器
xTaskCreate(vLcdTask,   "LCD",    256, NULL, 2, NULL);     // 屏幕
xTaskCreate(vLedTask,   "led",    128, NULL, 1, NULL);     // LED
vTaskStartScheduler();
```

```c
/* Lcd_task.c —— LCD 初始化放在任务里，调度器启动之后 */
void vLcdTask(void *pvParameter)
{
    Lcd_Init();                 // 此时调度器已经在跑，DMA/信号量才安全
    lcd_clear();
    ...
    for (;;)
    {
        vTaskDelay(pdMS_TO_TICKS(200));
    }
}
```

### 为什么这样分配优先级

```
优先级 3  keytask      —— 按键响应要快，且 10 ms 内一定能跑完
优先级 2  vLcdTask     —— 刷屏耗时长，但可以被按键抢占
优先级 1  Buzzer / LED —— 对实时性要求最低
```

- 按键扫描每 10 ms 只读 6 次 GPIO，几乎不占 CPU，但优先级给最高，保证不漏按键；
- 厂家库刷屏走 DMA，任务会阻塞在二值信号量上，不会一直占着 CPU；
- 蜂鸣器和 LED 都是低频动作，放在最低优先级，任何时候被打断都无所谓。

### 任务之间的协作关系

```
按键松手
  key1ClickCallBack() ── Buzzer_beep() ──▶ [二值信号量] ──▶ vBuzzerTask
                                                            PC8 高 100 ms

vLcdTask
  Lcd_Init / 绘图 ── SendData() ──▶ [DMA1_CH3] ──完成中断──▶ [二值信号量]
                                                              │
                                          SendData 里 take ◀──┘ 继续发送
```

---

## 六、厂家预编译 LCD 库怎么用

### 1. 库由哪几部分组成

| 文件 | 作用 |
|---|---|
| `Drivers/lcd.a` | 厂家预编译好的静态库（ARMv7-M / Thumb-2 / 软浮点），**没有源码** |
| `Drivers/lcd.h` | 头文件：`LCD_InitTypeDef` 回调结构体 + 所有绘图 API 声明 |
| `Drivers/lcd_tool.exe` | 厂家控制台工具，运行后在**自己所在目录**生成 `fp.h` |
| `Drivers/fp.h` | 本机硬件指纹，内容形如 `#define FP "K37FGx4v..."` |

`lcd.h` 在未定义宏 `LCD_LIB` 时会 `#include "fp.h"`，并把 `lcd_init(Init)` 展开成
`lcd_generic_init(Init, FP, 0)`——所以 **`fp.h` 是编译必需品**，不能只拷 `lcd.a` / `lcd.h`。

### 2. `fp.h` 是本机指纹，换电脑要重新生成

- `lcd_tool.exe` 会通过 WMI 读取本机的硬盘 / 主板序列号，算出一个 Base64 指纹写进 `fp.h`；
- 换一台电脑编译时，`lcd.a` 会校验指纹，**必须在新电脑上重新运行一次 `lcd_tool.exe`**；
- 它是控制台程序，生成位置是 **exe 所在目录**，所以请在 `Drivers/` 目录里运行它；
- 仓库里附带的 `fp.h` 是当前这台电脑生成的，仅供本机使用；别人克隆后请自行重新生成。

> 注意：`lcd.a` / `lcd_tool.exe` 是厂家提供的二进制文件，本仓库仅用于个人学习。
> 如需在其他项目复用，请先确认厂家的授权范围。

### 3. 初始化：4 个回调把库和硬件接起来

厂家库不直接操作 GPIO / SPI，而是通过结构体里的 4 个回调完成底层动作：

```c
typedef struct
{
    void (*back_light_control_callback)(uint8_t on);   // 背光：on=1 点亮
    void (*reset_callback)(void);                      // 复位屏幕
    void (*send_command_callback)(uint8_t cmd);        // 发送 1 字节命令
    void (*send_data_callback)(uint8_t *data, uint16_t size); // 发送数据
} LCD_InitTypeDef;
```

本版 `Lcd_task.c` 里的注册方式：

```c
void Lcd_Init(void)
{
    xSemphrToDma = xSemaphoreCreateBinary();       // DMA 完成同步用

    /* static：库如果保存了结构体指针，函数返回后内容仍然有效 */
    static LCD_InitTypeDef LCD_InitStruct;
    LCD_InitStruct.back_light_control_callback = LcdBlackLight_Control;
    LCD_InitStruct.reset_callback              = Lcd_Reset;
    LCD_InitStruct.send_command_callback       = SendComand;
    LCD_InitStruct.send_data_callback          = SendData;

    lcd_init(&LCD_InitStruct);                     // 宏 → lcd_generic_init(Init, FP, 0)
}
```

> ⚠️ 库内部有一套「默认 BSP」：**只在 4 个回调为 NULL 时启用**，走 `hspi1`、
> CS = PD4、RS = PC12。本板 CS 是 **PD2**，和默认值不一致，
> 所以这 4 个回调**必须全部提供**，否则片选会打错引脚。

### 4. 常用绘图 API

| 函数 | 作用 | 备注 |
|---|---|---|
| `lcd_set_cursor(x, y)` | 设置光标 | 坐标系**横向**：x 0~319，y 0~239，原点左上角 |
| `lcd_set_pen(color)` | 设置画笔颜色（前景） | RGB888，例如 `0xff0000` 红 |
| `lcd_set_brush(color)` | 设置画刷颜色（填充/背景） | RGB888 |
| `lcd_clear()` | 用当前画刷颜色清屏 | 清屏前先 `lcd_set_brush()` |
| `lcd_fill_rect(w, h)` | 从光标处填充矩形 | 左上角对齐光标 |
| `lcd_draw_rect(w, h)` | 画矩形（填充 + 边框） | 边框用画笔色 |
| `lcd_draw_rect_frame(w, h)` | 只画矩形边框 | |
| `lcd_print_string(str)` | 打印字符串 | 左下角对齐光标；前景=画笔，背景=画刷 |
| `lcd_print_integer(v, n)` | 打印整数，不足补 0 | |
| `lcd_print_float(...)` | 打印浮点数 | 内部用到 `ceil()`，所以链接需要 `-lm` |
| `lcd_draw_bitmap(w, h, p)` | 画位图 | 数据为 **RGB565**，可用来显示 `image1` / `image2` |
| `lcd_draw_waveform(...)` | 画示波器波形 | 参数见 `WaveformParamTypeDef` |

### 5. 本版的界面绘制

```c
lcd_clear();

// 粉色标题条 + 标题
lcd_set_cursor(260, 8);
lcd_set_brush(0xff7bff);        // 粉色
lcd_fill_rect(60, 15);          // 宽 60、高 15
lcd_set_cursor(262, 21);
lcd_set_brush(0x000000);
lcd_print_string("Run Mode");

// 灰色面板 + 文字
lcd_set_cursor(260, 23);
lcd_set_brush(0xcccccc);        // 浅灰
lcd_fill_rect(60, 35);          // 宽 60、高 35
lcd_set_cursor(265, 43);
lcd_set_brush(0x000000);
lcd_print_string("One-Shot");
```

- 所有元素都画在屏幕右侧（x ≈ 260~319）；
- 坐标沿用厂家库的 **320×240 横向**坐标系，不是 P3.3 里的 240×320 竖屏坐标。

### 6. CMake 里怎么链接

`lcd.a` 的文件名不是 `liblcd.a`，不能直接写 `-llcd`；而且它内部调用了 `ceil()`，
链接时必须保证 **`lcd.a` 在前、`libm` 在后**。本版用 `IMPORTED` 目标描述它：

```cmake
add_library(lcd STATIC IMPORTED)
set_target_properties(lcd PROPERTIES
    IMPORTED_LOCATION              "${CMAKE_CURRENT_SOURCE_DIR}/Drivers/lcd.a"
    INTERFACE_INCLUDE_DIRECTORIES  "${CMAKE_CURRENT_SOURCE_DIR}/Drivers"
    INTERFACE_LINK_LIBRARIES       m          # 保证链接顺序：先 lcd 后 m
)

target_link_libraries(${CMAKE_PROJECT_NAME}
    stm32cubemx
    lcd
)
```

`INTERFACE_INCLUDE_DIRECTORIES` 让 `#include "lcd.h"` 能自动找到；
`INTERFACE_LINK_LIBRARIES m` 把 `-lm` 排在 `lcd.a` 之后。

---

## 七、底层收发：SPI1 + DMA + 二值信号量

厂家库最终会调用我们提供的 `SendComand()` / `SendData()`，这两个函数自己实现：

```c
void SendComand(uint8_t cmd)
{
    HAL_GPIO_WritePin(Lcd_RS_GPIO_Port, Lcd_RS_Pin, GPIO_PIN_RESET);   // RS=0：命令
    HAL_GPIO_WritePin(Lcd_NSS_GPIO_Port, Lcd_NSS_Pin, GPIO_PIN_RESET); // 片选拉低
    HAL_SPI_Transmit(&hspi1, &cmd, 1, HAL_MAX_DELAY);                  // 阻塞发 1 字节
    HAL_GPIO_WritePin(Lcd_NSS_GPIO_Port, Lcd_NSS_Pin, GPIO_PIN_SET);   // 片选拉高
}

void SendData(uint8_t *pData, uint16_t size)
{
    HAL_GPIO_WritePin(Lcd_RS_GPIO_Port, Lcd_RS_Pin, GPIO_PIN_SET);     // RS=1：数据
    HAL_GPIO_WritePin(Lcd_NSS_GPIO_Port, Lcd_NSS_Pin, GPIO_PIN_RESET);
    HAL_SPI_Transmit_DMA(&hspi1, pData, size);                         // 启动 DMA，立即返回
    xSemaphoreTake(xSemphrToDma, portMAX_DELAY);                       // 等传输完成
    HAL_GPIO_WritePin(Lcd_NSS_GPIO_Port, Lcd_NSS_Pin, GPIO_PIN_SET);
}
```

命令只有 1 字节，没必要上 DMA；数据可能一次几万字节，必须用 DMA。

### DMA 完成中断：把中断事件变成任务等待

```c
void HAL_SPI_TxCpltCallback(SPI_HandleTypeDef *hspi)
{
    if (hspi->Instance == SPI1)
    {
        BaseType_t xHigherPriorityTaskWoken = pdFALSE;
        xSemaphoreGiveFromISR(xSemphrToDma, &xHigherPriorityTaskWoken);
        portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
    }
}
```

P3.3 的写法是下面这样（**本版已修正**）：

```c
BaseType_t* pxHigherPriorityTaskWoken = pdFALSE;                 // 空指针
xSemaphoreGiveFromISR(xSemphrToDma, pxHigherPriorityTaskWoken);  // 传 NULL
portYIELD_FROM_ISR(xSemphrToDma);                                // 句柄非 0 → 每次都 yield
```

`portYIELD_FROM_ISR()` 需要的是「**是否需要切换任务**」的布尔值，
不是信号量句柄；把信号量句柄传进去每次都会请求 PendSV，白白多一次上下文切换。

时序：

```
任务 vLcdTask                DMA1_CH3 / SPI1             中断
    │ SendData()
    │ RS=1, NSS=0
    │ HAL_SPI_Transmit_DMA ──▶ 逐字节搬运 ……
    │ xSemaphoreTake(阻塞)                             传输完成
    │                             ◀────────────────── HAL_SPI_TxCpltCallback
    │                                                    xSemaphoreGiveFromISR
    │ ◀────────────── 被唤醒 ───────────────────────────────┘
    │ NSS=1
    ▼ 继续执行（收尾 / 下一帧）
```

---

## 八、为什么 LCD 初始化必须放进任务里（HardFault 复盘）

P3.3 在 `main()` 里、`vTaskStartScheduler()` **之前**调用 `Lcd_Init()`，P4.1 把它挪进了 `vLcdTask`。

原因：

1. `Lcd_Init()` → `lcd_init()` → 库通过 `SendData()` 发数据；
2. `SendData()` 用 `HAL_SPI_Transmit_DMA()` + `xSemaphoreTake()` 等待；
3. 但此刻调度器还没启动——DMA 完成中断里 `portYIELD_FROM_ISR()` 请求 PendSV；
4. FreeRTOS 的 PendSV 上下文切换使用 **PSP**，而 PSP 还没被调度器初始化（仍为 0）；
5. 第一次上下文切换访问非法栈地址 → **HardFault，程序直接跑飞**。

所以正确顺序是：

```
main()：初始化外设 → 创建任务 → vTaskStartScheduler()
                                          │
vLcdTask 开始运行 ◀───────────────────────┘
  └─ Lcd_Init()  …此时 FreeRTOS 已就绪，DMA/信号量/中断嵌套都安全
```

> 同理，在调度器启动前也不要用 `xSemaphoreTake()` 这类会阻塞的 RTOS API。

---

## 九、蜂鸣器与按键联动

蜂鸣器的控制被拆成「请求」和「执行」两半（`User/task/buzzer_task.c`）：

```c
static SemaphoreHandle_t xBuzzerSemphr;

void vBuzzerTask(void *pvParameter)
{
    Buzzer_Init();                                       // 创建二值信号量
    for (;;)
    {
        xSemaphoreTake(xBuzzerSemphr, portMAX_DELAY);    // 没请求就一直睡
        HAL_GPIO_WritePin(Buzzer_GPIO_Port, Buzzer_Pin, GPIO_PIN_SET);   // 响
        vTaskDelay(pdMS_TO_TICKS(100));                  // 100 ms
        HAL_GPIO_WritePin(Buzzer_GPIO_Port, Buzzer_Pin, GPIO_PIN_RESET); // 停
    }
}

void Buzzer_beep(void)                                   // 谁都能调用
{
    xSemaphoreGive(xBuzzerSemphr);                       // 只是「放一个信号」
}
```

按键回调因此变得非常短：

```c
void key1ClickCallBack(void)
{
    Buzzer_beep();      // 不阻塞、不延时，按完立刻返回继续扫描
}
```

好处：

- **扫描任务不会被蜂鸣器的 100 ms 拖慢**，其他按键照样能被及时扫到；
- 蜂鸣器任务大部分时间阻塞在信号量上，不占 CPU；
- 以后要加「按键响不同的音」「长按报警」，只改 `Buzzer_beep` / 蜂鸣器任务即可。

---

## 十、本版新增 / 修改的文件

| 文件 | 类型 | 说明 |
|---|---|---|
| `Drivers/lcd.a` | 新增 | 厂家预编译 LCD 静态库 |
| `Drivers/lcd.h` | 新增 | 厂家库头文件（回调结构体 + 绘图 API） |
| `Drivers/lcd_tool.exe` | 新增 | 生成 `fp.h` 的厂家工具 |
| `Drivers/fp.h` | 新增 | 本机硬件指纹（由 `lcd_tool.exe` 生成） |
| `User/task/Lcd_task.c` / `.h` | 修改 | 改为注册厂家库回调；绘制 Run Mode 面板；修正 DMA 完成回调 |
| `Core/Src/main.c` | 修改 | 注释掉调度器启动前的 `Lcd_Init()`；SPI 分频 256 → 4 |
| `CMakeLists.txt` | 修改 | 工程名 `p3.3` → `p4.1`；用 `IMPORTED` 目标链接 `lcd.a` + `libm` |
| `p4.1.ioc` | 新增 | 本版 CubeMX 工程（替换 `p3.3.ioc`） |
| `.vscode/launch.json` | 新增 | ST-Link + GDB 调试配置（`p4.1.elf`） |
| `README.md` | 修改 | 更新为 P4.1 说明 |
| `User/task/image1.h` / `image2.h` | 保留未用 | P3.3 的图片数据，本版不再参与链接 |

---

## 十一、编译与烧录

### 命令行

```bash
# 首次配置
cmake --preset Debug

# 编译
cmake --build build/Debug
```

成功后生成：

```
build/Debug/p4.1.elf
```

本版实测编译结果（Debug，开启 `--gc-sections`）：

```
text   55456 B
data      96 B
bss    17632 B
----------------------------
FLASH  55564 B / 256 KB  ≈ 21.2%
RAM    17728 B / 48 KB   ≈ 36.1%
```

> FLASH 比 P3.3 小很多，是因为 `image1.h` / `image2.h` 已不再被引用，
> 链接时被 `--gc-sections` 丢弃了。
> RAM 中约 8 KB 是 FreeRTOS 堆（`configTOTAL_HEAP_SIZE`），厂家库本身也占用了一部分。

### VS Code

1. 安装 STM32Cube for VS Code 扩展；
2. 打开本目录，选择 `Debug` 预设；
3. 使用扩展的 Build / Debug / Flash 按钮；
4. 调试接口请选 **SWD**（PB3/PB4 已被用作 LCD，JTAG 已关闭）；
5. `.vscode/launch.json` 已配置 ST-Link（端口 6124）。

### CMake 说明

- `FreeRTOS/Src/*.c` —— RTOS 内核
- `User/*.c`（递归）—— 任务层 + 所有按键模块（新增文件会自动参与编译）
- `Drivers/key/*.c` —— 基础按键驱动
- `Drivers/lcd.a` —— 通过 `IMPORTED` 目标 `lcd` 链接，头文件目录自动加入
- `Drivers/STM32F1xx_HAL_Driver`、`Drivers/CMSIS` 由 `cmake/stm32cubemx` 子工程编译

> ⚠️ 不要递归收集整个 `Drivers/`，否则 HAL 源文件会被编译两次，
> 链接时报 `multiple definition`。

---

## 十二、版本管理（每版都有自己的 README 和代码）

本仓库用 **Git 标签（tag）** 保存每个完整版本，每个标签都是一个快照，
包含**当时的全部代码 + 当时的 README**：

| 标签 | 内容 | 查看方式 |
|---|---|---|
| `v1.2` | 单按键可复用驱动（结构体 + 回调 + 指针句柄） | GitHub 左上角分支/标签下拉选 `v1.2` |
| `v1.3` | 多按键分层重构（`User/keys` 模块化 + 单任务统一扫描） | 选 `v1.3` |
| `v3.3` | SPI+DMA 屏幕、蜂鸣器信号量、LED 任务 | 选 `v3.3` |
| `v4.1` | 接入厂家预编译 LCD 库、界面面板、HardFault 修复（本版） | 选 `v4.1` 或直接看 `main` |

- GitHub 页面：仓库首页左上角点击 `main`，在 **Tags** 里切换版本；
- 仓库的 **Releases** 页面也按版本列出了说明和源码压缩包；
- 命令行查看旧版：

  ```bash
  git tag                        # 列出所有版本
  git show v3.3:README.md        # 看 P3.3 版 README
  git checkout v3.3              # 切到 P3.3 版代码（看完记得切回 main）
  ```

> 切到旧标签后代码是只读的「快照」，不要在上面直接改；要基于旧版继续开发，应新建分支。

---

## 十三、已知局限 / 可改进

1. **厂家库初始化会动 PC1（LED3 的引脚）**

   `lcd_generic_init()` 会**无条件**调用内部的 `bsp_get_key4_state()`：
   它把 **PC1 重新配置成输入上拉并读取**（用来判断某个按键状态）。
   本板 PC1 是 LED3，本版没有使用 LED3，所以无影响；
   如果以后要用 LED3，必须在 `lcd_init()` 之后**重新把 PC1 初始化成推挽输出**。

2. **厂家库默认 BSP 与开发板不一致**

   回调全为 NULL 时，库会走 `hspi1`、CS = PD4、RS = PC12；本板 CS 是 PD2。
   因此 4 个回调必须全部提供（本版已提供），不要图省事传 NULL。

3. **`fp.h` 是机器指纹**

   换电脑编译要重新运行 `lcd_tool.exe` 生成新的 `fp.h`，否则可能初始化失败。
   仓库里附带的 `fp.h` 只对当前这台电脑有效。

4. **任务里仍在用 `HAL_Delay()`**
   `Lcd_Reset()` 里的 `HAL_Delay(1)` / `HAL_Delay(5)` 是**忙等**，期间不让出 CPU。
   时间很短，影响不大；严格可改成 `vTaskDelay()` 或延时回调。

5. **没有软件消抖**
   目前靠 10 ms 扫描周期天然过滤部分抖动。严格场合可加入「连续 N 次电平一致才确认」。

6. **没有长按 / 双击**
   可在句柄结构体中加入计时字段与状态机，扩展出长按、连击等事件。

7. **按键回调全部相同**
   六个 `keyN.c` 的回调现在都只有 `Buzzer_beep()` 一行。
   进阶可用「配置表 + 统一初始化」的表驱动写法，或宏批量生成模块，减少重复。

8. **二值信号量会「合并」连续按键**
   蜂鸣器正在响时再按一次，信号量只会保留一个「待响」请求，
   不会排队响两次。需要逐次响应时可改用计数信号量或队列。

9. **DMA 中断优先级固定为 5**
   等于 `configMAX_SYSCALL_INTERRUPT_PRIORITY`（5<<4），刚好允许调用 `FromISR` API。
   若以后调高该中断优先级，必须同步检查 FreeRTOS 的优先级上限，否则会破坏内核临界区。

10. **UART 已初始化但未使用**
    本版按键改为蜂鸣器反馈，USART1（PA9/PA10，115200）暂时闲置，可用于后续调试输出。

11. **图片数据头文件未删**
    `image1.h` / `image2.h` 已不参与编译，但仍在仓库里，各约 168 KB 文本。
    想继续显示图片时，可以用厂家库的 `lcd_draw_bitmap(120, 160, (uint8_t*)image1)` 直接画（RGB565），
    或者干脆删除这两个文件。

12. **SPI 已提升到 18 MHz**
    如果杜邦线较长、屏幕排线质量一般，可能出现花屏 / 错位；
    可以先把分频从 4 调回 8 或 16 验证，排除时序问题。

---

## 十四、版本说明

| 版本 | 内容 |
|---|---|
| 1.2 | 可复用按键驱动：结构体 + 回调 + 指针句柄，一套驱动支持任意多个按键 |
| 1.3 | 多按键分层重构：按键模块化（`User/keys`）+ 单任务统一扫描（`User/task`） |
| 3.3 | 外设与多任务联动：SPI+DMA 驱动 LCD 显示图片、蜂鸣器信号量、LED 任务 |
| 4.1 | 接入厂家预编译 LCD 库（回调 + IMPORTED 链接 + `fp.h`）、界面面板绘制、初始化时机与中断回调修复 |

---

## 许可

本项目仅用于个人学习与交流。
`Drivers/lcd.a`、`Drivers/lcd_tool.exe` 为厂家提供的二进制文件，版权归厂家所有。
