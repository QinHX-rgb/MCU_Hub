# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Project Overview

NUEDC smart car competition firmware for the **TI MSPM0G3507** (Cortex-M0+, 32 MHz). The car follows a black line on a white track using an 8-channel grayscale sensor, with dual-speed modes, OLED display, encoder odometry, and automatic stop-line detection. This is a bare-metal (NoRTOS) CCS Theia project.

## Build System

- **IDE:** TI Code Composer Studio Theia (v71+)
- **Compiler:** TI ARM Clang (`ti-cgt-armllvm_5.1.1.LTS`)
- **SysConfig:** Peripheral configuration via `empty.syscfg` → generates `Debug/ti_msp_dl_config.h` and `Debug/ti_msp_dl_config.c`. Edit the `.syscfg` file in CCS to change pin mux, clock, or peripheral settings — never hand-edit the generated files.
- **Connect:** `tiarmclang` must be on PATH. The generated makefile is in `Debug/makefile`. Build from CCS or at command line:
  ```sh
  cd Debug && make all   # produces Car_T.out
  ```
- **Clean:**
  ```sh
  cd Debug && make clean
  ```
- **clangd:** Compilation database at `Debug/.clangd/compile_commands.json` (auto-generated).

## Hardware Pin Map

| Function | Pin | Port | Package Pin |
|---|---|---|---|
| LED (finish indicator) | PA0 | GPIOA | 33 |
| Button S1 (fast start) | PA18 | GPIOA | 11 |
| Button Key1 | PA27 | GPIOA | 31 |
| Button Key2 / Slow start | PB16 | GPIOB | (J2.20) |
| Motor L PWM | PB8 | TIMA0_CCP0 | 54 |
| Motor R PWM | PB20 | TIMA0_CCP1 | 23 |
| Motor L1/L2 dir | PA8/PA22 | GPIOA | 54/18 |
| Motor R1/R2 dir | PB9/PB24 | GPIOB | 61/23 |
| Motor STBY | PA24 | GPIOA | 25 |
| Encoder R A/B | PB0/PB6 | GPIOB | 47/58 |
| Encoder L A/B | PB12/PA13 | GPIOB/GPIOA | 64/6 |
| Gray PL/SCK/SDA | PB13/PB15/PB1 | GPIOB | 1/3/48 |
| OLED I2C SDA/SCL | PA28/PA31 | I2C0 | – |
| UART0 TX (debug) | PA10 | UART0 | – |

Motor driver: **TB6612** dual H-bridge. PWM via TIMA0 edge-aligned mode, period=1000.

## Architecture

```
empty.c                    — Main application: init, 3-phase main loop, button handling, display
├── Driver/
│   ├── Motor_Control.c    — High-level: line-follow PD controller with multi-stage filtering
│   ├── motor.c            — Low-level: TB6612 PWM + direction pin control (Set_Speed)
│   ├── grayscale_sensor.c — 8-ch bit-banged sensor readout → weighted centroid (cx)
│   ├── Encoder.c          — Dual quadrature encoder (GPIO polling + interrupt fallback), EMA speed filter
│   ├── PID.c              — Generic float PID (defined but NOT used for line-follow)
│   ├── app_config.h       — ALL tunable parameters live here
│   └── OLED/              — SSD1306 I2C OLED driver
└── System/
    ├── clock.c             — SysTick → `tick_ms` millisecond counter
    ├── delay.c             — delay_us/ms (SysTick polling)
    ├── uart.c              — UART0 printf debug output (115200, PA10 TX)
    ├── interrupt.c         — SysTick_Handler + GROUP1 IRQ for encoder/sensor interrupts
    └── Key.c               — 3-button scanning (Key1/Key2/S1)
```

## Main Loop Flow

```
Phase 1: Wait for button → S1=fast mode, Key2=slow mode
Phase 2: Record start_time (tick_ms), engage motors
Phase 3: Driving loop (every CONTROL_PERIOD_MS = 5ms):
  1. Encoder_Poll()      — read encoder edge changes
  2. Grayscale_Update()  — read sensor → update global `cx` (centroid 0–70)
  3. Encoder_Update()    — compute EMA-filtered speed, reset pulse accumulators
  4. Stop-line check     — arm after START_LINE_CLEAR_FRAMES of 1–2 black, stop at ≥3 black
  5. Motor_FollowLine(cx)— PD controller → Motor_SetDifferential
  6. Display refresh     — every DISPLAY_UPDATE_FRAMES (40 cycles ≈ 200ms), also real-time timer
→ Return to Phase 1 after finish (infinite re-runs)
```

