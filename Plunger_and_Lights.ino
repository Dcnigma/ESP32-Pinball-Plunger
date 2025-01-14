#include <WiFi.h>
#include <WebServer.h>
#include <Preferences.h>
#include <Adafruit_NeoPixel.h>
#include <BleGamepad.h> 
#include <Arduino.h>
//USE Release 6.0 of BleGamepad (6.2 issues)
//Thanks to @Mystfit and @Sab1e-GitHub for some pull requests to update the library to be inline with NimBLE changes, and to add output report functionality
//
//Release 6.0 is released to bring compatibility with @h2zero's NimBLE version 2.0.0 which was recently released through Arduino Library Manager

// Create BLE Gamepad instance
BleGamepad gamepad("DCnigma's Pinball Plunger", "DcNigma 2025", 100); // Name, Manufacturer, Battery Level
BleGamepadConfiguration bleGamepadConfig;

// Retrieve the value of lastLeds from preferences

// Wi-Fi Credentials
const char* ssid = "ESP32_Plunger";
const char* password = "12345678";

// Potentiometer Configuration (default values)
int potPin = 34;
int restValue = 2350;
int maxValue = 2700;
int WS2812BPin = 25;
int TotalLEDS = 25;
int brightness = 50;
int lastLeds = 10;
int deadZone = 10;          // Dead zone range for stable detection for ui

const int smoothingWindow = 5; // Smooth outs the pot vauonConnectles 



// Define thresholds for animations based on plunger position
int quarterThreshold = restValue + (maxValue - restValue) / 4;
int halfThreshold = restValue + (maxValue - restValue) / 1.6;
int threeQuarterThreshold = restValue + (maxValue - restValue) / 1.1;
// + 4 * /removed

enum AnimationMode {
    ANIM_OFF,
    ANIM_BALL_LAUNCH,
    ANIM_RAINBOW,
    ANIM_FIRE,
    ANIM_TRAIL,
    ANIM_KNIGHT_RIDER,
    ANIM_BAR_KNIGHT_RIDER
};

AnimationMode quarterAnimationMode = ANIM_OFF; // Default mode


enum HalfAnimationList {
    ANIM_HALF_FULL_OFF,
    ANIM_HALF_RED_GREEN,
    ANIM_HALF_FULL_RAINBOW,
    ANIM_HALF_PURPLE_PINK,
    ANIM_HALF_RED_BLUE,
    ANIM_HALF_RED_YELLOW
};

HalfAnimationList HalfAnimationSelect = ANIM_HALF_FULL_OFF; // Default mode


enum ThreeQuartersAnimationList {
    ANIM3_FULL_OFF,
    ANIM3_RED_GREEN,
    ANIM3_FULL_RAINBOW,
    ANIM3_PURPLE_PINK,
    ANIM3_RED_BLUE,
    ANIM3_RED_YELLOW
};

ThreeQuartersAnimationList threeQuartersAnimationSelect = ANIM3_FULL_OFF; // Default mode

enum FullPullMode {
    ANIM_FULL_OFF,
    ANIM_RED_GREEN,
    ANIM_FULL_RAINBOW,
    ANIM_PURPLE_PINK,
    ANIM_RED_BLUE,
    ANIM_RED_YELLOW
};
FullPullMode fullAnimationMode = ANIM_RED_YELLOW; // Default mode

// WebServer
WebServer server(80);
Preferences preferences;

// WS2812B LED Configuration
#define LED_PIN WS2812BPin
#define NUM_LEDS TotalLEDS
Adafruit_NeoPixel strip(NUM_LEDS, LED_PIN, NEO_GRB + NEO_KHZ800);


// Smoothing Variables
int potValues[smoothingWindow] = {0};
int potIndex = 0;

// Function: Save Preferences
void savePreferences() {
  preferences.begin("plunger-config", false); // Open in write mode 
  preferences.putInt("quarterMode", quarterAnimationMode); // working
  preferences.putInt("HALF_UI", HalfAnimationSelect); // new
  preferences.putInt("threeQuartersUI", threeQuartersAnimationSelect); // working
  preferences.putInt("fullMode", fullAnimationMode); //not working
  preferences.putInt("deadZone", deadZone);
  preferences.putInt("potPin", potPin);
  preferences.putInt("lastLeds", lastLeds);
  preferences.putInt("WS2812BPin", WS2812BPin);
  preferences.putInt("TotalLEDS", TotalLEDS);
  preferences.putInt("restValue", restValue);
  preferences.putInt("maxValue", maxValue);
  preferences.putInt("brightness", brightness);
  preferences.end();
  delay(100); // Ensure the preferences are committed to flash
}

