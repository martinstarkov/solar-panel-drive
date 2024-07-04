#pragma once

#include <Arduino.h>
#include <Servo.h>

enum class Command {
  Invalid = -1,
  Rotation,
  ManualSwitch
};

class ServoMotor {
public:
  ServoMotor(int pin, int cw, int stop, int ccw, int delay)
    : pin_{ pin }, cw_{ cw }, stop_{ stop }, ccw_{ ccw }, delay_{ delay } {}

  void Init() {
    servo.attach(pin_);
    delay(250);
    servo.write(stop_);
    Serial.println("Initialized switch servo");
  }

  void Switch(int direction) {
    int write_value = stop_;
    if (direction == 1) write_value = cw_;
    else if (direction == -1) write_value = ccw_;
    Serial.println("Switching stepper motors");
    delay(delay_);
    servo.writeMicroseconds(write_value);
    delay(delay_);
    servo.writeMicroseconds(stop_);
  }
private:

  const int pin_{ 0 };
  const int cw_{ 0 };
  const int stop_{ 0 };
  const int ccw_{ 0 };
  const int delay_{ 0 }; // milliseconds
  Servo servo;
};

struct StepperPins {
  StepperPins(int pin1, int pin2, int pin3, int pin4) : pins{ pin1, pin2, pin3, pin4 } {}
  int operator[](size_t index) const {
    return pins[index];
  } 
  int pins[4];
};

class StepperMotor {
public:
    StepperMotor() = default;

    StepperMotor(StepperPins pins, int principal_direction, float step_per_deg) 
      : pins_{ pins }, principal_direction_{ principal_direction }, step_per_deg_{ step_per_deg } {
      SetPinMode(OUTPUT);
    }

    void SetPinMode(uint8_t mode) {
      for(int i = 0; i < 4; i++){
          pinMode(pins_[i], mode);
      }
    }

    bool operator==(const StepperMotor& o) const {
      return pins_[0] == o.pins_[0] && pins_[1] == o.pins_[1] &&
             pins_[2] == o.pins_[2] && pins_[3] == o.pins_[3];
    }

    int GetPrincipalDirection() const {
      return principal_direction_;
    }

    void Rotate(float degrees) {
      float dir_degrees{ degrees * principal_direction_ };
      Step(GetSteps(dir_degrees));
      Serial.println("Rotating current primary servo by " + String(dir_degrees));
    }

    bool IsConnected() {
      SetPinMode(INPUT);

      for (int i = 0; i < 4; i++) {
        delay(10);
        int state = digitalRead(pins_[i]);
        if (state != 0) {
          SetPinMode(OUTPUT);
          return false;          
        }
      }

      SetPinMode(OUTPUT);
      return true;
    }

    void DigitalWrite(int value) {
      for (int i = 0; i < 4; i++) {
        digitalWrite(pins_[i], value);
      } 
    }
private:
    void Step(int steps) {
        int factor = abs(steps) / steps;
        steps = abs(steps);

        for (int s = 0;  s <= steps / 8; s++) {
            for (int p = 0; p < 8 && p < steps - s * 8; p++) {
                delay(1);
                for (int i = 0; i < 4; i++) {
                  digitalWrite(pins_[i], sequence_[(int)(3.5 * (1 - factor) + factor * p)][i]);
                }
            } 
        }
        DigitalWrite(LOW);
    }

    int GetSteps(float degrees) {
      return (int)(degrees * step_per_deg_);
    }

private:
    // The sequence that must be used to acheive rotation.
    // The rows correspond to each step, and the columns correspond to each input.
    const bool sequence_[8][4] = {
      {  LOW,  LOW,  LOW, HIGH }, {  LOW,  LOW, HIGH, HIGH },
      {  LOW,  LOW, HIGH,  LOW }, {  LOW, HIGH, HIGH,  LOW },
      {  LOW, HIGH,  LOW,  LOW }, { HIGH, HIGH,  LOW,  LOW },
      { HIGH,  LOW,  LOW,  LOW }, { HIGH,  LOW,  LOW, HIGH }
    };

    StepperPins pins_; // Input pin numbers.
    const int principal_direction_{ 0 }; // Flipped direction for servos as each face each other. 
    const float step_per_deg_; // Conversion factor between degrees and steps.
};
