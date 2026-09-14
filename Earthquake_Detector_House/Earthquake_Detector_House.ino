/*
  Earthquake Detector House Prototype
  Course: Digital Signal Processing
  Year: 2024

  Platform: Arduino Uno (or compatible)

  Main functions:
  - MPU6050 detects vibration / acceleration.
  - A small DSP-style pipeline removes the gravity/baseline component,
    smooths the vibration signal and calculates short-window RMS energy.
  - When the vibration level exceeds a threshold:
      * LCD shows an earthquake warning.
      * Red LED flashes.
      * Buzzer sounds an alarm pattern.
      * Servo opens the evacuation door.
      * Multiple vibration motors run to simulate the earthquake.
  - When the vibration stops for a defined time, the system returns to READY.

  Libraries:
    Adafruit MPU6050
    Adafruit Unified Sensor
    LiquidCrystal_I2C
    Servo

  IMPORTANT HARDWARE NOTE:
  Do NOT power vibration motors directly from Arduino GPIO pins.
  Drive each motor using a transistor/MOSFET + flyback diode and use
  a suitable external power supply with COMMON GROUND to the Arduino.

  Typical I2C addresses:
    MPU6050: 0x68
    LCD:     0x27

  The LCD contrast potentiometer is hardware-only; no software control is needed.
*/

#include <Wire.h>
#include <Adafruit_MPU6050.h>
#include <Adafruit_Sensor.h>
#include <LiquidCrystal_I2C.h>
#include <Servo.h>
#include <math.h>

// ------------------------------------------------------------
// Hardware objects
// ------------------------------------------------------------

Adafruit_MPU6050 mpu;
LiquidCrystal_I2C lcd(0x27, 16, 2);
Servo doorServo;

// ------------------------------------------------------------
// Pin configuration
// Change these pins if your physical wiring is different.
// ------------------------------------------------------------

const byte LED_PIN = 11;
const byte BUZZER_PIN = 10;
const byte SERVO_PIN = 9;

// Three vibration motors used to shake different parts of the model.
// Use transistor/MOSFET drivers for these outputs.
const byte MOTOR_PINS[] = {5, 6, 7};
const byte MOTOR_COUNT = sizeof(MOTOR_PINS) / sizeof(MOTOR_PINS[0]);

// ------------------------------------------------------------
// Servo positions
// ------------------------------------------------------------

const int DOOR_CLOSED_ANGLE = 10;
const int DOOR_OPEN_ANGLE   = 95;

// ------------------------------------------------------------
// DSP / detection parameters
// ------------------------------------------------------------

// MPU6050 acceleration is returned in m/s^2.
// At rest, acceleration magnitude is close to gravity (~9.81 m/s^2).

const float GRAVITY = 9.80665;

// Exponential baseline tracker.
// Smaller value = slower baseline movement and better vibration isolation.
const float BASELINE_ALPHA = 0.01;

// Exponential smoothing for the absolute vibration signal.
const float FILTER_ALPHA = 0.25;

// RMS window length.
// At ~100 Hz sampling, 25 samples is roughly 250 ms.
const int RMS_WINDOW = 25;
float rmsBuffer[RMS_WINDOW] = {0};
int rmsIndex = 0;
float rmsSumSquares = 0;

// Trigger threshold in m/s^2 RMS.
// Tune this value experimentally for your prototype.
float EARTHQUAKE_THRESHOLD = 1.15;

// Lower threshold used to decide that the shaking has stopped.
// Hysteresis prevents rapid ON/OFF switching around one threshold.
float CLEAR_THRESHOLD = 0.55;

// Number of milliseconds the vibration must remain below CLEAR_THRESHOLD
// before the system leaves earthquake mode.
const unsigned long CLEAR_DELAY_MS = 3000;

// Minimum delay between two separate earthquake events.
const unsigned long RETRIGGER_DELAY_MS = 1500;

// Target sampling interval: 10 ms ≈ 100 Hz
const unsigned long SAMPLE_INTERVAL_MS = 10;

// ------------------------------------------------------------
// Runtime state
// ------------------------------------------------------------

float gravityBaseline = GRAVITY;
float filteredVibration = 0;

bool earthquakeActive = false;

unsigned long lastSampleTime = 0;
unsigned long belowThresholdSince = 0;
unsigned long lastEarthquakeEnd = 0;

