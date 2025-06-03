#include <Arduino.h>
#include "config.h" // Nosso arquivo de configuração para o sistema de ração

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
float totalRacaoNoReservatorioSimulado = 2000.0; // Valor inicial, pode ser ajustado ou vir de uma configuração

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
    // As variáveis globais my_payLoad_racao e my_topic_racao podem ser substituídas
    // pelo uso direto de 'topic' e 'conc_payload' dentro desta função.
    my_payLoad_racao = conc_payload; 
    my_topic_racao = topic;

    Serial.print("Mensagem recebida no topico [Racao]: ");
    Serial.println(topic);
    Serial.print("Payload: ");
    Serial.println(my_payLoad_racao);

    if (String(topic) == MQTT_TOPICO_RACAO_DISPENSAR_CMD) { // Do config.h
        if (my_payLoad_racao == "LIBERAR") {
            dispensarRacao(); // Chama a função centralizada para dispensar
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
        long leitura = balancaRecipiente.get_units(10); // Média de 10 leituras
        // A conversão para gramas já estava no seu sketch, mantida aqui.
        // Se fatorCalibracaoRecipiente for para kg, então *1000 para gramas.
        // Seu sketch original fazia: (leitura / fator) * 100. Assumindo que o fator é para 10g por unidade.
        // Ajuste a fórmula conforme a unidade esperada pelo seu fator de calibração.
        // Exemplo: se fatorCalibracaoRecipiente dá o peso em gramas diretamente:
        // float pesoEmGramas = leitura / fatorCalibracaoRecipiente;
        // Exemplo: se fatorCalibracaoRecipiente dá o peso em kg:
        // float pesoEmGramas = (leitura / fatorCalibracaoRecipiente) * 1000.0;

        // Baseado no seu sketch: `pesoEmKg = leitura / fatorCalibracao; return pesoEmKg * 100;`
        // Isso é um pouco confuso. Se `fatorCalibracao` é para KG, então `pesoEmKg` está em KG.
        // Multiplicar por 100 daria Hectogramas.
        // Vamos assumir que `fatorCalibracaoRecipiente` é ajustado para que a saída direta seja em gramas.
        // Se `fatorCalibracaoRecipiente` foi calibrado para dar leitura em KG:
        // return (balancaRecipiente.get_units(10) / fatorCalibracaoRecipiente) * 1000.0; // Converte para gramas
        // Se `fatorCalibracaoRecipiente` foi calibrado para dar leitura em GRAMAS:
        return balancaRecipiente.get_units(10) / fatorCalibracaoRecipiente;

    } else {
        Serial.println("Sensor de peso do recipiente (HX711) nao esta pronto!");
        return -1; // Indica erro
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
    if (totalRacaoNoReservatorioSimulado >= QUANTIDADE_RACAO_A_DISPENSAR_G) { // Do config.h
        petServo.write(ANGULO_SERVO_ABERTO); // Do config.h
        Serial.println("Dispensador: Servo Aberto.");
        delay(TEMPO_SERVO_ABERTO_PARA_DISPENSA_MS); // Do config.h
        petServo.write(ANGULO_SERVO_FECHADO); // Do config.h
        Serial.println("Dispensador: Servo Fechado.");
        
        totalRacaoNoReservatorioSimulado -= QUANTIDADE_RACAO_A_DISPENSAR_G; // Do config.h
        
        Serial.print("Racao dispensada. Reservatorio simulado agora com: ");
        Serial.print(totalRacaoNoReservatorioSimulado);
        Serial.println("g");
        Serial.println("------------------------------------------------------");

        // Publicar novo total do reservatório simulado
        char totalStr[10];
        dtostrf(totalRacaoNoReservatorioSimulado, 6, 2, totalStr);
        client.publish(MQTT_TOPICO_RACAO_RESERVATORIO_SIMULADO_TOTAL, totalStr);

    } else {
        Serial.println("Nao ha racao suficiente no reservatorio (simulado) para dispensar!");
        client.publish(MQTT_TOPICO_RACAO_RESERVATORIO_SIMULADO_ALERTA, "Tentativa de dispensar, mas reservatorio SIMULADO de racao baixo!");
    }
}

void publicarDadosRacao() {
    // Publicar o peso do recipiente
    float pesoNoRecipiente = obterPesoRecipiente();
    if (pesoNoRecipiente != -1) {
        Serial.print("Peso no recipiente: ");
        Serial.print(pesoNoRecipiente);
        Serial.println("g");
        char pesoStr[10];
        dtostrf(pesoNoRecipiente, 6, 2, pesoStr);
        client.publish(MQTT_TOPICO_RACAO_RECIPIENTE_PESO, pesoStr); // Do config.h
    }

    // Publicar o total de ração no reservatório (simulado)
    char totalRacaoStr[10];
    dtostrf(totalRacaoNoReservatorioSimulado, 6, 2, totalRacaoStr);
    client.publish(MQTT_TOPICO_RACAO_RESERVATORIO_SIMULADO_TOTAL, totalRacaoStr); // Do config.h
    Serial.print("Reservatorio (simulado) total: ");
    Serial.print(totalRacaoNoReservatorioSimulado);
    Serial.println("g");

    // Verifica se o total de ração (simulado) está baixo e envia o alerta
    if (totalRacaoNoReservatorioSimulado <= LIMITE_ALERTA_RESERVATORIO_SIMULADO_G) { // Do config.h
        client.publish(MQTT_TOPICO_RACAO_RESERVATORIO_SIMULADO_ALERTA, "Reservatorio SIMULADO de racao baixo! Adicione mais racao."); // Do config.h
        Serial.println("ALERTA: Reservatorio SIMULADO de racao baixo!");
    }

    // Lógica de dispensar ração automaticamente se o recipiente estiver com pouco
    if (pesoNoRecipiente != -1 && pesoNoRecipiente <= PESO_RECIPIENTE_ACIONA_DISPENSA_AUTO_G) { // Do config.h
        Serial.println("Pouca racao detectada no recipiente, acionando dispensa automatica...");
        dispensarRacao(); // Chama a função centralizada
    } else if (pesoNoRecipiente != -1) {
        // Serial.println("Nao liberando racao automaticamente (recipiente com quantidade suficiente).");
    }
    Serial.println("------------------------------------------------------");
}


// --- Função setup() ---
void setup() {
    pinMode(PIN_LED_STATUS_WIFI_RACAO, OUTPUT);   // Do config.h
    pinMode(PIN_LED_STATUS_MQTT_RACAO, OUTPUT);   // Do config.h
    
    Serial.begin(115200);
    setup_wifi_racao();
    
    client.setServer(MQTT_SERVER_RACAO, MQTT_PORT_RACAO); // Do config.h
    client.setCallback(callback_racao);

    petServo.attach(PIN_SERVO_DISPENSADOR); // Do config.h
    petServo.write(ANGULO_SERVO_FECHADO);   // Do config.h - Servo fechado inicialmente

    balancaRecipiente.begin(PIN_HX711_RECIPIENTE_DT, PIN_HX711_RECIPIENTE_SCK); // Do config.h
    balancaRecipiente.set_scale(fatorCalibracaoRecipiente); // Usa o fator de calibração
    balancaRecipiente.tare(); // Zera a balança do recipiente

    Serial.println("Sistema de Racao Iniciado e Funcionando.");
    Serial.print("Fator de Calibracao Inicial (Recipiente): ");
    Serial.println(fatorCalibracaoRecipiente);
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