// Function: Load Preferences
void loadPreferences() {
  preferences.begin("plunger-config", true); // Open in read mode  
  potPin = preferences.getInt("potPin", 34);
 
  deadZone = preferences.getInt("deadZone", 10);    
  WS2812BPin = preferences.getInt("WS2812BPin", 25);
  TotalLEDS = preferences.getInt("TotalLEDS", 25);
  lastLeds = preferences.getInt("lastLeds", 10);
  restValue = preferences.getInt("restValue", 2350);
  maxValue = preferences.getInt("maxValue", 2700);
  brightness = preferences.getInt("brightness", 50);
  quarterAnimationMode = (AnimationMode)preferences.getInt("quarterMode", ANIM_OFF); //not working
  HalfAnimationSelect = (HalfAnimationList)preferences.getInt("HALF_UI", ANIM_HALF_FULL_OFF);  
  threeQuartersAnimationSelect = (ThreeQuartersAnimationList)preferences.getInt("threeQuartersUI", ANIM3_FULL_OFF);  
  fullAnimationMode = (FullPullMode)preferences.getInt("fullMode", ANIM_RED_YELLOW); //not working
  preferences.end(); 
}




// Function: Read and Smooth Potentiometer Values
int readSmoothedPotValue() {
  int currentValue = analogRead(potPin);
  potValues[potIndex] = currentValue;
  potIndex = (potIndex + 1) % smoothingWindow;

  int smoothedValue = 0;
  for (int i = 0; i < smoothingWindow; i++) {
    smoothedValue += potValues[i];
  }
  return smoothedValue / smoothingWindow;
}

// Function: Calculate Z-Axis Value
float getPlungerValue(int smoothedValue) {
  float mappedValue = map(smoothedValue, restValue, maxValue, -100, 100) / 100.0;
  mappedValue = constrain(mappedValue, -1.0, 1.0);

  if (abs(mappedValue) < (float)deadZone / 100.0) {
    mappedValue = 0.0;
  }
  //Serial.printf("Smoothed Value: %d, Quarter Threshold: %d, Half Threshold: %d, Three-Quarter Threshold: %d\n",smoothedValue, quarterThreshold, halfThreshold, threeQuarterThreshold);

  //Serial.printf("Pot Value: %d, Smoothed Value: %d, Mapped Z: %.2f\n", analogRead(potPin), smoothedValue, mappedValue);
  return mappedValue;
}

//// Function: Send Gamepad Report
//void sendGamepadReport(float zAxisValue) {
//  uint8_t report[3] = {127, 127, (uint8_t)((zAxisValue + 1.0) * 127)};
//  inputReport->setValue(report, sizeof(report));
//  inputReport->notify();
////  Serial.printf("Sending Report - Z: %d\n", report[2]);
//}

// Function: Send Gamepad Report
void sendGamepadReport(float zAxisValue) {
  // Map Z-Axis value to the gamepad range (-127 to 127)
  int zAxisGamepadValue = (int)(zAxisValue * 127);
  gamepad.setAxes(0, 0, zAxisGamepadValue, 0, 0, 0); // X, Y, Z, Rx, Ry, Rz
  gamepad.sendReport();
}


void handleSetQuarterAnimation() {
  if (server.hasArg("quarterMode")) {
    int mode = server.arg("quarterMode").toInt();
    Serial.printf("Received AnimationMode: %d\n", mode);
    if (mode >= ANIM_OFF && mode <= ANIM_BAR_KNIGHT_RIDER) {
      quarterAnimationMode = static_cast<AnimationMode>(mode);
      server.send(200, "text/plain", "Animation mode updated");
    } else {
      Serial.println("Invalid AnimationMode received");
      server.send(400, "text/plain", "Invalid mode");
    }
  } else {
    Serial.println("Missing AnimationMode argument");
    server.send(400, "text/plain", "Missing mode argument");
  }
}

void handleSetHALFAnimation() {
  int mode = server.arg("HALF_UI").toInt();
  Serial.printf("Received HALF_UI: %d\n", mode);
  HalfAnimationSelect = static_cast<HalfAnimationList>(mode);
  
  if (server.hasArg("HALF_UI")) {
    int mode = server.arg("HALF_UI").toInt();
    Serial.printf("Received HALF_UI: %d\n", mode);
    if (mode >= ANIM_HALF_FULL_OFF && mode <= ANIM_HALF_RED_YELLOW) {
      HalfAnimationSelect = static_cast<HalfAnimationList>(mode);
      server.send(200, "text/plain", "HALF_UI updated");
    } else {
      Serial.println("Invalid HALF_UI received");
      server.send(400, "text/plain", "Invalid mode");
    }
  } else {
    Serial.println("Missing HALF_UI argument");
    server.send(400, "text/plain", "Missing mode argument");
  }
}

void handleSetThreeQuartersAnimation() {
  int mode = server.arg("threeQuartersUI").toInt();
  Serial.printf("Received threeQuartersUI: %d\n", mode);
  threeQuartersAnimationSelect = static_cast<ThreeQuartersAnimationList>(mode);
  
  if (server.hasArg("threeQuartersUI")) {
    int mode = server.arg("threeQuartersUI").toInt();
    Serial.printf("Received threeQuartersUI: %d\n", mode);
    if (mode >= ANIM3_FULL_OFF && mode <= ANIM3_RED_YELLOW) {
      threeQuartersAnimationSelect = static_cast<ThreeQuartersAnimationList>(mode);
      server.send(200, "text/plain", "threeQuartersUI updated");
    } else {
      Serial.println("Invalid threeQuartersUI received");
      server.send(400, "text/plain", "Invalid mode");
    }
  } else {
    Serial.println("Missing threeQuartersUI argument");
    server.send(400, "text/plain", "Missing mode argument");
  }
}

