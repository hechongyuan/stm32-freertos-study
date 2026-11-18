# STM32 FreeRTOS 学习项目 —— 多按键分层重构（1.3）

> 基于 STM32F103 + FreeRTOS 的按键驱动练习。
> 本版本的核心目标：在 1.2「一套驱动支持任意多个按键」的基础上，解决**多个按键如何统一组织、统一扫描**的问题，形成清晰的**三层结构**。

---

## 一、这一版做了什么（和 1.2 对比）

| 项目 | 1.2 版 | 1.3 版 |
|---|---|---|
| 按键数量 | 1 个（PC9） | 6 个（PC9 / PA8 / PC15 / PC0 / PC13 / PC14） |
| 任务数量 | 每个按键一个任务 | **一个 `keytask` 统一初始化并扫描全部按键** |
| 按键配置位置 | 直接写在任务函数里 | 每个按键一个独立模块（`User/keys/keyN.c`） |
| 任务目录 | `User/Task/Key_Task.c` | `User/task/key_task.c` |
| 基础驱动 | `Drivers/key`（结构体 + 回调） | **不变，继续复用** |
| 新增按键的成本 | 新建任务 + 修改任务文件 | 新增一个模块 + 任务里加两行调用 |
| 代码组织 | 单文件 | 基础驱动层 / 按键配置层 / 任务调度层 |

一句话：**1.2 解决了「驱动怎么复用」，1.3 解决了「多个按键怎么管理」。**

---

## 二、硬件与软件环境

| 项目 | 说明 |
|---|---|
| 主控 | STM32F103RCTx（256 KB Flash / 48 KB RAM，Cortex-M3） |
| 按键 | 6 个，输入上拉，按下接 GND |
| 串口 | USART1（115200），每个按键按下后输出一行提示 |
| RTOS | FreeRTOS（内核位于 `FreeRTOS/`，内存管理 `heap_4`） |
| HAL | STM32F1xx HAL Driver |
| 构建 | CMake ≥ 3.22 + Ninja |
| 工具链 | GNU Tools for STM32 `arm-none-eabi-gcc` |
| 开发环境 | VS Code + STM32Cube for VS Code 扩展 |

### 按键引脚对照表

引脚宏定义在 `Core/Inc/main.h`（由 CubeMX 生成）：

| 宏 | 引脚 | 驱动模块 | 串口输出 |
|---|---|---|---|
| `Key1_Pin` | PC9 | `User/keys/key1.c` | `key1 clicked.` |
| `Key2_Pin` | PA8 | `User/keys/key2.c` | `key2 clicked.` |
| `Key3_Pin` | PC15 | `User/keys/key3.c` | `key3 clicked.` |
| `Key4_Pin` | PC0 | `User/keys/key4.c` | `key4 clicked.` |
| `Key_UP_Pin` | PC13 | `User/keys/key_up.c` | `keyup clicked.` |
| `Key_DOWN_Pin` | PC14 | `User/keys/key_down.c` | `keydown clicked.` |

> GPIO 均为**输入上拉**：松开读到高电平，按下接 GND 读到低电平。
> 因此驱动检测的是「低 → 高」，即**松手瞬间**触发回调。
> 若你的按键是「按下接 3.3V」，请把 `main.c` 里的 `GPIO_PULLUP` 改为 `GPIO_PULLDOWN`。

---

## 三、目录结构

```
experiment1.3/
├── Core/                         # CubeMX 生成的启动、时钟、中断、主程序
│   ├── Inc/main.h                #   ★ 6 个按键的引脚宏
│   └── Src/main.c                #   GPIO/USART 初始化 + 创建 keytask
├── Drivers/
│   ├── key/                      # 【基础驱动层】与具体按键无关
│   │   ├── key.h                 #   KeyHandle_Typedef + 接口声明
│   │   └── key.c                 #   key_init / key_scan 实现
│   ├── STM32F1xx_HAL_Driver/     # ST HAL 库
│   └── CMSIS/
├── FreeRTOS/                     # FreeRTOS 内核（Src / Inc）
├── User/
│   ├── keys/                     # 【按键配置层】每个按键一个模块
│   │   ├── key1.c  / key1.h      #   PC9，注册回调 + 扫描包装
│   │   ├── key2.c  / key2.h      #   PA8
│   │   ├── key3.c  / key3.h      #   PC15
│   │   ├── key4.c  / key4.h      #   PC0
│   │   ├── key_up.c   / key_up.h   # PC13
│   │   └── key_down.c / key_down.h # PC14
│   └── task/
│       └── key_task.c / key_task.h # 【任务调度层】keytask 统一扫描
├── cmake/stm32cubemx/            # CubeMX 自动生成的 CMake 子工程
├── CMakeLists.txt                # 顶层构建脚本
├── CMakePresets.json
├── expriment1.3.ioc              # CubeMX 工程文件
└── README.md
```

---

## 四、三层结构设计

