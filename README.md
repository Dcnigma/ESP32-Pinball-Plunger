# ESP32 Pinball Plunger

An innovative solution to integrate an analog plunger into digital pinball games. This project utilizes an ESP32 microcontroller to convert potentiometer input into gamepad Z-axis movements over Bluetooth, allowing precise control of pinball plungers.
![alt text](https://github.com/Dcnigma/ESP32-Pinball-Plunger/blob/main/Pictures/IMG_3422.JPG?raw=true)

## Features

- Converts potentiometer values to Z-axis input for Bluetooth gamepads.
- Configurable via a built-in web server hosted on the ESP32.
- Smoothing and dead zone adjustment for stable and accurate control.
- Automatically restarts Bluetooth advertising when disconnected.
- Fully customizable potentiometer pin and value ranges.

## Requirements

- ESP32 microcontroller.
- Potentiometer (e.g., linear slider or rotary).
- A Windows or other Bluetooth-compatible device for connecting the ESP32.
- Optional: AutoHotkey (if you wish to map gamepad inputs to mouse movements).

## How It Works

1. The potentiometer reads the plunger's position and sends smoothed, mapped values to the ESP32.
2. The ESP32 acts as a Bluetooth HID device, transmitting Z-axis changes to the connected computer.
3. A web-based configuration page allows adjustments to the potentiometer settings.
4. Optional: Use AutoHotkey scripts to convert gamepad Z-axis inputs into mouse movements if required by specific pinball games.

## Setup

### Hardware

1. Connect the potentiometer to the ESP32:
   - **Signal** pin to the configured ADC pin (default: GPIO 34).
   - **Power** and **Ground** to appropriate ESP32 pins.

2. Power the ESP32 via USB or another suitable power source.

### Software

1. Install the Arduino IDE and set up ESP32 board support:
   - Follow the [ESP32 Board Installation Guide](https://docs.espressif.com/projects/arduino-esp32/en/latest/installing.html).

2. Clone this repository and open the `.ino` file in Arduino IDE.
3. Update the `ssid` and `password` variables with desired Wi-Fi credentials for the ESP32's web server.
4. Upload the code to the ESP32.

### Configuration
![alt text](https://github.com/Dcnigma/ESP32-Pinball-Plunger/blob/main/Pictures/WEBUI%20ESP32%20Plunger.png?raw=true)
1. Connect to the Wi-Fi network created by the ESP32 (`ESP32_Plunger` by default).
2. Access the web server at `http://192.168.4.1/`.
3. Configure the potentiometer pin, rest value, and max value as needed.
4. Save the settings to restart the ESP32 with the new configuration.

## Usage

1. Pair the ESP32 as a Bluetooth device on your computer.
2. Open your pinball game and map the plunger's Z-axis to the appropriate in-game controls.
3. For games that require mouse input for the plunger:
   - Install [AutoHotkey](https://www.autohotkey.com/).
   - Use an AutoHotkey script to map the Z-axis to mouse movements.

## Example AutoHotkey Script

```ahk
; Map gamepad Z-axis to mouse vertical movements
#Persistent
SetTimer, CheckJoystick, 10

CheckJoystick:
JoyZ := GetKeyState("JoyZ")
MouseMove, 0, JoyZ * 5 ; Adjust multiplier for sensitivity
return
```

## Troubleshooting

- **Bluetooth Issues**:
  - Ensure the ESP32 is powered and advertising.
  - Unpair and re-pair the device if it does not respond after a reboot.

- **Potentiometer Not Detected**:
  - Check wiring and ensure the correct GPIO pin is configured.

## Contributing

Contributions are welcome! Feel free to submit issues or pull requests.

## License

This project is licensed under the MIT License. See the LICENSE file for details.

## Acknowledgments

Special thanks to the pinball community for inspiring this project and to the developers of libraries that make ESP32 Bluetooth HID devices possible.