void handleSetFullAnimation() {
  if (server.hasArg("fullMode")) {
    int mode = server.arg("fullMode").toInt();
    Serial.printf("Received FullPullMode: %d\n", mode);
    if (mode >= ANIM_FULL_OFF && mode <= ANIM_RED_YELLOW) {
      fullAnimationMode = static_cast<FullPullMode>(mode);
      server.send(200, "text/plain", "Animation mode updated");
    } else {
      Serial.println("Invalid FullPullMode received");
      server.send(400, "text/plain", "Invalid mode");
    }
  } else {
    server.send(400, "text/plain", "Missing mode argument");
    Serial.println("Missing FullPullMode argument");
  }
}


void animationPull(int pullLevel) {
  // plungerLedsStart aka lastledsstart
  int plungerLedsStart = strip.numPixels() - lastLeds; // Calculate starting index for plunger LEDs
  
  // Pull level: 1 = Half, 2 = Three-Quarters, 3 = Full
  static uint32_t lastUpdate = 0;
  static uint32_t strobeLastUpdate = 0;
  uint32_t interval;
  uint32_t strobeInterval;

  // Set intervals based on pull level
  switch (pullLevel) {
    case 1:
      interval = 100;  // Slower cycling speed
      strobeInterval = 200; // Slower strobe speed
      break;
    case 2:
      interval = 75;
      strobeInterval = 100;
      break;
    case 3:
      interval = 50;  // Fastest cycling speed
      strobeInterval = 50;  // Fastest strobe speed
      break;
    default:
      interval = 100;
      strobeInterval = 200;
      break;
  }



  

  static int pixelIndex = 0; // For cycling trail animation
  static bool strobeOn = true; // For strobe effect toggle

  // Update cycling animation
  if (millis() - lastUpdate >= interval) {
    lastUpdate = millis();

    // Clear strip and create a trail for the first 15 LEDs
    strip.clear();
    for (int i = 0; i < 3; i++) { // Add a 3-pixel trail
      int trailIndex = (pixelIndex - i + plungerLedsStart) % plungerLedsStart; // Wrap around
      uint8_t brightness = 255 - (i * 85); // Gradually dim trail
      strip.setPixelColor(trailIndex, strip.Color(255, 255, 0, brightness)); // Yellow with brightness
    }

    // Move the pixel index for cycling
    pixelIndex = (pixelIndex + 1) % plungerLedsStart;
  }

  // Update strobe effect for LEDs after the 15th
  if (millis() - strobeLastUpdate >= strobeInterval) {
    strobeLastUpdate = millis();
    strobeOn = !strobeOn; // Toggle strobe state

    // Strobe effect for LEDs after the 15th
    for (int i = plungerLedsStart; i < strip.numPixels(); i++) {
      if (strobeOn) {
        // Bright color during "on" state
        uint8_t r = (pullLevel == 3) ? 255 : (pullLevel == 2) ? 255 : 255;  // Color based on level
        uint8_t g = (pullLevel == 3) ? 0 : (pullLevel == 2) ? 201 : 255;
        uint8_t b = 0;
        strip.setPixelColor(i, strip.Color(r, g, b));
      } else {
        // Turn off LEDs during "off" state
        strip.setPixelColor(i, strip.Color(0, 0, 0));
      }
    }
  }

  // Show the updated strip
  strip.show();
}

// plungerLedsStart aka lastledsstart
int plungerLedsStart = strip.numPixels() - lastLeds; // Calculate starting index for plunger LEDs

