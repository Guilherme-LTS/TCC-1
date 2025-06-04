#include <Arduino.h>
#include "config.h" 

#include <WiFi.h>
#include <PubSubClient.h>
#include "HX711.h"
#include <ESP32Servo.h> // Ou <Servo.h> se estiver usando a biblioteca Servo padrão do ESP32

// --- Declaração de Objetos Globais ---
Servo petServo; 
HX711 balancaRecipiente; 
HX711 balancaReservatorioPrincipal;

WiFiClient espClientRacao; // Nome do cliente WiFi específico para este ESP32
PubSubClient client(espClientRacao);

// --- Variáveis Globais ---
float fatorCalibracaoRecipiente = HX711_RECIPIENTE_CALIBRATION_FACTOR;
float fatorCalibracaoReservatorioPrincipal = HX711_RESERVATORIO_PRINCIPAL_CALIBRATION_FACTOR;
String my_payLoad_racao; // Variáveis globais do seu sketch original, considere torná-las locais se possível
String my_topic_racao;

// --- Protótipos de Funções (Opcional, mas boa prática) ---
void setup_wifi_racao();
void callback_racao(char* topic, byte* payload, unsigned int length);
void reconnect_mqtt_racao();
float obterPesoRecipiente();
void atualizaLedStatusWifiRacao();
void atualizaLedStatusMqttRacao();
void dispensarRacao();
void publicarDadosRacao();

// --- Implementação das Funções ---

void setup_wifi_racao() {
    delay(10);
    Serial.println();
    Serial.print("Conectando WiFi Racao a ");
    Serial.println(SSID_WIFI_RACAO); // Do config.h

    WiFi.mode(WIFI_STA);
    WiFi.begin(SSID_WIFI_RACAO, PASSWORD_WIFI_RACAO); // Do config.h

    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        Serial.print(".");
    }
    Serial.println("");
    Serial.println("WiFi Racao conectado!");
    Serial.print("Endereco IP: ");
    Serial.println(WiFi.localIP());
}

void callback_racao(char* topic, byte* payload, unsigned int length) {
    String conc_payload;
    for (int i = 0; i < length; i++) {
        conc_payload += ((char)payload[i]);
    }
    my_payLoad_racao = conc_payload; 
    my_topic_racao = topic;

    Serial.print("Mensagem recebida no topico [Racao]: ");
    Serial.println(topic);
    Serial.print("Payload: ");
    Serial.println(my_payLoad_racao);

    if (String(topic) == MQTT_TOPICO_RACAO_DISPENSAR_CMD) { // Do config.h
        if (my_payLoad_racao == "LIBERAR") {
            Serial.println("Comando 'LIBERAR' recebido via MQTT.");
            dispensarRacao(); // Chama a função simplificada para dispensar
        }
    }
}

void reconnect_mqtt_racao() {
    while (!client.connected()) {
        Serial.print("Tentando conexao MQTT Racao...");
        if (client.connect(MQTT_CLIENT_ID_RACAO)) { // Do config.h
            Serial.println("Conectado ao broker MQTT (Racao)!");
            client.subscribe(MQTT_TOPICO_RACAO_DISPENSAR_CMD); // Do config.h
        } else {
            Serial.print("falhou, rc=");
            Serial.print(client.state());
            Serial.println(" Tentando novamente em 5 segundos");
            delay(5000); // Bloqueante, considere alternativas em produção
        }
    }
}

float obterPesoReservatorioPrincipal() {
    if (balancaReservatorioPrincipal.is_ready()) {
        long valor_bruto_apos_tare = balancaReservatorioPrincipal.get_value(10); // Pega (leitura_media - offset)
        // Se fatorCalibracaoReservatorioPrincipal for 0, evite divisão por zero
        if (fatorCalibracaoReservatorioPrincipal == 0) {
            Serial.println("ERRO: Fator de Calibracao do Reservatorio Principal eh ZERO!");
            return -2; // Ou outro código de erro
        }
        float peso_calculado = (float)valor_bruto_apos_tare / fatorCalibracaoReservatorioPrincipal;
        return peso_calculado;
    } else {
        Serial.println("Sensor de peso do RESERVATORIO PRINCIPAL (HX711) nao esta pronto!");
        return -1; 
    }
}

