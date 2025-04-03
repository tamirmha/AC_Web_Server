# WebServer with BLE Multi Client

## Overview

This project is designed to control and monitor air conditioning (AC) and damper devices using an ESP32-S3 microcontroller. It combines BLE (Bluetooth Low Energy) multi-client functionality and a WebSocket server to enable real-time communication with connected devices. The project also includes 3D printable case designs for housing the hardware.

The BLE peripheral code used in this project is available in the repository: [AC-Control](https://github.com/tamirmha/AC-Control).

## Developer

Developed by Tamir Mhabary

## Features

- WebSocket server for real-time communication with clients.
- BLE multi-client functionality to interact with multiple BLE peripherals.
- SPIFFS for saving and loading device states.
- Integration with Adafruit NeoPixel for status indication.
- Control and monitor AC and damper devices via WebSocket and BLE.

## Folder Structure

The project is organized as follows:
```
data/
├── index.html                # Web interface for the WebSocket server
lib/
├── ble_multi_Client/
│   ├── ble_multi_client.cpp  # BLE multi-client implementation
│   └── ble_multi_client.h    # BLE multi-client header
├── websocket_server/
│   └── websocket_server.h    # WebSocket server logic
src/
├── main.cpp                  # Main application code
platformio.ini                # PlatformIO configuration file
Readme.md                     # Project documentation
.gitignore                    # Git ignore file
```

## 3D Printed Parts

The project includes 3D printable STL files for the custom case required for the setup.  
These files, along with their images, are located in the `3d` folder.

### Preview of 3D Parts

 **Case**  

  View: 3D isometric view  
  ![Case Top](3d/Case.png)

  View: Board without top part  
  ![Case Bottom](3d/Case_w_out_top.png)

## Prerequisites

- ESP32-S3 16MB development board.
- PlatformIO IDE for development.
- Required libraries:
  - `NimBLE-Arduino`
  - `ESPAsyncWebServer`
  - `WebSocketsServer`
  - `Adafruit NeoPixel`
- Wi-Fi network for WebSocket communication.
- BLE-enabled devices for interaction.

## Installation

1. Clone the repository to your local machine.
2. Open the project in PlatformIO IDE.
3. Ensure the `platformio.ini` file is configured for the ESP32-S3 16MB board:
   ```
   [env:4d_systems_esp32s3_gen4_r8n16]
   board = 4d_systems_esp32s3_gen4_r8n16 
   platform = espressif32
   framework = arduino
   ```
4. Install the required libraries via PlatformIO's Library Manager.
5. Upload the code to your ESP32-S3 board.

## Usage

1. Power on the ESP32-S3 board.
2. The WebSocket server will start and connect to the specified Wi-Fi network.
3. Access the WebSocket server via the IP address displayed in the serial monitor.
4. Control and monitor the AC and damper devices using WebSocket messages:
   - `toggle_damperX` (e.g., `toggle_damper1`) to toggle damper states.
   - `power_damperX:state` (e.g., `power_damper1:high`) to set damper power.
   - `toggle_ac` to toggle AC state.
   - `power_ac:state` (e.g., `power_ac:low`) to set AC power.
   - `set_ac_mode:mode` (e.g., `set_ac_mode:heat`) to set AC mode.
   - `set_ac_temp:temp` (e.g., `set_ac_temp:24`) to set AC temperature.

## Contributions

Contributions are welcome! Please fork the repository and submit a pull request.

## License

This project is licensed under the MIT License. See the LICENSE file for details.
