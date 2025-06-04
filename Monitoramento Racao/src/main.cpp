#include <Arduino.h>
#include "config.h" 

// Bibliotecas necessárias (conforme platformio.ini e seu sketch original)
#include <WiFi.h>
#include <PubSubClient.h>
#include "HX711.h"
#include <ESP32Servo.h> // Ou <Servo.h> se estiver usando a biblioteca Servo padrão do ESP32

// --- Declaração de Objetos Globais ---
Servo petServo; // Renomeado de 'pet' para clareza
HX711 balancaRecipiente; // Renomeado de 'balanca' para clareza, pois é do recipiente

WiFiClient espClientRacao; // Nome do cliente WiFi específico para este ESP32
PubSubClient client(espClientRacao);

// --- Variáveis Globais ---
float fatorCalibracaoRecipiente = HX711_RECIPIENTE_CALIBRATION_FACTOR; // Do config.h
String my_payLoad_racao; // Variáveis globais do seu sketch original, considere torná-las locais se possível
String my_topic_racao;

// Variável para simular o total de ração no RESERVATÓRIO PRINCIPAL (em gramas)
// No futuro, isso será substituído pela leitura de um sensor de peso real para o reservatório.
//float totalRacaoNoReservatorioSimulado = 2000.0;  Valor inicial, pode ser ajustado ou vir de uma configuração

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

float obterPesoRecipiente() {
    if (balancaRecipiente.is_ready()) {
        // get_units() retorna o valor na unidade para a qual a escala foi calibrada.
        // Se fatorCalibracaoRecipiente foi ajustado para gramas, isto retornará gramas.
        return balancaRecipiente.get_units(10); 
    } else {
        Serial.println("Sensor de peso do recipiente (HX711) nao esta pronto!");
        return -1; // Indica erro (ou 0.0 se preferir não usar -1)
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
    // Publicar status do dispensador (opcional)
    // client.publish(MQTT_TOPICO_RACAO_DISPENSADOR_STATUS, "ABERTO");

    delay(TEMPO_SERVO_ABERTO_PARA_DISPENSA_MS); // Do config.h

    petServo.write(ANGULO_SERVO_FECHADO); // Do config.h
    Serial.println("Dispensador: Servo Fechado.");
    // Publicar status do dispensador (opcional)
    // client.publish(MQTT_TOPICO_RACAO_DISPENSADOR_STATUS, "FECHADO");
    
    Serial.println("Racao dispensada.");
    Serial.println("------------------------------------------------------");

    // Não há mais decremento de 'totalRacaoNoReservatorioSimulado' aqui
    // Não há mais publicação do total do reservatório simulado daqui
}

void publicarDadosRacao() {
    // Publicar o peso do recipiente
    float pesoNoRecipiente = obterPesoRecipiente();
    if (pesoNoRecipiente != -1) { // Se a leitura do peso foi válida
        Serial.print("Peso no recipiente: ");
        Serial.print(pesoNoRecipiente);
        Serial.println("g");
        char pesoStr[10];
        dtostrf(pesoNoRecipiente, 6, 2, pesoStr);
        client.publish(MQTT_TOPICO_RACAO_RECIPIENTE_PESO, pesoStr); // Do config.h
    }

    // REMOVIDO: Publicação do total de ração no reservatório (simulado)
    // REMOVIDO: Verificação e alerta para o total de ração (simulado) baixo

    // Lógica de dispensar ração automaticamente se o recipiente estiver com pouco
    if (pesoNoRecipiente != -1 && pesoNoRecipiente <= PESO_RECIPIENTE_ACIONA_DISPENSA_AUTO_G) { // Do config.h
        Serial.println("Pouca racao detectada no recipiente, acionando dispensa automatica...");
        dispensarRacao(); // Chama a função simplificada
    } else if (pesoNoRecipiente != -1) {
        // Serial.println("Nao liberando racao automaticamente (recipiente com quantidade suficiente).");
    }
    // A linha abaixo pode ser removida se não houver mais nada a imprimir aqui.
    // Serial.println("------------------------------------------------------"); 
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

    balancaRecipiente.begin(PIN_HX711_RECIPIENTE_DT, PIN_HX711_RECIPIENTE_SCK);

    Serial.println("Aguardando estabilizacao do sensor de peso...");
    delay(1000); 

    // Definir a escala PRIMEIRO com o fator calibrado
    balancaRecipiente.set_scale(fatorCalibracaoRecipiente); 
    // DEPOIS fazer o tare para zerar com a escala já aplicada
    balancaRecipiente.tare(); 

    Serial.print("Offset apos tare: "); Serial.println(balancaRecipiente.get_offset());
    Serial.print("Escala definida com (config.h): "); Serial.println(fatorCalibracaoRecipiente); // Deve mostrar 0.42

    Serial.println("Sistema de Racao Iniciado e Funcionando.");
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