float obterPesoRecipiente() {
    if (balancaRecipiente.is_ready()) {
        long valor_bruto_apos_tare = balancaRecipiente.get_value(10); // Pega (leitura_media - offset)
        // Se fatorCalibracaoRecipiente for 0, evite divisão por zero
        if (fatorCalibracaoRecipiente == 0) {
            Serial.println("ERRO: Fator de Calibracao do Recipiente eh ZERO!");
            return -2; // Ou outro código de erro
        }
        float peso_calculado = (float)valor_bruto_apos_tare / fatorCalibracaoRecipiente;
        return peso_calculado;
    } else {
        Serial.println("Sensor de peso do recipiente (HX711) nao esta pronto!");
        return -1; 
    }
}

void publicarDadosRacao() {
    // Publicar o peso do recipiente (como antes)
    float pesoNoRecipiente = obterPesoRecipiente();
    if (pesoNoRecipiente != -1) {
        Serial.print("Peso no recipiente: "); Serial.print(pesoNoRecipiente); Serial.println("g");
        char pesoRecStr[10];
        dtostrf(pesoNoRecipiente, 6, 2, pesoRecStr);
        client.publish(MQTT_TOPICO_RACAO_RECIPIENTE_PESO, pesoRecStr);
    }

    // Publicar o peso do RESERVATÓRIO PRINCIPAL de ração
    float pesoNoReservatorioPrincipal = obterPesoReservatorioPrincipal();
    if (pesoNoReservatorioPrincipal != -1) {
        Serial.print("Peso no RESERVATORIO PRINCIPAL: "); Serial.print(pesoNoReservatorioPrincipal); Serial.println("g");
        char pesoResStr[10];
        dtostrf(pesoNoReservatorioPrincipal, 6, 2, pesoResStr);
        client.publish(MQTT_TOPICO_RACAO_RESERVATORIO_PRINCIPAL_PESO, pesoResStr);

        // Verificar se o reservatório principal está baixo e enviar alerta
        if (pesoNoReservatorioPrincipal <= LIMITE_PESO_BAIXO_RESERVATORIO_PRINCIPAL_G) {
            client.publish(MQTT_TOPICO_RACAO_RESERVATORIO_PRINCIPAL_ALERTA, "Reservatorio PRINCIPAL de racao BAIXO!");
            Serial.println("ALERTA: Reservatorio PRINCIPAL de racao baixo!");
        }
    }

    // Lógica de dispensar ração automaticamente (como antes)
    if (pesoNoRecipiente != -1 && pesoNoRecipiente <= PESO_RECIPIENTE_ACIONA_DISPENSA_AUTO_G) {
        Serial.println("Pouca racao detectada no recipiente, acionando dispensa automatica...");
        dispensarRacao();
    }
}

void atualizaLedStatusWifiRacao() {
    if (WiFi.status() == WL_CONNECTED)
        digitalWrite(PIN_LED_STATUS_WIFI_RACAO, HIGH); // Do config.h
    else
        digitalWrite(PIN_LED_STATUS_WIFI_RACAO, LOW); // Do config.h
}

void atualizaLedStatusMqttRacao() {
    if (client.connected())
        digitalWrite(PIN_LED_STATUS_MQTT_RACAO, HIGH); // Do config.h
    else
        digitalWrite(PIN_LED_STATUS_MQTT_RACAO, LOW); // Do config.h
}

void dispensarRacao() {
    // Esta função não verifica mais o 'totalRacaoNoReservatorioSimulado'
    // A decisão de chamar esta função já foi tomada (recipiente baixo ou comando MQTT)

    petServo.write(ANGULO_SERVO_ABERTO); // Do config.h
    Serial.println("Dispensador: Servo Aberto.");
    client.publish(MQTT_TOPICO_RACAO_DISPENSADOR_STATUS, "ABERTO"); // <-- NOVA LINHA

    delay(TEMPO_SERVO_ABERTO_PARA_DISPENSA_MS); // Do config.h

    petServo.write(ANGULO_SERVO_FECHADO); // Do config.h
    Serial.println("Dispensador: Servo Fechado.");
    client.publish(MQTT_TOPICO_RACAO_DISPENSADOR_STATUS, "FECHADO"); // <-- NOVA LINHA
    
    Serial.println("Racao dispensada.");
    Serial.println("------------------------------------------------------");
}


