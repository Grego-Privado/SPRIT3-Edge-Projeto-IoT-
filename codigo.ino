#include <WiFi.h>
#include <PubSubClient.h>
#include <math.h>
#include <DHT.h>
 
const char* SSID            = "Wokwi-GUEST";
const char* PASSWORD        = "";
const char* BROKER_MQTT     = "20.151.88.70";   // IP do broker MQTT (Mosquitto)
const int   BROKER_PORT     = 1883;
const char* ID_MQTT         = "fiware_field001";
const char* APIKEY          = "TEF";            // apikey do Service Group
const char* DEVICEID        = "field001";       // device_id provisionado
 
// Tópicos UL 2.0 (payload = valor)
String TOPIC_PUBLISH_TEMP;  // /TEF/field001/attrs/t
String TOPIC_PUBLISH_HUM;   // /TEF/field001/attrs/h
 
// ================== DHT22 (Wokwi) ==================
#define DHT_PIN   4
#define DHT_TYPE  DHT22
DHT dht(DHT_PIN, DHT_TYPE);
 
// ================== MQTT/NET ==================
WiFiClient   espClient;
PubSubClient MQTT(espClient);
 
// ================== Protótipos ==================
void initSerial();
void initWiFi();
void initMQTT();
void reconnectWiFi();
void reconnectMQTT();
void ensureConnections();
void handleDHT();
 
// ================== Setup / Loop ==================
void setup() {
  initSerial();
  initWiFi();
  initMQTT();
 
  // Monta tópicos a partir de apikey/deviceId
  TOPIC_PUBLISH_TEMP = String("/") + APIKEY + "/" + DEVICEID + "/attrs/t";
  TOPIC_PUBLISH_HUM  = String("/") + APIKEY + "/" + DEVICEID + "/attrs/h";
 
  dht.begin();
}
 
void loop() {
  ensureConnections();
  handleDHT();
  MQTT.loop();
}
 
// ================== Implementações ==================
void initSerial() {
  Serial.begin(115200);
}
 
void initWiFi() {
  delay(10);
  Serial.println("------ Conexao WI-FI ------");
  Serial.print("Conectando-se na rede: ");
  Serial.println(SSID);
  Serial.println("Aguarde...");
  reconnectWiFi();
}
 
void initMQTT() {
  MQTT.setServer(BROKER_MQTT, BROKER_PORT);
}
 
void reconnectWiFi() {
  if (WiFi.status() == WL_CONNECTED) return;
  WiFi.begin(SSID, PASSWORD);
  while (WiFi.status() != WL_CONNECTED) {
    delay(250);
    Serial.print(".");
  }
  Serial.println();
  Serial.print("WiFi OK - IP: ");
  Serial.println(WiFi.localIP());
}
 
void reconnectMQTT() {
  while (!MQTT.connected()) {
    Serial.printf("* Conectando ao MQTT: %s:%d\n", BROKER_MQTT, BROKER_PORT);
    if (MQTT.connect(ID_MQTT)) {
      Serial.println("MQTT conectado!");
    } else {
      Serial.println("Falha ao conectar. Nova tentativa em 2s...");
      delay(2000);
    }
  }
}
 
void ensureConnections() {
  if (!MQTT.connected()) reconnectMQTT();
  reconnectWiFi();
}
 
// Publica temperatura/umidade a cada 5s
void handleDHT() {
  static unsigned long last = 0;
  const unsigned long intervalo = 5000;
  if (millis() - last < intervalo) return;
  last = millis();
 
  float t = dht.readTemperature(); // °C
  float h = dht.readHumidity();    // %UR
 
  if (isnan(t) || isnan(h)) {
    Serial.println("[DHT] Falha na leitura, tentando novamente...");
    return;
  }
 
  char bufT[16], bufH[16];
  dtostrf(t, 0, 2, bufT);
  dtostrf(h, 0, 2, bufH);
 
  MQTT.publish(TOPIC_PUBLISH_TEMP.c_str(), bufT);
  MQTT.publish(TOPIC_PUBLISH_HUM.c_str(),  bufH);
 
  Serial.print("[PUB] ");
  Serial.print(TOPIC_PUBLISH_TEMP);
  Serial.print(" = ");
  Serial.print(bufT);
  Serial.print(" | ");
  Serial.print(TOPIC_PUBLISH_HUM);
  Serial.print(" = ");
  Serial.println(bufH);
}