//Plunger in rest animation
void animationQuarter() {
  switch (quarterAnimationMode) {
    case ANIM_OFF:
      strip.clear();
      strip.show();
      break;

    case ANIM_BALL_LAUNCH: {
      static bool toggle = false;
      static uint32_t lastUpdate = 0;
      const uint32_t interval = 200;

      if (millis() - lastUpdate >= interval) {
        lastUpdate = millis();
        toggle = !toggle;
      }

      for (int i = 0; i < plungerLedsStart; i++) strip.setPixelColor(i, 0); // Off
      for (int i = plungerLedsStart; i < strip.numPixels(); i++) {
        strip.setPixelColor(i, toggle ? strip.Color(255, 0, 0) : strip.Color(0, 255, 0)); // Red/Green
      }
      strip.show();
      break;
    }

    case ANIM_RAINBOW: {
      static uint16_t rainbowOffset = 0;

      for (int i = 0; i < plungerLedsStart; i++) {
        uint16_t hue = (i * 65536L / strip.numPixels() + rainbowOffset) % 65536;
        strip.setPixelColor(i, strip.ColorHSV(hue, 255, 255));
      }
      for (int i = plungerLedsStart; i < strip.numPixels(); i++) {
        uint16_t hue2 = (i * 65536L / strip.numPixels() + rainbowOffset) % 65536;
        strip.setPixelColor(i, strip.ColorHSV(hue2, 255, 255));
      }

      strip.show();
      rainbowOffset += 512;
      if (rainbowOffset >= 65536) rainbowOffset = 0;
      break;
    }

    case ANIM_FIRE: {
      static uint32_t lastUpdate = 0;
      const uint32_t interval = 100;

      if (millis() - lastUpdate >= interval) {
        lastUpdate = millis();
        for (int i = 0; i < plungerLedsStart; i++) {
          int chance = random(0, 100);
          if (chance < 20) {
            strip.setPixelColor(i, 0); // Off
          } else if (chance < 50) {
            strip.setPixelColor(i, strip.Color(255, 0, 0)); // Dim red
          } else {
            strip.setPixelColor(i, strip.Color(random(128, 255), random(128, 255), 0)); // Flame
          }
        }
        for (int i = plungerLedsStart; i < strip.numPixels(); i++) {
          strip.setPixelColor(i, strip.Color(255, 255, 0)); // Yellow
        }
        strip.show();
      }
      break;
    }

    case ANIM_TRAIL: {
      static int pixelIndex = 0;

      strip.clear();
      strip.setPixelColor(pixelIndex, strip.Color(0, 0, 255)); // Blue
      strip.show();

      pixelIndex = (pixelIndex + 1) % strip.numPixels();
      break;
    }

    case ANIM_KNIGHT_RIDER: {
    static int direction = 1; // 1 for forward, -1 for backward
    static int position = plungerLedsStart; // Start at the first LED in the range
    static uint32_t lastUpdate = 0;
    const uint32_t interval = 100; // Adjust speed by changing this interval

    if (millis() - lastUpdate >= interval) {
      lastUpdate = millis();

      // Clear all LEDs in the range
      for (int i = plungerLedsStart; i < strip.numPixels(); i++) {
        strip.setPixelColor(i, 0); // Turn off
      }

      // Light up 3 LEDs with fading effect
      for (int offset = 0; offset < 3; offset++) {
        int ledIndex = position - offset * direction;

        // Make sure the index is within bounds
        if (ledIndex >= plungerLedsStart && ledIndex < strip.numPixels()) {
          uint8_t brightness = 255 - (offset * 100); // Trail fades with distance
          strip.setPixelColor(ledIndex, strip.Color(brightness, 0, 0)); // Red trail
        }
      }

      // Move the position
      position += direction;

      // Reverse direction if we hit the edges
      if (position >= strip.numPixels() || position < plungerLedsStart) {
        direction *= -1;
        position += direction; // Correct overshoot
      }

      strip.show();
    }
    break;
  }

    case ANIM_BAR_KNIGHT_RIDER: {
     static int direction = 1; // 1 for forward, -1 for backward
    static int position = 0; // Start at the first LED in the range
    static uint32_t lastUpdate = 0;
    const uint32_t interval = 100; // Adjust speed by changing this interval
  
    if (millis() - lastUpdate >= interval) {
      lastUpdate = millis();
  
      // Clear all LEDs in the range
      for (int i = 0; i < plungerLedsStart; i++) {
        strip.setPixelColor(i, 0); // Turn off
      }
  
      // Light up 3 LEDs with fading effect
      for (int offset = 0; offset < 3; offset++) {
        int ledIndex = position - offset * direction;
  
        // Make sure the index is within bounds
        if (ledIndex >= 0 && ledIndex < plungerLedsStart) {
          uint8_t brightness = 255 - (offset * 100); // Trail fades with distance
          strip.setPixelColor(ledIndex, strip.Color(brightness, 0, 0)); // Red trail
        }
      }
  
      // Move the position
      position += direction;
  
      // Reverse direction if we hit the edges
      if (position >= plungerLedsStart || position < 0) {
        direction *= -1;
        position += direction; // Correct overshoot
      }
  
      strip.show();
    }
    break;
  }

  }
}


