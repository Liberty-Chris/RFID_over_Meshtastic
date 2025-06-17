#include <Arduino.h>
#include <SPI.h>
#include <MFRC522.h>
#include "variant.h"

SPIClass rfidSPI(HSPI);
MFRC522 rfid(RC522_SS_PIN, RC522_RST_PIN);

const unsigned long SCAN_DELAY_MS = 2000;
unsigned long lastScanTime = 0;
String lastUid = "";

void sendToMeshtastic(const String& msg) {
    Serial2.println(msg);
}

void setup() {
    Serial.begin(115200);
    Serial2.begin(115200);
    pinMode(LED_PIN, OUTPUT);
    digitalWrite(LED_PIN, LOW);
    rfidSPI.begin(RC522_SCK_PIN, RC522_MISO_PIN, RC522_MOSI_PIN, RC522_SS_PIN);
    rfid.PCD_Init();
}

void loop() {
    if (rfid.PICC_IsNewCardPresent() && rfid.PICC_ReadCardSerial()) {
        String uidStr = "";
        for (byte i = 0; i < rfid.uid.size; i++) {
            uidStr += String(rfid.uid.uidByte[i], HEX);
        }

        if (millis() - lastScanTime > SCAN_DELAY_MS && uidStr != lastUid) {
            String payload = "{\"scanner_id\": \"RFID01\", \"tag_uid\": \"" + uidStr + "\", \"timestamp\": " + String(millis()) + "}";
            sendToMeshtastic(payload);

            for (int i = 0; i < 3; i++) {
                digitalWrite(LED_PIN, HIGH);
                delay(100);
                digitalWrite(LED_PIN, LOW);
                delay(100);
            }

            lastScanTime = millis();
            lastUid = uidStr;
        }

        rfid.PICC_HaltA();
        rfid.PCD_StopCrypto1();
    }
}
