#include <EEPROM.h>
#include <Adafruit_NeoPixel.h>
#include <ezButton.h>
#include <Arduino.h>
#include <WiFi.h>
#include <Firebase_ESP_Client.h>
#include "addons/TokenHelper.h"
#include "addons/RTDBHelper.h"

// EEPROM Configuration
#define EEPROM_SIZE 1 
#define EEPROM_CYCLE_COUNT_ADDRESS 0

// Device Identifier
#define DEVICE_ID "device_1"

// Network Credentials
#define WIFI_SSID "YourSSID"
#define WIFI_PASSWORD "YourPassword"

// Firebase Configuration
#define API_KEY "YourAPIKey"
#define DATABASE_URL "https://your-firebase-project.firebaseio.com/"

// Firebase Data and Authentication Objects
FirebaseData fbdo;
FirebaseAuth auth;
FirebaseConfig config;
bool signupOK = false;

// Joystick and LED Pins
const int JOY_VRX = 33;
const int JOY_VRY = 34;
const int JOY_CLICK = 17;
const int LED_PIN = 16;
const int LED_CHARGING_INDICATOR = 35;

// Button Setup
ezButton button(JOY_CLICK);

// LED Colors
int currentStartRed = 0;
int currentStartGreen = 100;
int currentStartBlue = 255;

int currentEndRed = 0;
int currentEndGreen = 255;
int currentEndBlue = 100;

// LED Strip Configuration
const int NUM_LEDS = 32;
Adafruit_NeoPixel pixels(NUM_LEDS, LED_PIN, NEO_GRBW + NEO_KHZ800);

// Cycle Tracking Variables
int cycle_state = 1;
int cycle_count = 0;
int weekly_target = 0;
int mapped_x = 0;
int mapped_y = 0;
bool target_mode = false;
bool charging_mode = false;

void setup() {
  Serial.begin(115200);

  // Joystick and LED Pin Configurations
  pinMode(JOY_VRX, INPUT);
  pinMode(JOY_VRY, INPUT);
  pinMode(LED_CHARGING_INDICATOR, INPUT);
  pinMode(JOY_CLICK, INPUT_PULLUP);

  // Initialize LED strip
  pixels.begin();
  pixels.setBrightness(255);
  pixels.clear();
  pixels.show();

  // Initialize EEPROM
  EEPROM.begin(EEPROM_SIZE);
  cycle_count = EEPROM.read(EEPROM_CYCLE_COUNT_ADDRESS);

  // Setup Wi-Fi and Firebase
  WIFI_SETUP();
}

