\# Smart Lock IoT: Control de Acceso RFID con Puente ESP32 \& Telegram



Sistema embebido de control de acceso físico con autenticación por RFID/NFC, almacenamiento persistente en EEPROM (mapeo relacional UID ↔ Nombre de usuario), servomecanismo de apertura y puente IoT para telemetría y alertas push en tiempo real vía Telegram.



\## 🚀 Características



\- \*\*Autenticación RFID (RC522):\*\* Lectura e identificación de tarjetas y llaveros Mifare (13.56 MHz).

\- \*\*Gestión persistente de usuarios (EEPROM):\*\* Base de datos local integrada que almacena hasta 50 credenciales asociando UID de 4 bytes a un nombre legible de hasta 15 caracteres.

\- \*\*Administración interactiva por consola:\*\* Modo registro interactivo (`\[R]`), listado completo (`\[L]`), borrado individual con corrimiento de memoria (`\[B]`) y restablecimiento de fábrica (`\[E]`).

\- \*\*Control de actuadores y señalización:\*\*

&#x20; - Motor paso a paso 28BYJ-48 con driver ULN2003 (desbloqueo, retardo de cortesía y rebloqueo automático).

&#x20; - Interfaz visual en Display LCD 16x2 vía bus I2C.

&#x20; - Indicadores LED y retroalimentación sonora mediante buzzer con frecuencias personalizadas para cada estado.

\- \*\*Puente IoT con ESP32 \& Telegram API:\*\*

&#x20; - Comunicación inter-chip mediante enlace UART serie (SoftwareSerial en Arduino a Serial2 en ESP32).

&#x20; - Envío automático de notificaciones HTTPS a Telegram informando el nombre del usuario habilitado o disparando alertas inmediatas ante accesos denegados.



\## 🛠️ Arquitectura y Hardware



| Componente | Función |

| :--- | :--- |

| \*\*Arduino Uno\*\* | Controlador principal, lectura RFID, gestión de memoria EEPROM, control de motor y display |

| \*\*ESP32 DevKit\*\* | Gateway Wi-Fi y cliente HTTPS para la API de bots de Telegram |

| \*\*MFRC522\*\* | Lector RFID SPI |

| \*\*28BYJ-48 + ULN2003\*\* | Motor paso a paso y placa de potencia para pestillo |

| \*\*LCD 16x2 + PCF8574\*\* | Retroalimentación en pantalla por bus I2C (`0x27`) |

| \*\*Buzzer piezoeléctrico\*\* | Generador de tonos auditivos (frecuencias PWM) |



\## 📐 Interconexión UART

\- \*\*Arduino TX (Pin A1)\*\* ➡️ \*\*ESP32 RX2 (GPIO 16)\*\* \*(Recomendado: divisor de tensión 5V a 3.3V)\*

\- \*\*Arduino RX (Pin A0)\*\* ⬅️ \*\*ESP32 TX2 (GPIO 17)\*\*

\- \*\*GND común\*\* entre ambas placas.



\## 💻 Instalación y Configuración



1\. Clonar el repositorio:

&#x20;  ```bash

&#x20;  git clone \[https://github.com/thesimom91/smart-lock.git](https://github.com/thesimon91/smart-lock.git)

