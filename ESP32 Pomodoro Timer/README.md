# ESP32 Pomodoro Timer ⏱️

פרויקט שעון פומודורו שולחני מבוסס מיקרו-בקר **ESP32** ומסך תווים **LCD1602 (16x2) בחיבור I2C**.

---

## 📁 מבנה הפרויקט (Project Structure)

```text
ESP32 Pomodoro Timer/
│
├── 📂 Hardware/                     <-- קבצי תכנון החומרה והמעגל ב-KiCad
│   ├── ESP32 Pomodoro Timer.kicad_pro   (קובץ פרויקט KiCad הראשי)
│   ├── ESP32 Pomodoro Timer.kicad_sch   (סכמת המעגל החשמלי)
│   ├── ESP32 Pomodoro Timer.kicad_pcb   (תכנון לוח המעגל המודפס)
│   └── .history/                        (היסטוריית גרסאות KiCad)
│
├── 📂 Firmware/                     <-- קבצי התוכנה והקוד ל-VS Code
│   ├── 📂 src/
│   │   └── main.cpp                     (קוד C++ מלא ומעודכן עבור מסך LCD1602)
│   ├── platformio.ini                   (קובץ הגדרות תוכנה וספריות)
│   └── .pio/                            (קבצי בנייה והידור)
│
├── platformio.ini                   (קובץ הגדרות ראשי עבור VS Code PlatformIO)
└── 📄 README.md                     <-- מסמך הסבר וריכוז הפרויקט
```

---

## 🔌 סכמת חיבורים פיזית (Pinout Mapping)

| רכיב | פין ברכיב | פין ב-ESP32 | תפקיד |
|---|---|---|---|
| **מסך LCD1602 (I2C Backpack)** | `SDA` | `GPIO 21` | קו נתוני תקשורת I2C |
| | `SCL` | `GPIO 22` | קו שעון תקשורת I2C |
| | `VCC` | `5V` (או `VIN`) | מתח הזנה למסך ותאורה אחורית |
| | `GND` | `GND` | הארקה משותפת |
| **כפתור 1 (Start/Pause)** | רגל לחצן | `GPIO 18` | הפעלה / השהיה (Toggle) |
| **כפתור 2 (Reset)** | רגל לחצן | `GPIO 19` | איפוס הטיימר לזמן המוגדר |
| **כפתור 3 (Set Time)** | רגל לחצן | `GPIO 23` | הוספת 5 דקות בכל לחיצה (5 עד 60 דק') |
| **זמזם (Buzzer)** | רגל חיובית | `GPIO 25` | צפצוף חיווי לחיצה והתראת סיום |

---

## 🖥️ ממשק התצוגה במסך ה-LCD1602 (16 תווים x 2 שורות)

* **מצב בחירת זמן (IDLE):**
  ```text
  [READY]  Set:25m
  Time:    25:00
  ```
* **מצב עבודה בריכוז (RUNNING):**
  ```text
  FOCUS TIME...
  24:35 [===>  ]
  ```
* **מצב השהיה (PAUSED):**
  ```text
  [PAUSED]
  Time:    24:35
  ```
* **סיום מקטע (DONE):**
  ```text
  *** TIME'S UP **
  GREAT JOB!   :D
  ```
