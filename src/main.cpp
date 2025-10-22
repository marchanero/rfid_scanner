#include <Arduino.h>
// RFID RC522 (SPI) example for Wemos D1 mini (ESP8266)
// Using MakerSpaceLeiden library for better clone support

#include <SPI.h>
#include <MFRC522.h>

// Pines SPI: usar números GPIO para evitar confusiones D# vs GPIO#
// Muchos usuarios confunden Dx con GPIOx. Según tu mapeo original los pines físicos
// que usaste parecen ser: RST -> D4 (GPIO2) y SS -> D8 (GPIO15).
// Vamos a probar con esa asignación (si tu hardware está cableado con D# labels, usa D4/D8):
#define RST_PIN   2   // GPIO2 (D4 on Wemos)
#define SS_PIN    15  // GPIO15 (D8 on Wemos)

// Create an SPI device instance (usar 1 MHz primero para diagnóstico)
MFRC522_SPI spiDev(SS_PIN, RST_PIN, &SPI, SPISettings(1000000, MSBFIRST, SPI_MODE0));
// Pass the bus device to the high-level class
MFRC522 mfrc522(&spiDev);

/**
 * Helper routine to dump a byte array as hex values to Serial.
 */
void dump_byte_array(byte *buffer, byte bufferSize) {
  for (byte i = 0; i < bufferSize; i++) {
    Serial.print(buffer[i] < 0x10 ? " 0" : " ");
    Serial.print(buffer[i], HEX);
  }
}

// Forward declarations for diagnostic routines (defined below)
void runREQATest();
void runForceTxCombos();
// Forward declaration for status string helper (used in diagnostic anticollision)
const char* statusCodeToStr(MFRC522::StatusCode code);

void setup() {
  Serial.begin(115200); // Initialize serial communications with the PC
  delay(100);
  Serial.println("RC522 SPI example - Wemos D1 mini (diagnostico avanzado)");

  SPI.begin();        // Init SPI bus

  Serial.println("Inicializando RC522 via SPI...");

  // Configurar pines SS/RST como salidas y forzar niveles por defecto
  pinMode(SS_PIN, OUTPUT);
  pinMode(RST_PIN, OUTPUT);
  digitalWrite(SS_PIN, HIGH); // deselect
  digitalWrite(RST_PIN, HIGH); // no en reset

  // Forzar ciclo hardware de reset: algunos clones requieren reset físico
  Serial.println("Forzando ciclo hardware de RST...");
  digitalWrite(RST_PIN, LOW);
  delay(50);
  digitalWrite(RST_PIN, HIGH);
  delay(50);

  mfrc522.PCD_Init(); // Init MFRC522 card

  // Asegurar que la antena esté encendida (algunos clones no activan por defecto)
  Serial.println("Activando antena (PCD_AntennaOn)");
  mfrc522.PCD_AntennaOn();
  // Forzar bits TX control (si la antena sigue sin salir)
  byte txc = mfrc522.PCD_ReadRegister(MFRC522::TxControlReg);
  Serial.print("TxControlReg antes: 0x"); Serial.println(txc, HEX);
  mfrc522.PCD_WriteRegister(MFRC522::TxControlReg, txc | 0x03);
  txc = mfrc522.PCD_ReadRegister(MFRC522::TxControlReg);
  Serial.print("TxControlReg despues: 0x"); Serial.println(txc, HEX);

  // Aumentar ganancia de antena para mejor lectura
  mfrc522.PCD_SetAntennaGain(mfrc522.RxGain_max);

  // Leer versión
  byte version = mfrc522.PCD_ReadRegister(MFRC522::VersionReg);
  if (version == 0x00 || version == 0xFF) {
    Serial.println("ERROR: no se pudo inicializar el lector RC522 por SPI. Verifica conexiones.");
  } else {
    Serial.println("RC522 inicializado correctamente.");
    mfrc522.PCD_DumpVersionToSerial();

    // Volcar registros clave para diagnóstico (lecturas repetidas)
    Serial.println("Volcando registros MFRC522 para diagnóstico (3 ciclos)...");
    for (int cycle = 0; cycle < 3; cycle++) {
      byte v;
      Serial.print("Ciclo "); Serial.println(cycle + 1);
      v = mfrc522.PCD_ReadRegister(MFRC522::VersionReg);
      Serial.print(" VersionReg: 0x"); Serial.println(v, HEX);
      v = mfrc522.PCD_ReadRegister(MFRC522::TxControlReg);
      Serial.print(" TxControlReg: 0x"); Serial.println(v, HEX);
      v = mfrc522.PCD_ReadRegister(MFRC522::TxASKReg);
      Serial.print(" TxASKReg: 0x"); Serial.println(v, HEX);
      v = mfrc522.PCD_ReadRegister(MFRC522::ModeReg);
      Serial.print(" ModeReg: 0x"); Serial.println(v, HEX);
      v = mfrc522.PCD_ReadRegister(MFRC522::TxModeReg);
      Serial.print(" TxModeReg: 0x"); Serial.println(v, HEX);
      v = mfrc522.PCD_ReadRegister(MFRC522::RxModeReg);
      Serial.print(" RxModeReg: 0x"); Serial.println(v, HEX);
      v = mfrc522.PCD_ReadRegister(MFRC522::RFCfgReg);
      Serial.print(" RFCfgReg: 0x"); Serial.println(v, HEX);
      delay(100);
    }

    // Intentar autotest si disponible (algunos clones no implementan)
    Serial.println("Realizando auto-prueba del MFRC522 (si está soportada)...");
    bool selfTest = mfrc522.PCD_PerformSelfTest();
    if (selfTest) {
      Serial.println("Auto-prueba PASADA.");
    } else {
      Serial.println("Auto-prueba FALLIDA (o no soportada por el clon).");
    }
  }

  // Ejecutar pruebas activas de diagnóstico: REQA y combinaciones TX
  runREQATest();
  runForceTxCombos();
}

