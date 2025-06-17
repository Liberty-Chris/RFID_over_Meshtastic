# Modifying Meshtastic Firmware for Dual SPI on Heltec V3 (LoRa + MFRC522)

This guide explains how to modify your own firmware to enable simultaneous operation of the onboard LoRa transceiver and an MFRC522 RFID reader using separate SPI buses on the Heltec V3. After struggling with this personally and discovering that many people were running into problems and resorting to removing the main chip on the MFRC522 in order to cut traces and modify the PCB for I2C and UART operation, I figured there had to be a better way. This was my solution.

This documentation walks you through how to integrate the necessary pieces into your existing project, define a second SPI bus, configure pin assignments, and build RFID-based messages that can be transmitted over Meshtastic.

## Quick Start

1. **Wire the MFRC522 module** to the Heltec V3 using the default pins:

   | MFRC522 pin | Heltec V3 pin |
   |-------------|---------------|
   | SCK         | GPIO7         |
   | MOSI        | GPIO6         |
   | MISO        | GPIO5         |
   | SDA/SS      | GPIO26        |
   | RST         | GPIO4         |
   | 3.3&nbsp;V    | 3V3           |
   | GND         | GND           |

   *(See `docs/wiring.png` for a simple wiring diagram.)*

2. **Install libraries** – PlatformIO will automatically fetch the `MFRC522`
   library and the Meshtastic bridge library as specified in `platformio.ini`.

3. **Build and upload** the firmware:

   ```bash
   pio run            # compile
   pio run -t upload  # flash to the device
   ```
## Overview

Default SPI Bus (VSPI): Reserved for the onboard LoRa radio (Heltec default).
Secondary SPI Bus (HSPI): Assigned to the MFRC522 RFID reader.
Custom SPI Bus Name: `rfidSPI`
Board: Heltec ESP32 LoRa V3 

## Defining the RFID SPI Bus

To use a custom SPI bus for the MFRC522 reader, define a new `SPIClass` instance in your source and assign it the `HSPI` hardware bus:

```cpp
SPIClass rfidSPI(HSPI);
```

In your RFID driver header (e.g. `MFRC522.h`), ensure this is respected:

```cpp
#ifndef MFRC522_SPI
#define MFRC522_SPI SPI
#endif
extern SPIClass MFRC522_SPI;
```

Override the SPI instance in your `platformio.ini`:

```ini
[env:heltec-v3]
build_flags =
  -DMFRC522_SPI=rfidSPI
```

## Pin Definitions

Default pins for HSPI operation can be defined in your variant or overridden per build:

```ini
[env:heltec-v3]
build_flags =
  -DRC522_SCK_PIN=7
  -DRC522_MOSI_PIN=6
  -DRC522_MISO_PIN=5
  -DRC522_SS_PIN=26
  -DRC522_RST_PIN=4
```

Or default to the values below if not overridden (my curent usage):

```cpp
#define RC522_SCK_PIN   7
#define RC522_MOSI_PIN  6
#define RC522_MISO_PIN  5
#define RC522_SS_PIN    26
#define RC522_RST_PIN   4
```

## Initialization in Your Firmware

Declare the RFID reader and bus:

```cpp
SPIClass rfidSPI(HSPI);
MFRC522 rfid(RC522_SS_PIN, RC522_RST_PIN);
```

Set up the bus and initialize the reader:

```cpp
rfidSPI.begin(RC522_SCK_PIN, RC522_MISO_PIN, RC522_MOSI_PIN, RC522_SS_PIN);
rfid.PCD_Init();
```

## Reading Tags and Building Messages

In your loop or polling section, check for tags and construct a message:

```cpp
if (rfid.PICC_IsNewCardPresent() && rfid.PICC_ReadCardSerial()) {
  String uidStr = "";
  for (byte i = 0; i < rfid.uid.size; i++) {
    uidStr += String(rfid.uid.uidByte[i], HEX);
  }
  String payload = "{\"scanner_id\": \"RFID01\", \"tag_uid\": \"" + uidStr + "\", \"timestamp\": " + String(millis()) + "}";
  sendToMeshtastic(payload);
  rfid.PICC_HaltA();
  rfid.PCD_StopCrypto1();
}
```

This sends a structured JSON payload like:

```json
{
  "scanner_id": "RFID01",
  "tag_uid": "f41298ab",
  "timestamp": 172344123
}
```

## Sending Data Over Meshtastic

Assuming Meshtastic is running and the serial port is active, you can send messages via Python or C++ API bindings. Here's an example C++ stub if you're bridging through serial:

```cpp
void sendToMeshtastic(const String& msg) {
  Serial2.println(msg); // or write to the Meshtastic port
}
```

## Dependencies

* MFRC522 library (or compatible fork)
* SPI and Arduino libraries
* Meshtastic Serial Bridge or native integration
* `platformio.ini` macro overrides for pin and SPI assignment

The ESP32-S3 supports multiple SPI buses. By separating the MFRC522 onto its own bus (HSPI) while leaving LoRa on VSPI, you maintain performance and reliability without hardware modifications or mode switching. This keeps your firmware clean and modular.

## LED Feedback and Scan Debounce (optional but useful)

After each successful tag read, the built-in LED is blinked three times to provide immediate user feedback. To avoid spamming the same UID, scans are ignored until a configurable delay has passed and the UID differs from the previous read.

```cpp
const unsigned long SCAN_DELAY_MS = 2000;
unsigned long lastScanTime = 0;
String lastUid = "";

void loop() {
  if (rfid.PICC_IsNewCardPresent() && rfid.PICC_ReadCardSerial()) {
    String uid = "";
    for (byte i = 0; i < rfid.uid.size; i++) {
      uid += String(rfid.uid.uidByte[i], HEX);
    }
    if (millis() - lastScanTime > SCAN_DELAY_MS && uid != lastUid) {
      // send payload here
      for (int i = 0; i < 3; i++) {
        digitalWrite(LED_PIN, HIGH);
        delay(100);
        digitalWrite(LED_PIN, LOW);
        delay(100);
      }
      lastScanTime = millis();
      lastUid = uid;
    }
    rfid.PICC_HaltA();
    rfid.PCD_StopCrypto1();
  }
}
```