```
┌─────────────────────────────────────────────┐
│ 任务调度层  User/task/key_task.c             │  keytask：
│  初始化 6 个按键 → 循环扫描 6 个按键         │  每 10 ms 一轮
└───────────────┬─────────────────────────────┘
                │ 调用 keyN_init() / keyN_scan()
┌───────────────▼─────────────────────────────┐
│ 按键配置层  User/keys/key1.c … key_down.c    │  每个按键：
│  绑定 GPIO 引脚 + 注册回调 + 包装扫描函数    │  一个 .c/.h 模块
└───────────────┬─────────────────────────────┘
                │ 调用 key_init() / key_scan(句柄)
┌───────────────▼─────────────────────────────┐
│ 基础驱动层  Drivers/key/key.c                │  完全不知道
│  结构体句柄 + 边沿检测 + 调用回调            │  有几个按键
└─────────────────────────────────────────────┘
```

### 1. 基础驱动层（`Drivers/key`）

从 1.2 沿用，没有改动。它只认识一个 `KeyHandle_Typedef` 句柄：

```c
typedef struct
{
    GPIO_TypeDef* GPIO_Port;           // 按键所在端口
    uint16_t      GPIO_Pin;            // 按键引脚号
    GPIO_PinState previous;            // 上次电平
    GPIO_PinState current;             // 本次电平
    void (*ClickedCallBack)(void);     // 点击回调（函数指针）
} KeyHandle_Typedef;

void key_init(KeyHandle_Typedef *handle);
void key_scan(KeyHandle_Typedef *handle);
```

`key_scan()` 的边沿检测（`Drivers/key/key.c`）：

```c
void key_scan(KeyHandle_Typedef *handle)
{
    handle->current = HAL_GPIO_ReadPin(handle->GPIO_Port, handle->GPIO_Pin);

    // 上一次为低（按下），这次为高（松开）→ 松开瞬间触发一次回调
    if ((handle->current == GPIO_PIN_SET) && (handle->previous == GPIO_PIN_RESET))
    {
        handle->ClickedCallBack();
    }
    handle->previous = handle->current;   // 保存本次电平，供下一轮比较
}
```

### 2. 按键配置层（`User/keys`）

每个按键一个模块，职责只有三件事：**设置引脚、注册回调、包装扫描**。
以 `key1.c` 为例：

```c
KeyHandle_Typedef hkey1;                       // 这个按键的句柄（全局唯一）
extern UART_HandleTypeDef huart1;

void key1_init(void)
{
    hkey1.ClickedCallBack = key1ClickCallBack;  // 注册按下后要做的事
    hkey1.GPIO_Pin        = Key1_Pin;           // 来自 main.h
    hkey1.GPIO_Port       = Key1_GPIO_Port;
    key_init(&hkey1);                           // 交给基础驱动初始化
}

void key1_scan(void)
{
    key_scan(&hkey1);                           // 包装一层，调用方不用管句柄
}

void key1ClickCallBack(void)                    // 业务逻辑：打印一行
{
    HAL_UART_Transmit(&huart1,
                      (const uint8_t*)"key1 clicked.\n",
                      strlen("key1 clicked.\n"),
                      HAL_MAX_DELAY);
}
```

好处：**上层只看到 `key1_init()` / `key1_scan()`，看不到句柄和引脚细节**。
想改 key1 的行为，只动 `key1.c`；想换引脚，只改 `main.h` 或 `key1_init()`。

### 3. 任务调度层（`User/task/key_task.c`）

只创建一个任务，负责把所有按键初始化一次，然后按固定周期全部扫一遍：

```c
void keytask(void *pvParameter)
{
    key1_init();
    key2_init();
    key3_init();
    key4_init();
    keydown_init();
    keyup_init();

    for (;;)
    {
        key1_scan();
        key2_scan();
        key3_scan();
        key4_scan();
        keyup_scan();
        keydown_scan();
        vTaskDelay(pdMS_TO_TICKS(10));      // 10 ms 扫描一轮
    }
}
```

在 `main.c` 中创建：

```c
xTaskCreate(keytask, "Key", 128, NULL, 1, NULL);
vTaskStartScheduler();
```

### 为什么从「每键一个任务」改成一个任务？

| 角度 | 说明 |
|---|---|
| RAM | 每个 FreeRTOS 任务都有独立栈，`128 words ≈ 512 B`。6 个按键就是 6 个任务、约 3 KB 栈，而整个芯片只有 48 KB RAM |
| 实时性 | 读一次 GPIO 只需微秒级，10 ms 内扫描 6 个按键绰绰有余，不需要 6 个任务并行 |
| 结构 | 一个任务 = 一处初始化 + 一处循环，新增按键只加两行，调度关系简单 |
| 代价 | 如果某个回调阻塞（例如 UART 用 `HAL_MAX_DELAY` 发送），会拖慢同一任务里的其他按键。学习阶段可接受，进阶时可改成「扫描任务只置标志，发送交给另一个任务」 |

