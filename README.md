# STM32 FreeRTOS 学习项目 —— P3.3：SPI 屏幕 + 蜂鸣器 + 多任务联动

> 基于 STM32F103 + FreeRTOS 的练习项目。
> 1.3 版解决了「多个按键怎么统一管理」；本版（P3.3）在此基础上扩展成一个完整的小系统：
> **按键 → 蜂鸣器**、**SPI + DMA 驱动 TFT 屏幕显示图片**、**LED 闪烁**，四个任务交给 FreeRTOS 调度。

---

## 一、这一版做了什么（和 1.3 对比）

| 项目 | 1.3 版 | P3.3 版 |
|---|---|---|
| 按键 | 6 个，松手后串口打印一行 | 6 个，松手后**蜂鸣器响 100 ms** |
| 按键扫描 | `keytask` 统一扫描 | **不变**，继续复用 |
| 显示 | 无 | **SPI1 + DMA 驱动 TFT 彩屏**，交替显示两张图片 |
| 蜂鸣器 | 无 | 新增 `vBuzzerTask`，用**二值信号量**接收「响一声」请求 |
| LED | 无 | 新增 `vLedTask`，LED1 每 100 ms 翻转 |
| 串口 | 每次按键打印 | USART1 仍初始化（115200），本版未使用 |
| 新增外设 | GPIO / USART1 | **SPI1 + DMA1_CH3**、TIM6 作为 HAL 时基 |
| FreeRTOS 堆 | 4096 B | **8192 B**（4 个用户任务需要更多堆） |
| 任务数量 | 1 个用户任务 | 4 个用户任务（按键 / LCD / 蜂鸣器 / LED） |
| 工程名 | experiment1.3 | **p3.3**（`p3.3.ioc`，产物 `p3.3.elf`） |

一句话：**1.3 把按键做成了能复用的三层结构，P3.3 在这套结构上接入了屏幕、声音和执行器，开始练多任务协作。**

---

## 二、运行现象

1. 上电后先点亮 LCD 背光并复位屏幕（`Lcd_Init()`，在调度器启动前调用）；
2. LCD 任务启动后发送初始化命令（退出睡眠、RGB565、开显示）；
3. 屏幕中央先显示 `image1`，200 ms 后切换为 `image2`，之后不断交替；
4. 任意按键**松手瞬间**，蜂鸣器响 100 ms；
5. LED1（PC3）以 100 ms 亮 / 100 ms 灭的节奏闪烁；
6. 串口 USART1 已配置为 115200，本版没有往串口发数据。

---

## 三、硬件与软件环境

| 项目 | 说明 |
|---|---|
| 主控 | STM32F103RCTx（256 KB Flash / 48 KB RAM，Cortex-M3） |
| 主频 | 72 MHz（HSE 8 MHz × PLL9） |
| 屏幕 | SPI 接口 TFT 彩屏，按 240×320 坐标使用，命令集与 ST7789 兼容 |
| 按键 | 6 个，输入上拉，按下接 GND（松手触发） |
| 蜂鸣器 | PC8，高电平响（有源蜂鸣器） |
| LED | LED1 = PC3（本版使用）；LED3 = PC1（未使用） |
| RTOS | FreeRTOS **V11.1.0**，内存管理 `heap_4`，堆 8 KB，tick 100 Hz |
| HAL 时基 | **TIM6 1 ms**（`stm32f1xx_hal_timebase_tim.c`，避免和 FreeRTOS 抢 SysTick） |
| HAL | STM32F1xx HAL Driver（启用 GPIO / USART / SPI / DMA / TIM） |
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

### LCD 引脚（本版新增）

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
| SPI1 | PB3 / PB5 | 主机、8 位、MSB、模式 0、**分频 256** |
| DMA1_CH3 | — | SPI1_TX，内存→外设，字节对齐，普通模式，中断优先级 5 |

> SPI1 位于 APB2（72 MHz），分频 256 后 SCLK ≈ **281 kHz**。
> 一帧 120×160 的图像有 38400 字节，单帧传输约需 1.1 秒，
> 所以屏幕切换的实际节奏会比代码里的 200 ms 慢很多（详见「已知局限」）。

---

## 四、目录结构

