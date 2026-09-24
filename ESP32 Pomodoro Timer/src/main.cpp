/*
 * ===================================================================================
 * Project: ESP32 Pomodoro Timer
 * File: main.cpp
 * Author: Tamar
 * Description:
 *   שעון פומודורו שולחני מבוסס בקר ESP32 ומסך תווים LCD1602 (16x2 בחיבור I2C).
 *   
 * לחצנים ולוגיקת שליטה:
 *   - כפתור 1 (GPIO 18): הפעלה / השהיה (Start / Pause Toggle)
 *   - כפתור 2 (GPIO 19): איפוס (Reset לזמן שנבחר)
 *   - כפתור 3 (GPIO 23): קביעת זמן בקפיצות של 5 דקות (5, 10, 15 ... 60 דקות)
 *   - זמזם (GPIO 25): חיווי קולי ללחיצות והתראת סיום זמן
 *   - מסך LCD1602 I2C: SDA -> GPIO 21, SCL -> GPIO 22, VCC (5V / VIN), GND
 * ===================================================================================
 */

#include <Arduino.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>

// -----------------------------------------------------------------------------------
// הגדרות מסך LCD1602 (I2C)
// -----------------------------------------------------------------------------------
#define LCD_COLUMNS 16
#define LCD_ROWS    2

// מופע מסך סטנדרטי (כתובת ברירת מחדל 0x27, תשתנה אוטומטית ל-0x3F בעת הצורך)
LiquidCrystal_I2C lcd(0x27, LCD_COLUMNS, LCD_ROWS);

// -----------------------------------------------------------------------------------
// מיפוי פינים (Pinout) ב-ESP32
// -----------------------------------------------------------------------------------
const int PIN_BTN_START_PAUSE = 18; // כפתור 1: הפעלה / השהיה
const int PIN_BTN_RESET       = 19; // כפתור 2: איפוס
const int PIN_BTN_SET_TIME    = 23; // כפתור 3: קפיצות של 5 דקות
const int PIN_BUZZER          = 25; // זמזם (Active או Passive Buzzer)

const int PIN_I2C_SDA         = 21; // קו נתונים I2C למסך
const int PIN_I2C_SCL         = 22; // קו שעון I2C למסך

// -----------------------------------------------------------------------------------
// קבועים עבור זמנים והגדרות
// -----------------------------------------------------------------------------------
const unsigned long STEP_MINUTES    = 5;   // קפיצות של 5 דקות
const unsigned long MIN_MINUTES     = 5;   // מינימום 5 דקות
const unsigned long MAX_MINUTES     = 60;  // מקסימום 60 דקות (שעה)
const unsigned long DEFAULT_MINUTES = 25;  // ברירת מחדל לפומודורו קלאסי

// -----------------------------------------------------------------------------------
// מכונת מצבים (Finite State Machine)
// -----------------------------------------------------------------------------------
enum TimerState {
  STATE_IDLE,       // במנוחה / בחירת זמן (מוכן להפעלה)
  STATE_RUNNING,    // סופר לאחור (זמן עבודה / פוקוס)
  STATE_PAUSED,     // מושהה זמנית
  STATE_COMPLETED   // הזמן הסתיים (צפצוף והודעת סיום)
};

TimerState currentState = STATE_IDLE;

// משתני ניהול הזמן (נמדדים בשניות)
unsigned long configuredSeconds = DEFAULT_MINUTES * 60; // הזמן שנבחר
unsigned long remainingSeconds  = DEFAULT_MINUTES * 60; // הזמן שנותר לסיום
unsigned long lastTickMillis    = 0;                    // סנכרון שעון millis()

// -----------------------------------------------------------------------------------
// מנגנון Debounce תוכנתי לסינון רעשי מגע בלחצנים
// -----------------------------------------------------------------------------------
const unsigned long DEBOUNCE_DELAY = 50; // 50ms למניעת רטט לחצנים

struct Button {
  int pin;
  int lastReading;
  int stableState;
  unsigned long lastDebounceTime;
};

Button btnStart = {PIN_BTN_START_PAUSE, HIGH, HIGH, 0};
Button btnReset = {PIN_BTN_RESET,       HIGH, HIGH, 0};
Button btnSet   = {PIN_BTN_SET_TIME,    HIGH, HIGH, 0};

