# 🚗 Bare-Metal STM32 Firmware: Car Alarm & Fan Controller

Two firmware projects for the STM32F103T6 (Cortex-M3), written in C directly against the peripheral registers with only the CMSIS device headers — no HAL, no LL, no code generators. The first one is a car alarm / central locking module built on GPIO, EXTI and hardware timers. The second one is a ventilation controller that combines PWM, an interrupt-driven ADC and a UART command interface. Both are PlatformIO projects and come with a Proteus schematic for simulation.

🎓 IoT / microprocessor systems course, Software Engineering, Igor Sikorsky Kyiv Polytechnic Institute, spring 2026.

## How the firmware is built

Both projects follow the same structure:

- **One module per peripheral** (GPIO, timers, EXTI, ADC, PWM, UART), with the application logic kept in a separate state machine.
- **Event-driven design** — interrupt handlers only record events, the main loop sleeps in `__WFI()` and handles them when it wakes up.
- **Hardware timing** — every time interval comes from a hardware timer, there are no delay loops.
- **Debounced inputs** — a button or sensor edge is accepted only 30 ms after the previous accepted change.

## 🔐 Car alarm (`car-alarm/`)

Three buttons (lock, unlock, action) and a PIR motion sensor on PA0–PA2 and PA7, two relays (lock, window) on PB0/PB1, a buzzer on PB4 and an RGB LED on PB5–PB7. The pin map lives in `src/hardware.h`.

| State | Indication | Transitions |
|---|---|---|
| OPEN (initial) | solid green | lock → 250 ms lock-relay pulse, 400 ms beep → CLOSED; action held ≥ 800 ms → WINDOW_ACTION |
| CLOSED | red, 200 ms on / 800 ms off | motion → WARNING; unlock → OPEN |
| WARNING | yellow blinking 200 / 200 ms | second motion → ALARM; 4 s without it → CLOSED; unlock → OPEN |
| ALARM | red toggling every 100 ms, buzzer 200 / 200 ms | unlock → OPEN |
| WINDOW_ACTION | blue toggling every 150 ms, window relay on | after 6 s → relay off, short beep → OPEN |

Unlocking pulses the lock relay, silences the siren and gives a 150 ms beep. A motion trigger only counts after the sensor has returned to idle, so one long PIR pulse cannot raise the alarm on its own.

Under the hood:

- **Events** are bit flags OR-ed into a `volatile` mask by the ISRs; the main loop takes and clears the mask with interrupts disabled and dispatches it.
- **TIM2** produces a 1 ms tick (`millis()`) and drives five software timers: LED blink step, warning timeout, window timeout, siren step and long-press detection.
- **TIM3** and **TIM1** run in one-pulse mode for the lock-relay pulse and single buzzer beeps.
- **EXTI** on both edges handles the buttons and the motion sensor; the action button starts the long-press timer on press and cancels it on release.
- The GPIO driver configures `CRL` nibbles through named macros and switches outputs atomically with `BSRR`.

## 🌀 Fan controller (`fan-controller/`)

| Signal | Pin | Peripheral |
|---|---|---|
| Fan PWM | PA8 | TIM1_CH1 |
| Level LED PWM | PA1 | TIM2_CH2 |
| Potentiometer / LM35 | PA2 / PA3 | ADC1 channels 2 and 3 |
| UART TX / RX | PA9 / PA10 | USART1, 9600 baud, 8N1 |
| Mode button | PB0 | EXTI0, both edges |

The controller has three modes:

| Mode | Fan speed | Entered by |
|---|---|---|
| MANUAL (initial) | potentiometer position (0–100 %) or the last `F` command | mode button, `M` command |
| AUTO | < 30 °C → 30 %, 30–39 °C → 60 %, ≥ 40 °C → 100 % | mode button, `A` command |
| WARNING | 100 % | temperature ≥ 60 °C, prints `WARNING: OVERHEAT` |

WARNING is left with a 5 °C hysteresis: at ≤ 55 °C the controller goes back to the mode it was in. In manual mode a speed set over UART stays in effect until the potentiometer is moved by at least 50 ADC counts. The LED brightness always follows the fan speed.

UART commands (case-insensitive, every character is echoed):

| Command | Effect |
|---|---|
| `M` | manual mode (potentiometer control) |
| `A` | automatic mode |
| `S` | print the status right away |
| `F` + `0`…`100` + Enter | set the fan speed in manual mode |

Once a second the controller prints its status:

```
MODE:AUTO TEMP:27 SPEED:30 POT_RAW:2048 POT_%:50
```

Under the hood:

- **Event queue** — a 32-slot ring buffer of `{type, data}` events filled by the ISRs (button, potentiometer, temperature, UART command, status timer). UART commands are packed into the 16-bit payload as `(cmd << 8) | value`.
- **ADC** — calibrated at start-up, single conversions with the EOC interrupt; the ISR alternates between the two channels, so both inputs are sampled continuously without polling. A new reading is posted only when it differs from the last one by at least 8 counts, which keeps the queue free for button and UART events. Temperature: `T[°C] = raw · 330 / 4095` (LM35, 10 mV/°C, 3.3 V reference).
- **PWM** — TIM1 (with `BDTR.MOE`) and TIM2 with a 0–100 range, so the duty cycle is written to `CCRx` directly in percent.
- **Timing** — TIM3 posts the status event every second; SysTick gives a 1 ms time base for debouncing the mode button.
- **UART** — `BRR` is derived from `SystemCoreClock`, reception is interrupt-driven with a small parser for the `F<number>` command.

## 🧪 Simulation

Both firmwares were tested in Proteus 8 on the STM32F103T6 model. The car alarm schematic has the three buttons, a PIR sensor, two relays, a buzzer and an RGB LED; the fan controller schematic has a potentiometer, an LM35, a DC fan, an LED, the mode button and a virtual terminal at 9600 baud. The MCU in each schematic loads `.pio/build/genericSTM32F103T6/firmware.elf`, so a fresh build is picked up on the next simulation run.

## 🗂 Project Structure

```
stm32-baremetal-firmware/
├── car-alarm/
│   ├── src/        # GPIO, timers, EXTI buttons, state machine
│   └── proteus/    # Proteus simulation project
└── fan-controller/
    ├── src/        # PWM, ADC, UART, events, state machine
    └── proteus/    # Proteus simulation project
```

## 🖥️ Running Locally

You need [PlatformIO Core](https://platformio.org/install/cli) or the PlatformIO IDE extension; Proteus 8 for simulation, or an ST-Link and an STM32F103 board for real hardware.

```bash
git clone https://github.com/pgerasymchuk/stm32-baremetal-firmware.git
cd stm32-baremetal-firmware/car-alarm      # or fan-controller
pio run                 # build
pio run -t upload       # flash via ST-Link
```

To simulate, build the project and open `proteus/car-alarm.pdsprj` or `proteus/fan-controller.pdsprj`. For the fan controller, open the virtual terminal and try `M`, `A`, `S` or `F75` + Enter.

## 🛠️ Tech Stack

- C, CMSIS device headers (`stm32f103x6.h`)
- STM32F103T6 — ARM Cortex-M3, 32 KB Flash, 10 KB RAM
- PlatformIO (`ststm32` platform, `cmsis` framework), GNU Arm toolchain
- Proteus 8 Professional
