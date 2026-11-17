# STM32 FreeRTOS 学习项目 —— 可复用的按键驱动（1.2）

> 基于 STM32F103 + FreeRTOS 的按键驱动练习。
> 本版本的核心目标：**把「只支持一个按键」的驱动，改造成「一套代码支持任意多个按键」的可复用驱动。**

---

## 一、这一版做了什么

按键驱动从「一堆全局变量 + 写死的逻辑」，重构为 **结构体 + 函数指针回调 + 指针句柄** 的面向对象式写法：

| 项目 | 改造前（全局变量版） | 改造后（本版本） |
|---|---|---|
| 按键状态存放 | 全局变量 `previous` / `current` | 每个按键一个 `KeyHandle_Typedef` 实例 |
| 支持按键数量 | 只能 1 个 | 任意多个 |
| 按键动作 | 写死在 `key_scan()` 里（固定发串口） | 通过 `ClickedCallBack` 注册，上层自由决定 |
| 驱动与业务耦合 | 强耦合 | 解耦，驱动只负责「检测 + 回调」 |
| 函数接口 | `key_init(void)` / `key_scan(void)` | `key_init(KeyHandle_Typedef *)` / `key_scan(KeyHandle_Typedef *)` |

一句话：**驱动不再关心「按下键之后干什么」，它只负责检测并调用你注册的函数。**

---

## 二、功能特性

- ✅ 结构体化按键句柄，天然支持多按键实例
- ✅ 回调函数机制（Callback），业务逻辑与驱动解耦
- ✅ 指针传参，避免结构体拷贝，节省单片机 RAM 与栈开销
- ✅ 非阻塞式扫描，配合 FreeRTOS 的 `vTaskDelay` 周期调用
- ✅ 检测「按下 → 松开」的下降沿（松开变按下时触发一次）
- ✅ 使用 STM32 HAL 库 + FreeRTOS，代码带详细中文注释
- ✅ CMake + Ninja 构建，VS Code 开箱即用

---

## 三、硬件与软件环境

| 项目 | 说明 |
|---|---|
| 主控 | STM32F103RCTx（256 KB Flash / 48 KB RAM，Cortex-M3） |
| 按键 | PC9（`Key1_Pin`），输入上拉，按下接 GND |
| 串口 | USART1，用于输出按键事件 |
| RTOS | FreeRTOS（内核源码位于 `FreeRTOS/`，内存管理 `heap_4`） |
| HAL | STM32F1xx HAL Driver |
| 构建 | CMake ≥ 3.22 + Ninja |
| 工具链 | GNU Tools for STM32 `arm-none-eabi-gcc 14.3.1` |
| 开发环境 | VS Code + STM32Cube for VS Code 扩展（clangd / CMake） |

> GPIO 配置位于 `Core/Src/main.c` 的 `USER CODE` 区域：PC9 设为输入上拉。
> 若你的按键是「按下接 3.3V」，请把 `GPIO_PULLUP` 改为 `GPIO_PULLDOWN`。

---

## 四、目录结构

```
1.2_key/
├── Core/                       # CubeMX 生成的启动、时钟、中断、主程序
│   ├── Inc/
│   └── Src/main.c              # 创建 key1task、GPIO/USART 初始化
├── Drivers/
│   ├── key/                    # ★ 本项目的核心：可复用按键驱动
│   │   ├── key.h               #   按键句柄结构体 + 接口声明
│   │   └── key.c               #   初始化与扫描实现
│   ├── STM32F1xx_HAL_Driver/   # ST HAL 库
│   └── CMSIS/                  # CMSIS 内核与设备头文件
├── FreeRTOS/                   # FreeRTOS 内核（Src / Inc）
├── User/Task/
│   ├── Key_Task.c              # ★ 按键任务：注册回调并周期扫描
│   └── Key_Task.h
├── cmake/stm32cubemx/          # CubeMX 自动生成的 CMake 子工程
├── CMakeLists.txt              # 顶层构建脚本
├── CMakePresets.json
├── 1.2_key.ioc                 # CubeMX 工程文件
└── README.md
```

---

## 五、核心设计讲解

### 1. 按键句柄结构体（`key.h`）

```c
typedef struct
{
    GPIO_TypeDef* GPIO_Port;              // 按键所在 GPIO 端口（如 GPIOC）
    uint16_t      GPIO_Pin;               // 按键引脚号（如 GPIO_PIN_9）

    GPIO_PinState previous;               // 上一次扫描到的电平
    GPIO_PinState current;                // 本次扫描到的电平

    void (*ClickedCallBack)(void);        // 点击回调：按下时执行的函数
} KeyHandle_Typedef;
```

| 字段 | 作用 |
|---|---|
| `GPIO_Port` / `GPIO_Pin` | 描述「这个按键接在哪个引脚」，使同一份驱动能服务不同引脚 |
| `previous` / `current` | 保存前后两次电平，用于判断下降沿（松开 → 按下） |
| `ClickedCallBack` | 函数指针。**注册后，驱动不知道也不关心它具体做什么**，这就是解耦的关键 |

### 2. 这个函数指针到底是什么意思

```c
void (*ClickedCallBack)(void);
```

按「由内向外、先右后左」读：

- `ClickedCallBack` —— 变量名，一个**指针**
- `(void)` 在右边 —— 它指向的函数**不带参数**
- `void` 在左边 —— 那个函数**没有返回值**

合起来：**指向「无参数、无返回值」函数的指针**。

> 注意括号不能省：
> `void (*fp)(void);` 是函数指针变量；
> `void *fp(void);` 是返回 `void*` 的普通函数。

### 3. 初始化与扫描（`key.c`）

