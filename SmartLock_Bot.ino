#include <SPI.h>
#include <MFRC522.h>
#include <EEPROM.h>
#include <Stepper.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <SoftwareSerial.h>

#define RST_PIN   9
#define SS_PIN    10
#define LED_ROJO  8
#define LED_VERDE 7
#define BUZZER 2

MFRC522 mfrc522(SS_PIN, RST_PIN);
const int stepsPerRevolution = 2048;
Stepper myStepper(stepsPerRevolution, 3, 5, 4, 6);
LiquidCrystal_I2C lcd(0x27, 16, 2);
SoftwareSerial espSerial(A0, A1);

#define UID_SIZE 4
#define NAME_LENGTH 16
#define MAX_CARDS 50

struct CardRecord {
  byte uid[UID_SIZE];
  char name[NAME_LENGTH];
};

#define EEPROM_COUNT_ADDR 0
#define EEPROM_START_ADDR 1

byte cardCount = 0;

void setup() {
  Serial.begin(9600);
  espSerial.begin(9600);
  myStepper.setSpeed(12);

  SPI.begin();
  mfrc522.PCD_Init();

  pinMode(LED_ROJO, OUTPUT);
  pinMode(LED_VERDE, OUTPUT);
  pinMode(BUZZER, OUTPUT);

  lcd.init();
  lcd.backlight();

  Serial.println(F("Sistema de Control de Acceso v2.1 (con borrado)"));

  cardCount = EEPROM.read(EEPROM_COUNT_ADDR);
  if (cardCount == 0xFF) {
    cardCount = 0;
    EEPROM.write(EEPROM_COUNT_ADDR, 0);
  }

  Serial.print(F("Tarjetas registradas: "));
  Serial.println(cardCount);

  Serial.println(F("-----------------------------------"));
  Serial.println(F("Comandos: [R] Registrar, [L] Listar, [B] Borrar Tarjeta, [E] Borrar TODO"));

  lcd_mostrar_esperando();
}

void loop() {
  revisar_comandos_serial();

  if ( ! mfrc522.PICC_IsNewCardPresent()) {
    delay(50);
    return;
  }

  if ( ! mfrc522.PICC_ReadCardSerial()) {
    delay(50);
    return;
  }

  Serial.print(F("Tarjeta detectada: "));
  imprimir_uid(mfrc522.uid.uidByte, mfrc522.uid.size);

  int index = encontrar_indice_tarjeta(mfrc522.uid.uidByte, mfrc522.uid.size);

  if (index != -1) {
    CardRecord foundCard;
    int addr = EEPROM_START_ADDR + (index * sizeof(CardRecord));
    EEPROM.get(addr, foundCard);

    Serial.print(F(">>> ACCESO CONCEDIDO: "));
    Serial.println(foundCard.name);
    
    String mensaje = "✅ Acceso Concedido: " + String(foundCard.name);
    espSerial.println(mensaje);

    lcd_acceso_concedido(foundCard.name);
    digitalWrite(LED_VERDE, HIGH);
    sonido_acceso_concedido();
    myStepper.step(stepsPerRevolution);
    sonido_puerta_desbloqueada();
    sonido_cronometro_10s();
    myStepper.step(-stepsPerRevolution);
    motor_en_espera();
    digitalWrite(LED_VERDE, LOW);

  } else {
    Serial.println(F("--- ACCESO DENEGADO ---"));
    espSerial.println("⛔ ¡ALERTA! Intento de acceso denegado.");
    
    lcd_acceso_denegado();
    digitalWrite(LED_ROJO, HIGH);
    sonido_acceso_denegado();
    delay(1000);
    digitalWrite(LED_ROJO, LOW);
  }

  mfrc522.PICC_HaltA();
  Serial.println(F("-----------------"));
  delay(1000);
  lcd_mostrar_esperando();
}

