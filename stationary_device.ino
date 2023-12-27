#include <Adafruit_NeoPixel.h>
#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <Firebase_ESP_Client.h>
#include "addons/TokenHelper.h"
#include "addons/RTDBHelper.h"

// Device Configuration
#define DEVICE_ID "device_1"
const int LED_PIN = D3;
const int MAGNET_SENSOR = A0;
const int NUM_LEDS = 45;

Adafruit_NeoPixel pixels(NUM_LEDS, LED_PIN, NEO_GRB + NEO_KHZ800);
FirebaseData fbdo;
FirebaseAuth auth;
FirebaseConfig config;
bool signupOK = false;
bool hasShowed = false;

// WiFi Configuration
#define WIFI_SSID "YourSSID"
#define WIFI_PASSWORD "YourPassword"

// Firebase Configuration
#define API_KEY "YourAPIKey"
#define DATABASE_URL "https://your-firebase-project.firebaseio.com/"

// Function prototypes
void showProgress();
void newGoalLEDs();
int getInt(String field);
void setInt(String field, int value);
void WIFI_SETUP();
void WIFI_Connection_LEDS();

void setup() {
  Serial.begin(115200);

  pinMode(MAGNET_SENSOR, INPUT);

  // Initialize the LED strip
  pixels.begin();
  pixels.setBrightness(200);
  pixels.show();

  WIFI_SETUP();
  Serial.println("WiFi is connected");
}

void loop() {
  int magnetValue = analogRead(MAGNET_SENSOR);
  delay(100);

  // If magnet value is above a threshold and not yet showed progress
  if (magnetValue >= 620 && !hasShowed) {
    showProgress();
    hasShowed = true;
  } else if (magnetValue <= 570) {
    // If magnet value is below a threshold, clear LEDs
    pixels.clear();
    pixels.show();
    hasShowed = false;
  }

  int weekly_goal_changed = getInt("weekly_goal_changed");

  // If weekly goal changed, show new goal LEDs
  if (weekly_goal_changed > 0) {
    newGoalLEDs();
    setInt("number_of_cycles_today", 0);
    setInt("this_week_progress", 0);
    setInt("weekly_goal_changed", 0);
    hasShowed = false;
  }
}

void showProgress() {
  int current_prog = getInt("this_week_progress");
  int weekly_goal = getInt("weekly_goal");
  int current = 0;

  if (weekly_goal > 0) {
    current = (current_prog * NUM_LEDS) / weekly_goal;
  }

  // Show progress LEDs
  for (int i = 0; i < current; i++) {
    pixels.setPixelColor(i, 0, 255, 100);  // Green color
    pixels.show();
    delay(25);
  }

  // Show remaining LEDs
  for (int i = current; i < NUM_LEDS; i++) {
    pixels.setPixelColor(i, 255, 255, 255);  // White color
    pixels.show();
    delay(25);
  }

  delay(1000);

  int is_today_updated = getInt("today_number_updated");

  // Wait until today's data is updated
  while (is_today_updated == 0) {
    is_today_updated = getInt("today_number_updated");
  }

  int today_prog = getInt("number_of_cycles_today");
  int updated = 0;

  if (weekly_goal > 0) {
    updated = current + ((today_prog * NUM_LEDS) / weekly_goal);
  }

  // Show updated progress LEDs
  for (int i = current; i < updated; i++) {
    pixels.setPixelColor(i, 0, 255, 100);  // Green color
    pixels.show();
    delay(100);
  }

  setInt("today_number_updated", 0);
  setInt("number_of_cycles_today", 0);
  setInt("this_week_progress", current_prog + today_prog);
}

void newGoalLEDs() {
  // Show new goal LEDs
  for (int i = 0; i < NUM_LEDS; i++) {
    pixels.clear();
    pixels.setPixelColor(i, 51, 0, 200, 0);  // Purple color
    pixels.show();
    delay(30);
  }

  for (int i = 0; i < NUM_LEDS; i++) {
    pixels.setPixelColor(i, 51, 0, 200, 0);
    pixels.show();
  }

  delay(200);

  pixels.clear();
  pixels.show();
}

int getInt(String field) {
  String path = String("/") + DEVICE_ID + "/" + field;
  if (Firebase.RTDB.getInt(&fbdo, path.c_str())) {
    if (fbdo.dataType() == "int") {
      int current_number = fbdo.intData();
      // Uncomment the line below for debugging
      // Serial.println(current_number);
      return current_number;
    }
  } else {
    // Uncomment the line below for debugging
    // Serial.println(fbdo.errorReason());
  }
  return -100;
}

void setInt(String field, int value) {
  String path = String("/") + DEVICE_ID + "/" + field;
  if (Firebase.RTDB.setInt(&fbdo, path.c_str(), value)) {
    // Uncomment the lines below for debugging
    // Serial.println("PASSED");
    // Serial.println("PATH: " + fbdo.dataPath());
    // Serial.println("TYPE: " + fbdo.dataType());
  } else {
    // Uncomment the lines below for debugging
    // Serial.println("FAILED");
    // Serial.println("REASON: " + fbdo.errorReason());
  }
}

void WIFI_SETUP() {
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  Serial.print("Connecting to Wi-Fi");

  // Wait for Wi-Fi connection
  while (WiFi.status() != WL_CONNECTED) {
    Serial.print(".");
    delay(300);
  }

  Serial.println();
  Serial.print("Connected with IP: ");
  Serial.println(WiFi.localIP());
  Serial.println();

  // Firebase configuration
  config.api_key = API_KEY;
  config.database_url = DATABASE_URL;

  // Sign up to Firebase
  if (Firebase.signUp(&config, &auth, "", "")) {
    // Uncomment the line below for debugging
    // Serial.println("Authentication successful");
    signupOK = true;
    WIFI_Connection_LEDS();
  } else {
    // Uncomment the line below for debugging
    // Serial.printf("%s\n", config.signer.signupError.message.c_str());
  }

  // Assign the callback function for the long-running token generation task
  config.token_status_callback = tokenStatusCallback;  // see addons/TokenHelper.h

  // Begin Firebase
  Firebase.begin(&config, &auth);
  Firebase.reconnectWiFi(true);
}

void WIFI_Connection_LEDS() {
  // Show LEDs indicating successful Wi-Fi connection
  for (int i = 0; i < NUM_LEDS; i++) {
    pixels.clear();
    pixels.setPixelColor(i, 255, 255, 255, 0);  // White color
    pixels.show();
    delay(30);
  }

  for (int i = 0; i < NUM_LEDS; i++) {
    pixels.setPixelColor(i, 255, 255, 255, 0);  // White color
    pixels.show();
  }

  delay(200);

  pixels.clear();
  pixels.show();
}