---

## 五、如何新增一个按键

以新增 `Key5`（引脚 PC1）为例：

1. **CubeMX 里配引脚**：PC1 设为 `GPIO_Input` + `Pull-up`，重新生成代码；
   生成后在 `Core/Inc/main.h` 会多出 `Key5_Pin` / `Key5_GPIO_Port` 宏。
2. **复制一个按键模块**：把 `User/keys/key1.c/h` 复制为 `key5.c/h`，
   把里面的 `key1` 全部改成 `key5`，引脚改成 `Key5_Pin` / `Key5_GPIO_Port`。
3. **在任务里加两行**（`User/task/key_task.c`）：
   ```c
   key5_init();      // 初始化里加
   key5_scan();      // 循环里加
   ```
4. **不用改 CMake**：`CMakeLists.txt` 用 `GLOB_RECURSE` 自动收集 `User/*.c`，
   新文件只要放进 `User` 目录就会自动参与编译，头文件路径也已包含 `User/keys`。
5. 编译、烧录、按一下 PC1，串口应输出 `key5 clicked.`。

---

## 六、编译与烧录

### 命令行

```bash
# 首次配置
cmake --preset Debug

# 编译
cmake --build build/Debug
```

成功后生成：

```
build/Debug/experiment1.3.elf
```

编译输出参考（Debug）：

```
RAM:   6656 B / 48 KB   13.54%
FLASH: 18812 B / 256 KB  7.18%
```

### VS Code

1. 安装 STM32Cube for VS Code 扩展；
2. 打开本目录，选择 `Debug` 预设；
3. 使用扩展的 Build / Debug / Flash 按钮。

### CMake 说明

- `FreeRTOS/Src/*.c` —— RTOS 内核
- `User/*.c`（递归）—— 任务层 + 所有按键模块
- `Drivers/key/*.c` —— 基础按键驱动

> ⚠️ 不要递归收集整个 `Drivers/`：`Drivers/STM32F1xx_HAL_Driver` 和 `Drivers/CMSIS`
> 已由 `cmake/stm32cubemx` 子工程编译，重复收集会导致链接时 `multiple definition`。

---

## 七、版本管理（每版都有自己的 README 和代码）

本仓库用 **Git 标签（tag）** 保存每个完整版本，每个标签都是一个快照，
包含**当时的全部代码 + 当时的 README**：

| 标签 | 内容 | 查看方式 |
|---|---|---|
| `v1.2` | 单按键可复用驱动（结构体 + 回调 + 指针句柄） | GitHub 左上角分支/标签下拉选 `v1.2` |
| `v1.3` | 多按键分层重构（本版） | 选 `v1.3` 或直接看 `main` |

- GitHub 页面：仓库首页左上角点击 `main`，在 **Tags** 里切换版本；
- 仓库的 **Releases** 页面也按版本列出了说明和源码压缩包；
- 命令行查看旧版：

  ```bash
  git tag                        # 列出所有版本
  git show v1.2:README.md        # 看 1.2 版 README
  git checkout v1.2              # 切到 1.2 版代码（看完记得切回 main）
  ```

> 切到旧标签后代码是只读的「快照」，不要在上面直接改；要基于旧版继续开发，应新建分支。

---

## 八、当前版本的已知局限 / 后续可改进

1. **回调前没有判空**
   `key_scan()` 直接调用 `handle->ClickedCallBack()`。若某个按键忘记注册回调，
   会跳到空指针导致 HardFault。建议加：
   ```c
   if (handle->ClickedCallBack != NULL)
   {
       handle->ClickedCallBack();
   }
   ```
   并把句柄定义改为 `KeyHandle_Typedef hkey1 = {0};`。

2. **没有软件消抖**
   目前靠 10 ms 扫描周期天然过滤部分抖动。严格场合可加入「连续 N 次电平一致才确认」。

3. **没有长按 / 双击**
   可在句柄结构体中加入计时字段与状态机，扩展出长按、连击等事件。

4. **`keyN.c` 之间高度重复**
   六个模块除了引脚和回调外几乎相同。进阶可用「配置表 + 统一初始化」的表驱动写法，
   或宏批量生成模块，进一步减少重复。

5. **回调里使用阻塞式串口发送**
   `HAL_UART_Transmit(..., HAL_MAX_DELAY)` 会阻塞扫描任务。
   可改为中断/DMA 发送，或「扫描置标志 → 另一任务负责输出」。

6. **每个模块各自 `extern huart1`**
   可把公共外设句柄统一放进一个公共头文件，减少重复声明。

---

## 九、版本说明

| 版本 | 内容 |
|---|---|
| 1.2 | 可复用按键驱动：结构体 + 回调 + 指针句柄，一套驱动支持任意多个按键 |
| 1.3 | 多按键分层重构：按键模块化（`User/keys`）+ 单任务统一扫描（`User/task`） |

---

## 许可

本项目仅用于个人学习与交流。