int encontrar_indice_tarjeta(byte scannedID[], byte size) {
  if (size != UID_SIZE) {
    return -1;
  }

  CardRecord tempCard;

  for (int i = 0; i < cardCount; i++) {
    int addr = EEPROM_START_ADDR + (i * sizeof(CardRecord));
    EEPROM.get(addr, tempCard);
    if (comparar_uid(scannedID, tempCard.uid)) {
      return i;
    }
  }

  return -1;
}

void revisar_comandos_serial() {
  if (Serial.available() > 0) {
    char cmd = Serial.read();
    while (Serial.available() > 0) Serial.read();

    if (cmd == 'R' || cmd == 'r') {
      registrar_nueva_tarjeta();
    }
    if (cmd == 'L' || cmd == 'l') {
      listar_todas_las_tarjetas();
    }
    if (cmd == 'B' || cmd == 'b') {
      borrar_tarjeta_individual();
    }
    if (cmd == 'E' || cmd == 'e') {
      borrar_todas_las_tarjetas();
    }
    lcd_mostrar_esperando();
  }
}

void registrar_nueva_tarjeta() {
  if (cardCount >= MAX_CARDS) {
    Serial.println(F("¡Memoria llena! No se pueden registrar más tarjetas."));
    lcd.clear(); lcd.print(F("MEMORIA LLENA")); delay(2000);
    return;
  }

  Serial.println(F("MODO REGISTRO: Por favor, escanea la nueva tarjeta..."));
  lcd.clear();
  lcd.print(F("MODO REGISTRO"));
  lcd.setCursor(0, 1);
  lcd.print(F("Acerque tarjeta"));

  unsigned long startTime = millis();

  while (millis() - startTime < 10000) {
    if (mfrc522.PICC_IsNewCardPresent() && mfrc522.PICC_ReadCardSerial()) {

      if (mfrc522.uid.size != UID_SIZE) {
        Serial.println(F("Error: Tarjeta no válida."));
        mfrc522.PICC_HaltA(); delay(1000);
        return;
      }

      if (encontrar_indice_tarjeta(mfrc522.uid.uidByte, mfrc522.uid.size) != -1) {
        Serial.println(F("Error: Esta tarjeta ya está registrada."));
        lcd.clear(); lcd.print(F("YA EXISTE")); delay(2000);
        mfrc522.PICC_HaltA();
        return;
      }

      CardRecord newCard;
      memcpy(newCard.uid, mfrc522.uid.uidByte, UID_SIZE);

      Serial.println(F("Tarjeta escaneada."));
      Serial.print(F("Escribe el nombre (max 15 chars) y presiona Enter:"));
      lcd.clear();
      lcd.print(F("Tarjeta OK"));
      lcd.setCursor(0, 1);
      lcd.print(F("Escriba nombre..."));

      leer_nombre_serial(newCard.name, NAME_LENGTH);

      Serial.print(F("Registrando '"));
      Serial.print(newCard.name);
      Serial.println(F("'..."));

      int newAddr = EEPROM_START_ADDR + (cardCount * sizeof(CardRecord));
      EEPROM.put(newAddr, newCard);

      cardCount++;
      EEPROM.write(EEPROM_COUNT_ADDR, cardCount);

      Serial.println(F("¡REGISTRADA CON ÉXITO!"));
      lcd.clear();
      lcd.print(F("¡REGISTRADA!"));
      lcd.setCursor(0, 1);
      lcd.print(newCard.name);
      delay(2000);

      mfrc522.PICC_HaltA();
      return;
    }
  }

  Serial.println(F("Tiempo agotado. Saliendo del modo registro."));
  lcd.clear(); lcd.print(F("TIEMPO AGOTADO")); delay(2000);
}

