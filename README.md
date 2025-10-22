# rfid_i2c

Proyecto PlatformIO para Wemos D1 mini (ESP8266) con lector RFID RC522 HW-126 (clon) conectado por SPI. Incluye diagnósticos avanzados y reconstrucción manual de UIDs para compatibilidad con módulos clones.

## Características

- Lectura de tarjetas MIFARE Classic (UIDs) usando MFRC522 SPI.
- Diagnósticos integrados: volcado de registros, pruebas REQA, anticollision cruda.
- Reconstrucción automática de UID desde anticollision cuando la función estándar falla (común en clones HW-126).
- Soporte para clones MFRC522 con firmware no estándar (ej. VersionReg 0xB2).
- Librería: `makerspaceleiden/rfid` (compatible con clones).

## Contenido

- `platformio.ini` - Configuración PlatformIO (board: d1_mini, framework: arduino, lib_deps: makerspaceleiden/rfid).
- `src/main.cpp` - Sketch principal con inicialización SPI, diagnósticos y loop de lectura.
- `README.md` - Esta documentación.

## Conexiones (Wemos D1 mini / RC522 HW-126 SPI)

- 3.3V -> VCC
- GND -> GND
- D2 (GPIO4) -> SS (Chip Select)
- D1 (GPIO5) -> RST
- D7 (GPIO13) -> MOSI
- D6 (GPIO12) -> MISO
- D5 (GPIO14) -> SCK

## Notas Importantes

- El módulo HW-126 es un clon SPI-only; no soporta I2C.
- Pines SPI fijos en ESP8266: D5=SCK, D6=MISO, D7=MOSI; SS/RST configurables.
- Si no lee tarjetas: Verifica jumpers en SPI, conecta antena externa (la integrada es débil), acerca tarjeta a 1-2 cm, usa tarjetas MIFARE Classic.
- Problema común en clones: `PICC_ReadCardSerial()` falla, pero REQA/anticollision funcionan. El código reconstruye UID manualmente.
- Firmware clon: VersionReg ~0xB2 (no estándar); auto-prueba puede fallar, pero SPI responde.

## Cómo Flashear (PlatformIO)

1. Conecta Wemos D1 mini via USB.
2. Desde la carpeta del proyecto:

```bash
pio run -t upload -e d1_mini
```

3. Abrir monitor serie (115200 baudios):

```bash
pio device monitor -e d1_mini
```

## Salida Esperada en Monitor Serie

- Inicialización: "RC522 inicializado correctamente.", Firmware Version, volcado registros.
- Diagnósticos: REQA Status, combinaciones TX forzadas.
- Lectura: "PICC_IsNewCardPresent: SI", luego UID reconstruido (ej. "Card UID: 32 B8 FA 05").
- Si falla: Volcado ErrorReg/ComIrq/FIFO, REQA/anticollision para debug.

Si hay problemas (compilación o no lee), envía logs del compilador y salida serie para revisión.
