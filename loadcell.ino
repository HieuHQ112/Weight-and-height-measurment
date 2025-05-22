#include <HX711_ADC.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

// Use defines instead of constants to save RAM
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define HX711_DOUT 4
#define HX711_SCK 5
#define SERIAL_INTERVAL 150      // Faster update interval (was 500)
#define DISPLAY_ADDRESS 0x3C
#define SMOOTH_FACTOR 0.15      // Smoothing factor (0.15 = 15% new data, 85% old data)

HX711_ADC LoadCell(HX711_DOUT, HX711_SCK);
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire);

// Combine related variables
struct {
  float weight;
  float smoothedWeight;
  unsigned long lastUpdate;
  bool newDataReady;
} state = {0, 0, 0, false};

void setup() {
  Serial.begin(115200);
  Wire.begin();

  if (!display.begin(SSD1306_SWITCHCAPVCC, DISPLAY_ADDRESS)) {
    Serial.println(F("SSD1306 failed"));
    while (1);
  }

  // Initialize display with minimal calls
  display.clearDisplay();
  display.setTextColor(SSD1306_WHITE);
  display.display();

  LoadCell.begin();
  LoadCell.setSamplesInUse(4);    // Reduce samples for faster response
  LoadCell.start(1500, true);     // Reduced stabilizing time (was 2000)

  if (LoadCell.getTareTimeoutFlag() || LoadCell.getSignalTimeoutFlag()) {
    Serial.println(F("Timeout, check MCU>HX711 wiring"));
    while (1);
  }
  
  LoadCell.setCalFactor(1.0);
  while (!LoadCell.update());
  calibrate();
}

void loop() {
  // Check for new data
  if (LoadCell.update()) state.newDataReady = true;

  // Update display if new data and interval passed
  if (state.newDataReady && millis() - state.lastUpdate >= SERIAL_INTERVAL) {
    updateDisplay();
    state.newDataReady = false;
    state.lastUpdate = millis();
  }

  // Handle commands
  if (Serial.available()) {
    switch(Serial.read()) {
      case 't': 
        LoadCell.tareNoDelay(); 
        state.smoothedWeight = 0;  // Reset smoothed weight after tare
        break;
      case 'r': 
        calibrate(); 
        break;
    }
  }

  if (LoadCell.getTareStatus()) {
    Serial.println(F("Tare complete"));
  }
}

void updateDisplay() {
  state.weight = LoadCell.getData() / 1000.0;
  
  // Apply exponential moving average for smoothing
  state.smoothedWeight = (SMOOTH_FACTOR * state.weight) + ((1 - SMOOTH_FACTOR) * state.smoothedWeight);
  
  float roundedWeight = round(state.smoothedWeight * 10.0) / 10.0;
  
  Serial.print(F("Weight: "));
  Serial.print(roundedWeight, 1);
  Serial.println(F(" kg"));
  
  display.clearDisplay();
  display.setTextSize(roundedWeight < 0 ? 1 : 2);

  if (roundedWeight < 0) {
    display.setCursor((SCREEN_WIDTH - 84) / 2, SCREEN_HEIGHT / 2 - 4);
    display.print(F("0.0"));
  } else {
    char weightStr[8];
    dtostrf(roundedWeight, 4, 1, weightStr);
    strcat(weightStr, " Kg");
    
    // Calculate position for center alignment
    int16_t x1, y1;
    uint16_t width, height;
    display.getTextBounds(weightStr, 0, 0, &x1, &y1, &width, &height);
    display.setCursor((SCREEN_WIDTH - width) / 2, (SCREEN_HEIGHT - height) / 2);
    display.print(weightStr);
  }
  
  display.display();
}

void calibrate() {
  Serial.println(F("*** Calibration:"));
  Serial.println(F("1. Place on stable surface"));
  Serial.println(F("2. Remove all load"));
  Serial.println(F("3. Send 't' to tare"));

  display.clearDisplay();
  display.setTextSize(1);
  display.setCursor(0, 0);
  display.println(F("Calibration Mode"));
  display.println(F("Send 't' to tare"));
  display.display();

  while (true) {
    LoadCell.update();
    if (Serial.available() && Serial.read() == 't') {
      LoadCell.tareNoDelay();
    }
    if (LoadCell.getTareStatus()) {
      Serial.println(F("Tare complete"));
      break;
    }
  }

  display.clearDisplay();
  display.setCursor(0, 0);
  display.println(F("Place known"));
  display.println(F("weight & enter"));
  display.println(F("value (g) in"));
  display.println(F("Serial Monitor"));
  display.display();

  Serial.println(F("Place known weight."));
  Serial.println(F("Enter weight in g:"));

  float knownMass = 0;
  while (knownMass <= 0) {
    LoadCell.update();
    if (Serial.available()) {
      knownMass = Serial.parseFloat();
    }
  }

  Serial.print(F("Mass: "));
  Serial.print(knownMass, 1);
  Serial.println(F(" g"));

  LoadCell.refreshDataSet();
  float newCalFactor = LoadCell.getNewCalibration(knownMass);
  LoadCell.setCalFactor(newCalFactor);

  Serial.print(F("New cal factor: "));
  Serial.println(newCalFactor, 3);
  
  display.clearDisplay();
  display.println(F("Calibration"));
  display.println(F("Complete!"));
  display.display();
  delay(2000);
}