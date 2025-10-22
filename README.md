# rfid_i2c

Proyecto ejemplo PlatformIO para Wemos D1 mini (ESP8266) usando un lector RC522 HW-126 conectado por SPI.

Contenido:
- `platformio.ini` - configuración del entorno
- `src/main.cpp` - sketch que usa la clase `MFRC522_SPI` de la librería `makerspaceleiden/rfid` para leer UIDs
- `README.md` - instrucciones y esquema de pines

Conexiones (Wemos D1 mini / RC522 HW-126 SPI según tu esquema)
- 3.3V -> VCC
- GND -> GND
- D2 (GPIO4) -> SS (Chip Select)
- D1 (GPIO5) -> RST
- D7 (GPIO13) -> MOSI
- D6 (GPIO12) -> MISO
- D5 (GPIO14) -> SCK

Notas:
- El módulo HW-126 usa SPI por defecto. No soporta I2C nativamente.
- La librería usada es `makerspaceleiden/rfid` que incluye `MFRC522_SPI`.
- Pines SPI en Wemos D1 mini: D5=SCK, D6=MISO, D7=MOSI, D8=SS (configurable).
- Si no lee tarjetas: Verifica jumpers (debe estar en SPI), conecta antena externa (la integrada es débil), aumenta distancia a 1-2 cm, usa tarjetas MIFARE Classic.

Cómo flashear (PlatformIO):
1. Conecta tu Wemos via USB.
2. Desde la carpeta del proyecto ejecuta:

```bash
pio run -t upload -e d1_mini
```

3. Abrir monitor serie:

```bash
pio device monitor -e d1_mini
```

Si hay problemas de compilación o el lector no responde, envíame el log del compilador y la salida serie y lo reviso.