// Alert timing
unsigned long lastLedToggle = 0;
unsigned long lastBuzzerToggle = 0;

bool ledState = false;
bool buzzerState = false;

// ------------------------------------------------------------
// Utility functions
// ------------------------------------------------------------

void setVibrationMotors(bool state) {
  for (byte i = 0; i < MOTOR_COUNT; i++) {
    digitalWrite(MOTOR_PINS[i], state ? HIGH : LOW);
  }
}

void closeDoor() {
  doorServo.write(DOOR_CLOSED_ANGLE);
}

void openDoor() {
  doorServo.write(DOOR_OPEN_ANGLE);
}

void showReadyScreen() {
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Earthquake Sys.");
  lcd.setCursor(0, 1);
  lcd.print("System Ready");
}

void showAlertScreen(float rmsValue) {
  lcd.setCursor(0, 0);
  lcd.print("EARTHQUAKE!     ");
  lcd.setCursor(0, 1);
  lcd.print("Evacuate ");
  lcd.print(rmsValue, 1);
  lcd.print("   ");
}

// ------------------------------------------------------------
// Earthquake response
// ------------------------------------------------------------

void startEarthquakeAlert(float rmsValue) {
  earthquakeActive = true;
  belowThresholdSince = 0;

  openDoor();
  setVibrationMotors(true);

  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("EARTHQUAKE!");
  lcd.setCursor(0, 1);
  lcd.print("EVACUATE NOW!");

  // Immediate visual/audio response
  digitalWrite(LED_PIN, HIGH);
  tone(BUZZER_PIN, 1800);

  ledState = true;
  buzzerState = true;

  Serial.print("EARTHQUAKE DETECTED | RMS = ");
  Serial.println(rmsValue, 3);
}

void updateEarthquakeAlert(float rmsValue) {
  unsigned long now = millis();

  // Flash LED every 180 ms
  if (now - lastLedToggle >= 180) {
    lastLedToggle = now;
    ledState = !ledState;
    digitalWrite(LED_PIN, ledState);
  }

  // Alternate buzzer frequency to create a danger siren
  if (now - lastBuzzerToggle >= 280) {
    lastBuzzerToggle = now;
    buzzerState = !buzzerState;

    if (buzzerState) {
      tone(BUZZER_PIN, 2100);
    } else {
      tone(BUZZER_PIN, 1200);
    }
  }

  // Keep motors running while alarm is active
  setVibrationMotors(true);

  // Refresh the bottom LCD line occasionally
  static unsigned long lastLcdUpdate = 0;
  if (now - lastLcdUpdate >= 500) {
    lastLcdUpdate = now;
    showAlertScreen(rmsValue);
  }
}

void stopEarthquakeAlert() {
  earthquakeActive = false;
  lastEarthquakeEnd = millis();

  noTone(BUZZER_PIN);
  digitalWrite(LED_PIN, LOW);
  setVibrationMotors(false);

  ledState = false;
  buzzerState = false;

  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Shaking Stopped");
  lcd.setCursor(0, 1);
  lcd.print("Door Closing...");

  delay(1500);

  closeDoor();
  showReadyScreen();

  Serial.println("Earthquake alert cleared.");
}

// ------------------------------------------------------------
// DSP: update short-window RMS of vibration
// ------------------------------------------------------------

float updateRMS(float sample) {
  float squared = sample * sample;

  rmsSumSquares -= rmsBuffer[rmsIndex];
  rmsBuffer[rmsIndex] = squared;
  rmsSumSquares += squared;

  rmsIndex++;
  if (rmsIndex >= RMS_WINDOW) {
    rmsIndex = 0;
  }

  return sqrt(rmsSumSquares / RMS_WINDOW);
}

// ------------------------------------------------------------
// Read sensor and calculate vibration level
// ------------------------------------------------------------

