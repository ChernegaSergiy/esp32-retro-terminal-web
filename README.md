# ESP32 Retro Terminal Web Interface

[![License: CSSM Unlimited License v2.0](https://img.shields.io/badge/License-CSSM%20Unlimited%20License%20v2.0-blue.svg?logo=opensourceinitiative)](LICENSE)
[![Platform: ESP32](https://img.shields.io/badge/Platform-ESP32-E7352C.svg?logo=espressif)](https://www.espressif.com/)

This project implements a lightweight web server for the ESP32 microcontroller featuring a stylized **Unix-like TUI (Text User Interface)**. It allows users to control GPIO pins and configure Wi-Fi credentials via a web browser while mimicking the aesthetic of a CRT monitor running a Linux terminal.

## Interface Preview

The UI is designed with a specific "Cyberpunk/Linux" console aesthetic:

- **Visual Style:** CRT scanline effects, glowing text, and high-contrast Green (#00FF00) on Black.
- **Elements:** Simulated Unix prompts (`root@esp32:~#`), `htop`-style status reporting, and `vi`-style configuration forms.
- **Dynamic Data:** Real-time RSSI scanning (`iwlist`) and system uptime monitoring.

> **Note:** [Insert a screenshot of your web interface here]

## Features

- **SoftAP Configuration Portal:** Connect to the ESP32 wirelessly to scan for networks and save credentials without recompiling code.
- **Persistent Storage:** Wi-Fi credentials are saved to the ESP32's non-volatile storage (NVS) using `Preferences.h`.
- **CRT Simulation:** CSS-based scanlines and blinking block cursors for authentic retro feel.
- **Hardware Control:** Toggle LED states using simulated terminal commands.
- **Physical Reset:** Hold the BOOT button to force the device back into Configuration Mode.

## Hardware Requirements

- ESP32 Development Board (e.g., ESP32 DevKit V1).
- Built-in LED (GPIO 2) or external LED.
- BOOT Button (GPIO 0) for resetting network settings.

## Installation

To deploy the terminal uplink and initialize the system, proceed with the following sequence:

1. Clone the repository:
   ```bash
   git clone https://github.com/ChernegaSergiy/esp32-retro-terminal-web.git
   ```
2. Open the project in Arduino IDE or PlatformIO.
3. Upload the code to your ESP32.

> [!NOTE]
> You do not need to hardcode Wi-Fi credentials in the code anymore.

## Network Configuration

Upon the first launch (or if no Wi-Fi is configured), the device acts as a Hotspot:

1. Connect your phone/PC to the Wi-Fi network: `esp32_tty`.
2. Password: `root1234`.
3. Open a browser and navigate to `192.168.4.1`.
4. You will see a simulated `iwlist` scan of available networks.
5. Enter your SSID and Password in the `vi`-style prompts and click `[ :wq! ]`.
6. The device will reboot and connect to your local network.

**To reset Wi-Fi settings:** Hold the **BOOT** button (GPIO 0) on the ESP32 for **3 seconds** until the LED blinks.

## Code Overview

The HTML is generated dynamically using raw string literals to create the TUI structure:

```cpp
// Example: Generating the 'htop' style status page
String content = R"rawliteral(
<pre>
<span class="prompt">root@esp32:~#</span> htop -b -n 1
MEM_FREE: )rawliteral" + String(free_heap) + R"rawliteral( bytes
UPTIME:   )rawliteral" + String(millis()/1000) + R"rawliteral( sec

<span class="prompt">root@esp32:~#</span> service led status
Status: )rawliteral" + led_visual + R"rawliteral(
</pre>
)rawliteral";
```

## Contributing

Contributions are welcome and appreciated! Here's how you can contribute:

1. Fork the project
2. Create your feature branch (`git checkout -b feature/AmazingFeature`)
3. Commit your changes (`git commit -m 'Add some AmazingFeature'`)
4. Push to the branch (`git push origin feature/AmazingFeature`)
5. Open a Pull Request

Please make sure to update tests as appropriate and adhere to the existing coding style.

## License

This project is licensed under the CSSM Unlimited License v2.0 (CSSM-ULv2). See the [LICENSE](LICENSE) file for details.
