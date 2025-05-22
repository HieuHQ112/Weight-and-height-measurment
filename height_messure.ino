#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <HCSR04.h>

// Set custom I2C pins (SDA = GPIO 8, SCL = GPIO 9)
#define SDA_PIN 8
#define SCL_PIN 9

#define SCREEN_WIDTH 128 // OLED display width, in pixels
#define SCREEN_HEIGHT 64 // OLED display height, in pixels

// Set up the OLED display
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire);

// Define ultrasonic sensor pins
#define TRIGGER_PIN 12
#define ECHO_PIN 20

// Distance variables
double distance_cm, distance_inch, distance;
const double CM_TO_INCH = 0.393701;

// Initialize ultrasonic sensor
UltraSonicDistanceSensor distanceSensor(TRIGGER_PIN, ECHO_PIN);

void setup() {
  // Initialize I2C with custom pins
  Wire.begin(SDA_PIN, SCL_PIN);

  Serial.begin(9600); // Initialize serial communication
  if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) { // Initialize with the I2C address 0x3C
    Serial.println(F("SSD1306 allocation failed"));
    for (;;); // Don't proceed, loop forever
  }

  display.clearDisplay();
  display.setTextSize(2); // Set initial text size
  display.setTextColor(SSD1306_WHITE); // Draw white text
  display.display();
}

void loop() {
  // Measure distance in cm
  distance_cm = distanceSensor.measureDistanceCm();
  distance = (160 - distance_cm)/100;
  distance_inch = distance_cm * CM_TO_INCH;

  // Clear the display buffer
  display.clearDisplay();

  if (distance_cm == -1.0) {
    Serial.println("No distance detected");
    display.setTextSize(1); // Smaller text for error message
    display.setCursor((SCREEN_WIDTH - 84) / 2, SCREEN_HEIGHT / 2 - 4); // Approximate center for error
    display.println("No distance detected");
  } else {
    // Prepare the distance strings
    String distanceCmStr = String(distance) + " m";
    String distanceInchStr = String(distance_inch) + " inch";

    // Calculate the width of the text strings to center them
    int16_t x1, y1;
    uint16_t width1, height1, width2, height2;
    display.setTextSize(2); // Set larger text size for distances
    display.getTextBounds(distanceCmStr.c_str(), 0, 0, &x1, &y1, &width1, &height1);
    display.getTextBounds(distanceInchStr.c_str(), 0, 0, &x1, &y1, &width2, &height2);

    // Center the text on the display
    int16_t xPos1 = (SCREEN_WIDTH - width1) / 2;
    int16_t xPos2 = (SCREEN_WIDTH - width2) / 2;
    int16_t yPos1 = (SCREEN_HEIGHT / 2) - (height1 + 2); // Center vertically for cm
    int16_t yPos2 = (SCREEN_HEIGHT / 2) + 2; // Center vertically below cm for inch

    display.setCursor(xPos1, yPos1);
    display.println(distanceCmStr);
    display.setCursor(xPos2, yPos2);
    display.println(distanceInchStr);
  }

  // Update the display with the new content
  display.display();
  delay(1000);
}