float readVibrationRMS() {
  sensors_event_t accel, gyro, temp;
  mpu.getEvent(&accel, &gyro, &temp);

  // Magnitude of 3-axis acceleration vector
  float magnitude = sqrt(
    accel.acceleration.x * accel.acceleration.x +
    accel.acceleration.y * accel.acceleration.y +
    accel.acceleration.z * accel.acceleration.z
  );

  // Slowly estimate the baseline acceleration.
  gravityBaseline =
    (1.0 - BASELINE_ALPHA) * gravityBaseline +
    BASELINE_ALPHA * magnitude;

  // High-pass-like vibration component:
  // remove slowly changing gravity/orientation baseline.
  float vibration = fabs(magnitude - gravityBaseline);

  // Smooth short spikes/noise.
  filteredVibration =
    (1.0 - FILTER_ALPHA) * filteredVibration +
    FILTER_ALPHA * vibration;

  // Calculate short-window RMS energy.
  float rms = updateRMS(filteredVibration);

  // Debug output for threshold tuning.
  Serial.print("Mag:");
  Serial.print(magnitude, 3);
  Serial.print(", Vibration:");
  Serial.print(filteredVibration, 3);
  Serial.print(", RMS:");
  Serial.println(rms, 3);

  return rms;
}

// ------------------------------------------------------------
// Setup
// ------------------------------------------------------------

void setup() {
  Serial.begin(115200);

  pinMode(LED_PIN, OUTPUT);
  pinMode(BUZZER_PIN, OUTPUT);

  for (byte i = 0; i < MOTOR_COUNT; i++) {
    pinMode(MOTOR_PINS[i], OUTPUT);
    digitalWrite(MOTOR_PINS[i], LOW);
  }

  digitalWrite(LED_PIN, LOW);
  noTone(BUZZER_PIN);

  doorServo.attach(SERVO_PIN);
  closeDoor();

  Wire.begin();

  lcd.init();
  lcd.backlight();

  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Initializing...");
  lcd.setCursor(0, 1);
  lcd.print("MPU6050");

  // Initialize MPU6050
  if (!mpu.begin()) {
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("MPU6050 ERROR");
    lcd.setCursor(0, 1);
    lcd.print("Check wiring");

    Serial.println("Failed to find MPU6050.");

    while (true) {
      digitalWrite(LED_PIN, !digitalRead(LED_PIN));
      delay(250);
    }
  }

  // Sensor configuration
  mpu.setAccelerometerRange(MPU6050_RANGE_4_G);
  mpu.setGyroRange(MPU6050_RANGE_500_DEG);
  mpu.setFilterBandwidth(MPU6050_BAND_44_HZ);

  // Initial baseline calibration
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Calibrating...");
  lcd.setCursor(0, 1);
  lcd.print("Keep Still");

  float baselineSum = 0;
  const int CALIBRATION_SAMPLES = 200;

  for (int i = 0; i < CALIBRATION_SAMPLES; i++) {
    sensors_event_t accel, gyro, temp;
    mpu.getEvent(&accel, &gyro, &temp);

    float magnitude = sqrt(
      accel.acceleration.x * accel.acceleration.x +
      accel.acceleration.y * accel.acceleration.y +
      accel.acceleration.z * accel.acceleration.z
    );

    baselineSum += magnitude;
    delay(5);
  }

  gravityBaseline = baselineSum / CALIBRATION_SAMPLES;

  Serial.print("Initial gravity baseline: ");
  Serial.println(gravityBaseline, 4);

  showReadyScreen();
}

// ------------------------------------------------------------
// Main loop
// ------------------------------------------------------------

void loop() {
  unsigned long now = millis();

  if (now - lastSampleTime < SAMPLE_INTERVAL_MS) {
    if (earthquakeActive) {
      updateEarthquakeAlert(0);
    }
    return;
  }

  lastSampleTime = now;

  float vibrationRMS = readVibrationRMS();

  // ----------------------------------------------------------
  // Trigger earthquake mode
  // ----------------------------------------------------------

  if (!earthquakeActive) {
    bool retriggerReady =
      (now - lastEarthquakeEnd >= RETRIGGER_DELAY_MS);

    if (vibrationRMS >= EARTHQUAKE_THRESHOLD && retriggerReady) {
      startEarthquakeAlert(vibrationRMS);
    }
  }

  // ----------------------------------------------------------
  // Earthquake mode
  // ----------------------------------------------------------

  else {
    updateEarthquakeAlert(vibrationRMS);

    // Start timing only after signal drops below clear threshold.
    if (vibrationRMS < CLEAR_THRESHOLD) {
      if (belowThresholdSince == 0) {
        belowThresholdSince = now;
      }

      if (now - belowThresholdSince >= CLEAR_DELAY_MS) {
        stopEarthquakeAlert();
      }
    } else {
      belowThresholdSince = 0;
    }
  }
}
