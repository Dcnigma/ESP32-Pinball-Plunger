#include <WiFi.h>
#include <WebServer.h>
#include <Preferences.h>
#include <BLEDevice.h>
#include <BLEHIDDevice.h>
#include <HIDTypes.h>

// Wi-Fi Credentials
const char* ssid = "ESP32_Plunger";
const char* password = "12345678";

// Potentiometer Configuration (default values)
int potPin = 34;
int restValue = 2300;
int maxValue = 2700;
const int deadZone = 10;          // Dead zone range for stable detection
const int smoothingWindow = 5;   // Number of samples for smoothing

// WebServer
WebServer server(80);
Preferences preferences;

// BLE HID Device
BLEHIDDevice* hid;
BLECharacteristic* inputReport;

// Smoothing Variables
int potValues[smoothingWindow] = {0};
int potIndex = 0;

// BLE Server Callback Class
class ServerCallbacks : public BLEServerCallbacks {
  void onConnect(BLEServer* server) override {
    Serial.println("Client Connected");
  }

  void onDisconnect(BLEServer* server) override {
    Serial.println("Client Disconnected");
    BLEDevice::getAdvertising()->start();
    Serial.println("Advertising Restarted");
  }
};

// Function: Setup BLE HID Gamepad
void setupBluetoothHID() {
  BLEDevice::init("ESP32 Pinball Plunger");

  BLEServer* server = BLEDevice::createServer();
  server->setCallbacks(new ServerCallbacks()); // Use the named callback class

  hid = new BLEHIDDevice(server);
  inputReport = hid->inputReport(1);

  // Set manufacturer and device info
  hid->manufacturer()->setValue("ESP32 Pinball");
  hid->pnp(0x02, 0xe502, 0xa111, 0x0210);
  hid->hidInfo(0x00, 0x01);

  // Set the report map
  const uint8_t reportMap[] = {
    0x05, 0x01, 0x09, 0x05, 0xA1, 0x01, 0x85, 0x01, 0x09, 0x30,
    0x09, 0x31, 0x09, 0x32, 0x15, 0x00, 0x26, 0xFF, 0x00, 0x75,
    0x08, 0x95, 0x03, 0x81, 0x02, 0xC0
  };
  hid->reportMap((uint8_t*)reportMap, sizeof(reportMap));
  hid->startServices();

  // Start advertising
  BLEAdvertising* advertising = BLEDevice::getAdvertising();
  advertising->setAppearance(HID_GAMEPAD);
  advertising->addServiceUUID(hid->hidService()->getUUID());
  advertising->start();

  Serial.println("Bluetooth HID Gamepad Initialized.");
}

// Function: Read and Smooth Potentiometer Values
int readSmoothedPotValue() {
  // Read the current potentiometer value
  int currentValue = analogRead(potPin);

  // Add the current value to the smoothing array
  potValues[potIndex] = currentValue;
  potIndex = (potIndex + 1) % smoothingWindow;

  // Calculate the average of the smoothing array
  int smoothedValue = 0;
  for (int i = 0; i < smoothingWindow; i++) {
    smoothedValue += potValues[i];
  }
  smoothedValue /= smoothingWindow;

  return smoothedValue;
}

// Function: Calculate Z-Axis Value
float getPlungerValue(int smoothedValue) {
  // Map and constrain the smoothed potentiometer value
  float mappedValue = map(smoothedValue, restValue, maxValue, -100, 100) / 100.0;
  mappedValue = constrain(mappedValue, -1.0, 1.0);

  // Apply dead zone
  if (abs(mappedValue) < (float)deadZone / 100.0) {
    mappedValue = 0.0; // Set to 0 if within the dead zone
  }

  Serial.printf("Pot Value: %d, Smoothed Value: %d, Mapped Z: %.2f\n", analogRead(potPin), smoothedValue, mappedValue);
  return mappedValue;
}

// Function: Send Gamepad Report
void sendGamepadReport(float zAxisValue) {
  uint8_t report[3] = {127, 127, (uint8_t)((zAxisValue + 1.0) * 127)};
  inputReport->setValue(report, sizeof(report));
  inputReport->notify();
  Serial.printf("Sending Report - Z: %d\n", report[2]);
}

// Function: Web Page HTML
String htmlPage() {
  return String(
    "<html>"
    "<head><title>ESP32 Plunger Config</title></head>"
    "<body>"
    "<h1>Pinball Plunger Configuration</h1>"
    "<form action='/save' method='POST'>"
    "Potentiometer Pin: <input type='number' name='potPin' value='" + String(potPin) + "'><br>"
    "Rest Value: <input type='number' name='restValue' value='" + String(restValue) + "'><br>"
    "Max Value: <input type='number' name='maxValue' value='" + String(maxValue) + "'><br>"
    "<input type='submit' value='Save'>"
    "</form>"
    "</body></html>"
  );
}

// Function: Handle Root Request
void handleRoot() {
  server.send(200, "text/html", htmlPage());
}

// Function: Handle Save Request
void handleSave() {
  if (server.hasArg("potPin") && server.hasArg("restValue") && server.hasArg("maxValue")) {
    potPin = server.arg("potPin").toInt();
    restValue = server.arg("restValue").toInt();
    maxValue = server.arg("maxValue").toInt();

    preferences.putInt("potPin", potPin);
    preferences.putInt("restValue", restValue);
    preferences.putInt("maxValue", maxValue);

    server.send(200, "text/html", "<h1>Settings Saved!</h1><a href='/'>Go Back</a>");
    ESP.restart(); // Restart to apply changes
  } else {
    server.send(400, "text/html", "<h1>Invalid Input!</h1><a href='/'>Go Back</a>");
  }
}

// Function: Load Saved Settings
void loadSettings() {
  potPin = preferences.getInt("potPin", potPin);
  restValue = preferences.getInt("restValue", restValue);
  maxValue = preferences.getInt("maxValue", maxValue);
  Serial.printf("Loaded Settings - potPin: %d, restValue: %d, maxValue: %d\n", potPin, restValue, maxValue);
}

// Function: Setup
void setup() {
  Serial.begin(115200);
  pinMode(potPin, INPUT);

  preferences.begin("plunger-config", false);
  loadSettings();

  setupBluetoothHID();

  WiFi.softAP(ssid, password);
  server.on("/", handleRoot);
  server.on("/save", handleSave);
  server.begin();

  Serial.println("WebServer Started");
  Serial.printf("Connect to Wi-Fi: %s, Access: http://192.168.4.1/\n", ssid);
}

// Loop
void loop() {
  server.handleClient();

  // Read smoothed value and calculate Z-axis
  int smoothedValue = readSmoothedPotValue();
  float plungerValue = getPlungerValue(smoothedValue);

  // Send the gamepad report
  sendGamepadReport(plungerValue);

  delay(50); // Adjust delay as needed
}