void loop() {
  Serial.println("Buscando nueva tarjeta...");

  // Check for new cards
  bool cardPresent = mfrc522.PICC_IsNewCardPresent();
  Serial.print("PICC_IsNewCardPresent: ");
  Serial.println(cardPresent ? "SI" : "NO");

  if (cardPresent) {
    // Intentar leer UID con reintentos cortos (algunos clones requieren varios intentos)
    bool readSerial = false;
    const int maxAttempts = 3;
    for (int attempt = 1; attempt <= maxAttempts; ++attempt) {
      readSerial = mfrc522.PICC_ReadCardSerial();
      Serial.print("PICC_ReadCardSerial (intento "); Serial.print(attempt); Serial.print("): ");
      Serial.println(readSerial ? "SI" : "NO");
      if (readSerial) break;
      // Si falla, volcar registros de error y hacer un REQA corto para ver respuesta
      byte err = mfrc522.PCD_ReadRegister(MFRC522::ErrorReg);
      byte com = mfrc522.PCD_ReadRegister(MFRC522::ComIrqReg);
      byte fifoLevel = mfrc522.PCD_ReadRegister(MFRC522::FIFOLevelReg);
      Serial.print("  ErrorReg=0x"); Serial.print(err, HEX);
      Serial.print(" ComIrqReg=0x"); Serial.print(com, HEX);
      Serial.print(" FIFOLevel=0x"); Serial.println(fifoLevel, HEX);
      if (fifoLevel) {
        Serial.print("  FIFO: ");
        for (byte i = 0; i < fifoLevel && i < 32; i++) {
          byte b = mfrc522.PCD_ReadRegister(MFRC522::FIFODataReg);
          Serial.print(b < 0x10 ? " 0" : " "); Serial.print(b, HEX);
        }
        Serial.println();
      }
      // Ejecutar REQA de diagnóstico (muestra si antenna/tx responde)
      runREQATest();
      delay(150);
    }

    if (readSerial) {
      Serial.println("Tarjeta detectada!");
      // Show some details of the PICC (that is: the tag/card)
      Serial.print(F("Card UID:"));
      dump_byte_array(mfrc522.uid.uidByte, mfrc522.uid.size);
      Serial.println();
      Serial.print(F("PICC type: "));
      MFRC522::PICC_Type piccType = mfrc522.PICC_GetType(mfrc522.uid.sak);
      Serial.println(mfrc522.PICC_GetTypeName(piccType));

      // Halt PICC
      mfrc522.PICC_HaltA();
      // Stop encryption on PCD
      mfrc522.PCD_StopCrypto1();

      Serial.println("Listo para nueva tarjeta.");
      delay(2000); // Pausa antes de buscar otra
    }
    else {
      // Diagnóstico adicional: enviar anticollision (cascade level 1) directamente
      Serial.println("Intentando anticollision crudo (0x93 0x20) para volcar respuesta)...");
      byte acCmd[2] = { 0x93, 0x20 };
      byte backLen = 10;
      byte backBuf[10];
      byte validBits = 0;
      MFRC522::StatusCode st = mfrc522.PCD_TransceiveData(acCmd, 2, backBuf, &backLen, &validBits, 0, false);
      Serial.print("Anticollision Status: "); Serial.print(statusCodeToStr(st)); Serial.print(" ("); Serial.print(st); Serial.println(")");
      if (st == MFRC522::STATUS_OK) {
        Serial.print("Anticollision BackLen: "); Serial.println(backLen);
        Serial.print("Anticollision data:");
        for (byte i = 0; i < backLen; i++) {
          Serial.print(backBuf[i] < 0x10 ? " 0" : " "); Serial.print(backBuf[i], HEX);
        }
        Serial.println();
        // Si BackLen == 5 (típico para anticollision CL1), intentar reconstruir UID
        if (backLen == 5) {
          Serial.println("Reconstruyendo UID desde anticollision crudo...");
          // Copiar los primeros 4 bytes como UID (ignorar BCC en backBuf[4])
          for (byte i = 0; i < 4; i++) {
            mfrc522.uid.uidByte[i] = backBuf[i];
          }
          mfrc522.uid.size = 4;
          mfrc522.uid.sak = 0x08; // Asumir MIFARE Classic 1K
          Serial.println("Tarjeta detectada (UID reconstruido)!");
          Serial.print(F("Card UID:"));
          dump_byte_array(mfrc522.uid.uidByte, mfrc522.uid.size);
          Serial.println();
          Serial.print(F("PICC type: "));
          MFRC522::PICC_Type piccType = mfrc522.PICC_GetType(mfrc522.uid.sak);
          Serial.println(mfrc522.PICC_GetTypeName(piccType));
          // Halt PICC
          mfrc522.PICC_HaltA();
          // Stop encryption on PCD
          mfrc522.PCD_StopCrypto1();
          Serial.println("Listo para nueva tarjeta.");
          delay(2000); // Pausa antes de buscar otra
        }
      }
    }
  } else {
    delay(1000); // Esperar 1 segundo antes de volver a buscar
  }
}