void listar_todas_las_tarjetas() {
  Serial.println(F("--- LISTA DE TARJETAS REGISTRADAS ---"));
  if (cardCount == 0) {
    Serial.println(F("No hay tarjetas registradas."));
    lcd.clear(); lcd.print(F("LISTA VACIA")); delay(2000);
    return;
  }

  CardRecord tempCard;
  for (int i = 0; i < cardCount; i++) {
    int addr = EEPROM_START_ADDR + (i * sizeof(CardRecord));
    EEPROM.get(addr, tempCard);

    Serial.print(i + 1);
    Serial.print(F(": "));
    Serial.print(tempCard.name);
    Serial.print(F(" [UID:"));
    imprimir_uid(tempCard.uid, UID_SIZE);
    Serial.println(F("]"));
  }
  Serial.println(F("--------------------------------------"));
}

void borrar_tarjeta_individual() {
  if (cardCount == 0) {
    Serial.println(F("No hay tarjetas registradas para borrar."));
    lcd.clear(); lcd.print(F("LISTA VACIA")); delay(2000);
    return;
  }

  Serial.println(F("--- BORRAR TARJETA ---"));
  listar_todas_las_tarjetas();
  Serial.println(F("--------------------------"));
  Serial.print(F("Ingrese el numero de la tarjeta a borrar (1-"));
  Serial.print(cardCount);
  Serial.println(F(")"));
  Serial.println(F("O envie '0' para cancelar."));

  lcd.clear();
  lcd.print(F("Ingrese Nro (1-"));
  lcd.print(cardCount);
  lcd.print(F(")"));
  lcd.setCursor(0, 1);
  lcd.print(F("o '0' p/cancelar"));

  int numToDelete = leer_entero_serial();

  while (Serial.available() > 0) Serial.read();

  if (numToDelete == -1) {
    Serial.println(F("Tiempo agotado. Cancelado."));
    lcd.clear(); lcd.print(F("TIEMPO AGOTADO")); delay(1500);
    return;
  }
  if (numToDelete == 0) {
    Serial.println(F("Cancelado por el usuario."));
    lcd.clear(); lcd.print(F("CANCELADO")); delay(1500);
    return;
  }
  if (numToDelete < 0 || numToDelete > cardCount) {
    Serial.println(F("Numero invalido. Cancelado."));
    lcd.clear(); lcd.print(F("NUMERO INVALIDO")); delay(1500);
    return;
  }

  int indexToDelete = numToDelete - 1;

  CardRecord tempCard;
  int addr = EEPROM_START_ADDR + (indexToDelete * sizeof(CardRecord));
  EEPROM.get(addr, tempCard);

  Serial.print(F("Borrando '"));
  Serial.print(tempCard.name);
  Serial.println(F("'..."));
  lcd.clear();
  lcd.print(F("Borrando..."));
  lcd.setCursor(0, 1);
  lcd.print(tempCard.name);
  delay(1000);

  for (int i = indexToDelete; i < cardCount - 1; i++) {
    int addrNext = EEPROM_START_ADDR + ((i + 1) * sizeof(CardRecord));
    EEPROM.get(addrNext, tempCard);

    int addrCurrent = EEPROM_START_ADDR + (i * sizeof(CardRecord));
    EEPROM.put(addrCurrent, tempCard);
  }

  cardCount--;
  EEPROM.write(EEPROM_COUNT_ADDR, cardCount);

  Serial.println(F("¡Tarjeta borrada con exito!"));
  lcd.clear(); lcd.print(F("¡BORRADA!")); delay(2000);
}

