#ifndef CONFIG_H
#define CONFIG_H

// --- Configurações de WiFi ---
// !! IMPORTANTE: Substitua pelas suas credenciais reais quando for para o ESP32 físico !!
const char* SSID_WIFI = "Wokwi-GUEST"; // Para simulação Wokwi
// const char* SSID_WIFI = "SUA_REDE_WIFI"; // Para uso real
const char* PASSWORD_WIFI = "";
// const char* PASSWORD_WIFI = "SUA_SENHA_WIFI"; // Para uso real


// --- Configurações MQTT ---
// !! IMPORTANTE: Use seu broker MQTT se tiver um, ou mantenha test.mosquitto.org para testes !!
const char* MQTT_SERVER = "test.mosquitto.org";
const int MQTT_PORT = 1883;
const char* MQTT_CLIENT_ID = "ESP32_Canil_Client"; // ID único para o cliente MQTT

// Tópicos MQTT (prefixo para evitar colisões, se necessário)
#define MQTT_PREFIX "canilGuilherme/" // Adicione um prefixo se quiser
#define MQTT_TOPIC_NIVEL_AGUA             MQTT_PREFIX "agua/nivel"
#define MQTT_TOPICO_PESO_RESERVATORIO     MQTT_PREFIX "racao/reservatorio/peso"
#define MQTT_TOPICO_TEMPERATURA           MQTT_PREFIX "ambiente/temperatura"
#define MQTT_TOPICO_CONTROLE_AGUA_CMD     MQTT_PREFIX "agua/valvula/comando" // Para receber comandos
#define MQTT_TOPICO_STATUS_VALVULA_STAT   MQTT_PREFIX "agua/valvula/status"  // Para enviar status
#define MQTT_TOPICO_ALERTA_PESO           MQTT_PREFIX "racao/reservatorio/alerta"
#define MQTT_TOPICO_ALERTA_TEMP           MQTT_PREFIX "ambiente/temperatura/alerta"
// Tópico que estava no seu código original como "ESP32_RECEBE", unificar se for o mesmo que CONTROLE_AGUA_CMD
#define MQTT_TOPICO_GENERICO_RECEBE       MQTT_PREFIX "geral/recebe" 
// Tópico "TOPICO_WOKWI" que estava no seu código (associado ao analogRead(34))
#define MQTT_TOPICO_POTENCIOMETRO_WOKWI   MQTT_PREFIX "debug/potenciometro" 


// --- Pinos dos Sensores e Atuadores ---
#define PIN_LED_VALVULA 14         // LED Azul no seu Wokwi (simula válvula)
#define PIN_DHT 4                  // Pino de dados do DHT22
#define PIN_LED_ARCONDICIONADO 16  // LED Ciano no seu Wokwi (simula ar condicionado)
#define PIN_TRIG_ULTRASONIC 13     // HC-SR04 Trigger
#define PIN_ECHO_ULTRASONIC 12     // HC-SR04 Echo
#define PIN_HX711_DT 27            // HX711 DT
#define PIN_HX711_SCK 26           // HX711 SCK
#define PIN_LED_WIFI_STATUS 2      // LED Verde no seu Wokwi (status Wi-Fi)
#define PIN_LED_MQTT_STATUS 15     // LED Roxo no seu Wokwi (status MQTT)

// --- Parâmetros de Sensores e Lógica ---
#define DHT_TYPE DHT22
#define HX711_CALIBRATION_FACTOR 420.0f // Ajuste conforme sua célula de carga
#define LIMITE_NIVEL_AGUA_ACIONAMENTO 15.0f // Nível de água para acionar válvula (verificar lógica)
#define TEMPO_ACIONAMENTO_MANUAL_MS 10000 // 10 segundos para modo manual da válvula
#define INTERVALO_PUBLICACAO_PESO_MS 1000 // Intervalo para publicar peso
#define INTERVALO_PUBLICACAO_TEMPERATURA_MS 5000 // Intervalo para publicar temperatura
#define LIMITE_PESO_BAIXO_RESERVATORIO_KG 5.0f // Limite para alerta de peso baixo
#define LIMITE_TEMPERATURA_ALTA_C 35.0f // Limite para alerta de temperatura alta

#endif