// Call these functions with appropriate pull level
void animationHalf() {
  switch (HalfAnimationSelect) {
    case ANIM_HALF_FULL_OFF:
      strip.clear();
      strip.show();
//      animationPull(1);
      break;

    case ANIM_HALF_RED_GREEN: {
      static bool toggle = false;
      static uint32_t lastUpdate = 0;
      const uint32_t interval = 50;

      if (millis() - lastUpdate >= interval) {
        lastUpdate = millis();
        toggle = !toggle;
      }

      for (int i = 0; i < plungerLedsStart; i++) strip.setPixelColor(i, 0); // Off
      for (int i = plungerLedsStart; i < strip.numPixels(); i++) {
        strip.setPixelColor(i, toggle ? strip.Color(255, 0, 0) : strip.Color(0, 255, 0)); // Red/Green
      }
      strip.show();
      break;
    }

    case ANIM_HALF_FULL_RAINBOW: {
//        animationPull(3);
      static uint16_t rainbowOffset = 0;

      for (int i = 0; i < plungerLedsStart; i++) {
        uint16_t hue = (i * 65536L / strip.numPixels() + rainbowOffset) % 65536;
        strip.setPixelColor(i, strip.ColorHSV(hue, 255, 255));
      }
      for (int i = plungerLedsStart; i < strip.numPixels(); i++) {
        uint16_t hue2 = (i * 65536L / strip.numPixels() + rainbowOffset) % 65536;
        strip.setPixelColor(i, strip.ColorHSV(hue2, 255, 255));
      }

      strip.show();
      rainbowOffset += 512;
      if (rainbowOffset >= 65536) rainbowOffset = 0;
      break;
    }

    case ANIM_HALF_PURPLE_PINK: {
      static bool toggle = false;
      static uint32_t lastUpdate = 0;
      const uint32_t interval = 50;

      if (millis() - lastUpdate >= interval) {
        lastUpdate = millis();
        toggle = !toggle;
      }

      for (int i = 0; i < plungerLedsStart; i++) strip.setPixelColor(i, 0); // Off
      for (int i = plungerLedsStart; i < strip.numPixels(); i++) {
        strip.setPixelColor(i, toggle ? strip.Color(255, 0, 255) : strip.Color(255, 150, 0)); // PURPLE PINK
      }
      strip.show();
      break;
    }

    case ANIM_HALF_RED_BLUE: {
      static bool toggle = false;
      static uint32_t lastUpdate = 0;
      const uint32_t interval = 50;

      if (millis() - lastUpdate >= interval) {
        lastUpdate = millis();
        toggle = !toggle;
      }

      for (int i = 0; i < plungerLedsStart; i++) strip.setPixelColor(i, 0); // Off
      for (int i = plungerLedsStart; i < strip.numPixels(); i++) {
        strip.setPixelColor(i, toggle ? strip.Color(255, 0, 0) : strip.Color(0, 0, 255)); // Red/Blue
      }
      strip.show();
      break;
    }

    case ANIM_HALF_RED_YELLOW: {  
  animationPull(1);
  }
}
}  

void animationThreeQuarters() {
  switch (threeQuartersAnimationSelect) {
    case ANIM3_FULL_OFF:
      strip.clear();
      strip.show();
//      animationPull(3);
      break;

    case ANIM3_RED_GREEN: {
      static bool toggle = false;
      static uint32_t lastUpdate = 0;
      const uint32_t interval = 50;

      if (millis() - lastUpdate >= interval) {
        lastUpdate = millis();
        toggle = !toggle;
      }

      for (int i = 0; i < plungerLedsStart; i++) strip.setPixelColor(i, 0); // Off
      for (int i = plungerLedsStart; i < strip.numPixels(); i++) {
        strip.setPixelColor(i, toggle ? strip.Color(255, 0, 0) : strip.Color(0, 255, 0)); // Red/Green
      }
      strip.show();
      break;
    }

    case ANIM3_FULL_RAINBOW: {
//        animationPull(3);
      static uint16_t rainbowOffset = 0;

      for (int i = 0; i < plungerLedsStart; i++) {
        uint16_t hue = (i * 65536L / strip.numPixels() + rainbowOffset) % 65536;
        strip.setPixelColor(i, strip.ColorHSV(hue, 255, 255));
      }
      for (int i = plungerLedsStart; i < strip.numPixels(); i++) {
        uint16_t hue2 = (i * 65536L / strip.numPixels() + rainbowOffset) % 65536;
        strip.setPixelColor(i, strip.ColorHSV(hue2, 255, 255));
      }

      strip.show();
      rainbowOffset += 512;
      if (rainbowOffset >= 65536) rainbowOffset = 0;
      break;
    }

    case ANIM3_PURPLE_PINK: {
      static bool toggle = false;
      static uint32_t lastUpdate = 0;
      const uint32_t interval = 50;

      if (millis() - lastUpdate >= interval) {
        lastUpdate = millis();
        toggle = !toggle;
      }

      for (int i = 0; i < plungerLedsStart; i++) strip.setPixelColor(i, 0); // Off
      for (int i = plungerLedsStart; i < strip.numPixels(); i++) {
        strip.setPixelColor(i, toggle ? strip.Color(255, 0, 255) : strip.Color(255, 150, 0)); // PURPLE PINK
      }
      strip.show();
      break;
    }

    case ANIM3_RED_BLUE: {
      static bool toggle = false;
      static uint32_t lastUpdate = 0;
      const uint32_t interval = 50;

      if (millis() - lastUpdate >= interval) {
        lastUpdate = millis();
        toggle = !toggle;
      }

      for (int i = 0; i < plungerLedsStart; i++) strip.setPixelColor(i, 0); // Off
      for (int i = plungerLedsStart; i < strip.numPixels(); i++) {
        strip.setPixelColor(i, toggle ? strip.Color(255, 0, 0) : strip.Color(0, 0, 255)); // Red/Blue
      }
      strip.show();
      break;
    }

    case ANIM3_RED_YELLOW: {  
  animationPull(2);
  }
}
}  

