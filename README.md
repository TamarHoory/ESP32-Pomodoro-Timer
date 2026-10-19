# ESP32 Pomodoro Timer ⏱️

A standalone, tactile desktop Pomodoro productivity timer powered by an **ESP32** microcontroller and a **16x2 I2C LCD (LCD1602)** display. Complete with custom KiCad hardware schematics, PCB layout, and robust, non-blocking Arduino/PlatformIO firmware.

---

## 🌟 Highlights

- **Microcontroller**: ESP32 (ESP32-WROOM-32 / ESP32 DevKit)
- **Display**: 16x2 Character LCD via I2C backpack (PCF8574) with auto-address discovery (`0x27` / `0x3F`) and real-time visual progress bar.
- **Tactile Push Buttons**:
  - **Start / Pause**: Toggle countdown or resume paused sessions.
  - **Reset**: Instantly reset session to the configured interval.
  - **Set Time**: Cycle duration in 5-minute steps (5 min up to 60 min).
- **Audio Feedback**: Piezo buzzer delivers audible clicks on button press and a celebratory victory fanfare when the timer completes.
- **Robust Firmware**:
  - Clean **Finite State Machine (FSM)**: `IDLE`, `RUNNING`, `PAUSED`, and `COMPLETED`.
  - Fully **non-blocking timekeeping** using `millis()`.
  - Software debouncing (50 ms) utilizing internal ESP32 pull-ups (`INPUT_PULLUP`).
  - Flicker-free LCD updates (no full-screen clears during ticks).
- **Hardware Ready**: Includes complete **KiCad** electrical schematics (`.kicad_sch`) and printed circuit board design (`.kicad_pcb`).

---

## 📁 Repository Structure

```text
ESP32-Pomodoro-Timer/
├── Hardware/                          # KiCad electronics design files
│   ├── ESP32 Pomodoro Timer.kicad_sch # Schematic diagram
│   ├── ESP32 Pomodoro Timer.kicad_pcb # PCB routing and layout
│   └── ESP32 Pomodoro Timer.kicad_pro # KiCad project configuration
│
├── ESP32 Pomodoro Timer/              # PlatformIO embedded firmware
│   ├── include/                       # Header declarations
│   ├── lib/                           # Third-party libraries
│   │   └── LiquidCrystal_I2C/         # I2C LCD control library
│   ├── src/
│   │   └── main.cpp                   # Core application logic & state machine
│   ├── test/                          # Unit tests
│   └── platformio.ini                 # PlatformIO environment configuration
│
├── .gitignore                         # Build and temporary files exclusion
└── README.md                          # Project documentation
```

---

## 🔌 Hardware Pinout & Wiring

| Component | Pin / Terminal | ESP32 GPIO | Description |
|---|---|---|---|
| **LCD1602 (I2C Backpack)** | `SDA` | `GPIO 21` | I2C Data Line (50 kHz clock rate) |
| | `SCL` | `GPIO 22` | I2C Clock Line |
| | `VCC` | `VIN` / `5V` | Module power supply & backlight |
| | `GND` | `GND` | Common ground |
| **Start / Pause Button** | Pin 1 / Switch | `GPIO 18` | Active-LOW (Internal `INPUT_PULLUP`) |
| | Pin 2 / Switch | `GND` | Ground connection |
| **Reset Button** | Pin 1 / Switch | `GPIO 19` | Active-LOW (Internal `INPUT_PULLUP`) |
| | Pin 2 / Switch | `GND` | Ground connection |
| **Set Time Button** | Pin 1 / Switch | `GPIO 23` | Active-LOW (Internal `INPUT_PULLUP`) |
| | Pin 2 / Switch | `GND` | Ground connection |
| **Piezo Buzzer** | Positive (+) | `GPIO 25` | PWM tone signal |
| | Negative (-) | `GND` | Ground connection |

> **Note on LCD Power**: Most LCD1602 I2C backpacks require 5V (`VIN`) for bright backlight and optimal character contrast. The I2C data lines (`SDA`/`SCL`) operate smoothly at 3.3V logic levels on the ESP32.

---

## 🖥️ Display UI & State Machine

The firmware implements a responsive, non-blocking finite state machine displayed across 2 rows of 16 characters:

### 1. Ready / Idle State (`STATE_IDLE`)
Allows setting the session duration using the **Set Time** button:
```text
+----------------+
|[READY]  Set:25m|
|Time:    25:00  |
+----------------+
```

### 2. Focus / Running State (`STATE_RUNNING`)
Active countdown with elapsed ratio visual progress bar:
```text
+----------------+
|FOCUS TIME...   |
|24:12 [===   ]  |
+----------------+
```

### 3. Paused State (`STATE_PAUSED`)
Temporarily pauses countdown without losing elapsed progress:
```text
+----------------+
|[PAUSED]        |
|Time:    18:45  |
+----------------+
```

### 4. Completed State (`STATE_COMPLETED`)
Fires when the timer reaches zero, accompanied by a triple-beep audio chime:
```text
+----------------+
|*** TIME'S UP **|
|GREAT JOB!   :D |
+----------------+
```

---

## 🚀 Getting Started

### Prerequisites

- [VS Code](https://code.visualstudio.com/) with the [PlatformIO IDE extension](https://platformio.org/) installed, OR [PlatformIO Core (CLI)](https://docs.platformio.org/en/latest/core/index.html).
- [KiCad](https://www.kicad.org/) (version 8 or newer) to open and inspect the PCB and schematic.
- A standard micro-USB or USB-C cable to connect the ESP32 to your PC.

### Flashing the Firmware

1. **Clone the repository**:
   ```bash
   git clone https://github.com/TamarHoory/ESP32-Pomodoro-Timer.git
   cd ESP32-Pomodoro-Timer
   ```

2. **Open in VS Code**:
   - Open VS Code.
   - Go to **File > Open Folder...** and select the `ESP32 Pomodoro Timer` folder (or the root workspace).

3. **Build & Upload**:
   - Click the PlatformIO **Build** (✓) icon in the bottom status bar, or run:
     ```bash
     pio run
     ```
   - Connect your ESP32 board and click **Upload** (→), or run:
     ```bash
     pio run --target upload
     ```

4. **Monitor Serial Output**:
   - Open the serial monitor at 115200 baud:
     ```bash
     pio device monitor
     ```

---

## 🛠️ Hardware Fabrication (KiCad)

The `Hardware/` folder contains native KiCad files:
- **`ESP32 Pomodoro Timer.kicad_sch`**: Electrical schematic connecting the ESP32 Dev board, tactile buttons, buzzer, and I2C connector header.
- **`ESP32 Pomodoro Timer.kicad_pcb`**: 2-layer PCB layout with traces, component placements, and mounting holes.

To export Gerber files for PCB manufacturing (JLCPCB, PCBWay, etc.):
1. Open `ESP32 Pomodoro Timer.kicad_pro` in KiCad.
2. In the PCB editor, navigate to **File > Fabrication Outputs > Gerbers (.gbr)**.
3. Generate drill files via **File > Fabrication Outputs > Drill Files (.drl)**.
4. Compress the output directory into a `.zip` archive for ordering.

---

## 👤 Author

**Tamar Hoory**  
- GitHub: [@TamarHoory](https://github.com/TamarHoory)

---

## 📜 License

This project is open-source and available under the [MIT License](LICENSE).
