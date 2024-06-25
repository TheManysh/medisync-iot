#include <Wire.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include <Arduino.h>
#include <Adafruit_MPU6050.h>
#include <Adafruit_Sensor.h>
#include <WiFiManager.h>
#define BUZZER_PIN 4 

float values[6];
// Global objects
Adafruit_MPU6050 mpu;
HTTPClient http;
String url = "http://192.168.43.190:8000/api/v1/notifications/12345";
void setup() {
    Serial.begin(115200);
    while (!Serial)
        delay(10); // will pause Zero, Leonardo, etc until serial console opens

    Serial.println("MPU6050 test!");
    WiFiManager wm;
    wm.resetSettings();
    bool res;
    res = wm.autoConnect("AutoConnectAP", "password");

    // Try to initialize!
    if (!mpu.begin()) {
        Serial.println("Failed to find MPU6050 chip");
        while (1) {
            delay(10);
        }
    }
    Serial.println("MPU6050 Found!");

    // Setup motion detection
    mpu.setHighPassFilter(MPU6050_HIGHPASS_0_63_HZ);
    mpu.setMotionDetectionThreshold(1);
    mpu.setMotionDetectionDuration(20);
    mpu.setInterruptPinLatch(true); // Keep it latched. Will turn off when reinitialized.
    mpu.setInterruptPinPolarity(true);
    mpu.setMotionInterrupt(true);

    while (!res) {
        Serial.println("Connecting to WiFi...");
        Serial.print(".");
    }

    Serial.println("Connected to WiFi");
}

void loop() {
    if (mpu.getMotionInterruptStatus()) {
        /* Get new sensor events with the readings */
        sensors_event_t a, g, temp;
        mpu.getEvent(&a, &g, &temp);

        /* Print out the values */
        Serial.print("AccelX:");
        Serial.print(a.acceleration.x);
        Serial.print(",");
        Serial.print("AccelY:");
        Serial.print(a.acceleration.y);
        Serial.print(",");
        Serial.print("AccelZ:");
        Serial.print(a.acceleration.z);
        Serial.print(", ");
        Serial.print("GyroX:");
        Serial.print(g.gyro.x);
        Serial.print(",");
        Serial.print("GyroY:");
        Serial.print(g.gyro.y);
        Serial.print(",");
        Serial.print("GyroZ:");
        Serial.print(g.gyro.z);
        Serial.println(" ");

        values[0] = a.acceleration.x;
        values[1] = a.acceleration.y;
        values[2] = a.acceleration.z;
        values[3] = g.gyro.x;
        values[4] = g.gyro.y;
        values[5] = g.gyro.z;

        // Fall detection thresholds
        const float acc_threshold = 6.5; // Corresponds to approximately 1.5g
        const float gyro_threshold = 0.7; // 300 degrees/second

          digitalWrite(BUZZER_PIN, HIGH);
        if (abs(values[0]) > acc_threshold || abs(values[1]) > acc_threshold || abs(values[2]) > acc_threshold ||
            abs(values[3]) > gyro_threshold || abs(values[4]) > gyro_threshold || abs(values[5]) > gyro_threshold) {
            sendFallDatatoServer("{\"type\" : \"fall\"}");
           
        }
    }
}

void sendFallDatatoServer(String payload) {
      if (WiFi.status() == WL_CONNECTED) {
          http.begin(url);
          http.addHeader("Content-Type", "application/json");

          int httpResponseCode = http.POST(payload);
          if (httpResponseCode >  200) {
              Serial.println("Data sent successfully");
          } else {
              Serial.println(httpResponseCode);
              Serial.println("No data sent");
          }

          http.end();
      } else {
          Serial.println("Error: Cannot send data without a WiFi connection.");
      }
  }
