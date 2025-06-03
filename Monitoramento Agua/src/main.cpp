#include <Arduino.h>

// Bibliotecas necessárias
#include <WiFi.h>
#include <PubSubClient.h>
#include "HX711.h" // Para o sensor de peso
#include <DHT.h>   // Para o sensor de temperatura e umidade

// --- Declaração de Objetos Globais ---
WiFiClient espClient; // Renomeado de WOKWI_Client para maior generalidade
PubSubClient client(espClient);
DHT dht(PIN_DHT, DHT_TYPE); // Usa PIN_DHT e DHT_TYPE de config.h
HX711 balanca;

// --- Variáveis Globais ---
// String my_payload; // Considerar se são realmente necessárias globalmente
// String my_topic;   // ou se podem ser locais. Se usadas apenas em 'callback', podem ser locais.
bool manualMode = false;
unsigned long manualStartTime = 0;
bool autoDisabled = false; // Relacionado ao controle manual da válvula
// unsigned long startTime = 0; // Usada em Publica_dados, que tem um delay(1000)
bool alertaReservatorioEnviado = false; // Para controlar o envio único do alerta de peso


// --- Funções Auxiliares ---

void atualizaEstadoValvula(bool estado) {
    if (estado) {
        client.publish(MQTT_TOPICO_STATUS_VALVULA_STAT, "ON");
    } else {
        client.publish(MQTT_TOPICO_STATUS_VALVULA_STAT, "OFF");
    }
}

void setup_wifi() {
    delay(10);
    Serial.println();
    Serial.print("Conectando a ");
    Serial.println(SSID_WIFI); // De config.h

    WiFi.mode(WIFI_STA);
    WiFi.begin(SSID_WIFI, PASSWORD_WIFI); // De config.h

    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        Serial.print(".");
    }

    Serial.println("");
    Serial.println("WiFi conectado!");
    Serial.print("Endereco IP: ");
    Serial.println(WiFi.localIP());
}

void callback(char* topic, byte* payload, unsigned int length) {
    String command = "";
    for (int i = 0; i < length; i++) {
        command += (char)payload[i];
    }
    Serial.print("Mensagem recebida no topico: ");
    Serial.println(topic);
    Serial.print("Comando: ");
    Serial.println(command);

    if (String(topic) == MQTT_TOPICO_CONTROLE_AGUA_CMD) { // De config.h
        if (command == "manual_on") {
            manualMode = true;
            autoDisabled = true; // Indica que o controle automático da válvula está temporariamente desabilitado
            manualStartTime = millis();
            digitalWrite(PIN_LED_VALVULA, HIGH); // De config.h
            atualizaEstadoValvula(true);
            Serial.println("Valvula acionada manualmente!");
        } else if (command == "manual_off") {
            manualMode = false;
            // autoDisabled não é resetado aqui, mas em verificaTempoManual()
            digitalWrite(PIN_LED_VALVULA, LOW); // De config.h
            atualizaEstadoValvula(false);
            Serial.println("Valvula desligada manualmente!");
        }
    }
}

void reconnect() {
    while (!client.connected()) {
        Serial.print("Tentando conexao MQTT...");
        if (client.connect(MQTT_CLIENT_ID)) { // MQTT_CLIENT_ID de config.h
            Serial.println("Conectado ao broker MQTT!");
            // Inscrever-se nos tópicos necessários
            client.subscribe(MQTT_TOPICO_CONTROLE_AGUA_CMD); // De config.h
            // client.subscribe(MQTT_TOPICO_GENERICO_RECEBE); // Se for usar este tópico também
        } else {
            Serial.print("falhou, rc=");
            Serial.print(client.state());
            Serial.println(" Tentando novamente em 5 segundos");
            delay(5000);
        }
    }
}

float readWaterLevel() {
    digitalWrite(PIN_TRIG_ULTRASONIC, LOW); // De config.h
    delayMicroseconds(2);
    digitalWrite(PIN_TRIG_ULTRASONIC, HIGH); // De config.h
    delayMicroseconds(10);
    digitalWrite(PIN_TRIG_ULTRASONIC, LOW); // De config.h

    long duration = pulseIn(PIN_ECHO_ULTRASONIC, HIGH); // De config.h
    float distance = duration * 0.034 / 2; // Calcula distância em cm
    return distance;
}

void atualizaLedStatusWifi() { // Renomeada de Conexao_WIFI
    if (WiFi.status() == WL_CONNECTED) {
        digitalWrite(PIN_LED_WIFI_STATUS, HIGH); // De config.h
    } else {
        digitalWrite(PIN_LED_WIFI_STATUS, LOW); // De config.h
    }
}

void atualizaLedStatusMqtt() { // Renomeada de Conexao_MQTT
    if (client.connected()) {
        digitalWrite(PIN_LED_MQTT_STATUS, HIGH); // De config.h
    } else {
        digitalWrite(PIN_LED_MQTT_STATUS, LOW); // De config.h
    }
}

