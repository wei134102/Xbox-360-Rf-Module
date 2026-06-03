# Xbox-360-Rf-Module

通过 Arduino 控制拆机 Xbox 360 RF 模块，实现无线手柄配对。本仓库提供 **两套 PlatformIO 环境**，对应不同硬件方案。

| 环境 | 开发板 | 主程序 | 说明 |
|------|--------|--------|------|
| `nano` | Arduino Nano (ATmega328) | `src/main.cpp` | 上游完整逻辑：睡眠、长按关手柄、环形灯反馈 |
| `rp2040zero` | RP2040-zero (Pico) | `src/pico_zero.cpp` | OLED 状态显示 + 独立配对键 |

`src/` 下的 `slim.cpp`、`original.cpp`、`almost_worked.cpp` 为历史实验代码，**不参与编译**。

## 项目简介 / Project Introduction

- **Nano**：复用 RF 模块 Sync 引脚，短按配对、长按 1s 释放后关闭所有手柄，支持 AVR 低功耗睡眠。
- **RP2040-zero**：连接 Xbox 360 RF 模块 + 0.96 寸 SSD1306 I2C OLED，一键配对并显示状态（配对计数为估算值，可后续扩展）。

This project drives a salvaged Xbox 360 RF module for wireless controller pairing. Choose the PlatformIO environment that matches your board.

## 硬件连接 / Hardware Wiring

### 环境 `rp2040zero`（RP2040-zero + OLED）

| 功能 / Function      | RP2040-zero GPIO | 连接对象 / To           |
|----------------------|------------------|-------------------------|
| OLED SDA             | GP4              | OLED 屏 SDA             |
| OLED SCL             | GP5              | OLED 屏 SCL             |
| 配对按钮 / Pair Btn  | **GP7**（主）    | 按钮一脚接 GP7，另一脚接 GND；可同时接 RF Pin5 |
| 可选第二按钮         | GP6              | 未接可忽略              |
| RF 模块 Data         | GP3              | Xbox RF Pin6            |
| RF 模块 Clock        | GP2              | Xbox RF Pin7（建议 4.7k 上拉到 3.3V） |
| RF 模块 Sync         | GP7              | Xbox RF Pin5（与配对键共用） |
| GND / 3.3V           | GND / 3V3        | 所有模块                |

### 环境 `nano`（Arduino Nano）

| 功能 / Function | Nano 引脚 | RF 模块引脚 |
|-----------------|-----------|-------------|
| Sync（按钮）    | D2        | Pin5        |
| Data            | D3        | Pin6        |
| Clock           | D4        | Pin7        |
| GND / 5V        | GND / 5V  | 按模块规格  |

参考：[appliedcarbon.org/xboxrf](https://www.appliedcarbon.org/xboxrf.html)

## 注意事项 / Notes

- RP2040 方案：OLED 与 RF 模块使用 **3.3V**，勿接 5V。
- Nano 方案：按原上游设计使用模块 Sync 键逻辑，无需额外 OLED。
- 更换引脚时请同步修改对应 `src/*.cpp` 中的宏定义。
- RF 命令说明见 `commands.md`；ATTiny 参考实现见根目录 `xboxrf.c`。

## 云端编译固件（GitHub Actions，仅手动）

无需本地 PlatformIO，在 GitHub 上**手动触发**编译：

1. 打开仓库 → **Actions** → **Build Firmware**
2. 点击 **Run workflow**
3. 选择编译目标：`all` / `rp2040zero` / `nano` → **Run workflow**
4. 等待完成后，在页面底部 **Artifacts** 下载：
   - `xbox360-rf-rp2040zero` → `firmware.uf2`（拖入 RPI-RP2 盘符烧录）
   - `xbox360-rf-nano` → `firmware.hex`（用 avrdude / IDE 烧录）

> 不会在 push / PR 时自动编译，仅支持手动 Run workflow。

## 固件编译与上传 / Build & Upload

### 1. 安装 PlatformIO

在 VSCode 中安装 PlatformIO 扩展。

### 2. 选择环境

在 VSCode 底部 **PlatformIO 环境** 下拉框中选择：

- `rp2040zero` — RP2040 + OLED
- `nano` — Arduino Nano

或在项目根目录执行：

```bash
pio run -e rp2040zero -t upload
pio run -e nano -t upload
```

`platformio.ini` 中 `[platformio] default_envs` 已包含上述两个环境；仅编译当前选中环境时，在 UI 中只勾选目标 env 即可。

### 3. 编译与烧录

1. 用 USB 连接对应开发板  
2. PlatformIO → **Build**  
3. PlatformIO → **Upload**

### 4. 运行

- **rp2040zero**：上电约 3s 后初始化 RF；**按 GP7 配对键**（或 GP6）触发 SYNC。初始化成功后中间一颗绿灯常亮属正常。板载 RGB（GP16 WS2812）：蓝闪=启动，绿灯=就绪，蓝快闪=配对中，亮绿=配对成功，红灯=RF 通信异常。  
- **nano**：串口 9600 可查看日志；按 Sync 短按配对，长按约 1s 后释放关闭手柄。

## 软件依赖 / Dependencies

| 环境 | 框架 | 额外库 |
|------|------|--------|
| `nano` | Arduino AVR | — |
| `rp2040zero` | Arduino RP2040 | U8g2（SSD1306）；板载 RGB 由固件直接驱动（GP16） |

## OLED 图标工具

从图片生成 U8g2 位图头文件（支持手工改像素）：

```bat
tools\oled-icon-maker\run.bat
```

详见 [tools/oled-icon-maker/README.md](tools/oled-icon-maker/README.md)。

## 参考 / References

- [appliedcarbon.org](https://www.appliedcarbon.org/xboxrf.html)
- 更多链接见 `fuentes.md`
