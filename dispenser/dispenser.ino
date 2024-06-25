#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <AccelStepper.h>
#include <SPI.h>
#include <MFRC522.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <WiFi.h>
#include <WiFiManager.h>
#include <Arduino.h>



// Define stepper motor connections
#define STEPPER_PULSE_PIN 14
#define STEPPER_DIR_PIN 12
// RFID setup
#define SS_PIN 2
#define RST_PIN 4


bool tag_scanned = false;
String rfid_miss = "{\"type\" : \"missed\"}";

//acessing time from server for syncing purposes or something
int SERVER_HOUR = 0;
int SERVER_MINUTE = 0;

const char* WIFI_SSID = "Vagabond_2.4";
const char* WIFI_PASSWORD = "Overkill";
HTTPClient http;
String tag_payload = "12345";

String url = "http://192.168.1.72:8000/api/v1/";


bool medicine_missed = false;
//plus the tagID,
// LCD setup
LiquidCrystal_I2C lcd(0x27, 16, 2);  // Change 0x27 to the address found using the I2C scanner

// Stepper setup
AccelStepper stepper(AccelStepper::DRIVER, STEPPER_PULSE_PIN, STEPPER_DIR_PIN);
static long position = 0;

// Timekeeping variables
unsigned long previousMillis = 0;
const long interval = 1000;
int seconds = 0;
int minutes = 0;
int hours = 0;


// Stepper movement variables
int angle = 0;
const int angleIncrement = 133;


MFRC522 rfid(SS_PIN, RST_PIN);
MFRC522::MIFARE_Key key;
byte nuidPICC[4];

void setup() {
  // Start serial communication
  Serial.begin(115200);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

  // create a hotspot using wifi manager
  // WiFiManager wm;
  // wm.resetSettings();
  // bool res;
  // res = wm.autoConnect("AutoConnectAP","password");

  while (WiFi.status() != WL_CONNECTED) {
    Serial.print(".");
  }
  Serial.println("Connected to WiFi");
  Serial.println(WiFi.localIP());


  // Initialize LCD
  lcd.init();
  lcd.backlight();
  lcd.setCursor(0, 0);
  lcd.print("Time:  00:00:00");

  // Set maximum speed and acceleration for the stepper motor
  stepper.setMaxSpeed(2000);      // Steps per second
  stepper.setAcceleration(8000);  // Steps per second^2
  pinMode(STEPPER_PULSE_PIN, OUTPUT);
  pinMode(STEPPER_DIR_PIN, OUTPUT);

  // Initialize SPI and RFID reader
  SPI.begin();
  rfid.PCD_Init();
  for (byte i = 0; i < 6; i++) {
    key.keyByte[i] = 0xFF;
  }
  Serial.println(F("RFID Reader Initialized"));
  Serial.print(F("Using the following key:"));
  printHex(key.keyByte, MFRC522::MF_KEY_SIZE);
  Serial.println();
}

void loop() {
  
  updateLCDTime(hours, minutes, seconds);
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
  }

  if (minutes == 5 || seconds == 10 || seconds == 45) {
    checkReminder();
    delay(1000);
  }

  //  here if the function called and reminder missed then




  // Stepper control




  // RFID reader logic
  if (!rfid.PICC_IsNewCardPresent())
    return;
  if (!rfid.PICC_ReadCardSerial())
    return;

  Serial.print(F("PICC type: "));
  MFRC522::PICC_Type piccType = rfid.PICC_GetType(rfid.uid.sak);
  Serial.println(rfid.PICC_GetTypeName(piccType));
  if (piccType != MFRC522::PICC_TYPE_MIFARE_MINI && piccType != MFRC522::PICC_TYPE_MIFARE_1K && piccType != MFRC522::PICC_TYPE_MIFARE_4K) {
    Serial.println(F("Your tag is not of type MIFARE Classic."));
    return;
  }

  if (isNewCardDetected(rfid, nuidPICC)) {
    rfid_id_to_server();

    Serial.println(F("A new card has been detected."));

    for (byte i = 0; i < 4; i++) {
      nuidPICC[i] = rfid.uid.uidByte[i];
    }
    Serial.println(F("The NUID tag is:"));
    Serial.print(F("In hex: "));
    printHex(rfid.uid.uidByte, rfid.uid.size);
    Serial.println();
    Serial.print(F("In dec: "));
    printDec(rfid.uid.uidByte, rfid.uid.size);
    Serial.println();
  } else {
    check_rfid_tag();

    Serial.println(F("Card read previously."));
  }

  rfid.PICC_HaltA();
  rfid.PCD_StopCrypto1();
}