// בדיקה האם לחצן נלחץ כעת (זיהוי קצה יורד - מעבר מ-HIGH ל-LOW עם Pull-up)
bool isButtonPressed(Button &b) {
  int reading = digitalRead(b.pin);
  bool pressed = false;

  if (reading != b.lastReading) {
    b.lastDebounceTime = millis();
  }

  if ((millis() - b.lastDebounceTime) > DEBOUNCE_DELAY) {
    if (reading != b.stableState) {
      b.stableState = reading;
      if (b.stableState == LOW) { // חיבור ל-GND בעת לחיצה
        pressed = true;
      }
    }
  }

  b.lastReading = reading;
  return pressed;
}

// -----------------------------------------------------------------------------------
// שליטה בזמזם (Buzzer)
// -----------------------------------------------------------------------------------
void playBeep(int frequency, int durationMs) {
  tone(PIN_BUZZER, frequency, durationMs);
  delay(durationMs);
  noTone(PIN_BUZZER);
}

// מנגינת סיום פומודורו
void playAlarmDone() {
  for (int i = 0; i < 3; i++) {
    playBeep(1800, 180);
    delay(100);
    playBeep(2400, 250);
    delay(140);
  }
}

// -----------------------------------------------------------------------------------
// בדיקת כתובת I2C (בודק האם הרכיב עונה)
// -----------------------------------------------------------------------------------
bool testI2CAddress(uint8_t addr) {
  Wire.beginTransmission(addr);
  return (Wire.endTransmission() == 0);
}

// -----------------------------------------------------------------------------------
// פונקציית עדכון מסך LCD1602 (16 תווים x 2 שורות, ללא הבהובים)
// -----------------------------------------------------------------------------------
void updateDisplay() {
  char line1[17];
  char line2[17];

  unsigned long mins = remainingSeconds / 60;
  unsigned long secs = remainingSeconds % 60;

  switch (currentState) {
    case STATE_IDLE:
      snprintf(line1, sizeof(line1), "[READY]  Set:%2lum", configuredSeconds / 60);
      snprintf(line2, sizeof(line2), "Time:    %02lu:%02lu  ", mins, secs);
      break;

    case STATE_RUNNING: {
      snprintf(line1, sizeof(line1), "FOCUS TIME...   ");
      
      int totalBars = 6;
      int filledBars = 0;
      if (configuredSeconds > 0) {
        float progress = (float)(configuredSeconds - remainingSeconds) / (float)configuredSeconds;
        filledBars = (int)(progress * totalBars);
      }
      char bar[7] = "      ";
      for (int i = 0; i < filledBars && i < totalBars; i++) {
        bar[i] = '=';
      }
      snprintf(line2, sizeof(line2), "%02lu:%02lu [%s]", mins, secs, bar);
      break;
    }

    case STATE_PAUSED:
      snprintf(line1, sizeof(line1), "[PAUSED]        ");
      snprintf(line2, sizeof(line2), "Time:    %02lu:%02lu  ", mins, secs);
      break;

    case STATE_COMPLETED:
      snprintf(line1, sizeof(line1), "*** TIME'S UP **");
      snprintf(line2, sizeof(line2), "GREAT JOB!   :D ");
      break;
  }

  // הדפסה במיקום קבוע ללא lcd.clear() למניעת הבהובים
  lcd.setCursor(0, 0);
  lcd.print(line1);

  lcd.setCursor(0, 1);
  lcd.print(line2);
}

// -----------------------------------------------------------------------------------
// Setup - אתחול המערכת
// -----------------------------------------------------------------------------------
void setup() {
  Serial.begin(115200);
  delay(300); // השהייה לייצוב מתח לאחר הדלקה

  Serial.println(F("\n============================================="));
  Serial.println(F("   ESP32 Pomodoro Timer (LCD1602 Edition)   "));
  Serial.println(F("============================================="));

  // הגדרת לחצנים עם Pull-up פנימי
  pinMode(PIN_BTN_START_PAUSE, INPUT_PULLUP);
  pinMode(PIN_BTN_RESET,       INPUT_PULLUP);
  pinMode(PIN_BTN_SET_TIME,    INPUT_PULLUP);

  // הגדרת זמזם
  pinMode(PIN_BUZZER, OUTPUT);
  digitalWrite(PIN_BUZZER, LOW);

  // אתחול אפיק I2C בפינים 21 ו-22
  Wire.begin(PIN_I2C_SDA, PIN_I2C_SCL);
  Wire.setClock(50000); // 50kHz – קצב אופטימלי ויציב במיוחד למניעת שיבושי תווים (Gibberish)
  delay(150);

  // בדיקה חכמה של כתובת ה-I2C (0x27 ברירת מחדל, אם לא עונה נבדק 0x3F)
  if (!testI2CAddress(0x27)) {
    if (testI2CAddress(0x3F)) {
      Serial.println(F("[LCD] Detected I2C address: 0x3F"));
      lcd = LiquidCrystal_I2C(0x3F, LCD_COLUMNS, LCD_ROWS);
    } else {
      Serial.println(F("[LCD WARNING] No response at 0x27 or 0x3F, keeping 0x27"));
    }
  } else {
    Serial.println(F("[LCD] Detected I2C address: 0x27"));
  }

  // אתחול המסך
  lcd.init();
  delay(50);
  lcd.backlight();
  lcd.clear();

  // הודעת פתיחה
  lcd.setCursor(0, 0);
  lcd.print("POMODORO TIMER  ");
  lcd.setCursor(0, 1);
  lcd.print("ESP32 Starting..");

  playBeep(2000, 100);
  delay(1500);

  lcd.clear();
  updateDisplay();
}