// --- Função setup() ---

void setup() {
    pinMode(PIN_LED_STATUS_WIFI_RACAO, OUTPUT);
    pinMode(PIN_LED_STATUS_MQTT_RACAO, OUTPUT);
    
    Serial.begin(115200);
    setup_wifi_racao(); 
    
    client.setServer(MQTT_SERVER_RACAO, MQTT_PORT_RACAO); 
    client.setCallback(callback_racao); 

    petServo.attach(PIN_SERVO_DISPENSADOR); 
    petServo.write(ANGULO_SERVO_FECHADO);   

    // Inicialização do sensor de peso do RECIPIENTE
    balancaRecipiente.begin(PIN_HX711_RECIPIENTE_DT, PIN_HX711_RECIPIENTE_SCK);
    Serial.println("Tentando inicializar sensor de peso do RECIPIENTE...");
    int retriesRecipiente = 0;
    while (!balancaRecipiente.is_ready() && retriesRecipiente < 10) { // Tenta por até 5 segundos
        Serial.print(".");
        delay(500);
        retriesRecipiente++;
    }
    Serial.println(); // Nova linha após os pontos

    if (balancaRecipiente.is_ready()) {
        Serial.println("Sensor do RECIPIENTE pronto. Aguardando estabilizacao...");
        delay(1000); 
        balancaRecipiente.set_scale(fatorCalibracaoRecipiente); 
        balancaRecipiente.tare(); 
        
    } else {
        Serial.println("ERRO: Sensor de peso do RECIPIENTE nao encontrado apos tentativas!");
    }

    // Inicialização do sensor de peso do RESERVATÓRIO PRINCIPAL
    balancaReservatorioPrincipal.begin(PIN_HX711_RESERVATORIO_PRINCIPAL_DT, PIN_HX711_RESERVATORIO_PRINCIPAL_SCK);
    Serial.println("Tentando inicializar sensor de peso do RESERVATORIO PRINCIPAL...");
    int retriesReservatorio = 0;
    while (!balancaReservatorioPrincipal.is_ready() && retriesReservatorio < 10) { // Tenta por até 5 segundos
        Serial.print(".");
        delay(500);
        retriesReservatorio++;
    }
    Serial.println(); // Nova linha após os pontos

    if (balancaReservatorioPrincipal.is_ready()) {
        Serial.println("Sensor do RESERVATORIO PRINCIPAL pronto. Aguardando estabilizacao...");
        delay(1000); 
        balancaReservatorioPrincipal.set_scale(fatorCalibracaoReservatorioPrincipal); 
        balancaReservatorioPrincipal.tare(); 
        
    } else {
        Serial.println("ERRO: Sensor de peso do RESERVATORIO PRINCIPAL nao encontrado apos tentativas!");
    }
    
    Serial.println("Sistema de Racao (com 2 sensores) Iniciado.");
}

// --- Função loop() ---
void loop() {
    if (WiFi.status() != WL_CONNECTED) {
        setup_wifi_racao(); 
    }
    if (!client.connected()) {
        reconnect_mqtt_racao();
    }
    client.loop(); // Essencial para o PubSubClient

    atualizaLedStatusWifiRacao();
    atualizaLedStatusMqttRacao();

    publicarDadosRacao(); // Função que agora engloba publicações e lógica de auto-dispensa

    // Ajuste do fator de calibração pelo Monitor Serial (do seu sketch original)
    if (Serial.available()) {
        char input = Serial.read();
        if (input == '+') {
            fatorCalibracaoRecipiente += 10; // Incrementa o fator
            balancaRecipiente.set_scale(fatorCalibracaoRecipiente);
            Serial.print("Novo fator de calibracao (Recipiente): ");
            Serial.println(fatorCalibracaoRecipiente);
        } else if (input == '-') {
            fatorCalibracaoRecipiente -= 10; // Decrementa o fator
            balancaRecipiente.set_scale(fatorCalibracaoRecipiente);
            Serial.print("Novo fator de calibracao (Recipiente): ");
            Serial.println(fatorCalibracaoRecipiente);
        }
    }

    delay(1000); // ATENÇÃO: delay() bloqueia. Considere alternativas com millis() para um código mais responsivo.
}