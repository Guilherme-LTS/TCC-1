#ifndef CONFIG_RACAO_H
#define CONFIG_RACAO_H

// --- Configurações de WiFi ---
const char* SSID_WIFI_RACAO = "Wokwi-GUEST";
const char* PASSWORD_WIFI_RACAO = "";

// --- Configurações MQTT ---
const char* MQTT_SERVER_RACAO = "test.mosquitto.org";
const int MQTT_PORT_RACAO = 1883;
const char* MQTT_CLIENT_ID_RACAO = "ESP32_Canil_Racao_Client";
#define MQTT_PREFIX_RACAO "canilGuilherme/" // Mesmo prefixo dos outros projetos

// --- Tópicos MQTT Específicos para Ração ---

// Sensor de Peso do RECIPIENTE de Ração (comedouro - HX711 existente no seu Wokwi da ração)
#define MQTT_TOPICO_RACAO_RECIPIENTE_PESO       MQTT_PREFIX_RACAO "racao/recipiente/peso"     // ESP32 publica o peso do comedouro aqui
// (Seu sketch atual publica em "RACAO_ESP32_ENVIA")

// RESERVATÓRIO Principal de Ração (atualmente SIMULADO no seu sketch com a variável 'totalRacao')
// Quando você adicionar um sensor de PESO REAL para o reservatório principal:
#define MQTT_TOPICO_RACAO_RESERVATORIO_PRINCIPAL_PESO   MQTT_PREFIX_RACAO "racao/reservatorio_principal/peso"
#define MQTT_TOPICO_RACAO_RESERVATORIO_PRINCIPAL_ALERTA MQTT_PREFIX_RACAO "racao/reservatorio_principal/alerta"
// Para o total SIMULADO que seu sketch atual publica:
// #define MQTT_TOPICO_RACAO_RESERVATORIO_SIMULADO_TOTAL   MQTT_PREFIX_RACAO "racao/reservatorio_simulado/total" // ESP32 publica 'totalRacao' aqui
// (Seu sketch atual publica em "RACAO_ESP32_REPOSITÓRIO")
// #define MQTT_TOPICO_RACAO_RESERVATORIO_SIMULADO_ALERTA  MQTT_PREFIX_RACAO "racao/reservatorio_simulado/alerta" // ESP32 publica alerta de 'totalRacao' baixo aqui
// (Seu sketch atual publica em "ALERTA_RACAO")


// Comando para Dispensar Ração (Node-RED -> ESP32)
#define MQTT_TOPICO_RACAO_DISPENSAR_CMD         MQTT_PREFIX_RACAO "racao/dispensador/comando" // ESP32 ouve comandos aqui
// (Seu sketch atual ouve "RACAO_ESP32_RECEBE")

// --- Pinos dos Sensores e Atuadores para Ração (Conforme seu Wokwi da Ração) ---

// Sensor de Peso HX711 para o RECIPIENTE de Ração (comedouro)
#define PIN_HX711_RECIPIENTE_DT   2   // Conforme seu diagram.json: cell1:DT -> esp:2
#define PIN_HX711_RECIPIENTE_SCK  4   // Conforme seu diagram.json: cell1:SCK -> esp:4

// Atuador para Dispensar Ração (Servo Motor)
#define PIN_SERVO_DISPENSADOR 26  // Conforme seu diagram.json: servo1:PWM -> esp:26

// LEDs de Status (Conforme seu Wokwi da Ração e sketch)
#define PIN_LED_STATUS_WIFI_RACAO   0   // Conforme seu sketch e diagram.json (led1 -> r1 -> esp:0)
#define PIN_LED_STATUS_MQTT_RACAO   15  // Conforme seu sketch e diagram.json (led2 -> r2 -> esp:15)

// --- Pinos para o FUTURO Sensor de Peso do RESERVATÓRIO Principal de Ração ---
// !! ESCOLHA PINOS DISPONÍVEIS QUANDO FOR IMPLEMENTAR ESTE SENSOR !!
// #define PIN_HX711_RESERVATORIO_PRINCIPAL_DT 16  // Exemplo
// #define PIN_HX711_RESERVATORIO_PRINCIPAL_SCK 17 // Exemplo


// --- Parâmetros de Sensores e Lógica para Ração ---
#define HX711_RECIPIENTE_CALIBRATION_FACTOR   420.0f // Do seu sketch (era 'CALIBRACAO')

// Limites e quantidades (do seu sketch)
#define PESO_RECIPIENTE_ACIONA_DISPENSA_AUTO_G 100.0f // Se peso <= 100g no recipiente, dispensa auto
#define QUANTIDADE_RACAO_A_DISPENSAR_G        50.0f  // Quantidade dispensada por vez
#define LIMITE_MINIMO_RESERVATORIO_PARA_DISPENSAR_G 50.0f // Precisa ter pelo menos isso no reservatório para dispensar
#define LIMITE_ALERTA_RESERVATORIO_SIMULADO_G 1800.0f // No seu sketch, totalRacao <= 1800.0 dispara alerta (ajuste se totalRacao inicial mudar de 2000g)


// Parâmetros do dispensador (Servo)
#define ANGULO_SERVO_FECHADO 90
#define ANGULO_SERVO_ABERTO  0
#define TEMPO_SERVO_ABERTO_PARA_DISPENSA_MS 3000 // Mantém aberto por 3 segundos


#endif // CONFIG_RACAO_H