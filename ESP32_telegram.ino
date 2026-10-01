#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <UniversalTelegramBot.h>

const char* WIFI_SSID = "RED_WIFI";
const char* WIFI_PASS = "WIFI_PASSWORD";

#define BOTtoken "TELEGRAM_BOT_TOKEN " 

WiFiClientSecure client;
UniversalTelegramBot bot(BOTtoken, client);

unsigned long bot_last_time; 
const long bot_mtbs = 1000; 

String adminChatId = "";

void setup() {
  Serial.begin(9600); 
  
  Serial2.begin(9600, SERIAL_8N1, 16, 17); 
  
  Serial.println("Iniciando ESP32...");
  client.setInsecure();

  Serial.print("Conectando a Wi-Fi: ");
  WiFi.begin(WIFI_SSID, WIFI_PASS);
  while (WiFi.status() != WL_CONNECTED) {
    Serial.print(".");
    delay(500);
  }
  Serial.println("\n¡Conectado a Wi-Fi!");

  bot.getMe();
  Serial.println("Bot de Telegram listo.");
  Serial.println("¡IMPORTANTE! Envíe /start al bot para registrarse como admin.");
}

void manejar_mensajes_telegram(int numNewMessages) {
  for (int i = 0; i < numNewMessages; i++) {
    String chat_id = String(bot.messages[i].chat_id);
    String text = bot.messages[i].text;
    String from_name = bot.messages[i].from_name;

    Serial.print("Mensaje recibido de ");
    Serial.println(from_name);
    
    if (text == "/start") {
      adminChatId = chat_id; 
      String welcome = "¡Hola, " + from_name + "! Te has registrado como Admin. Te avisaré cuando usen la puerta.";
      bot.sendMessage(chat_id, welcome, "");
      Serial.print("¡Admin registrado! Chat ID: ");
      Serial.println(adminChatId);
    }
  }
  
  bot.last_message_received = bot.messages[numNewMessages - 1].update_id;
}

void revisar_mensajes_del_uno() {
  if (Serial2.available() > 0) {
    String mensaje = Serial2.readStringUntil('\n');
    mensaje.trim();
    
    Serial.print("¡Mensaje recibido del Arduino Uno!: ");
    Serial.println(mensaje);

    if (adminChatId != "") {
      bot.sendMessage(adminChatId, mensaje, "");
    } else {
      Serial.println("Error: Aún no sé a quién enviar el mensaje. (Envía /start al bot)");
    }
  }
}

void loop() {
  
  if (millis() > bot_last_time + bot_mtbs)  {
    
    int numNewMessages = bot.getUpdates(bot.last_message_received + 1);

    while (numNewMessages) {
      manejar_mensajes_telegram(numNewMessages);
      numNewMessages = bot.getUpdates(bot.last_message_received + 1);
    }
    
    bot_last_time = millis();
  }
  
  revisar_mensajes_del_uno();
}