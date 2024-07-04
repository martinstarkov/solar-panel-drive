#include "types.h"

#include <string.h>

// Parsing user inputs

const byte buffer_limit = 32;
char reception_buffer[buffer_limit];
char start_marker = '<';
char end_marker = '>';

bool new_data_available = false;

static void CheckForData() {
    static bool receiving = false;
    static byte ndx = 0;
    char rc;
 
    while (Serial.available() > 0 && !new_data_available) {
        rc = Serial.read();

        if (receiving) {
            if (rc != end_marker) {
                if (rc != '\n') {
                  reception_buffer[ndx] = rc;
                  ndx++;
                  if (ndx >= buffer_limit) {
                      ndx = buffer_limit - 1;
                  }
                }
            } else {
                reception_buffer[ndx] = '\0'; // terminate the string
                receiving = false;
                ndx = 0;
                new_data_available = true;
            }
        } else if (rc == start_marker) {
            receiving = true;
        }
    }
}

// Stepper and servo initialization

const float step_per_deg = 64.0f / 5.625f;

// This creates an instance of the StepperMotor class and specifies the 4 pins the motor controller is connected to.
// You can create more of these to control more than just one motor.
StepperMotor first_stepper({ 4, 5, 6, 7 }, 1, step_per_deg);
StepperMotor second_stepper({ 8, 9, 10, 11 }, -1, step_per_deg);
ServoMotor switch_servo(3, 1000, 1450, 1900, 400);
StepperMotor* primary_stepper{ nullptr };
float rotate_amount = 0.0f;
Command command{ Command::Invalid };

// Parsing user inputs

Command ParseInput(char* input) { 
  if (String(input).equals("switch")) {
    Serial.println("Received manual switch command");
    return Command::ManualSwitch;
  }
  rotate_amount = atof(input);
  if (rotate_amount != 0) {
    Serial.println("Received rotate command: " + String(rotate_amount) + " deg");
    return Command::Rotation;
  }
  Serial.println("Invalid user input: " + String(input));
  return Command::Invalid;
}

// Arduino functions

void setup() {
  primary_stepper = &first_stepper;
  Serial.begin(9600);
  delay(1000);
  switch_servo.Init();
  Serial.println("Initialized program");
}

void loop() {
  static float elapsed_degrees{ 0.0f };

  CheckForData();

  if (new_data_available) {
      command = ParseInput(reception_buffer);
      new_data_available = false;
  }

  if (command == Command::Rotation) {
    primary_stepper->Rotate(rotate_amount);
    elapsed_degrees += rotate_amount;
  }

  bool primary = primary_stepper->IsConnected();
  
  if (command == Command::ManualSwitch || !primary) {
    primary_stepper->DigitalWrite(LOW);
    primary_stepper = *primary_stepper == second_stepper ? &first_stepper : &second_stepper;

    bool new_primary = primary_stepper->IsConnected();

    if (new_primary) {
      Serial.println("Synchronizing new primary servo...");
      primary_stepper->Rotate(elapsed_degrees);
      elapsed_degrees = 0;

      switch_servo.Switch(-primary_stepper->GetPrincipalDirection());
    } else {
      Serial.println("CRITICAL WARNING: No valid steppers connected");
    }
  }

  command = Command::Invalid;
}