void animationFull() {
  switch (fullAnimationMode) {
    case ANIM_FULL_OFF:
      strip.clear();
      strip.show();
      animationPull(3);
      break;

    case ANIM_RED_GREEN: {
      static bool toggle = false;
      static uint32_t lastUpdate = 0;
      const uint32_t interval = 50;

      if (millis() - lastUpdate >= interval) {
        lastUpdate = millis();
        toggle = !toggle;
      }

      for (int i = 0; i < plungerLedsStart; i++) strip.setPixelColor(i, 0); // Off
      for (int i = plungerLedsStart; i < strip.numPixels(); i++) {
        strip.setPixelColor(i, toggle ? strip.Color(255, 0, 0) : strip.Color(0, 255, 0)); // Red/Green
      }
      strip.show();
      break;
    }

    case ANIM_FULL_RAINBOW: {
//        animationPull(3);
      static uint16_t rainbowOffset = 0;

      for (int i = 0; i < plungerLedsStart; i++) {
        uint16_t hue = (i * 65536L / strip.numPixels() + rainbowOffset) % 65536;
        strip.setPixelColor(i, strip.ColorHSV(hue, 255, 255));
      }
      for (int i = plungerLedsStart; i < strip.numPixels(); i++) {
        uint16_t hue2 = (i * 65536L / strip.numPixels() + rainbowOffset) % 65536;
        strip.setPixelColor(i, strip.ColorHSV(hue2, 255, 255));
      }

      strip.show();
      rainbowOffset += 512;
      if (rainbowOffset >= 65536) rainbowOffset = 0;
      break;
    }

    case ANIM_PURPLE_PINK: {
      static bool toggle = false;
      static uint32_t lastUpdate = 0;
      const uint32_t interval = 50;

      if (millis() - lastUpdate >= interval) {
        lastUpdate = millis();
        toggle = !toggle;
      }

      for (int i = 0; i < plungerLedsStart; i++) strip.setPixelColor(i, 0); // Off
      for (int i = plungerLedsStart; i < strip.numPixels(); i++) {
        strip.setPixelColor(i, toggle ? strip.Color(255, 0, 255) : strip.Color(255, 150, 0)); // PURPLE PINK
      }
      strip.show();
      break;
    }

    case ANIM_RED_BLUE: {
      static bool toggle = false;
      static uint32_t lastUpdate = 0;
      const uint32_t interval = 50;

      if (millis() - lastUpdate >= interval) {
        lastUpdate = millis();
        toggle = !toggle;
      }

      for (int i = 0; i < plungerLedsStart; i++) strip.setPixelColor(i, 0); // Off
      for (int i = plungerLedsStart; i < strip.numPixels(); i++) {
        strip.setPixelColor(i, toggle ? strip.Color(255, 0, 0) : strip.Color(0, 0, 255)); // Red/Blue
      }
      strip.show();
      break;
    }

    case ANIM_RED_YELLOW: {
    animationPull(3);
    
  }


  }
}