```
p3.3/
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
│   ├── key/                      # 【基础驱动层】与具体按键无关
│   │   ├── key.h                 #   KeyHandle_Typedef + 接口声明
│   │   └── key.c                 #   key_init / key_scan 实现
│   ├── STM32F1xx_HAL_Driver/     # ST HAL 库（本版新增 SPI 驱动）
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
│       ├── Lcd_task.c / Lcd_task.h     # LCD 初始化、SPI+DMA 刷屏任务
│       ├── buzzer_task.c / buzzer_task.h # 蜂鸣器任务
│       ├── image1.h                    # 第一帧图片数据（RGB565）
│       └── image2.h                    # 第二帧图片数据（RGB565）
├── cmake/stm32cubemx/            # CubeMX 自动生成的 CMake 子工程
├── CMakeLists.txt                # 顶层构建脚本（工程名 p3.3）
├── CMakePresets.json
├── p3.3.ioc                      # CubeMX 工程文件
└── README.md
```

---

## 五、任务划分与调度

`main.c` 在启动调度器前依次创建 4 个任务（栈大小单位为字，1 字 = 4 字节）：

| 任务 | 函数 | 栈（字 / 字节） | 优先级 | 运行方式 | 职责 |
|---|---|---|---|---|---|
| 按键 | `keytask` | 128 / 512 B | **3**（最高） | 每 10 ms 扫描一轮 | 扫描 6 个按键，松手 → `Buzzer_beep()` |
| LCD | `vLcdTask` | 256 / 1024 B | 2 | 循环刷两帧 | 发初始化命令、设置窗口、SPI+DMA 送图 |
| 蜂鸣器 | `vBuzzerTask` | 64 / 256 B | 1 | 等信号量 | 收到信号 → PC8 拉高 100 ms 再拉低 |
| LED | `vLedTask` | 128 / 512 B | 1 | 每 100 ms 翻转 | LED1 闪烁 |
| 空闲 | Idle | 128 / 512 B | 0 | 空闲时运行 | FreeRTOS 内核任务 |

```c
/* main.c */
Lcd_Init();                                                // 调度器启动前：背光 + 复位
xTaskCreate(keytask,    "Key",    128, NULL, 3, NULL);     // 按键扫描
xTaskCreate(vBuzzerTask,"Buzzer",  64, NULL, 1, NULL);     // 蜂鸣器
xTaskCreate(vLcdTask,   "LCD",    256, NULL, 2, NULL);     // 屏幕
xTaskCreate(vLedTask,   "led",    128, NULL, 1, NULL);     // LED
vTaskStartScheduler();
```

### 为什么这样分配优先级

```
优先级 3  keytask      —— 按键响应要快，且 10 ms 内一定能跑完
优先级 2  vLcdTask     —— 刷屏耗时长，但可以被按键抢占
优先级 1  Buzzer / LED —— 对实时性要求最低
```

- 按键扫描每 10 ms 只读 6 次 GPIO，几乎不占 CPU，但优先级给最高，保证不漏按键；
- LCD 刷一帧要等 DMA 搬运完（单帧约 1.1 s），期间任务阻塞在信号量上，不会空转；
- 蜂鸣器和 LED 都是低频动作，放在最低优先级，任何时候被打断都无所谓。

### 任务之间的协作关系

```
按键松手
  key1ClickCallBack() ── Buzzer_beep() ──▶ [二值信号量] ──▶ vBuzzerTask
                                                            PC8 高 100 ms
按键任务只做「发请求」，真正控制蜂鸣器的是蜂鸣器任务：
扫描任务不会被 100 ms 的响声拖住。
```

---

## 六、LCD 驱动：SPI1 + DMA + 二值信号量

本版的 LCD 驱动全部写在 `User/task/Lcd_task.c` 中，分四层理解：**底层命令/数据发送 → DMA 同步 → 绘图命令 → 图片显示**。

### 1. 命令与数据

屏幕靠一根 `RS` 引脚区分「命令」和「数据」：

```c
void SendComand(uint8_t cmd)                    // 发 1 字节命令
{
    HAL_GPIO_WritePin(Lcd_RS_GPIO_Port, Lcd_RS_Pin, GPIO_PIN_RESET);  // RS=0：命令
    HAL_GPIO_WritePin(Lcd_NSS_GPIO_Port, Lcd_NSS_Pin, GPIO_PIN_RESET);// 片选拉低
    HAL_SPI_Transmit(&hspi1, &cmd, 1, HAL_MAX_DELAY);                 // 阻塞发送 1 字节
    HAL_GPIO_WritePin(Lcd_NSS_GPIO_Port, Lcd_NSS_Pin, GPIO_PIN_SET);  // 片选拉高
}

void SendData(uint8_t *pData, uint16_t size)    // 发数据（可能是整帧图片）
{
    HAL_GPIO_WritePin(Lcd_RS_GPIO_Port, Lcd_RS_Pin, GPIO_PIN_SET);    // RS=1：数据
    HAL_GPIO_WritePin(Lcd_NSS_GPIO_Port, Lcd_NSS_Pin, GPIO_PIN_RESET);
    HAL_SPI_Transmit_DMA(&hspi1, pData, size);                        // 启动 DMA，立即返回
    xSemaphoreTake(xSemphrToDma, portMAX_DELAY);                      // 等传输完成
    HAL_GPIO_WritePin(Lcd_NSS_GPIO_Port, Lcd_NSS_Pin, GPIO_PIN_SET);
}
```