void loop() {
  // Handle button input
  button.loop();

  // Read sensor values
  int led_charging_indicator = analogRead(LED_CHARGING_INDICATOR);
  int joy_vrx = analogRead(JOY_VRX);
  int joy_vry = analogRead(JOY_VRY);
  int joy_click = button.getState();

  // Activate target mode if conditions are met
  if (charging_mode && !target_mode && joy_click == 0) {
    Serial.println("Target Mode on");
    target_mode = true;
    currentStartRed = 51;
    currentStartGreen = 0;
    currentStartBlue = 200;
    currentEndRed = 51;
    currentEndGreen = 0;
    currentEndBlue = 200;
    start_target_mode_visual();
  }

  // Store previous joystick positions
  int prev_x = mapped_x;
  int prev_y = mapped_y;

  // Map joystick values to a grid
  mapped_x = map(joy_vrx, 0, 4950, 0, 8);
  mapped_y = map(joy_vry, 0, 4950, 0, 8);

  // Check if charging mode is active
  if (led_charging_indicator > 100) {
    if (!charging_mode) {
      charging_mode = true;
      Serial.println("Charging Mode on");
      new_day();
    }
  } else {
    // Deactivate modes and reset LED colors
    target_mode = false;
    charging_mode = false;
    currentStartRed = 0;
    currentStartGreen = 100;
    currentStartBlue = 255;
    currentEndRed = 0;
    currentEndGreen = 255;
    currentEndBlue = 100;
  }

  // Update the target value in Firebase when joystick is clicked
  if (target_mode && joy_click == 0 && weekly_target > 0) {
    Serial.println("Updating the target value now!");
    Serial.println(weekly_target);
    Serial.println("------------------");
    upload_fire_base_target_mode();
    target_mode = false;
    // Reset LED colors and clear the display
    currentStartRed = 0;
    currentStartGreen = 100;
    currentStartBlue = 255;
    currentEndRed = 0;
    currentEndGreen = 255;
    currentEndBlue = 100;
    pixels.clear();
    pixels.show();
  }

  // Check joystick position and update LED strip accordingly
  if (mapped_x == 0 && mapped_y == 2 && cycle_state != 24) {
    cycle_state = 1;
    for (int i = 0; i < cycle_state; i++) {
      pixels.setPixelColor(i, pixels.Color(currentStartRed, currentStartGreen, currentStartBlue, 0));
    }
    pixels.show();
  }

  // Check for joystick movement and update cycle state
  if (mapped_x != prev_x || mapped_y != prev_y) {
    if ((((1 <= cycle_state && cycle_state <= 2) || (21 <= cycle_state && cycle_state <= 24)) && mapped_x == prev_x && mapped_y == prev_y - 1)
        || ((3 <= cycle_state && cycle_state <= 8) && mapped_x == prev_x + 1 && mapped_y == prev_y)
        || ((9 <= cycle_state && cycle_state <= 14) && mapped_x == prev_x && mapped_y == prev_y + 1)
        || ((15 <= cycle_state && cycle_state <= 20) && mapped_x == prev_x - 1 && mapped_y == prev_y)) {
      // Increase cycle state and update LED strip
      cycle_state++;
      pixels.clear();
      for (int i = 0; i < cycle_state; i++) {
        pixels.setPixelColor(i, pixels.Color(currentStartRed, currentStartGreen, currentStartBlue, 0));
      }
      pixels.show();
    } else if ((((1 <= cycle_state && cycle_state <= 3) || (22 <= cycle_state && cycle_state <= 24)) && mapped_x == prev_x && mapped_y == prev_y + 1)
               || ((4 <= cycle_state && cycle_state <= 9) && mapped_x == prev_x - 1 && mapped_y == prev_y)
               || ((10 <= cycle_state && cycle_state <= 15) && mapped_x == prev_x && mapped_y == prev_y - 1)
               || ((16 <= cycle_state && cycle_state <= 21) && mapped_x == prev_x + 1 && mapped_y == prev_y)) {
      // Decrease cycle state and update LED strip
      cycle_state--;
      pixels.clear();
      for (int i = 0; i < cycle_state; i++) {
        pixels.setPixelColor(i, pixels.Color(currentStartRed, currentStartGreen, currentStartBlue, 0));
      }
      pixels.show();
    } else {
      // Reset cycle state and clear LED strip
      cycle_state = 0;
      pixels.clear();
      pixels.show();
      if (target_mode) {
        // Display target mode LEDs
        for (int i = 0; i < weekly_target; i++) {
          pixels.setPixelColor(i, pixels.Color(currentEndRed, currentEndGreen, currentEndBlue, 0));
          pixels.show();
        }
      }
    }
  }

  // Check if a full cycle is completed
  if (cycle_state == 25) {
    cycle_state = 0;
    // Display target mode LEDs
    for (int i = 0; i < NUM_LEDS; i++) {
      pixels.setPixelColor(i, pixels.Color(currentEndRed, currentEndGreen, currentEndBlue, 0));
    }
    pixels.show();
    if (target_mode) {
      // Increment target value and display LEDs
      Serial.print("target_mode++ :");
      weekly_target++;
      Serial.println(weekly_target);
      pixels.clear();
      current_target_visual();
    } else {
      // Increment cycle count, update EEPROM, and clear LEDs
      cycle_count++;
      Serial.print("cycle_count++ :");
      Serial.println(cycle_count);
      EEPROM.write(EEPROM_CYCLE_COUNT_ADDRESS, cycle_count);
      EEPROM.commit();
      //upload_fire_base_cycle_count();
      delay(1500);
      pixels.clear();
    }
  }
}

void new_day() {
  // Upload the cycle count to Firebase
  upload_fire_base_cycle_count();
  
  // Reset cycle count and update EEPROM
  cycle_count = 0;
  EEPROM.write(EEPROM_CYCLE_COUNT_ADDRESS, 0);
  EEPROM.commit();
}

void current_target_visual() {
  // Display the current target value on the serial monitor
  Serial.print("CURRENT TARGET:");
  Serial.println(weekly_target);

  // Display LEDs based on the current target value
  for (int i = 0; i < weekly_target; i++) {
    pixels.setPixelColor(i, pixels.Color(currentEndRed, currentEndGreen, currentEndBlue, 0));
    pixels.show();
  }

  // Flash the last LED for visual effect if the target value is greater than 0
  if (weekly_target > 0) {
    for (int i = 0; i < 5; i++) {
      pixels.setPixelColor(weekly_target - 1, pixels.Color(0, 0, 0, 0));
      pixels.show();
      delay(200);
      pixels.setPixelColor(weekly_target - 1, pixels.Color(currentEndRed, currentEndGreen, currentEndBlue, 0));
      pixels.show();
      delay(200);
    }
  }
}