// Helper: convert StatusCode to string (partial)
const char* statusCodeToStr(MFRC522::StatusCode code) {
  switch (code) {
    case MFRC522::STATUS_OK: return "STATUS_OK";
    case MFRC522::STATUS_ERROR: return "STATUS_ERROR";
    case MFRC522::STATUS_COLLISION: return "STATUS_COLLISION";
    case MFRC522::STATUS_TIMEOUT: return "STATUS_TIMEOUT";
    case MFRC522::STATUS_NO_ROOM: return "STATUS_NO_ROOM";
    case MFRC522::STATUS_INTERNAL_ERROR: return "STATUS_INTERNAL_ERROR";
    case MFRC522::STATUS_INVALID: return "STATUS_INVALID";
    case MFRC522::STATUS_CRC_WRONG: return "STATUS_CRC_WRONG";
    case MFRC522::STATUS_MIFARE_NACK: return "STATUS_MIFARE_NACK";
    default: return "STATUS_UNKNOWN";
  }
}

// A) Enviar REQA (0x26, 7 bits) con PCD_TransceiveData y mostrar resultado
void runREQATest() {
  Serial.println("=== REQA test (7 bits) ===");
  byte req = 0x26;
  byte validBits = 7;
  byte backLen = 16;
  byte backData[16];
  // PCD_TransceiveData signature: sendData, sendLen, backData, &backLen, &validBits, rxAlign, checkCRC
  MFRC522::StatusCode status = mfrc522.PCD_TransceiveData(&req, 1, backData, &backLen, &validBits, 0, false);
  Serial.print("REQA Status: "); Serial.print(statusCodeToStr(status)); Serial.print(" ("); Serial.print(status); Serial.println(")");
  if (status == MFRC522::STATUS_OK) {
    Serial.print("BackLen: "); Serial.println(backLen);
    Serial.print("Back: ");
    for (byte i = 0; i < backLen; i++) {
      Serial.print(backData[i], HEX); Serial.print(' ');
    }
    Serial.println();
  }
}

// B) Forzar combinaciones de TxControl/TxASK/TxMode y ejecutar REQA tras cada uno
void runForceTxCombos() {
  Serial.println("=== Forzando combinaciones TX (diagnostico) ===");
  // Algunas combinaciones típicas
  struct Combo { byte txControl; byte txAsk; byte txMode; } combos[] = {
    { 0x00, 0x40, 0x00 },
    { 0x03, 0x40, 0x00 },
    { 0x83, 0x40, 0x00 },
    { 0x83, 0x60, 0x00 }
  };
  for (auto &c : combos) {
    mfrc522.PCD_WriteRegister(MFRC522::TxControlReg, c.txControl);
    mfrc522.PCD_WriteRegister(MFRC522::TxASKReg, c.txAsk);
    mfrc522.PCD_WriteRegister(MFRC522::TxModeReg, c.txMode);
    Serial.print("Wrote TxControl=0x"); Serial.print(c.txControl, HEX);
    Serial.print(" TxASK=0x"); Serial.print(c.txAsk, HEX);
    Serial.print(" TxMode=0x"); Serial.println(c.txMode, HEX);
    delay(50);
    runREQATest();
    delay(200);
  }
  // Restore antenna on
  mfrc522.PCD_AntennaOn();
}