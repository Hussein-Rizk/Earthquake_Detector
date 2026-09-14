# 🏠 Earthquake Detector House Prototype

**Course:** Digital Signal Processing  
**Year:** 2024  
**Platform:** Arduino / Embedded System  
**Design Tools:** SolidWorks  
**Programming Language:** Arduino C/C++  

This project is a **house prototype earthquake detector system** designed to simulate an emergency response during an earthquake.  
The prototype combines **mechanical design**, **embedded electronics**, and **signal-based event detection**.

When earthquake-like vibration is detected, the system reacts automatically by:

- detecting vibration using the **MPU6050 sensor**
- displaying a warning message on the **LCD**
- flashing an **LED**
- sounding a **buzzer alarm**
- activating **multiple vibration motors** to simulate earthquake shaking
- opening the **servo-controlled house door** so people can evacuate

The body of the prototype was designed as a **screw-assembled house structure**, so the physical movement and shaking behavior can better represent an earthquake scenario.

---

## 📌 Project Idea

The system represents a small smart safety prototype for **earthquake monitoring and emergency response**.

### Main concept

```text
MPU6050 detects vibration / acceleration
                ↓
      Arduino processes sensor data
                ↓
 Earthquake warning is triggered
                ↓
 LCD + LED + Buzzer + Servo Door + Vibration Motors
```

The system is intended as an educational prototype showing how an embedded system can combine **sensing, signal processing, alerting, and actuation** in one project.

---

## ⚙️ Features

- **Earthquake / vibration detection** using **MPU6050**
- **16x2 LCD warning display**
- **LED flashing alert**
- **Buzzer danger sound**
- **Servo motor door opening**
- **Multiple vibration motors** to simulate earthquake shaking
- **House prototype body** designed in SolidWorks
- **Screw-connected mechanical structure** for realistic movement

---

## 🧩 Hardware Components

The project is based on a house prototype containing the following hardware:

- Arduino Uno (or compatible microcontroller)
- MPU6050 accelerometer / gyroscope sensor
- 16x2 LCD display
- Potentiometer (used for LCD contrast adjustment)
- LED
- Buzzer
- Servo motor
- Multiple vibration motors
- Breadboard and jumper wires
- Wooden / laser-cut / CAD-designed house body
- External driver stage for motors (recommended)

---

## 🛠️ Mechanical / Design Concept

The outer body of the prototype was designed as a **small house enclosure**.

Key mechanical ideas:

- front panel includes the **entrance / main door**
- the **servo motor** opens the evacuation door
- all body panels are **assembled with screws**
- the structure is intentionally designed to visually react to shaking
- the internal space holds the electronics and wiring

This makes the project a combination of:

- **mechanical design**
- **embedded control**
- **signal detection**
- **safety simulation**

---

## 🧪 System Behavior

### Normal state
- The system continuously reads acceleration and vibration data from the MPU6050 sensor
- The LCD can show that the system is ready / monitoring
- The door remains closed

### Earthquake detected
When the vibration level exceeds the defined threshold:

- LCD displays an earthquake warning
- LED flashes
- buzzer produces a danger alarm
- vibration motors run to simulate shaking
- servo motor opens the door for evacuation

### After shaking stops
- alarm outputs stop
- motors stop
- door can return to closed position
- system returns to monitoring mode

---

## 💻 Code

The repository contains an Arduino implementation for the system inside its own code folder.

The code integrates:

- MPU6050 reading
- simple vibration / threshold detection
- LCD messaging
- LED alerting
- buzzer alarm
- servo door control
- vibration motor control

---

## 📷 Hardware Photo

<p align="center">
  <img src="./Pics/Hardware_View.jpg" width="55%">
</p>



---

## 🚀 How to Use

### Arduino code
1. Open the `.ino` file in **Arduino IDE**
2. Install the required libraries
3. Connect the components to the correct pins
4. Upload the sketch to the Arduino board

### SolidWorks design
- Open the `.SLDPRT` and `.SLDASM` files in **SolidWorks**
- Use the `.STL` files for 3D previewing or manufacturing workflows

### Presentation
- Open the PowerPoint to review the project explanation and documentation

---

## 📚 Libraries Used in the Arduino Code

Depending on the final wiring and implementation, the code uses libraries such as:

- `Wire.h`
- `Adafruit_MPU6050.h`
- `Adafruit_Sensor.h`
- `LiquidCrystal_I2C.h`
- `Servo.h`

---


## 🎯 Learning Outcomes

This project demonstrates:

- embedded system integration
- sensor-based event detection
- alert generation
- servo actuation
- prototype safety systems
- combining **DSP concepts** with hardware prototyping
- integrating CAD design with electronics

---

## 👩‍🏫 Course Information

**Course:** Digital Signal Processing  
**Year:** 2024  
**Project Type:** Embedded prototype / smart house safety system  

> A prototype earthquake detector house that combines MPU6050-based vibration sensing, emergency alerts, actuator control, and a custom-designed mechanical enclosure.