命令只有 1 字节，不值得上 DMA；数据可能多达几万字节，必须用 DMA。

### 2. DMA + 信号量：把中断事件变成任务等待

DMA 传输完成后会进中断，但**中断里不能做耗时的后续工作**，
所以用「二值信号量」把「传输完成」这个事件交给任务：

```c
SemaphoreHandle_t xSemphrToDma;

// 任务端：启动 DMA 后阻塞等待
HAL_SPI_Transmit_DMA(&hspi1, pData, size);
xSemaphoreTake(xSemphrToDma, portMAX_DELAY);

// 中断端：SPI 发送完成回调
void HAL_SPI_TxCpltCallback(SPI_HandleTypeDef *hspi)
{
    if (hspi->Instance == SPI1)
    {
        xSemaphoreGiveFromISR(xSemphrToDma, ...);   // 通知任务：发完了
        portYIELD_FROM_ISR(...);                     // 必要时立刻切换
    }
}
```

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
    ▼ 继续执行（收尾/下一帧）
```

这样在 DMA 搬运整帧图片的时间里，CPU 可以运行按键任务、LED 任务，而不是空等。

> ⚠️ 本版回调里的 `pxHigherPriorityTaskWoken` 写法有误（传进去的是信号量句柄而不是标志变量），
> 详见「已知局限」第 1 条。功能上能跑，但每次传输都会无条件触发一次上下文切换。

### 3. 显示图片

ST7789 风格的绘图流程是：**设置列范围 → 设置行范围 → 写显存**：

```c
static void SetRowRange(uint16_t start, uint16_t end)   // 0x2B
static void SetColumnRange(uint16_t start, uint16_t end) // 0x2A

static void ShowImage(uint16_t *pImage, uint16_t size)  // 0x2C + 图像数据
{
    SendComand(0x2C);                       // 写显存命令
    SendData((uint8_t*)pImage, size * 2);   // 每个像素 2 字节（RGB565）
}

void ShowImage1(void)
{
    SetRowRange(80, 239);                   // 160 行
    SetColumnRange(60, 179);                // 120 列
    ShowImage(image1, 120 * 160);           // 19200 个像素
}
```

- 窗口为 **120 列 × 160 行**，位于 240×320 屏幕的正中央（左右各留 60 列，上下各留 80 行）；
- 每帧 19200 像素 × 2 字节 = **38400 字节**；
- `image1.h` / `image2.h` 用 `const uint16_t` 存了整帧数据，放在 **Flash** 里，不占 RAM；
  两帧合计约 75 KB Flash。

### 4. 初始化序列

```c
void Lcd_Init(void)                 // 调度器启动前调用
{
    xSemphrToDma = xSemaphoreCreateBinary();
    LcdBlackLight_Control(1);       // 背光开
    Lcd_Reset();                    // RST 低 1 ms → 高 5 ms
}