void printHex(byte* buffer, byte bufferSize) {
  for (byte i = 0; i < bufferSize; i++) {
    Serial.print(buffer[i] < 0x10 ? " 0" : " ");
    Serial.print(buffer[i], HEX);
  }
}

void printDec(byte* buffer, byte bufferSize) {
  for (byte i = 0; i < bufferSize; i++) {
    Serial.print(buffer[i] < 0x10 ? " 0" : " ");
    Serial.print(buffer[i], DEC);
  }
}

void rfid_id_to_server() {
  if (angle > 800) {
    angle = 0;
  }
  int angleMovement = angle + angleIncrement;

  stepper.move(angleMovement);
  stepper.runToPosition();
  delay(1000);

  medicine_missed = false;

  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Time:");
  updateLCDTime(hours, minutes, seconds);
  lcd.setCursor(0, 1);
  lcd.print("Status: Good");
  tag_scanned = false;
  String payload = "{ \"uid\": \"";
  for (byte i = 0; i < 4; i++) {
    if (nuidPICC[i] < 0x10) payload += "0";
    payload += String(nuidPICC[i], HEX);
  }
  payload += "\" }";

  if (WiFi.status() == WL_CONNECTED) {
    //String send_tag_url = url +  ""; // Adjust the endpoint as per your server implementation

    //http.begin(send_tag_url);
    //http.addHeader("Content-Type", "application/json");

    //int httpResponseCode = http.POST(payload);
    int httpResponseCode = 200;


    if (httpResponseCode == 200 || true) {
      int uidStart = payload.indexOf("\"uid\": \"") + 7;  // Find the start of the ID value
      int uidEnd = payload.indexOf("\"", uidStart);       // Find the end of the ID value
      tag_payload = payload.substring(uidStart, uidEnd);
      tag_scanned = true;
      Serial.println("RFID data sent successfully");

    } else {
      Serial.print("Error sending RFID data. HTTP response code: ");
      Serial.println(httpResponseCode);
    }

    // http.end();
  } else {
    Serial.println("Error: Cannot send RFID data without a WiFi connection.");
  }
}

bool check_rfid_tag() {

  if (angle > 800) {
    angle = 0;
  }
  int angleMovement = angle + angleIncrement;

  stepper.move(angleMovement);
  stepper.runToPosition();
  delay(1000);

  medicine_missed = false;
  tag_scanned = false;
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Time:");
  lcd.setCursor(0, 1);
  lcd.print("Status: Good");

  return true;

  //   tag_payload = "{ \"uid\": \"";
  //   String check_tag_url = url + "";
  //   for (byte i = 0; i < 4; i++) {
  //     if (nuidPICC[i] < 0x10) tag_payload += "0";
  //     tag_payload += String(nuidPICC[i], HEX);
  //   }
  //   tag_payload += "\" }";


  //   if (WiFi.status() == WL_CONNECTED) {
  //     // Adjust the endpoint as per your server implementation

  //     http.begin(check_tag_url);
  //     http.addHeader("Content-Type", "application/json");

  //     int httpResponseCode = http.POST(tag_payload);

  //     if (httpResponseCode == 200 ) {
  //       tag_scanned = true;
  //       String response = http.getString();
  //       Serial.print("Server response: ");
  //       Serial.println(response);
  //       if (response == "true") {
  //         return true; // Tag recognized by server
  //       } else {
  //         return false; // Tag not recognized by server
  //       }
  //     } else {
  //       Serial.print("Error checking RFID tag. HTTP response code: ");
  //       Serial.println(httpResponseCode);
  //     }

  //     http.end();
  //   } else {
  //     Serial.println("Error: Cannot check RFID tag without a WiFi connection.");
  //   }

  //   return false;
}

