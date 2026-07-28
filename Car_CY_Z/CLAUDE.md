# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Project Overview

Line-following robot car firmware for the TI MSPM0G3507 (Cortex-M0+) microcontroller on the LP_MSPM0G3507 LaunchPad. Bare-metal C project built with CCS Theia IDE and TI ARM Clang toolchain. All driver modules are in place and the main control loop is fully functional.

## Build System

- **IDE**: CCS Theia 71.0.0 (Eclipse CDT-based project)
- **Compiler**: TI ARM Clang (`tiarmclang`), targeting `thumbv6m` / `cortex-m0plus`
- **SDK**: MSPM0 SDK 2.11.00.07 (DriverLib)
- **SysConfig**: 1.28.0 — generates `ti_msp_dl_config.h` and `ti_msp_dl_config.c` in `Debug/`
- **Output**: `Debug/Car_L.out` (ELF executable)
- **CPUCLK**: 32 MHz (SYSOSC, no PLL/HFXT)
- **SysTick**: Period = 32 → 1 μs tick
- Build from CCS Theia: **Project → Build Project**
- **Do not hand-edit** `empty.syscfg` or any file under `Debug/` — they are generated.
- `.clangd` at project root points to `Debug/.clangd` for the compilation database.

## Code Architecture

```
Car_Test/
├── empty.c                    # main() — 系统初始化 → 等待按键选档 → 倒计时 → 循迹主循环
├── empty.syscfg               # SysConfig — do NOT hand-edit
├── Driver/
│   ├── motor.h / motor.c      # H-bridge底层: Motor_On/Off, Set_Speed(side, duty%)
│   ├── Motor_Control.h/.c     # 行驶状态控制: Line_Track / Straight / Corner
│   ├── PID.h / PID.c          # Generic positional PID (3 instances in Motor_Control.c)
│   ├── grayscale_sensor.h/.c  # 8-channel line sensor via 74HC165 shift register
│   ├── CY_Z.h / CY_Z.c        # CY-Z gyro UART protocol (16B telemetry + 8B ACK/CMD)
│   └── Encoder.h / Encoder.c  # Dual quadrature encoder via GPIO interrupts (X2 decoding)
├── System/
│   ├── delay.h / delay.c      # SysTick-based delay_us/delay_ms (polling)
│   ├── Key.h / Key.c          # 2-button scan: Key1=PB14, Key2=PB16 (Key3=PA12 shared w/ buzzer)
│   ├── Buzzer.h / Buzzer.c    # Buzzer on PA12 — short beep on line-loss
│   ├── uart.h / uart.c        # UART0 debug printf with %d/%u/%x/%s/%c/%f support
│   ├── clock.h / clock.c      # tick_ms counter via SysTick_Handler
│   ├── interrupt.h / interrupt.c  # GROUP1_IRQHandler: encoder X2 decode + multi-sensor dispatch
│   └── OLED.h                 # OLED display (I2C, 128x64)
├── Debug/                     # Build output + generated DriverLib init
└── targetConfigs/
    └── MSPM0G3507.ccxml       # XDS-110 JTAG config
```

## Main Control Flow (`empty.c`)

1. **Init**: `SYSCFG_DL_init()` → UART/Grayscale/Encoder/PID/CY-Z/OLED/Interrupt/Key/Buzzer
2. **Idle loop**: LED flash 3× → wait for PA18 (BUTTON) press, with Key1/Key2 to toggle angle profile
3. **Countdown**: LED slow flash 3× (300ms each)
4. **Main loop** (~3ms period, ~333Hz):

| Flag | Meaning | Action |
|------|---------|--------|
| 4 | Normal tracking (black line detected) | `Control_Line_Track()` — PID steering + base PWM |
| 3 | All white (line lost) | `Control_Straight()` — gyro heading hold + encoder speed loop |
| 2 | Right-turn pattern (111xx000) | `Control_Corner(1)` — fast left + slow right |
| 1 | Left-turn pattern (000xx111) | `Control_Corner(-1)` — slow left + fast right |
| 0 | All black (stop) | `Motor_Off()` |

**Important**: Flag meanings are non-standard. 0=stop, 1=left turn, 2=right turn, 3=straight (lost), 4=normal track.

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