void borrar_todas_las_tarjetas() {
  Serial.println(F("ATENCIÓN: ¿Estás seguro de borrar TODAS las tarjetas?"));
  Serial.println(F("Envía 'Y' para confirmar o cualquier otra tecla para cancelar."));
  lcd.clear();
  lcd.print(F("BORRAR TODO?"));
  lcd.setCursor(0, 1);
  lcd.print(F("Enviar 'Y'"));

  unsigned long startWait = millis();
  while (millis() - startWait < 5000) {
    if (Serial.available() > 0) {
      char cmd = Serial.read();
      if (cmd == 'Y' || cmd == 'y') {
        EEPROM.write(EEPROM_COUNT_ADDR, 0);
        cardCount = 0;
        Serial.println(F("¡TODAS LAS TARJETAS HAN SIDO BORRADAS!"));
        lcd.clear(); lcd.print(F("TARJETAS")); lcd.setCursor(0, 1); lcd.print(F("BORRADAS"));
        delay(2000);
        return;
      } else {
        Serial.println(F("Cancelado."));
        lcd.clear(); lcd.print(F("CANCELADO")); delay(1000);
        return;
      }
    }
  }
  Serial.println(F("Tiempo agotado. Cancelado."));
  lcd.clear(); lcd.print(F("CANCELADO")); delay(1000);
}

void motor_en_espera() {
  digitalWrite(3, LOW);
  digitalWrite(4, LOW);
  digitalWrite(5, LOW);
  digitalWrite(6, LOW);
}

boolean comparar_uid(byte uid1[], byte uid2[]) {
  for (byte i = 0; i < UID_SIZE; i++) {
    if (uid1[i] != uid2[i]) {
      return false;
    }
  }
  return true;
}

void imprimir_uid(byte id[], byte size) {
  for (byte i = 0; i < size; i++) {
    Serial.print(id[i] < 0x10 ? " 0" : " ");
    Serial.print(id[i], HEX);
  }
}

void leer_nombre_serial(char* buffer, int maxLength) {
  int index = 0;
  while (Serial.available() == 0) {
    delay(100);
  }

  while (true) {
    if (Serial.available() > 0) {
      char c = Serial.read();
      if (c == '\n' || c == '\r') {
        buffer[index] = '\0';
        break;
      } else if (index < maxLength - 1) {
        buffer[index] = c;
        index++;
      }
    }
  }
  while (Serial.available() > 0) Serial.read();
}

int leer_entero_serial() {
  String inputString = "";
  unsigned long startTime = millis();

  while (Serial.available() == 0) {
    if (millis() - startTime > 10000) {
      return -1;
    }
    delay(50);
  }

  while (true) {
    if (Serial.available() > 0) {
      char c = Serial.read();
      if (c == '\n' || c == '\r') {
        if (inputString.length() > 0) {
          return inputString.toInt();
        }
      } else if (isDigit(c)) {
        inputString += c;
      }
    }
  }
}

void lcd_mostrar_esperando() {
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print(F("Acerque su"));
  lcd.setCursor(0, 1);
  lcd.print(F("tarjeta..."));
}

void lcd_acceso_concedido(char* name) {
  lcd.clear();
  lcd.print(F("BIENVENIDO"));
  lcd.setCursor(0, 1);
  lcd.print(name);
}

void lcd_acceso_denegado() {
  lcd.clear();
  lcd.print(F("ACCESO"));
  lcd.setCursor(0, 1);
  lcd.print(F("DENEGADO"));
}

void sonido_acceso_concedido() {
  tone(BUZZER, 900);
  delay(100);
  noTone(BUZZER);
  delay(50);
  tone(BUZZER, 1200);
  delay(100);
  noTone(BUZZER);
}

void sonido_puerta_desbloqueada() {
  for (int i = 0; i < 50; i++) {
    tone(BUZZER, 90);
    delay(5);
    noTone(BUZZER);
    delay(5);
  }
  tone(BUZZER, 1500);
  delay(75);
  noTone(BUZZER);
}

void sonido_acceso_denegado() {
  tone(BUZZER, 300);
  delay(200);
  noTone(BUZZER);
  delay(50);
  tone(BUZZER, 300);
  delay(200);
  noTone(BUZZER);
}

void sonido_cronometro_10s() {
  long duracion_tick = 50;

  for (int i = 0; i < 30; i++) {
    long frec_actual = 800 + (i * 1200 / 29);
    long delay_actual = 500 - (i * 450 / 29);

    tone(BUZZER, frec_actual, duracion_tick);
    delay(delay_actual);
  }
}