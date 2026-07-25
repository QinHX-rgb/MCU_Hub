# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Project Overview

Line-following robot car firmware for the TI MSPM0G3507 (Cortex-M0+) microcontroller on the LP_MSPM0G3507 LaunchPad. Bare-metal C project built with CCS Theia IDE and TI ARM Clang toolchain. Core driver modules are in place; `empty.c` still needs a `main()` loop to wire everything together.

## Build System

- **IDE**: CCS Theia 71.0.0 (Eclipse CDT-based project)
- **Compiler**: TI ARM Clang (`tiarmclang`), targeting `thumbv6m` / `cortex-m0plus`
- **SDK**: MSPM0 SDK 2.11.00.07 (DriverLib)
- **SysConfig**: 1.28.0 — generates `ti_msp_dl_config.h` and `ti_msp_dl_config.c` in `Debug/`
- **Output**: `Debug/Car_L.out` (ELF executable)
- **CPUCLK**: 32 MHz (SYSOSC, no PLL/HFXT)
- **SysTick**: Period = 32 → 1 μs tick (used by `delay_us`/`delay_ms`)
- Build from CCS Theia: **Project → Build Project**
- **Do not hand-edit** `empty.syscfg` or any file under `Debug/` — they are generated.
- `.clangd` at project root points to `Debug/.clangd` for the compilation database.

## Code Architecture

```
Car/
├── empty.c                    # main() — 系统初始化 → 等待按键 → 循迹主循环
├── empty.syscfg               # SysConfig — do NOT hand-edit
├── Driver/
│   ├── motor.h / motor.c      # H-bridge底层: Motor_On/Off, Set_Speed(side, duty%)
│   ├── Motor_Control.h/.c     # 行驶状态控制: Control_Line_Track/Straight/Corner
│   ├── PID.h / PID.c          # Generic positional PID + g_pid_track instance
│   ├── grayscale_sensor.h/.c  # 8-channel line sensor via 74HC165 shift register
│   └── CY_Z.h / CY_Z.c        # CY-Z gyro UART protocol (16-byte telemetry + 8-byte ACK/CMD)
├── System/
│   ├── delay.h / delay.c      # SysTick-based delay_us/delay_ms (polling)
│   └── Key.h / Key.c          # 3-button scan: Key1=PB14, Key2=PB16, Key3=PA12
├── Debug/                     # Build output + generated DriverLib init
└── targetConfigs/
    └── MSPM0G3507.ccxml       # XDS-110 JTAG config
```

## Hardware Pin Map

### Motors (H-bridge via GPIO_Motor)

| Signal | Pin | Direction |
|---|---|---|
| L1 | PA8 | Output — left motor forward |
| L2 | PA22 | Output — left motor reverse |
| R1 | PA26 | Output — right motor forward |
| R2 | PB24 | Output — right motor reverse |
| STBY | PA24 | Output — H-bridge enable (active high) |

### PWM (TIMA0)

| Channel | Pin | Motor |
|---|---|---|
| CCP0 | PB8 | Left motor PWM |
| CCP1 | PB20 | Right motor PWM |

PWM period = 1000 counts, edge-aligned. `Set_Speed()` computes `cmp = 1000 - (1000 * abs_duty / 100)` — duty cycle as percentage.

### UART

| Instance | Pins | Purpose |
|---|---|---|
| UART_0 | PA10 (TX only) | Debug/telemetry output (115200) |
| UART_1 | PB7 (RX), PB4 (TX) | CY-Z gyro (115200, RX interrupt enabled) |

UART_0 is TX-only (no RX pin configured). UART_1 RX interrupt is enabled by `CY_Z_Init()`.

### Sensor — 74HC165 Shift Register (GPIO_Sensor)

| Signal | Pin | Notes |
|---|---|---|
| PL | PB13 | Parallel load latch (active low pulse) |
| SCK | PB15 | Shift clock |
| SDA/Q7 | PB1 | Serial data input with pull-up |

