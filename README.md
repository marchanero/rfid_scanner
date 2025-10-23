# RFID Reader for MERN Stack 🚀

[![GitHub stars](https://img.shields.io/github/stars/marchanero/rfid_scanner?style=social)](https://github.com/marchanero/rfid_scanner)
[![License](https://img.shields.io/badge/license-MIT-blue.svg)](LICENSE)
[![PlatformIO](https://img.shields.io/badge/PlatformIO-arduino-orange)](https://platformio.org/)

Proyecto optimizado para integrar un lector RFID RC522 (HW-126 clon) con Wemos D1 mini en aplicaciones MERN (MongoDB, Express, React, Node.js). Envía UIDs de tarjetas MIFARE por puerto serial en formato JSON para fácil parsing en el backend.

## ✨ Características

- 🔍 **Lectura precisa de UIDs**: Compatible con tarjetas MIFARE Classic, NTAG (botones azules, pegatinas), con fallback a anticollision cruda para clones.
- 📡 **Integración serial**: Salida JSON limpia para Node.js (`serialport`).
- 🛠️ **Optimizado para producción**: Código minimalista (~60 líneas), sin logs verbosos.
- 🔧 **Soporte clones**: Maneja firmwares no estándar (ej. VersionReg 0xB2) con reconstrucción manual de UIDs.
- ⚡ **Bajo consumo**: Inicialización rápida, loop eficiente, gain de antena optimizado (38dB).
- 💡 **Indicador LED**: LED builtin enciende 500ms al detectar tarjeta.

## 📦 Instalación

### Requisitos

- [PlatformIO](https://platformio.org/) (IDE recomendado).
- Wemos D1 mini (ESP8266).
- Módulo RFID RC522 HW-126 (SPI).
- Tarjetas MIFARE Classic para testing.

### Setup del Proyecto

1. Clona el repo:

   ```bash
   git clone https://github.com/marchanero/rfid_scanner.git
   cd rfid_scanner
   ```

2. Instala dependencias:

   ```bash
   pio install
   ```

   > **Nota**: El proyecto usa la librería estándar `miguelbalboa/MFRC522` para compatibilidad con NTAG y clones HW-126.

3. Flashea el firmware:

   ```bash
   pio run -t upload -e d1_mini
   ```

4. Monitor serie:

   ```bash
   pio device monitor -e d1_mini
   ```

## 🔌 Conexiones Hardware

| Wemos D1 mini | RC522 HW-126 | Descripción |
|---------------|--------------|-------------|
| 3.3V         | VCC          | Alimentación |
| GND          | GND          | Tierra común |
| D2 (GPIO4)   | SS           | Chip Select |
| D1 (GPIO5)   | RST          | Reset |
| D7 (GPIO13)  | MOSI         | SPI Data Out |
| D6 (GPIO12)  | MISO         | SPI Data In |
| D5 (GPIO14)  | SCK          | SPI Clock |

> **Nota**: Pines SPI son fijos en ESP8266. Verifica jumpers del módulo en modo SPI.

## 🚀 Uso

### Salida Serial (JSON)

El dispositivo envía mensajes JSON por serial (115200 baudios):

- **Ready**: `{"event":"ready"}` (inicialización exitosa).
- **Error**: `{"event":"error","message":"RC522 init failed"}` (fallo de init).
- **Card Detected**: `{"event":"card_detected","uid":"32B8FA05"}` (UID en hex mayúscula).

### Integración con MERN (Node.js Backend)

Instala `serialport`:

```bash
npm install serialport
```

Ejemplo de código en `server.js`:

```javascript
const SerialPort = require('serialport');
const port = new SerialPort('/dev/ttyUSB0', { baudRate: 115200 });

port.on('data', (data) => {
  const line = data.toString().trim();
  if (line.startsWith('{')) {
    const event = JSON.parse(line);
    if (event.event === 'card_detected') {
      console.log('UID detectado:', event.uid);
      // Guardar en MongoDB, enviar a React via Socket.io, etc.
    }
  }
});
```

### React Frontend (Ejemplo)

Usa WebSocket/Socket.io para recibir UIDs del backend y actualizar UI.

## 🛠️ API Serial

| Evento          | Payload                          | Descripción |
|-----------------|----------------------------------|-------------|
| `ready`        | `{}`                            | Dispositivo listo |
| `error`        | `{"message": "string"}`         | Error de inicialización |
| `card_detected`| `{"uid": "HEXSTRING"}`          | UID detectado (4 bytes hex) |

## 🔧 Troubleshooting

### No detecta tarjetas

- Verifica conexiones SPI (multímetro para continuidad).
- Acerca tarjeta a 1-2 cm del módulo.
- Conecta antena externa si la integrada falla.
- Firmware clon: Si VersionReg es 0xB2, es normal; el código maneja clones.
- Para botones azules (NTAG) y pegatinas: Asegúrate de usar la librería estándar MFRC522; gain ajustado a 38dB mejora detección de tags pequeños.

### Errores comunes

- **Init failed**: Verifica alimentación 3.3V estable, no 5V.
- **No JSON output**: Revisa baud rate (115200), puerto serial correcto.
- **UIDs inconsistentes**: Clones pueden requerir antena externa.

### Debug avanzado

Si el código optimizado no funciona, usa la rama `main` con diagnósticos para logs detallados.

## 📁 Estructura del Proyecto

```text
rfid_scanner/
├── platformio.ini          # Config PlatformIO
├── src/
│   └── main.cpp            # Sketch ESP8266
├── include/                # Headers (si aplica)
├── lib/                    # Librerías locales
└── README.md               # Esta doc
```

## 🤝 Contribuir

1. Fork el repo.
2. Crea una rama: `git checkout -b feature/nueva-funcion`.
3. Commit: `git commit -m 'Agrega nueva funcion'`.
4. Push: `git push origin feature/nueva-funcion`.
5. Abre un PR.

## 📄 Licencia

MIT License - ver [LICENSE](LICENSE) para detalles.

---

Hecho con ❤️ para proyectos IoT y RFID. ¡Issues y PRs bienvenidos!
