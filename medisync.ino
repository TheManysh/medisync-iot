#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <AccelStepper.h>

// Define stepper motor connections:
#define STEPPER_PULSE_PIN 14
#define STEPPER_DIR_PIN 12

// Adjust this address if necessary
LiquidCrystal_I2C lcd(0x27, 16, 2); // Change 0x27 to the address found using the I2C scanner
// Init stepper
AccelStepper stepper(AccelStepper::DRIVER, STEPPER_PULSE_PIN, STEPPER_DIR_PIN);
static long position = 0;

// Variables to keep track of time
unsigned long previousMillis = 0;
const long interval = 1000;
int seconds = 0;
int minutes = 0;
int hours = 0;

// rotation count
int angle = 0;
const int angleIncrement = 133;

void setup() {
  // Start the serial communication
  Serial.begin(115200);

  // Initialize the LCD
  lcd.init(); // Use lcd.init() instead of lcd.begin()
  lcd.backlight();

  // Initial display
  lcd.setCursor(0, 0);
  lcd.print("Time:  00:00:00");
  
  // set maximum speed and acceleration
  stepper.setMaxSpeed(2000); // Steps per second
  stepper.setAcceleration(8000); // Steps per second^2
  pinMode(STEPPER_PULSE_PIN, OUTPUT);
  pinMode(STEPPER_DIR_PIN, OUTPUT);

}

void loop() {
  // Check if a second has passed
  unsigned long currentMillis = millis();
  if (currentMillis - previousMillis >= interval) {
    previousMillis = currentMillis;
    seconds++;
    if (seconds >= 60) {
      seconds = 0;
      minutes++;
      if (minutes >= 60) {
        minutes = 0;
        hours++;
      }
    }
    
    // Update the time on the LCD
    lcd.setCursor(7, 0); // Move cursor to overwrite previous time
    if (hours < 10) lcd.print('0');
    lcd.print(hours);
    lcd.print(':');
    if (minutes < 10) lcd.print('0');
    lcd.print(minutes);
    lcd.print(':');
    if (seconds < 10) lcd.print('0');
    lcd.print(seconds);
  }
  
  // You can add additional logic to update the content on the second row here
  lcd.setCursor(0,1);
  lcd.print("Status: MISSED");


  // stepper control
  if( angle > 800) {
    angle = 0;
  }

  int angleMovement = angle + angleIncrement;
  Serial.println(angleMovement);
  stepper.move(angleMovement);
  stepper.runToPosition();
  delay(1000);
}