bool isNewCardDetected(MFRC522& rfid, byte* nuidPICC) {
  // Compare current tag UID with previous one
  if (rfid.uid.uidByte[0] != nuidPICC[0] || rfid.uid.uidByte[1] != nuidPICC[1] || rfid.uid.uidByte[2] != nuidPICC[2] || rfid.uid.uidByte[3] != nuidPICC[3]) {
    return true;
  }

  return false;
}

void updateLCDTime(int h, int m, int s) {
  lcd.setCursor(7, 0);  // Move cursor to overwrite previous time
  if (h < 10) lcd.print('0');
  lcd.print(h);
  lcd.print(':');
  if (m < 10) lcd.print('0');
  lcd.print(m);
  lcd.print(':');
  if (s < 10) lcd.print('0');
  lcd.print(s);
}

void getServerTime() {
  //gets server time that is current which h
  //helps in syncing , and after that checks for any notifications that are missed here.
  String url_time = url + "current-time";

  if (WiFi.status() == WL_CONNECTED) {
    http.begin(url_time);
    int httpResponseCode = http.GET();


    if (httpResponseCode == 200 || true) {
      String response = http.getString();
      DynamicJsonDocument doc(256);
      deserializeJson(doc, response);

      const char* hoursStr = doc["data"]["hours"];
      const char* minutesStr = doc["data"]["minutes"];

      SERVER_HOUR = atoi(hoursStr);
      SERVER_MINUTE = atoi(minutesStr);


      Serial.println("Server Time Checked");
    } else {
      Serial.print("Error checking server time. HTTP code: ");
      Serial.println(httpResponseCode);
    }
  } else {
    Serial.println("Error: WiFi not connected.");
  }
}

void checkReminder() {
  getServerTime();  // Fetch current server time

  String url_reminders = url + "reminders/" + tag_payload;
  Serial.println(url_reminders);  // Replace with actual user ID

  if (WiFi.status() == WL_CONNECTED) {
    http.begin(url_reminders);

    int httpResponseCode = http.GET();

    if (httpResponseCode == 200) {
      String response = http.getString();
      DynamicJsonDocument doc(256);  //make dynamic later ok
      deserializeJson(doc, response);

      const char* hoursStr = doc["data"]["hour"];
      const char* minutesStr = doc["data"]["minute"];

      int reminderHour = atoi(hoursStr);
      int reminderMinute = atoi(minutesStr);

      // need to remove later just for testing now
      // Compare reminder time with current server time

      if (SERVER_HOUR > reminderHour || (SERVER_HOUR == reminderHour && SERVER_MINUTE > reminderMinute) || !tag_scanned) {
        medicine_missed = true;

        // if (!tag_scanned){

        sendNotification();
        tag_scanned = false;  // Send notification if late

        // } //also need to add another flag which checks that
        //tag_scanned = false;
      }
    } else {
      Serial.print("Error fetching reminders. HTTP code: ");
      Serial.println(httpResponseCode);
    }

    //http.end();
  } else {
    Serial.println("WiFi not connected.");
  }
}

void sendNotification() {
  Serial.println("Missed");
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Time: ");
  updateLCDTime(hours, minutes, seconds);

  lcd.setCursor(0, 1);
  lcd.print("Status : Missed");
  // Trigger buzzer


  if (WiFi.status() == WL_CONNECTED) {
    http.begin(url + "notifications/12345");
    http.addHeader("Content-Type", "application/json");

    int httpResponseCode = http.POST(rfid_miss);
    if (httpResponseCode > 200) {
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