void vLcdTask(void *pvParameter)    // 任务启动后
{
    SendComand(0x11);               // 退出睡眠
    HAL_Delay(125);                 // 等屏幕苏醒
    SendComand(0x3A);               // 像素格式
    pDataToSend[0] = 0x55;          // 0x55 = 16 位/像素（RGB565）
    SendData(pDataToSend, 1);
    SendComand(0x29);               // 开显示
    ...
}
```

| 命令 | 含义 | 参数 |
|---|---|---|
| `0x11` | Sleep Out（退出睡眠） | — |
| `0x3A` | COLMOD（像素格式） | `0x55` = RGB565 |
| `0x2A` | 列地址范围 | 4 字节：起始高/低、结束高/低 |
| `0x2B` | 行地址范围 | 同上 |
| `0x2C` | 写显存 | 像素数据流 |
| `0x29` | Display ON（开显示） | — |

---

## 七、蜂鸣器与按键联动

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

## 八、本版新增 / 修改的文件

| 文件 | 类型 | 说明 |
|---|---|---|
| `User/task/Lcd_task.c` / `.h` | 新增 | LCD 初始化、SPI+DMA 发送、绘图、刷图任务 |
| `User/task/buzzer_task.c` / `.h` | 新增 | 蜂鸣器任务 + `Buzzer_beep()` 接口 |
| `User/task/image1.h` / `image2.h` | 新增 | 两帧 19200 像素的 RGB565 图像数据 |
| `User/task/key_task.c` | 修改 | 仍统一扫描 6 个按键（回调改由各 key 模块负责） |
| `User/keys/key*.c` | 修改 | 回调从「串口打印」改为 `Buzzer_beep()` |
| `Core/Src/main.c` | 修改 | 新增 SPI1 / DMA 初始化，创建 4 个任务 |
| `Core/Src/stm32f1xx_hal_msp.c` | 修改 | SPI1 重映射、PB3/PB5 复用、DMA1_CH3、关闭 JTAG |
| `Core/Src/stm32f1xx_it.c` / `stm32f1xx_it.h` | 修改 | 新增 `DMA1_Channel3_IRQHandler()` |
| `Core/Inc/main.h` | 修改 | 新增 LCD / 蜂鸣器引脚宏 |
| `Core/Inc/FreeRTOSConfig.h` | 修改 | `configTOTAL_HEAP_SIZE` 4096 → 8192 |
| `Core/Inc/stm32f1xx_hal_conf.h` | 修改 | 启用 `HAL_SPI_MODULE_ENABLED`、`HAL_DMA_MODULE_ENABLED` |
| `Drivers/STM32F1xx_HAL_Driver/*hal_spi*` | 新增 | HAL SPI 驱动（CubeMX 生成） |
| `p3.3.ioc` | 新增 | 本版 CubeMX 工程（替换 `expriment1.3.ioc`） |
| `CMakeLists.txt` | 修改 | 工程名 `experiment1.3` → `p3.3` |
| `cmake/stm32cubemx/CMakeLists.txt` | 修改 | 加入 `stm32f1xx_hal_spi.c` 参与编译 |

---

## 九、编译与烧录

### 命令行

```bash
# 首次配置
cmake --preset Debug

# 编译
cmake --build build/Debug
```

成功后生成：

```
build/Debug/p3.3.elf
```

本版编译结果（Debug）：

```
text   102608 B
data       96 B
bss     10816 B
----------------------------
FLASH  102704 B / 256 KB  ≈ 39.2%
RAM     10912 B / 48 KB   ≈ 22.2%
```

> RAM 中的 10816 B 包含 FreeRTOS 的 8 KB 堆（`configTOTAL_HEAP_SIZE`）；
> FLASH 中的大头是两帧图片（约 75 KB）和 HAL 库。

### VS Code

1. 安装 STM32Cube for VS Code 扩展；
2. 打开本目录，选择 `Debug` 预设；
3. 使用扩展的 Build / Debug / Flash 按钮；
4. 调试接口请选 **SWD**（PB3/PB4 已被用作 LCD，JTAG 已关闭）。

### CMake 说明

- `FreeRTOS/Src/*.c` —— RTOS 内核
- `User/*.c`（递归）—— 任务层 + 所有按键模块（新增文件会自动参与编译）
- `Drivers/key/*.c` —— 基础按键驱动
- `Drivers/STM32F1xx_HAL_Driver`、`Drivers/CMSIS` 由 `cmake/stm32cubemx` 子工程编译

> ⚠️ 不要递归收集整个 `Drivers/`，否则 HAL 源文件会被编译两次，
> 链接时报 `multiple definition`。

---

## 十、版本管理（每版都有自己的 README 和代码）

本仓库用 **Git 标签（tag）** 保存每个完整版本，每个标签都是一个快照，
包含**当时的全部代码 + 当时的 README**：

| 标签 | 内容 | 查看方式 |
|---|---|---|
| `v1.2` | 单按键可复用驱动（结构体 + 回调 + 指针句柄） | GitHub 左上角分支/标签下拉选 `v1.2` |
| `v1.3` | 多按键分层重构（`User/keys` 模块化 + 单任务统一扫描） | 选 `v1.3` |
| `v3.3` | SPI+DMA 屏幕、蜂鸣器信号量、LED 任务（本版） | 选 `v3.3` 或直接看 `main` |

- GitHub 页面：仓库首页左上角点击 `main`，在 **Tags** 里切换版本；
- 仓库的 **Releases** 页面也按版本列出了说明和源码压缩包；
- 命令行查看旧版：

  ```bash
  git tag                        # 列出所有版本
  git show v1.3:README.md        # 看 1.3 版 README
  git checkout v1.3              # 切到 1.3 版代码（看完记得切回 main）
  ```

> 切到旧标签后代码是只读的「快照」，不要在上面直接改；要基于旧版继续开发，应新建分支。

---

## 十一、已知局限 / 可改进

1. **SPI 完成回调里的「唤醒标志」写法有误**

   当前代码：

   ```c
   BaseType_t* pxHigherPriorityTaskWoken = pdFALSE;   // 这是指针，且初始化为 NULL
   xSemaphoreGiveFromISR(xSemphrToDma, pxHigherPriorityTaskWoken);  // 传 NULL（当前允许）
   portYIELD_FROM_ISR(xSemphrToDma);                  // 传的是信号量句柄 → 永远非空 → 每次都 yield
   ```

   正确写法是传一个变量的地址，让宏根据它决定是否需要切换：

   ```c
   BaseType_t xHigherPriorityTaskWoken = pdFALSE;
   xSemaphoreGiveFromISR(xSemphrToDma, &xHigherPriorityTaskWoken);
   portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
   ```

   现在功能正常，但每次 DMA 传输都会多触发一次 PendSV，属于不必要的开销。

2. **SPI 分频 256，刷屏很慢**
   SPI1 在 APB2（72 MHz），256 分频后 SCLK ≈ 281 kHz，一帧 38400 字节约需 1.1 s。
   代码里 `vTaskDelay(200 ms)` 的切换节奏实际上被传输时间主导。
   如果屏幕和走线允许，可把 `SPI_BAUDRATEPRESCALER_256` 改为 `/8` 或 `/4`，
   帧率能提升一到两个数量级，同时降低 CPU 被 DMA 中断占用的频率。

3. **任务里使用 `HAL_Delay()`**
   `vLcdTask` 启动时用 `HAL_Delay(125)` 等屏幕苏醒；`vLcdTask` 和 `Lcd_Reset()`
   里的 `HAL_Delay()` 都是**忙等**，期间不会让出 CPU（低优先级的蜂鸣器 / LED 任务会被拖住）。
   建议在任务里改用 `vTaskDelay(pdMS_TO_TICKS(...))`。

4. **没有软件消抖**
   目前靠 10 ms 扫描周期天然过滤部分抖动。严格场合可加入「连续 N 次电平一致才确认」。

5. **没有长按 / 双击**
   可在句柄结构体中加入计时字段与状态机，扩展出长按、连击等事件。

6. **按键回调全部相同**
   六个 `keyN.c` 的回调现在都只有 `Buzzer_beep()` 一行。
   进阶可用「配置表 + 统一初始化」的表驱动写法，或宏批量生成模块，减少重复。

7. **图片数据放在头文件里**
   `image1.h` / `image2.h` 各约 2400 行、168 KB 文本，每次改动都会让预处理变慢。
   可以改为 `image1.c` 里的 `const` 数组 + 头文件只做 `extern` 声明。

8. **二值信号量会「合并」连续按键**
   蜂鸣器正在响时再按一次，信号量只会保留一个「待响」请求，
   不会排队响两次。需要逐次响应时可改用计数信号量或队列。

9. **DMA 中断优先级固定为 5**
   等于 `configMAX_SYSCALL_INTERRUPT_PRIORITY`（5<<4），刚好允许调用 `FromISR` API。
   若以后调高该中断优先级，必须同步检查 FreeRTOS 的优先级上限，否则会破坏内核临界区。

10. **UART 已初始化但未使用**
    本版按键改为蜂鸣器反馈，USART1（PA9/PA10，115200）暂时闲置，可用于后续调试输出。

---

## 十二、版本说明

| 版本 | 内容 |
|---|---|
| 1.2 | 可复用按键驱动：结构体 + 回调 + 指针句柄，一套驱动支持任意多个按键 |
| 1.3 | 多按键分层重构：按键模块化（`User/keys`）+ 单任务统一扫描（`User/task`） |
| 3.3 | 外设与多任务联动：SPI+DMA 驱动 LCD 显示图片、蜂鸣器信号量、LED 任务 |

---

## 许可

本项目仅用于个人学习与交流。
