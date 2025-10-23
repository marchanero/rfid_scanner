#include <Arduino.h>
// RFID RC522 SPI for Wemos D1 mini - Optimized for MERN integration

#include <SPI.h>
#include <MFRC522.h>

#define RST_PIN   2   // GPIO2 (D4)
#define SS_PIN    15  // GPIO15 (D8)

MFRC522 mfrc522(SS_PIN, RST_PIN);

void setup() {
  Serial.begin(115200);
  delay(100);

  SPI.begin();
  pinMode(SS_PIN, OUTPUT);
  pinMode(RST_PIN, OUTPUT);
  digitalWrite(SS_PIN, HIGH);
  digitalWrite(RST_PIN, HIGH);

  // Hardware reset for clones
  digitalWrite(RST_PIN, LOW);
  delay(50);
  digitalWrite(RST_PIN, HIGH);
  delay(50);

  mfrc522.PCD_Init();
  mfrc522.PCD_AntennaOn();
  mfrc522.PCD_SetAntennaGain(mfrc522.RxGain_38dB);

  byte version = mfrc522.PCD_ReadRegister(MFRC522::VersionReg);
  if (version == 0x00 || version == 0xFF) {
    Serial.println("{\"event\":\"error\",\"message\":\"RC522 init failed\"}");
  } else {
    Serial.println("{\"event\":\"ready\"}");
  }
}

void loop() {
  // Check for new cards
  bool cardPresent = mfrc522.PICC_IsNewCardPresent();

  if (cardPresent) {
    // Intentar leer UID con reintentos cortos (algunos clones requieren varios intentos)
    bool readSerial = false;
    const int maxAttempts = 3;
    for (int attempt = 1; attempt <= maxAttempts; ++attempt) {
      readSerial = mfrc522.PICC_ReadCardSerial();
      if (readSerial) break;
      delay(50); // Pequeño delay entre intentos
    }

    if (readSerial) {
      // Enviar UID en formato JSON para fácil parsing en backend
      Serial.print("{\"event\":\"card_detected\",\"uid\":\"");
      for (byte i = 0; i < mfrc522.uid.size; i++) {
        if (mfrc522.uid.uidByte[i] < 0x10) Serial.print("0");
        Serial.print(mfrc522.uid.uidByte[i], HEX);
      }
      Serial.println("\"}");

      // Halt PICC
      mfrc522.PICC_HaltA();
      // Stop encryption on PCD
      mfrc522.PCD_StopCrypto1();

      delay(2000); // Pausa antes de buscar otra
    }
    else {
      // Si falla PICC_ReadCardSerial, intentar anticollision crudo
      byte acCmd[2] = { 0x93, 0x20 };
      byte backLen = 10;
      byte backBuf[10];
      byte validBits = 0;
      MFRC522::StatusCode st = mfrc522.PCD_TransceiveData(acCmd, 2, backBuf, &backLen, &validBits, 0, false);
      if (st == MFRC522::STATUS_OK && backLen == 5) {
        // Reconstruir UID
        for (byte i = 0; i < 4; i++) {
          mfrc522.uid.uidByte[i] = backBuf[i];
        }
        mfrc522.uid.size = 4;
        mfrc522.uid.sak = 0x08;

        // Enviar UID reconstruido en JSON
        Serial.print("{\"event\":\"card_detected\",\"uid\":\"");
        for (byte i = 0; i < mfrc522.uid.size; i++) {
          if (mfrc522.uid.uidByte[i] < 0x10) Serial.print("0");
          Serial.print(mfrc522.uid.uidByte[i], HEX);
        }
        Serial.println("\"}");

        // Halt PICC
        mfrc522.PICC_HaltA();
        mfrc522.PCD_StopCrypto1();
        delay(2000);
      }
    }
  } else {
    delay(100);
  }
}