void start_target_mode_visual() {
  // Display a visual effect when entering target mode
  for (int i = 0; i < NUM_LEDS; i++) {
    pixels.setPixelColor(i, pixels.Color(currentStartRed, currentStartGreen, currentStartBlue, 0));
    pixels.show();
    delay(75);
  }
  
  // Clear the LEDs and wait for a short duration
  pixels.clear();
  pixels.show();
  delay(100);

  // Display the initial state of LEDs
  for (int i = 0; i < NUM_LEDS; i++) {
    pixels.setPixelColor(i, pixels.Color(currentStartRed, currentStartGreen, currentStartBlue, 0));
  }
  pixels.show();

  // Wait for a short duration and clear the LEDs
  delay(100);
  pixels.clear();
  pixels.show();
}

void exit_target_mode_visual() {
  // Display a visual effect when exiting target mode
  for (int i = NUM_LEDS - 1; i >= 0; i--) {
    pixels.setPixelColor(i, pixels.Color(currentStartRed, currentStartGreen, currentStartBlue, 0));
    pixels.show();
    delay(75);
  }

  // Clear the LEDs and wait for a short duration
  pixels.clear();
  pixels.show();
  delay(100);

  // Display the initial state of LEDs
  for (int i = 0; i < NUM_LEDS; i++) {
    pixels.setPixelColor(i, pixels.Color(currentStartRed, currentStartGreen, currentStartBlue, 0));
  }
  pixels.show();

  // Wait for a short duration and clear the LEDs
  delay(100);
  pixels.clear();
  pixels.show();
}


// Function to upload cycle count to Firebase
void upload_fire_base_cycle_count() {
  int cycle_count = EEPROM.read(EEPROM_CYCLE_COUNT_ADDRESS);
  Serial.print("EEPROM reports cycle_count as:");
  Serial.println(cycle_count);

  if (Firebase.ready() && signupOK) {
    setInt("number_of_cycles_today", cycle_count);
    setInt("today_number_updated", 1);
  } else {
    upload_fire_base_cycle_count(); // Retry on failure
  }
}

// Function to upload target mode data to Firebase
void upload_fire_base_target_mode() {
  if (Firebase.ready() && signupOK) {
    setInt("weekly_goal", weekly_target);
    setInt("weekly_goal_changed", 1);
    weekly_target = 0;
  } else {
    upload_fire_base_target_mode(); // Retry on failure
  }
}

// Function to set integer data in Firebase
void setInt(String field, int value) {
  String path = String("/") + DEVICE_ID + "/" + field;

  if (Firebase.RTDB.setInt(&fbdo, path.c_str(), value)) {
    Serial.println("Set succeeded");
    Serial.println("Path: " + fbdo.dataPath());
    Serial.println("Type: " + fbdo.dataType());
  } else {
    Serial.println("Set failed");
    Serial.println("Reason: " + fbdo.errorReason());
  }
}

// Function to get integer data from Firebase
int getInt(String field) {
  String path = String("/") + DEVICE_ID + "/" + field;

  if (Firebase.RTDB.getInt(&fbdo, path.c_str())) {
    if (fbdo.dataType() == "int") {
      int current_number = fbdo.intData();
      Serial.println(current_number);
      return current_number;
    }
  } else {
    Serial.println(fbdo.errorReason());
  }

  return NULL;
}

// Function to set up Wi-Fi and Firebase
void WIFI_SETUP() {
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  Serial.print("Connecting to Wi-Fi");

  while (WiFi.status() != WL_CONNECTED) {
    Serial.print(".");
    delay(300);
  }

  Serial.println();
  Serial.print("Connected with IP: ");
  Serial.println(WiFi.localIP());
  Serial.println();

  // Firebase Configuration
  config.api_key = API_KEY;
  config.database_url = DATABASE_URL;

  // Sign up to Firebase
  if (Firebase.signUp(&config, &auth, "", "")) {
    Serial.println("Sign up successful");
    signupOK = true;
  } else {
    Serial.printf("Sign up failed: %s\n", config.signer.signupError.message.c_str());
  }

  // Set the callback function for token status
  config.token_status_callback = tokenStatusCallback; // See addons/TokenHelper.h

  // Begin Firebase
  Firebase.begin(&config, &auth);
  Firebase.reconnectWiFi(true);
}