// -----------------------------------------------------------------------------------
// Loop - הלולאה הראשית
// -----------------------------------------------------------------------------------
void loop() {
  unsigned long currentMillis = millis();

  // 1. קריאת הלחצנים בעזרת Debounce
  bool startPausePressed = isButtonPressed(btnStart);
  bool resetPressed      = isButtonPressed(btnReset);
  bool setTimePressed    = isButtonPressed(btnSet);

  // ---------------------------------------------------------------------------------
  // 2. כפתור 3: קביעת זמן בקפיצות של 5 דקות (5, 10, 15, ... 60)
  // ---------------------------------------------------------------------------------
  if (setTimePressed) {
    if (currentState == STATE_IDLE || currentState == STATE_PAUSED || currentState == STATE_COMPLETED) {
      unsigned long currentMinutes = configuredSeconds / 60;
      currentMinutes += STEP_MINUTES;

      if (currentMinutes > MAX_MINUTES) {
        currentMinutes = MIN_MINUTES; // מעגל חזרה ל-5 דקות
      }

      configuredSeconds = currentMinutes * 60;
      remainingSeconds  = configuredSeconds;
      currentState      = STATE_IDLE; // חזרה למצב מוכן

      playBeep(1200, 70);
      Serial.printf("Configured time set to: %lu minutes\n", currentMinutes);
      updateDisplay();
    }
  }

  // ---------------------------------------------------------------------------------
  // 3. כפתור 1: הפעלה / השהיה (Start / Pause Toggle)
  // ---------------------------------------------------------------------------------
  if (startPausePressed) {
    if (currentState == STATE_IDLE) {
      currentState = STATE_RUNNING;
      lastTickMillis = currentMillis;
      playBeep(1500, 90);
      Serial.println(F("Timer started"));
    } else if (currentState == STATE_RUNNING) {
      currentState = STATE_PAUSED;
      playBeep(1000, 90);
      Serial.println(F("Timer paused"));
    } else if (currentState == STATE_PAUSED) {
      currentState = STATE_RUNNING;
      lastTickMillis = currentMillis;
      playBeep(1500, 90);
      Serial.println(F("Timer resumed"));
    } else if (currentState == STATE_COMPLETED) {
      // הפעלה מחדש של פרק הזמן האחרון
      remainingSeconds = configuredSeconds;
      currentState = STATE_RUNNING;
      lastTickMillis = currentMillis;
      playBeep(1500, 90);
      Serial.println(F("Timer restarted"));
    }
    updateDisplay();
  }

  // ---------------------------------------------------------------------------------
  // 4. כפתור 2: איפוס (Reset)
  // ---------------------------------------------------------------------------------
  if (resetPressed) {
    currentState = STATE_IDLE;
    remainingSeconds = configuredSeconds;
    playBeep(800, 120);
    Serial.println(F("Timer reset"));
    updateDisplay();
  }

  // ---------------------------------------------------------------------------------
  // 5. ניהול ספירת הזמן לאחור (ספירה בלתי חוסמת בכל שנייה)
  // ---------------------------------------------------------------------------------
  if (currentState == STATE_RUNNING) {
    if (currentMillis - lastTickMillis >= 1000) {
      lastTickMillis += 1000;

      if (remainingSeconds > 0) {
        remainingSeconds--;
        updateDisplay();
      }

      if (remainingSeconds == 0) {
        currentState = STATE_COMPLETED;
        updateDisplay();
        Serial.println(F("Pomodoro session completed!"));
        playAlarmDone();
      }
    }
  }
}