void publicaPotenciometroDebug() { // Renomeada de Publica_dados
    // Esta função usa analogRead(34) que não está no Wokwi diagrama.
    // Se for manter, certifique-se que o pino 34 está conectado a um potenciômetro.
    // int valorPot = analogRead(34); // Pino ADC1_CH6
    // client.publish(MQTT_TOPICO_POTENCIOMETRO_WOKWI, String(valorPot).c_str()); // De config.h
    // delay(1000); // ATENÇÃO: delay() bloqueia outras operações. Considere alternativa com millis().
    Serial.println("Funcao publicaPotenciometroDebug chamada (atualmente comentada para evitar delay).");
    delay(1000); // Mantido como no original, mas atenção ao bloqueio.
}

void publicaNivelAguaEControlaValvula() { // Renomeada e lógica unificada
    static unsigned long lastPublishNivelAgua = 0;

    // Lógica de controle automático da válvula (apenas se o modo manual não estiver ativo)
    if (!manualMode && !autoDisabled) {
        float nivel = readWaterLevel();

        // Publica o nível da água periodicamente (ex: a cada 2 segundos)
        if (millis() - lastPublishNivelAgua > 2000) { // Publica a cada 2s para não floodar
            lastPublishNivelAgua = millis();
            client.publish(MQTT_TOPIC_NIVEL_AGUA, String(nivel).c_str()); // De config.h
            Serial.print("Nivel da agua: "); Serial.println(nivel);
        }
        
        // Lógica de acionamento da válvula (invertida em relação ao original, para ENCHER quando baixo)
        // Se o seu objetivo é esvaziar quando o nível está alto, mantenha a lógica original: if (nivel > LIMITE_NIVEL_AGUA_ACIONAMENTO)
        if (nivel > LIMITE_NIVEL_AGUA_ACIONAMENTO) { // Ex: Se nível > 15cm (considerando sensor no fundo e medindo distância até a água) = Água BAIXA
            if (digitalRead(PIN_LED_VALVULA) == LOW) { // Ligar apenas se estiver desligada
                 digitalWrite(PIN_LED_VALVULA, HIGH); // De config.h
                 atualizaEstadoValvula(true);
                 Serial.println("Valvula LIGADA automaticamente (nivel baixo)");
            }
        } else { // Nível <= LIMITE_NIVEL_AGUA_ACIONAMENTO = Água ALTA ou OK
            if (digitalRead(PIN_LED_VALVULA) == HIGH) { // Desligar apenas se estiver ligada
                digitalWrite(PIN_LED_VALVULA, LOW); // De config.h
                atualizaEstadoValvula(false);
                Serial.println("Valvula DESLIGADA automaticamente (nivel alto/ok)");
            }
        }
    } else if (manualMode) {
        // Se estiver em modo manual, apenas mantém o estado definido pelo comando MQTT
        // A lógica de desligar o modo manual por tempo está em verificaTempoManual()
    }
}

void verificaTempoManualValvula() { // Renomeada de verificaTempoManual
    if (autoDisabled && manualMode && (millis() - manualStartTime > TEMPO_ACIONAMENTO_MANUAL_MS)) { // TEMPO_ACIONAMENTO_MANUAL_MS de config.h
        Serial.println("Tempo de acionamento manual da valvula expirou.");
        manualMode = false;
        autoDisabled = false; // Reativa a possibilidade de controle automático

        // Se a válvula ficou ligada manualmente, desliga ao sair do modo manual por tempo
        if (digitalRead(PIN_LED_VALVULA) == HIGH) {
            digitalWrite(PIN_LED_VALVULA, LOW); // De config.h
            atualizaEstadoValvula(false);
            Serial.println("Valvula desligada automaticamente apos periodo manual.");
        }
        Serial.println("Controle automatico da valvula reativado.");
    }
}

float obterPesoReservatorio() {
    if (balanca.is_ready()) {
        // long leitura = balanca.get_units(10); // Média de 10 leituras
        // float pesoEmKg = leitura / HX711_CALIBRATION_FACTOR; // De config.h
        // return pesoEmKg;
        return balanca.get_units(10) / HX711_CALIBRATION_FACTOR; // De config.h
    }
    Serial.println("Sensor de peso HX711 nao esta pronto.");
    return -1; // Indica erro
}

void publicaPesoReservatorio() {
    static unsigned long lastPublishPeso = 0;

    if (millis() - lastPublishPeso > INTERVALO_PUBLICACAO_PESO_MS) { // De config.h
        lastPublishPeso = millis();
        float peso = obterPesoReservatorio();

        if (peso != -1) { // Se a leitura foi bem sucedida
            client.publish(MQTT_TOPICO_PESO_RESERVATORIO_AGUA, String(peso).c_str()); // De config.h
            Serial.print("Peso do reservatorio: "); Serial.println(peso);

            if (peso < LIMITE_PESO_BAIXO_RESERVATORIO_KG && !alertaReservatorioEnviado) { // De config.h
                client.publish(MQTT_TOPICO_ALERTA_PESO_AGUA, "Reservatorio com baixo nivel de água!"); // De config.h
                alertaReservatorioEnviado = true;
                Serial.println("ALERTA: Peso baixo no reservatorio!");
            } else if (peso >= LIMITE_PESO_BAIXO_RESERVATORIO_KG && alertaReservatorioEnviado) {
                // Opcional: Enviar mensagem de "normalizado"
                client.publish(MQTT_TOPICO_ALERTA_PESO_AGUA, "Reservatorio com peso normalizado.");
                alertaReservatorioEnviado = false;
                 Serial.println("Peso no reservatorio normalizado.");
            }
        }
    }
}