**Stale comments in `grayscale_sensor.h`** reference PB14(SCK) and PB13(SDA) — these are wrong. Trust `ti_msp_dl_config.h` macros.

### Buttons (GPIO_BUTTON)

| Button | Pin | Notes |
|---|---|---|
| BUTTON | PA18 | Start button (input, pull-down) |
| Key1 | PB14 | **SysConfig bug: initialized as output** — `Key_Read()` reads it anyway |
| Key2 | PB16 | **SysConfig bug: initialized as output** — `Key_Read()` reads it anyway |
| Key3 | PA12 | **SysConfig bug: initialized as output** — `Key_Read()` reads it anyway |

Key1/2/3 are configured as digital **outputs** in the generated `SYSCFG_DL_GPIO_init()`, but `Key_Read()` reads them as inputs. If these are actually used as buttons, the SysConfig needs to be updated to configure them as inputs with pull-up/down.

### Other GPIO

| Group | Pin | Purpose |
|---|---|---|
| GPIO_LED | PA0 | Status LED (output) |
| GPIO_Conder | PB12, PA13, PB0, PB6 | Spare outputs — unconnected |

## Driver Module Details

### Motor 底层 (`Driver/motor.c`)

H-bridge 基础操作，被 `Motor_Control.c` 调用：
- `Motor_On()` — set STBY high
- `Motor_Off()` — clear STBY and all L1/L2/R1/R2 pins
- `Set_Speed(uint8_t side, int8_t duty)` — side 0=left, 1=right; positive duty=forward, negative=reverse, 0=stop

### 行驶状态控制 (`Driver/Motor_Control.c`)

三种行驶状态，由主循环根据场景切换调用：

**`Control_Line_Track()`** — 循线行驶（循迹环 + 速度环）：
1. `Flag == 1` (全黑) 或 `Flag == 0` (全白丢线) → 停车 + 复位
2. `Flag == 2` (正常) → `error = cx - LINE_CENTER(35)`, 死区 ±2, PID → `turn_adj`
3. `target_L/R = BASE_PWM(28) ± turn_adj`
4. 一阶低通滤波 (α=0.65), 限幅 [MIN_PWM=8, MAX_PWM=45], `Set_Speed()`

**`Control_Straight()`** — 直行（速度环 + 角度环）：待实现

**`Control_Corner()`** — 直角转弯：待实现

Tunable parameters 在 `motor.h`: `LINE_CENTER`, `DEAD_ZONE`, `BASE_PWM`, `MAX_PWM`, `MIN_PWM`, `FILTER_ALPHA`.

### Grayscale Sensor (`Driver/grayscale_sensor.c`)

- Reads 8 channels from 74HC165 shift register via bit-banged protocol
- Centroid calculation: channel `i` (0=rightmost, 7=leftmost) has weight `(7-i) × 10` → `cx` range 0–70, center = 35
- Global variables set by `Grayscale_Update()`:
  - `g_sensor_raw_data` — raw 8-bit (0=black, 1=white)
  - `cx` — centroid 0–70
  - `Flag` — **0** = all white (line lost), **1** = all black (stop), **2** = normal tracking
  - `Status` — current raw value (always updated)
  - `Last_Status` — last valid raw value (frozen when all-white, updated otherwise)

Note: `Flag` meaning is different from typical convention — 0 = lost, 1 = stop, 2 = normal.

### CY-Z Gyro (`Driver/CY_Z.c`)

Full UART1 driver for CY-Z gyroscope module:
- **Telemetry frame**: 16 bytes — `AA 55` + seq(2B) + angle_deg(f32 LE) + gyro_dps(f32 LE) + CRC16(2B LE) + `55 AA`
- **Command/ACK frame**: 8 bytes — `A5 5A`/`A5 5B` + cmd + param/result + seq + CRC16(2B LE) + tail
- CRC: Modbus CRC16 over payload bytes only
- ISR: `UART_1_INST_IRQHandler` (expands to `UART1_IRQHandler`) → `CY_Z_UART1_IRQHandler()`
- Sliding window parser: shares a 16-byte window between telemetry (16B) and ACK (8B) detection
- Public API: `CY_Z_Init()`, `CY_Z_GetTelemetry()`, `CY_Z_GetAck()`, `CY_Z_SendZeroAngle()`, `CY_Z_SendZeroAngleFixed()`, `CY_Z_SetReportRate()`, print helpers
- `CY_Z_GetTelemetry()` and `CY_Z_GetAck()` briefly disable interrupts to safely copy data — call only from main loop context