PWM period = 1000 counts, edge-aligned. `Set_Speed()` computes `cmp = 1000 - (1000 * abs_duty / 100)`.

### Encoder (GPIO_Conder)

| Signal | Pin | Notes |
|---|---|---|
| Right A | PB0 | Quadrature A, dual-edge interrupt → GROUP1_IRQHandler |
| Right B | PB6 | Quadrature B, input only |
| Left A | PB12 | Quadrature A, dual-edge interrupt → GROUP1_IRQHandler |
| Left B | PA13 | Quadrature B, input only |

X2 decoding in `GROUP1_IRQHandler`: reads both A and B on each A edge to determine direction. Speed = EMA-filtered pulse count per `Encoder_Update()` call (~333Hz rate).

### UART

| Instance | Pins | Purpose |
|---|---|---|
| UART_0 | PA10 (TX only) | Debug/telemetry output (115200), printf formatting |
| UART_1 | PB7 (RX), PB4 (TX) | CY-Z gyro (115200, RX interrupt enabled) |

UART_0 is TX-only (no RX pin configured). UART_1 RX interrupt is enabled by `CY_Z_Init()`.

### Sensor — 74HC165 Shift Register (GPIO_Sensor)

| Signal | Pin | Notes |
|---|---|---|
| PL | PB13 | Parallel load latch (active low pulse) |
| SCK | PB15 | Shift clock |
| SDA/Q7 | PB1 | Serial data input with pull-up |

**Stale comments in `grayscale_sensor.h`** reference PB14(SCK) and PB13(SDA) — these are wrong. The macro names use correct SysConfig constants (`GPIO_Sensor_PIN_SCK_PIN`, `GPIO_Sensor_PIN_SDA_PIN`). Trust `ti_msp_dl_config.h`.

### Buttons (GPIO_BUTTON)

| Button | Pin | Notes |
|---|---|---|
| BUTTON | PA18 | Start button (input, pull-down, active high) |
| Key1 | PB14 | Profile select 0; `Key_Init()` reconfigures as input+pull-down |
| Key2 | PB16 | Profile select 1; `Key_Init()` reconfigures as input+pull-down |
| Key3 | PA12 | **Shared with buzzer** — not used as button in current code |

`Key_Init()` calls `DL_GPIO_disableOutput()` + `DL_GPIO_initDigitalInputFeatures(…PULL_DOWN)` to override SysConfig's output default. `Key_Read()` returns 1/2/0 for active-high detection.

### Other GPIO

| Group | Pin | Purpose |
|---|---|---|
| GPIO_LED | PA0 | Status LED (output) |
| Buzzer | PA12 | Short beep on line-loss entry (shared with Key3 pin) |
| OLED | (I2C pins) | 128x64 OLED display for debug info |

## Driver Module Details

### Motor 底层 (`Driver/motor.c`)

H-bridge 基础操作，被 `Motor_Control.c` 调用：
- `Motor_On()` — set STBY high
- `Motor_Off()` — clear STBY and all L1/L2/R1/R2 pins
- `Set_Speed(uint8_t side, int8_t duty)` — side 0=left, 1=right; positive duty=forward, negative=reverse, 0=coast (all pins low)

### 行驶状态控制 (`Driver/Motor_Control.c`)

Three control modes, all implemented:

**`Control_Line_Track()`** — 循线行驶（循迹环）：
1. Advances `g_angle_idx` for next straight reference, resets `g_straight_first`
2. `error = cx - LINE_CENTER(35)`, dead zone ±1, PID → `turn_adj`
3. `target_L/R = BASE_PWM(28) ± turn_adj`
4. 一阶低通滤波 (α=0.45), 限幅 [MIN_PWM=10, MAX_PWM=35], `Set_Speed()`

**`Control_Straight()`** — 丢线直行（速度环 + 角度环）：
- Gets CY-Z gyro angle, normalizes yaw error to [-180,180]
- Angle PID maintains heading toward `g_angle_refs[g_angle_idx]`
- Speed diff PID using encoder speeds → drives differential to 0
- Both corrections superimposed on BASE_PWM (left: -spd_corr + yaw_corr, right: +spd_corr - yaw_corr)

**`Control_Corner(dir)`** — 直角转弯：
- dir>0 right turn: left fast (35%), right slow (15%)
- dir<0 left turn: left slow (15%), right fast (35%)
- Resets PWM filter state on entry

