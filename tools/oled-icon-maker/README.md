# OLED 图标生成器

将 PNG/JPG/BMP 等图片转为 **U8g2 `drawXBMP`** 可用的 C 头文件，或在网格中手工改像素。

适用于本项目 128×64 SSD1306 OLED（也可生成任意宽高的 1bpp 位图）。

## 环境

- Windows / macOS / Linux
- Python 3.8+

## 运行

```bat
run.bat
```

或：

```bash
pip install -r requirements.txt
python oled_icon_maker.py
```

### 启动报错 `Can't find a usable init.tcl`

多因系统里 **`TCL_LIBRARY` 被其它软件改坏**（例如 CSR BlueSuite 指向 `Program Files (x86)\CSR\...`）。

- 优先用 **`run.bat`** 启动（会自动指向当前 Python 自带的 Tcl）。
- 直接运行 `oled_icon_maker.py` 时，脚本也会在加载 tkinter 前尝试修正路径。
- 仍失败：到「系统环境变量」里删除用户/系统级的 `TCL_LIBRARY`、`TK_LIBRARY`，或重装 [python.org](https://www.python.org/downloads/) 并勾选 **tcl/tk and IDLE**。

## 使用步骤

1. 设置目标宽高（例如右侧图标 `32×56`）。
2. **从图片导入**：选择图片，调整阈值/抖动/反色后点「应用到像素网格」。
3. 在网格上点击可切换单个像素（黑=点亮 OLED 像素）。
4. 填写变量名（如 `xbox360_icon`），点 **导出 C 头文件**。
5. 将生成的 `.h` 复制到 `include/`，在 `pico_zero.cpp` 中 `#include` 并用 `u8g2.drawXBMP(x, y, w, h, 数组名)` 绘制。

## 导出格式

- 单色 1bpp，MSB 优先（与 U8g2 `drawXBMP` 一致）
- 含 `ICON_W` / `ICON_H` 宏及 `U8X8_PROGMEM` 数组

## 与本项目配合示例

```cpp
#include "my_icon.h"
u8g2.drawXBMP(92, 4, MY_ICON_W, MY_ICON_H, my_icon);
```

右侧 X 坐标建议：`128 - 宽度 - 边距`。