// Function: Apply LED Mode
void applyLEDMode(int smoothedValue) {
  // Calculate speed scaling factor
  int range = maxValue - restValue;
  float positionFactor = (float)(smoothedValue - restValue) / range; // 0.0 to 1.0
  uint32_t interval = 50 - (positionFactor * 30); // Faster animation at higher positions

  // Update animations with scaled speed
  if (smoothedValue < quarterThreshold) {
    animationQuarter();
  } else if (smoothedValue < halfThreshold) {
    animationHalf();
  } else if (smoothedValue < threeQuarterThreshold) {
    animationThreeQuarters();
  } else {
    animationFull();
  }
}
// Function: Web Page HTML
String htmlPage() {
  String html = "<html><head><title>ESP32 Plunger Config</title></head><body>";
  html += "<meta name='viewport' content='width=device-width, initial-scale=1.0'>";
  html += "<h1>Pinball Plunger Configuration</h1>";
  html += "<form action='/save' method='POST'>";
  
  html += "<br></br>";
  html += "<h3>Potentiometer settings</h3>";
  // Potentiometer settings
  
  html += "Potentiometer Pin: <input type='number' name='potPin' value='" + String(potPin) + "'><br>";
  html += "Dead Zone: <input type='number' name='deadZone' value='" + String(deadZone) + "'><br>";
  html += "Rest Value: <input type='number' name='restValue' value='" + String(restValue) + "'><br>";
  html += "Max Value: <input type='number' name='maxValue' value='" + String(maxValue) + "'><br>";
  
  html += "<br></br>";
  html += "<h3>Light settings (WS2812B)</h3>";
  
  
  html += "WS2812B Pin: <input type='number' name='WS2812BPin' value='" + String(WS2812BPin) + "'><br>";
  html += "Total LEDS: <input type='number' name='TotalLEDS' value='" + String(TotalLEDS) + "'><br>";
  html += "Plunger LEDS (last): <input type='number' name='lastLeds' value='" + String(lastLeds) + "'><br>";
  html += "Brightness: <input type='number' name='brightness' value='" + String(brightness) + "' min='0' max='255'><br>";
  
  html += "<br></br>";
  html += "<h3>Animation settings</h3>";
  
  
  // AnimationQuarter mode
  html += "Plunger In Rest  LED Mode: <select name='quarterMode'>";
  html += "<option value='0'" + String(quarterAnimationMode == ANIM_OFF ? " selected" : "") + ">Off</option>";
  html += "<option value='1'" + String(quarterAnimationMode == ANIM_BALL_LAUNCH ? " selected" : "") + ">Ball Launch</option>";
  html += "<option value='2'" + String(quarterAnimationMode == ANIM_RAINBOW ? " selected" : "") + ">Rainbow</option>";
  html += "<option value='3'" + String(quarterAnimationMode == ANIM_FIRE ? " selected" : "") + ">Fire</option>";
  html += "<option value='4'" + String(quarterAnimationMode == ANIM_TRAIL ? " selected" : "") + ">Trail</option>";
  html += "<option value='5'" + String(quarterAnimationMode == ANIM_KNIGHT_RIDER? " selected" : "") + ">KNIGHT RIDER</option>";
  html += "<option value='6'" + String(quarterAnimationMode == ANIM_BAR_KNIGHT_RIDER? " selected" : "") + ">KNIGHT RIDER BAR</option>";
  html += "</select><br>";
  // AnimationThreeQuarters mode  
  html += "Plunger Half Way LED Mode: <select name='HALF_UI'>";
  html += "<option value='0'" + String(HalfAnimationSelect == ANIM_HALF_FULL_OFF ? " selected" : "") + ">Three Quarters Off</option>";
  html += "<option value='1'" + String(HalfAnimationSelect == ANIM_HALF_RED_GREEN ? " selected" : "") + ">Three Quarters Ball Launch</option>";
  html += "<option value='2'" + String(HalfAnimationSelect == ANIM_HALF_FULL_RAINBOW ? " selected" : "") + ">Three Quarters Rainbow</option>";
  html += "<option value='3'" + String(HalfAnimationSelect == ANIM_HALF_PURPLE_PINK ? " selected" : "") + ">Three Quarters Fire</option>";
  html += "<option value='4'" + String(HalfAnimationSelect == ANIM_HALF_RED_BLUE ? " selected" : "") + ">Three Quarters Trail</option>";
  html += "<option value='4'" + String(HalfAnimationSelect == ANIM_HALF_RED_YELLOW ? " selected" : "") + ">Three Quarters Trail</option>";
  html += "</select><br>";
  // AnimationThreeQuarters mode  
  html += "Plunger In ThreeQuarters LED Mode: <select name='threeQuartersUI'>";
  html += "<option value='0'" + String(threeQuartersAnimationSelect == ANIM3_FULL_OFF ? " selected" : "") + ">Three Quarters Off</option>";
  html += "<option value='1'" + String(threeQuartersAnimationSelect == ANIM3_RED_GREEN ? " selected" : "") + ">Three Quarters Ball Launch</option>";
  html += "<option value='2'" + String(threeQuartersAnimationSelect == ANIM3_FULL_RAINBOW ? " selected" : "") + ">Three Quarters Rainbow</option>";
  html += "<option value='3'" + String(threeQuartersAnimationSelect == ANIM3_PURPLE_PINK ? " selected" : "") + ">Three Quarters Fire</option>";
  html += "<option value='4'" + String(threeQuartersAnimationSelect == ANIM3_RED_YELLOW ? " selected" : "") + ">Three Quarters Trail</option>";
  html += "</select><br>";   
  // FullPull Mode
  html += "Plunger In Full Pull LED Mode: <select name='fullMode'>";
  html += "<option value='0'" + String(fullAnimationMode == ANIM_FULL_OFF ? " selected" : "") + ">Plunger Off</option>";
  html += "<option value='1'" + String(fullAnimationMode == ANIM_RED_GREEN ? " selected" : "") + ">Red + Green</option>";
  html += "<option value='2'" + String(fullAnimationMode == ANIM_FULL_RAINBOW ? " selected" : "") + ">Plunger Rainbow</option>";
  html += "<option value='3'" + String(fullAnimationMode == ANIM_PURPLE_PINK ? " selected" : "") + ">Purple + Pink</option>";
  html += "<option value='4'" + String(fullAnimationMode == ANIM_RED_BLUE ? " selected" : "") + ">RED + BLue</option>";
  html += "<option value='5'" + String(fullAnimationMode == ANIM_RED_YELLOW ? " selected" : "") + ">RED + YELLOW</option>";
  html += "</select><br>";

  // Save button
  html += "<input type='submit' value='Save'>";
  html += "</form></body></html>";

  return html;
}