**Angle profiles** (switchable via Key1/Key2 during idle):
- Profile 0: {0°, 180°} — forward/back
- Profile 1: {38.66°, 141.34°} — left/right diagonals
- Each time the car recovers from line-loss back to tracking, `g_angle_idx` advances to the next reference angle.

Tunable parameters in `motor.h`: `LINE_CENTER`, `DEAD_ZONE`, `BASE_PWM`, `MAX_PWM`, `MIN_PWM`, `FILTER_ALPHA`. PID gains are `#define`d in `Motor_Control.c` before each control function.

### Grayscale Sensor (`Driver/grayscale_sensor.c`)

- Reads 8 channels from 74HC165 via bit-banged SPI protocol
- Centroid calculation: channel `i` (0=rightmost, 7=leftmost) has weight `(7-i) × 10` → `cx` range 0–70, center = 35
- **Corner detection with lock**: detects patterns `000xx111` (left turn) and `111xx000` (right turn) with ≥4 black bits, locks direction for 150 cycles (~450ms) to ride through the turn
- Full-black `0x00` breaks the lock and triggers immediate stop
- Global variables set by `Grayscale_Update()`:
  - `g_sensor_raw_data` — raw 8-bit (0=black, 1=white)
  - `cx` — centroid 0–70 (set to 35 when not tracking)
  - `Flag` — 0=all black/stop, 1=left turn, 2=right turn, 3=all white/straight, 4=normal track
  - `Status` / `Last_Status` — declared extern but not actively used in current code

### Encoder (`Driver/Encoder.c`)

Dual quadrature encoder with X2 decoding:
- ISR runs in `GROUP1_IRQHandler` (GPIOB interrupt): reads A+B on each A edge to determine direction, increments/decrements `g_enc_R_pulses` / `g_enc_L_pulses`
- `Encoder_Update()` — called from main loop at ~333Hz: atomically reads and clears pulse counters, applies EMA filter (α=0.3) to produce speed values
- `Encoder_GetSpeedL()` / `Encoder_GetSpeedR()` — returns filtered speed as int16_t (pulses per 3ms period)

### CY-Z Gyro (`Driver/CY_Z.c`)

Full UART1 driver for CY-Z gyroscope module:
- **Telemetry frame**: 16 bytes — `AA 55` + seq(2B) + angle_deg(f32 LE) + gyro_dps(f32 LE) + CRC16(2B LE) + `55 AA`
- **Command/ACK frame**: 8 bytes — `A5 5A`/`A5 5B` + cmd + param/result + seq + CRC16(2B LE) + tail
- CRC: Modbus CRC16 over payload bytes only
- ISR: `UART_1_INST_IRQHandler` → `CY_Z_UART1_IRQHandler()` — sliding window parser that detects both 16B telemetry and 8B ACK frames
- Public API: `CY_Z_Init()`, `CY_Z_GetTelemetry()`, `CY_Z_GetAck()`, `CY_Z_SendZeroAngle()`, `CY_Z_SendZeroAngleFixed()`, `CY_Z_SetReportRate()`, `CY_Z_RequestTelemetry()`, print helpers
- `CY_Z_GetTelemetry()` and `CY_Z_GetAck()` briefly disable interrupts with `__disable_irq()`/`__enable_irq()` — call only from main loop context

### PID (`Driver/PID.c`)

Pure generic PID library — no project-specific instances:
- `PID_Init(pid, Kp, Ki, Kd, max, min)`
- `PID_Update(pid, error)` — returns computed output, clamped to `[output_min, output_max]`
- `PID_Reset(pid)` — clears integral and last_error
- Integral anti-windup: hard-coded to ±100 (not per-instance configurable)

PID instances and their gains are defined as static globals in `Motor_Control.c`:
- `g_pid_track` — steering PID for line tracking
- `g_pid_yaw` — heading hold PID for straight-line recovery
- `g_pid_spd_diff` — differential speed PID for straight-line recovery

### System Modules