```c
void key_init(KeyHandle_Typedef *handle)
{
    handle->previous = GPIO_PIN_SET;      // 初始电平视为「松开」
    handle->current  = GPIO_PIN_SET;
}

void key_scan(KeyHandle_Typedef *handle)
{
    handle->current = HAL_GPIO_ReadPin(handle->GPIO_Port, handle->GPIO_Pin);

    // 上一刻是低电平（按下），这一刻变高电平（松开）→ 判定为一次点击
    if ((handle->current == GPIO_PIN_SET) && (handle->previous == GPIO_PIN_RESET))
    {
        handle->ClickedCallBack();        // 触发上层注册的回调
    }
    handle->previous = handle->current;   // 更新历史状态
}
```

为什么参数是 `KeyHandle_Typedef *handle`：

1. **C 只有值传递**：按值传结构体会复制一份，函数里改的是副本，原对象不变；传指针才能把初始化结果写回原对象。
2. **省内存**：指针固定 4 字节，结构体可能很大，单片机栈空间有限。
3. **复用**：同一份 `key_init/key_scan` 通过传入不同地址，即可处理任意多个按键。

---

## 六、使用方法

### 第一步：定义按键对象和回调

```c
KeyHandle_Typedef hkey1;                  // 一个按键对应一个句柄

static void Key1ClickCallBack(void)       // 按键按下时要做什么
{
    extern UART_HandleTypeDef huart1;
    HAL_UART_Transmit(&huart1,
                      (const uint8_t *)"key1 clicked.\n",
                      strlen("key1 clicked.\n"),
                      HAL_MAX_DELAY);
}
```

### 第二步：在任务中配置并扫描

```c
void key1task(void *pvParameter)
{
    hkey1.GPIO_Pin        = Key1_Pin;         // GPIOC / GPIO_PIN_9
    hkey1.GPIO_Port       = Key1_GPIO_Port;
    hkey1.ClickedCallBack = Key1ClickCallBack; // 注册回调

    key_init(&hkey1);

    for (;;)
    {
        key_scan(&hkey1);                     // 周期扫描
        vTaskDelay(pdMS_TO_TICKS(10));        // 10 ms 扫一次
    }
}
```

### 第三步：创建任务

```c
xTaskCreate(key1task, "Key1", 128, NULL, 1, NULL);
```

### 想再加一个按键？

```c
KeyHandle_Typedef hkey2;

void key2task(void *pv)
{
    hkey2.GPIO_Pin        = Key2_Pin;
    hkey2.GPIO_Port       = Key2_GPIO_Port;
    hkey2.ClickedCallBack = Key2ClickCallBack;  // 换成自己的逻辑

    key_init(&hkey2);
    for (;;)
    {
        key_scan(&hkey2);
        vTaskDelay(pdMS_TO_TICKS(10));
    }
}
```

**驱动代码一行都不用改**，这就是「复用」的意义。

---

## 七、编译与烧录

### 命令行构建

```bash
# 配置（首次）
cmake --preset Debug

# 编译
cmake --build build/Debug
```

成功后会生成 `build/Debug/1.2_key.elf`。

### VS Code

1. 安装 STM32Cube for VS Code 扩展；
2. 打开本目录，选择 `Debug` 预设；
3. 使用扩展的 Build / Debug / Flash 按钮。

### CMake 说明

`CMakeLists.txt` 使用 `GLOB_RECURSE` 自动收集源文件：

- `FreeRTOS/Src/*.c` —— RTOS 内核
- `User/*.c` —— 用户任务代码
- `Drivers/key/*.c` —— 用户驱动代码

> ⚠️ 注意：**不要**把 `Drivers/` 整个目录递归收集，因为 `Drivers/STM32F1xx_HAL_Driver` 和 `Drivers/CMSIS` 中的文件已经由 `cmake/stm32cubemx` 子工程编译，重复收集会导致链接时 `multiple definition`。

---

## 八、当前版本的已知局限 / 后续可改进

本项目是学习用途，以下点可以继续完善（也是很好的练习）：

1. **回调前没有判空**  
   `key_scan()` 直接调用 `handle->ClickedCallBack()`。如果忘记注册回调，会跳到野指针导致 HardFault。建议加：
   ```c
   if (handle->ClickedCallBack != NULL) {
       handle->ClickedCallBack();
   }
   ```
   并在定义句柄时清零：`KeyHandle_Typedef hkey1 = {0};`

2. **没有软件消抖**  
   目前只靠 10 ms 扫描周期天然过滤部分抖动。严格场合可加入「连续 N 次电平一致才确认」的消抖逻辑。

3. **没有长按 / 双击等高级事件**  
   可在结构体中增加计时字段与状态机，扩展出长按、连击等语义。

4. **GPIO 初始化与驱动分离**  
   目前引脚在 `main.c` 中配置，若希望驱动完全自包含，可把 GPIO 初始化也收进 `key_init()`。

---

## 九、学习要点总结

- **结构体是 C 语言的「对象」**：把与一个按键相关的所有数据打包在一起。
- **函数指针是 C 语言的「多态」**：同一行调用代码，可以执行完全不同的行为。
- **指针传参是 C 语言的「引用」**：想要修改调用者的数据，就必须传地址。
- **回调 + 结构体 = 可复用驱动的标准写法**：FreeRTOS 的定时器回调、HAL 的 `xxxCallback` 都是同一思想。

---

## 十、版本说明

| 版本 | 内容 |
|---|---|
| 1.2 | 按键驱动重构：结构体 + 回调 + 指针句柄，实现多按键复用 |

---

## 许可

本项目仅用于个人学习与交流。