// Function: Handle Root Request
void handleRoot() {
  server.send(200, "text/html", htmlPage());
}

// Function: Handle Save Request
void handleSave() {
 if (server.hasArg("potPin")) {
   potPin = server.arg("potPin").toInt();
 }
 
 if (server.hasArg("deadZone")) {
   deadZone = server.arg("deadZone").toInt();
 } 
 if (server.hasArg("WS2812BPin")) {
   WS2812BPin = server.arg("WS2812BPin").toInt();
 }
 if (server.hasArg("TotalLEDS")) {
   TotalLEDS = server.arg("TotalLEDS").toInt();
 }
 if (server.hasArg("lastLeds")) {
   TotalLEDS = server.arg("lastLeds").toInt();
 } 
 if (server.hasArg("restValue")) {
   restValue = server.arg("restValue").toInt();
 }
 if (server.hasArg("maxValue")) {
   maxValue = server.arg("maxValue").toInt();
 }
 if (server.hasArg("brightness")) {
   brightness = server.arg("brightness").toInt();
 }
// if (server.hasArg("mode")) {
//   currentMode = static_cast<LEDMode>(server.arg("mode").toInt()); // Ensure currentMode uses LEDMode
// }
  if (server.hasArg("quarterMode")) {
    quarterAnimationMode = static_cast<AnimationMode>(server.arg("quarterMode").toInt());
  } 
  if (server.hasArg("HALF_UI")) {
    HalfAnimationSelect = static_cast<HalfAnimationList>(server.arg("HALF_UI").toInt()); 
      
  } 
  if (server.hasArg("threeQuartersUI")) {
    threeQuartersAnimationSelect = static_cast<ThreeQuartersAnimationList>(server.arg("threeQuartersUI").toInt()); 
      
  } 
  if (server.hasArg("fullMode")) {
    fullAnimationMode = static_cast<FullPullMode>(server.arg("fullMode").toInt());
 }
// Save preferences and restart
 savePreferences();
 server.send(200, "text/html", "<h1>Settings Saved!</h1><a href='/'>Go Back</a>");
 ESP.restart();
}



    

// Function: Setup
void setup() {
  Serial.begin(115200);

  pinMode(potPin, INPUT);
  pinMode(WS2812BPin, INPUT);
 
  
  preferences.begin("plunger-config", false);
  loadPreferences();

  strip.begin();
  strip.setBrightness(brightness);
  strip.show();


  WiFi.softAP(ssid, password);
  server.on("/", handleRoot);
  server.on("/save", handleSave);
  server.on("/setQuarterAnimation", handleSetQuarterAnimation);
  server.on("/SethandleSetHALFAnimation", handleSetHALFAnimation);  
  server.on("/SetThreeQuartersAnimation", handleSetThreeQuartersAnimation);  
  server.on("/setFullAnimation", handleSetFullAnimation);

  server.begin();
  Serial.printf("deadZone, %d\n",deadZone);
  Serial.println("WebServer Started\n");
  Serial.printf("Connect to Wi-Fi: %s, Access: http://192.168.4.1/\n", ssid);
  Serial.printf("Loaded Preferences - Brightness: %d, quarterAnimationMode: %d, fullAnimationMode: %d\n", brightness,quarterAnimationMode,fullAnimationMode);
  // Start the gamepad = bleGamepad (example)
  Serial.println("Starting BLE work!");
  bleGamepadConfig.setAutoReport(false);
  bleGamepadConfig.setButtonCount(1);  
  bleGamepadConfig.setControllerType(CONTROLLER_TYPE_GAMEPAD); // CONTROLLER_TYPE_JOYSTICK, CONTROLLER_TYPE_GAMEPAD (DEFAULT), CONTROLLER_TYPE_MULTI_AXIS
  // You can freely set the PID
  bleGamepadConfig.setPid(0x0099);  
  bleGamepadConfig.setModelNumber("1.0");
  bleGamepadConfig.setSoftwareRevision("Software Rev 1");
  bleGamepadConfig.setSerialNumber("9876543210");
  bleGamepadConfig.setFirmwareRevision("2.0");
  bleGamepadConfig.setHardwareRevision("1.7");
  Serial.println("BLE Gamepad started!");
  gamepad.begin(&bleGamepadConfig);

  
                      
}

unsigned long previousMillis = 0;
const unsigned long interval = 10;

// Function: Main Loop
void loop() {
  unsigned long currentMillis = millis();
  if (currentMillis - previousMillis >= interval) {
    previousMillis = currentMillis;
  // Start Webserver
  server.handleClient();
  // SMOOTH VALUE
  int smoothedValue = readSmoothedPotValue();
  float plungerValue = getPlungerValue(smoothedValue);
  // Apply led changes
  applyLEDMode(smoothedValue); // Pass the smoothedValue to determine animation


    if (gamepad.isConnected()){
      gamepad.setAxes(0, 0, (int16_t)(plungerValue * 32767)); // X, Y, Z axes
      gamepad.sendReport(); 
  }   
 }
}

   
