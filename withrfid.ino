#include <Servo.h>
#include <SPI.h>
#include <MFRC522.h>

// Define pin connections for Servo, Buzzer, IR Sensors, and RFID
#define SERVO_PIN 5
#define BUZZER_PIN 8
#define IR_ENTRY_PIN 2
#define IR_EXIT_PIN 3
#define SS_PIN 10
#define RST_PIN 9

// Servo object
Servo servo;

// RFID object
MFRC522 rfid(SS_PIN, RST_PIN);

// Variables
int carCount = 0;
const int maxCars = 4;                   // Maximum capacity of the garage
bool isAuthorized = false;               // To track if the UID check was successful
const String authorizedUID = "232975D";  // Authorized UID in hexadecimal string format

void setup() {
  Serial.begin(9600);
  SPI.begin();
  rfid.PCD_Init();
  servo.attach(SERVO_PIN);
  pinMode(BUZZER_PIN, OUTPUT);
  pinMode(IR_ENTRY_PIN, INPUT);
  pinMode(IR_EXIT_PIN, INPUT);

  // Initial state
  servo.write(0);  // Servo closed position
  displayWelcomeMessage();
}

void loop() {
  // Check for RFID card presence
  if (rfid.PICC_IsNewCardPresent() && rfid.PICC_ReadCardSerial()) {
    String inputUID = "";
    for (byte i = 0; i < rfid.uid.size; i++) {
      inputUID += String(rfid.uid.uidByte[i], HEX);
    }
    inputUID.toUpperCase();  // Convert UID to uppercase
    Serial.print("Received UID: ");
    Serial.println(inputUID);  // Debug: Print the received UID

    if (checkUID(inputUID)) {
      if (carCount < maxCars) {
        Serial.println("Access Granted. Opening gate...");
        Serial.println("Please enter.");
        isAuthorized = true;  // Allow entry IR sensor to work
      } else {
        Serial.println("Garage is full. Access denied.");
        soundBuzzer();
        isAuthorized = false;  // Prevent entry IR sensor from working
        displayWelcomeMessage();
      }
    } else {
      Serial.println("UID not recognized. Access denied.");
      soundBuzzer();
      isAuthorized = false;  // Prevent entry IR sensor from working
      displayWelcomeMessage();
    }
    delay(2000);  // Pause for the user to see the message
  }

  // Check for car entry only if UID was validated
  if (isAuthorized && digitalRead(IR_ENTRY_PIN) == LOW) {
    delay(500);  // Debounce
    if (carCount < maxCars) {
      while (digitalRead(IR_ENTRY_PIN) == LOW) {
        openGate();  // Open the gate when a car is detected
      }
      delay(3000);
      servo.write(0);  // Close gate
      carCount++;
      Serial.print("Car entered. Total cars: ");
      Serial.println(carCount);
      isAuthorized = false;  // Reset authorization after entry
      delay(1000);           // Avoid multiple counts
      displayWelcomeMessage();
    } else {
      Serial.println("Garage is full. No entry allowed.");
    }
  }

  // Check for car exit
  if (digitalRead(IR_EXIT_PIN) == LOW) {
    delay(500);  // Debounce
    if (carCount > 0) {
      while (digitalRead(IR_EXIT_PIN) == LOW) {
        openGateForExit();
      }
      delay(3000);     // Keep gate open for 3 seconds
      servo.write(0);  // Close gate
      carCount--;
      Serial.print("Car exited. Total cars: ");
      Serial.println(carCount);
      delay(1000);  // Avoid multiple counts
      displayWelcomeMessage();
    }
  }
}

void displayWelcomeMessage() {
  Serial.print("Welcome to our garage. Cars in garage: ");
  Serial.println(carCount);
  Serial.println("Please scan your RFID tag:");
}

bool checkUID(String inputUID) {
  // Convert the input UID to uppercase for consistent comparison
  inputUID.toUpperCase();

  // Debug: Print the authorized UID for comparison
  Serial.print("Authorized UID: ");
  Serial.println(authorizedUID);

  // Check if the input UID matches the authorized UID
  if (inputUID.equals(authorizedUID)) {
    return true;  // Authorized
  }
  return false;  // Unauthorized
}

void openGate() {
  servo.write(90);  // Open gate
}

void openGateForExit() {
  servo.write(90);  // Open gate for exit
}

void soundBuzzer() {
  digitalWrite(BUZZER_PIN, HIGH);
  delay(1000);  // Buzzer on for 1 second
  digitalWrite(BUZZER_PIN, LOW);
}