float lerTemperaturaDHT() { // Renomeada de lerTemperatura
    float temperatura = dht.readTemperature();
    if (isnan(temperatura)) {
        Serial.println("Falha ao ler temperatura do DHT!");
        return -999; // Valor para indicar erro
    }
    return temperatura;
}

void publicaTemperaturaEControlaAr() { // Renomeada e lógica unificada
    static unsigned long lastPublishTemp = 0;

    if (millis() - lastPublishTemp > INTERVALO_PUBLICACAO_TEMPERATURA_MS) { // De config.h
        lastPublishTemp = millis();
        float temperatura = lerTemperaturaDHT();

        if (temperatura != -999) { // Se a leitura foi bem sucedida
            client.publish(MQTT_TOPICO_TEMPERATURA, String(temperatura).c_str()); // De config.h
            Serial.print("Temperatura: "); Serial.println(temperatura);

            if (temperatura > LIMITE_TEMPERATURA_ALTA_C) { // De config.h
                if(digitalRead(PIN_LED_ARCONDICIONADO) == LOW) { // Ligar apenas se estiver desligado
                    digitalWrite(PIN_LED_ARCONDICIONADO, HIGH); // De config.h (simula ligar ar)
                    client.publish(MQTT_TOPICO_ALERTA_TEMP, "Temperatura ALTA! Verificar ar condicionado."); // De config.h
                    Serial.println("ALERTA: Temperatura ALTA! LED Arcondicionado LIGADO.");
                }
            } else {
                if(digitalRead(PIN_LED_ARCONDICIONADO) == HIGH) { // Desligar apenas se estiver ligado
                    digitalWrite(PIN_LED_ARCONDICIONADO, LOW); // De config.h (simula desligar ar)
                    // Opcional: Enviar mensagem de "normalizado"
                    // client.publish(MQTT_TOPICO_ALERTA_TEMP, "Temperatura normalizada.");
                    Serial.println("Temperatura OK. LED Arcondicionado DESLIGADO.");
                }
            }
        }
    }
}

// --- Função setup() ---
void setup() {
    Serial.begin(115200);
    Serial.println("Iniciando Setup...");

    pinMode(PIN_LED_VALVULA, OUTPUT);
    digitalWrite(PIN_LED_VALVULA, LOW);
    pinMode(PIN_TRIG_ULTRASONIC, OUTPUT);
    pinMode(PIN_ECHO_ULTRASONIC, INPUT);
    pinMode(PIN_LED_WIFI_STATUS, OUTPUT);
    digitalWrite(PIN_LED_WIFI_STATUS, LOW);
    pinMode(PIN_LED_MQTT_STATUS, OUTPUT);
    digitalWrite(PIN_LED_MQTT_STATUS, LOW);
    pinMode(PIN_LED_ARCONDICIONADO, OUTPUT);
    digitalWrite(PIN_LED_ARCONDICIONADO, LOW);

    setup_wifi();

    client.setServer(MQTT_SERVER, MQTT_PORT); // De config.h
    client.setCallback(callback);

    dht.begin();
    
    balanca.begin(PIN_HX711_DT, PIN_HX711_SCK); // De config.h
    Serial.println("Inicializando sensor de peso HX711...");
    if (!balanca.is_ready()) {
        Serial.println("Sensor de peso HX711 nao esta pronto! Verifique as conexoes.");
        // Considere uma forma de sinalizar este erro criticamente (ex: LED piscando rápido)
        // while(1) delay(1000); // Evitar bloqueio total em produção
    } else {
        Serial.println("Sensor de peso HX711 inicializado com sucesso.");
        // balanca.set_scale(HX711_CALIBRATION_FACTOR); // Definir o fator de calibração
        // balanca.tare(); // Zerar a balança após a inicialização e com o recipiente vazio, se necessário
    }
    Serial.println("Setup Concluido.");
}

// --- Função loop() ---
void loop() {
    if (WiFi.status() != WL_CONNECTED) {
        setup_wifi(); // Tenta reconectar ao Wi-Fi se a conexão cair
    }
    if (!client.connected()) {
        reconnect(); // Tenta reconectar ao MQTT se a conexão cair
    }
    client.loop(); // Essencial para o PubSubClient processar mensagens recebidas e manter a conexão

    atualizaLedStatusWifi();
    atualizaLedStatusMqtt();
    
    publicaNivelAguaEControlaValvula();
    verificaTempoManualValvula();
    publicaPesoReservatorio();
    publicaTemperaturaEControlaAr();
    
    // publicaPotenciometroDebug(); // Chamada da função que usa analogRead(34) e tem delay(1000)
                                // Descomente se for usar e estiver ciente do delay.
}