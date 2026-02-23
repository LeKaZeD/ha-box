# Connection between ESP32 and Raspberry

## Tableau fonctionnalités

| Fonction | Où ça vit (responsable) | Communication ESP32 ↔ Pi | Sens | Détails / remarques |
|---|---|---:|---:|---|
| **Bouton poussoir “Power” (déporté)** | ESP32 lit le bouton | **J2 (power button)** | ESP32 → Pi | L’ESP32 simule un appui (contact bref ~100–300 ms). |
| **Extinction propre (PC-like)** | Pi/HAOS gère le shutdown | **J2 (power button)** *(recommandé)* + UART *(optionnel)* | ESP32 → Pi | Le plus fiable : appui bouton via J2, le Pi exécute l’arrêt propre. UART peut servir à demander un shutdown “logique”. |
| **Allumage / Wake** (sans couper l’alim) | Pi se réveille | **J2 (power button)** | ESP32 → Pi | Indispensable. Pas de wake via UART si le Pi est éteint. |
| **Reboot / maintenance** | Pi/HAOS | UART | ESP32 → Pi | L’ESP32 envoie une commande “whitelistée” (ex: reboot). Le Pi exécute `ha host reboot` ou équivalent via un bridge. |
| **Écran e-paper (SPI)** | ESP32 pilote l’affichage | UART | Pi → ESP32 | Le Pi envoie des “screens” (ex: page onboarding, dashboard minimal). L’ESP32 affiche même pendant le boot du Pi. |
| **Tactile (I2C)** | ESP32 lit l’input | UART | ESP32 → Pi | L’ESP32 remonte des actions UI (tap, swipe, next, back) que HA transforme en events/automations. |
| **NFC (PN532 puis contrôleur avec émulation)** | ESP32 | UART | ESP32 → Pi | L’ESP32 remonte UID/NDEF + actions (pairing, onboarding). |
| **Onboarding “add device”** | Home Assistant (logique), ESP32 (UI) | UART | bidirectionnel | HA décide des étapes, ESP32 affiche + collecte inputs (NFC / touch). |
| **Capteurs temp/hum** | ESP32 | UART | ESP32 → Pi | Remontée vers HA pour dashboards et automatisations. |
| **Temp dédiée pour refroidissement + fan PWM** | ESP32 | UART *(optionnel)* | ESP32 → Pi | Contrôle ventilo autonome (sécurité) + télémétrie vers HA si souhaité. |
| **États Wi-Fi / BT / Zigbee / Thread / Matter / 433MHz** (opérationnels ?) | Pi/HA (source de vérité) | UART | Pi → ESP32 | HA agrège l’état des intégrations/add-ons et envoie un “status bundle” à l’ESP32 pour affichage. |
| **Health check add-ons** (Zigbee2MQTT, Thread, etc.) | Pi/HAOS Supervisor + HA | UART | Pi → ESP32 | Le Pi lit l’état via Supervisor/HA, push vers ESP32. |


## Hardware connection

ESP32 serial2, 0 is used by USB.
Pi TXD (GPIO14) → ESP32 RX2 (GPIO16)
Pi RXD (GPIO15) → ESP32 TX2 (GPIO17)
GND on GND to sync ground. (required)
Pi 5V → ESP32 5V (if connect without USB)

## Activate UART on Pi HAOS

On root ssh or direct: exit ha command prompt

```
login
```

Edit boot config file `vi /mnt/boot/config.txt` and change those line:

```
enable_uart=1
dtoverlay=disable-bt
```
note: i to switch in insert mode, esc then :wq to save en quite

Then reboot : `ha host reboot` or `reboot`

To know if it's enable use:
```
ls -l /dev/serial*
```
If there is something like that: `/dev/serial0` or `/dev/ttyAMA0` it's good

The serial have to be configured:
```
stty -F /dev/serial0 115200 cs8 -cstopb -parenb -ixon -ixoff -crtscts raw -echo
```

To read use `cat /dev/serial0` 
To write use `echo "hello from haos" > /dev/serial0`


## Activate UART on ESP32

Use this code to test the connection:

```
#include <Arduino.h>

// ESP32 UART2 pins (classique)
// TX2 = GPIO17, RX2 = GPIO16
static const int UART2_TX = 17;
static const int UART2_RX = 16;

static const uint32_t BAUD_USB  = 115200;
static const uint32_t BAUD_UART = 115200;

String usbLine;
String uartLine;

void setup() {
  Serial.begin(BAUD_USB);

  // UART vers Raspberry (3.3V TTL)
  Serial2.begin(BAUD_UART, SERIAL_8N1, UART2_RX, UART2_TX);

  Serial.println("ESP32 <-> Raspberry bridge ready");
  Serial.println("Type a line in Serial Monitor and press Enter.");
}

static void pumpSerialToUart() {
  while (Serial.available()) {
    char c = (char)Serial.read();
    if (c == '\r') continue;  // ignore CR
    if (c == '\n') {
      if (usbLine.length() > 0) {
        Serial2.println(usbLine);           // envoie au Raspberry
        Serial.print("USB -> UART: ");
        Serial.println(usbLine);
        usbLine = "";
      }
    } else {
      usbLine += c;
      if (usbLine.length() > 512) usbLine = ""; // garde-fou
    }
  }
}

static void pumpUartToSerial() {
  while (Serial2.available()) {
    char c = (char)Serial2.read();
    if (c == '\r') continue;
    if (c == '\n') {
      if (uartLine.length() > 0) {
        Serial.print("UART -> USB: ");
        Serial.println(uartLine);
        uartLine = "";
      }
    } else {
      uartLine += c;
      if (uartLine.length() > 512) uartLine = "";
    }
  }
}

void loop() {
  pumpSerialToUart();
  pumpUartToSerial();

  // Optionnel: heartbeat toutes les 2s
  static uint32_t last = 0;
  if (millis() - last > 2000) {
    last = millis();
    Serial2.println("ping");
  }
}
```