- **delay**: `delay_ms()` via SysTick polling. Chinese comments in GBK encoding.
- **Key**: `Key_Init()` overrides SysConfig output config to input+pull-down; `Key_Read()` returns 1/2/0 (active high). No debouncing — debounce is done inline at call sites.
- **Buzzer**: PA12, `Buzzer_Beep()` = 100ms on. Shared pin with Key3.
- **uart**: `UART_Printf()` provides printf-style formatting over UART0 with automatic `\r\n`.
- **clock**: `tick_ms` incremented by `SysTick_Handler()` in `interrupt.c`.
- **interrupt**: `GROUP1_IRQHandler` handles GPIOB interrupts for encoder X2 decoding (DIO0=right A, DIO12=left A) and GPIOA interrupts for sensor dispatch (MPU6050/LSM6DSV16X/VL53L0X/IMU660RB via `#ifdef`). Also contains `SysTick_Handler` and conditional UART DMA handlers for BNO08X and WIT IMU sensors.

### OLED Display

128x64 I2C OLED used for real-time debug display during both idle and running states. Shows sensor raw bits, centroid, yaw angle, heading reference, lost-count, and current state. Included via `OLED.h`.

## Interrupt Vector Usage

| IRQ | Handler | Source |
|-----|---------|--------|
| 13 (UART1) | `UART1_IRQHandler` | UART1 RX → `CY_Z_UART1_IRQHandler()` |
| 15 (SysTick) | `SysTick_Handler` | SysTick → increments `tick_ms` |
| 1 (GROUP1) | `GROUP1_IRQHandler` | GPIOB: encoder X2 decode (DIO0, DIO12); GPIOA: multi-sensor dispatch |
| — | `UART_BNO08X_INST_IRQHandler` | Conditional: BNO08X IMU via DMA (if UART_BNO08X defined in SysConfig) |
| — | `UART_WIT_INST_IRQHandler` | Conditional: WIT IMU via DMA (if UART_WIT defined in SysConfig) |

UART0 has no interrupt enabled.

## SysConfig Structure

`empty.syscfg` defines these module instances:

| Instance | Peripheral | Details |
|---|---|---|
| GPIO_Motor | GPIO | 5 pins: L1(PA8), L2(PA22), R1(PA26), R2(PB24), STBY(PA24) |
| GPIO_LED | GPIO | 1 pin: LED(PA0) |
| GPIO_BUTTON | GPIO | 4 pins: BUTTON(PA18, input+pull-down), Key1(PB14), Key2(PB16), Key3(PA12) |
| GPIO_Sensor | GPIO | 3 pins: PL(PB13), SCK(PB15), SDA(PB1, input+pull-up) |
| GPIO_Conder | GPIO | 4 pins: PB12, PA13, PB0, PB6 (encoder inputs) |
| PWM_Motor | TIMA0 | 2 channels: CCP0(PB8), CCP1(PB20), period=1000 |
| UART_1 | UART1 | RX(PB7)+TX(PB4), 115200, RX interrupt |
| UART_0 | UART0 | TX(PA10) only, 115200 |
| SYSCTL | SYSCTL | 32 MHz SYSOSC, no PLL/HFXT |
| SYSTICK | SysTick | Period=32, enabled |

## Debugging

- Debug probe: XDS-110 via SWD (PA20=SWCLK, PA19=SWDIO)
- Jumpers `J101 15:16` and `J101 13:14` must be ON for debugging
- Build configuration: Debug (`-O0`, `-g` DWARF)
- Firmware does not use low-power modes

## Known Issues

1. **Key1/2/3 SysConfig misconfiguration**: Initialized as digital **outputs** in `SYSCFG_DL_GPIO_init()`. `Key_Init()` runtime-fixes Key1 and Key2 by disabling output and reconfiguring as input+pull-down. Key3 (PA12) is shared with the buzzer.
2. **Sensor pin comments are stale**: `grayscale_sensor.h` comments say PB14(SCK) and PB13(SDA), but the actual SysConfig assigns SCK=PB15 and SDA=PB1. The generated `ti_msp_dl_config.h` is authoritative.
3. **`delay.c` Chinese comments**: Encoded in GBK, may display as garbled text depending on editor locale.
4. **Buzzer/Key3 pin conflict**: PA12 is used by both `Buzzer_Beep()` and `Key_Read()` (Key3). Key3 is not practically usable as a button in the current configuration.
5. **Speed loop is pulse-count-based, not RPM**: Encoder speeds are raw pulse counts per 3ms period — not calibrated to physical RPM or mm/s.