### PID (`Driver/PID.c`)

Pure generic PID library — no project-specific instances:
- `PID_Init(pid, Kp, Ki, Kd, max, min)`
- `PID_Update(pid, error)` — returns computed output, clamped to `[output_min, output_max]`
- `PID_Reset(pid)` — clears integral and last_error
- Integral anti-windup: hard-coded to ±100 (not per-instance configurable)

PID instances and their gains are defined in `Motor_Control.c`, right before each control function that uses them.

### Delay (`System/delay.c`)

SysTick-based polling delays. SysTick configured at 32 MHz / 32 = 1 μs per tick. Chinese comments are encoded in GBK.

### Key (`System/Key.c`)

`Key_Read()` scans three buttons, returns 1/2/3 if pressed, 0 if none. No debouncing.

## SysConfig Structure

`empty.syscfg` defines these module instances:

| Instance | Peripheral | Details |
|---|---|---|
| GPIO_Motor | GPIO | 5 pins: L1(PA8), L2(PA22), R1(PA26), R2(PB24), STBY(PA24) |
| GPIO_LED | GPIO | 1 pin: LED(PA0) |
| GPIO_BUTTON | GPIO | 4 pins: BUTTON(PA18, input+pull-down), Key1(PB14), Key2(PB16), Key3(PA12) |
| GPIO_Sensor | GPIO | 3 pins: PL(PB13), SCK(PB15), SDA(PB1, input) |
| GPIO_Conder | GPIO | 4 pins: PB12, PA13, PB0, PB6 (spare outputs) |
| PWM_Motor | TIMA0 | 2 channels: CCP0(PB8), CCP1(PB20), period=1000 |
| UART_1 | UART1 | RX(PB7)+TX(PB4), 115200, RX interrupt |
| UART_0 | UART0 | TX(PA10) only, 115200 |
| SYSCTL | SYSCTL | 32 MHz SYSOSC, no PLL/HFXT |
| SYSTICK | SysTick | Period=32, enabled |

## Interrupt Vector Usage

| IRQ | Handler | Source |
|-----|---------|--------|
| 13 (UART1) | `UART1_IRQHandler` | UART1 RX → `CY_Z_UART1_IRQHandler()` (Driver/CY_Z.c) |

UART0 has no interrupt enabled. No GPIO interrupts configured (no encoder ISRs).

## Debugging

- Debug probe: XDS-110 via SWD (PA20=SWCLK, PA19=SWDIO)
- Jumpers `J101 15:16` and `J101 13:14` must be ON for debugging
- Build configuration: Debug (`-O0`, `-g` DWARF)
- Firmware does not use low-power modes

## Known Issues

1. **Key1/2/3 misconfigured**: These are initialized as digital **outputs** in `SYSCFG_DL_GPIO_init()` but `Key_Read()` reads them as inputs. If they're meant to be buttons, the SysConfig GPIO_BUTTON instance needs fixing.
2. **No encoder support**: No `encoder.c` exists and no GPIO interrupts are configured for quadrature decoding. Speed loop is currently open-loop PWM.
3. **Sensor pin comments are stale**: `grayscale_sensor.h` macros reference PB14(SCK) and PB13(SDA), but the actual SysConfig assigns SCK=PB15 and SDA=PB1. The generated `ti_msp_dl_config.h` is authoritative.
4. **`delay.c` Chinese comments**: Encoded in GBK, may display as garbled text depending on editor locale.