## Line-Follow Control (Motor_Control.c)

The primary control algorithm is a **PD controller with multi-stage filtering**:

1. **Position EMA**: `g_line_pos_filt += LINE_FILTER_ALPHA * (cx - g_line_pos_filt)` — smooths the 8‑channel discrete centroid
2. **Continuous dead zone**: `ApplyContinuousDeadZone()` — subtracts `LINE_DEAD_ZONE` from the error magnitude, eliminating small corrections (no output discontinuity at the edge)
3. **Derivative EMA**: `g_error_rate_filt += DERIVATIVE_FILTER_ALPHA * (error_rate - g_error_rate_filt)` — separate smoothing on error rate to suppress jitter
4. **Turn = KP × error + KD × error_rate**, clamped to ±STEER_LIMIT
5. **Curve slowdown**: `base_pwm -= CURVE_SLOWDOWN × |turn|/STEER_LIMIT`, floor at `CURVE_MIN_PWM` — larger error → lower base speed
6. **PWM EMA**: `g_pwm_L_filt += PWM_FILTER_ALPHA * (target - filtered)` — final smoothing on motor outputs
7. Left = base + turn, Right = base − turn

The generic `PID.c` module exists but is not wired into line-following.

## All Tunable Parameters (app_config.h)

See also `可调参数说明.txt` for Chinese-language tuning guide. Every configurable value is a `#define`:

- **Speed**: `BASE_PWM_FAST` (30), `BASE_PWM_SLOW` (24), `MAX_PWM` (40), `CURVE_MIN_PWM` (18.0f), `CURVE_SLOWDOWN` (6.0f)
- **Steering**: `LINE_CENTER` (35), `STEER_KP` (0.32f), `STEER_KD` (0.12f), `STEER_LIMIT` (15.0f), `LINE_DEAD_ZONE` (1.5f)
- **Filtering**: `LINE_FILTER_ALPHA` (0.22f), `DERIVATIVE_FILTER_ALPHA` (0.20f), `PWM_FILTER_ALPHA` (0.18f)
- **Stop line**: `STOP_LINE_BLACK_MIN` (3), `START_LINE_CLEAR_FRAMES` (5), `FINISH_BRAKE_TIME_MS` (150)
- **System**: `CONTROL_PERIOD_MS` (5), `DISPLAY_UPDATE_FRAMES` (40), `ENABLE_OLED` (1), `ENABLE_UART_DEBUG` (1)

## Encoder Design

Dual-mode decoding for reliability:
- **Polling path** (`Encoder_Poll`): Called every 5ms in main loop. Reads A-phase, detects edges, checks B-phase for direction. Works reliably at low speed.
- **Interrupt path** (in `interrupt.c` GROUP1_IRQHandler): PB0 (right A) and PB12 (left A) trigger on both edges (X2 decoding). `DL_GPIO_IIDX_DIO0` / `DL_GPIO_IIDX_DIO12` cases handle the counting.

Both paths write `g_enc_R_pulses` / `g_enc_L_pulses` (volatile, accessed with `__disable_irq()`). `Encoder_Update()` atomically reads and clears them, then applies EMA speed filtering.

## Key Conventions

- `tick_ms` (volatile, incremented by SysTick at 1 kHz) is the global time base — all timing derives from it
- Global `cx` (centroid position, 0–70) is updated by `Grayscale_Update()`, consumed by `Motor_FollowLine()`
- `SENSOR_BLACK_IS_LOW` = 1 means the current sensor module outputs LOW on black — swap to 0 if using a different sensor module
- Sensor channels 0–3 are on the right side of the car, 4–7 on the left; weights go 70→0 from channel 0→7
- OLED I2C address is assumed in the OLED driver (typically 0x3C for SSD1306)
- The project file (`.ccsproject`) and makefile contain hardcoded absolute Windows paths (`D:/DOWDLOAD/CCSTUDIO/...`) — these need updating when moving the project to a different machine

## Debug Output

When `ENABLE_UART_DEBUG` = 1, UART0 outputs at 115200 baud on PA10:
- `UART_Printf("format", ...)` appends `\r\n` automatically, max 128 bytes per call
- Stop-line armed state, sensor readings, and timing are printed each display refresh cycle
- Finish time and max sensor reading are printed on stop
