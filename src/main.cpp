#include <Arduino.h>
#include <SPI.h>
#include <MFRC522.h>
#include "variant.h"

SPIClass rfidSPI(HSPI);
MFRC522 rfid(RC522_SS_PIN, RC522_RST_PIN);

void sendToMeshtastic(const String& msg) {
    Serial2.println(msg);
}

void setup() {
    Serial.begin(115200);
    Serial2.begin(115200);
    rfidSPI.begin(RC522_SCK_PIN, RC522_MISO_PIN, RC522_MOSI_PIN, RC522_SS_PIN);
    rfid.PCD_Init();
}

void loop() {
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
}
