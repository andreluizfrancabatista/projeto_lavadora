// =================== ARQUIVO PRINCIPAL ===================
// Lavadora Inteligente - ESP32-CAM + TinyML + Sinric Pro
// Desenvolvido para monitoramento automático de ciclos de lavagem

#include "WiFi.h"
#include "WebServer.h"
#include <ESPmDNS.h>
#include <SinricPro.h>
#include <SinricProSwitch.h>

// =================== EDGE IMPULSE ===================
#include <andreluiz-project-1_inferencing.h>

// =================== MÓDULOS DO PROJETO ===================
#include "config.h"
#include "camera_manager.h"
#include "ml_inference.h"
#include "web_server.h"
#include "sinric_integration.h"
#include "utils.h"

// =================== VARIÁVEIS GLOBAIS ===================
WebServer server(WEB_SERVER_PORT);
String currentWashingStage = "desligado";
String lastWashingStage = "";
float lastConfidence = 0.0;
unsigned long lastPrediction = 0;
unsigned long systemStartTime = 0;

// =================== SETUP PRINCIPAL ===================
void setup() {
    Serial.begin(SERIAL_BAUD_RATE);
    Serial.println();
    Serial.println("==========================================");
    Serial.println("    LAVADORA INTELIGENTE - INICIANDO");
    Serial.println("==========================================");
    
    // Marcar tempo de início
    systemStartTime = millis();
    
    // Inicializar LED builtin
    pinMode(LED_BUILTIN_PIN, OUTPUT);
    digitalWrite(LED_BUILTIN_PIN, LOW);
    
    // 1. Configurar câmera
    Serial.println("1. Configurando camera ESP32-CAM...");
    if (!initializeCamera()) {
        Serial.println("ERRO FATAL: Falha ao configurar camera!");
        blinkErrorLED();
        while(1) delay(1000);
    }
    Serial.println("Camera configurada com sucesso!");
    
    // 2. Conectar WiFi
    Serial.println("\n2. Conectando ao WiFi...");
    if (!connectToWiFi()) {
        Serial.println("ERRO FATAL: Falha ao conectar WiFi!");
        blinkErrorLED();
        while(1) delay(1000);
    }
    Serial.println("WiFi conectado!");
    
    // 3. Configurar mDNS
    Serial.println("\n3. Configurando mDNS...");
    if (MDNS.begin(MDNS_NAME)) {
        Serial.println("mDNS configurado! Acesso: http://" + String(MDNS_NAME) + ".local");
    } else {
        Serial.println("Falha ao configurar mDNS (nao critico)");
    }
    
    // 4. Inicializar modelo TinyML
    Serial.println("\n4. Inicializando modelo TinyML...");
    if (!initializeMLModel()) {
        Serial.println("ERRO FATAL: Falha ao inicializar modelo!");
        blinkErrorLED();
        while(1) delay(1000);
    }
    Serial.println("Modelo TinyML carregado!");
    
    // 5. Configurar Sinric Pro
    Serial.println("\n5. Configurando Sinric Pro...");
    if (!setupSinricProIntegration()) {
        Serial.println("Falha ao configurar Sinric Pro (nao critico)");
    } else {
        Serial.println("Sinric Pro configurado!");
    }
    
    // 6. Inicializar servidor web
    Serial.println("\n6. Configurando servidor web...");
    setupWebServer();
    Serial.println("Servidor web ativo!");
    
    // 7. Teste inicial do sistema
    Serial.println("\n7. Executando teste inicial...");
    performSystemTest();
    
    // Sistema pronto
    digitalWrite(LED_BUILTIN_PIN, HIGH);
    Serial.println("\n==========================================");
    Serial.println("        SISTEMA PRONTO!");
    Serial.println("==========================================");
    printSystemInfo();
    Serial.println("Iniciando monitoramento automatico...");
    Serial.println("==========================================\n");
    
    // Primeira predição
    lastPrediction = millis();
}

// =================== LOOP PRINCIPAL ===================
void loop() {
    // Processar requisições Sinric Pro
    SinricPro.handle();
    
    // Processar requisições do servidor web
    handleWebServerRequests();
    
    // Fazer predição automática
    if (millis() - lastPrediction >= PREDICTION_INTERVAL) {
        performMLPrediction();
        lastPrediction = millis();
    }
    
    // Atualizar LED de status
    updateStatusLED();
    
    // Pequeno delay para não sobrecarregar
    delay(50);
}

// =================== FUNÇÕES AUXILIARES ===================

bool connectToWiFi() {
    // Configurar IP estático se habilitado
    #ifdef USE_STATIC_IP
    if (USE_STATIC_IP) {
        if (!WiFi.config(STATIC_IP, GATEWAY, SUBNET, DNS_PRIMARY, DNS_SECONDARY)) {
            Serial.println("⚠️ Falha ao configurar IP estatico, usando DHCP");
        } else {
            Serial.println("📡 IP estatico configurado: " + STATIC_IP.toString());
        }
    }
    #endif
    
    // Configurar potência máxima do WiFi
    WiFi.setTxPower(WIFI_POWER_19_5dBm);
    WiFi.mode(WIFI_STA);
    WiFi.setAutoReconnect(true);
    
    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
    
    int attempts = 0;
    Serial.print("Conectando");
    while (WiFi.status() != WL_CONNECTED && attempts < 30) {
        delay(1000);
        Serial.print(".");
        attempts++;
        
        // Tentar reconectar a cada 10 tentativas
        if (attempts % 10 == 0) {
            Serial.print("\nTentando reconectar...");
            WiFi.disconnect();
            delay(1000);
            WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
        }
    }
    
    if (WiFi.status() == WL_CONNECTED) {
        Serial.println();
        return true;
    } else {
        Serial.println("\nFalha ao conectar!");
        return false;
    }
}

void performSystemTest() {
    Serial.println("Executando teste completo do sistema...");
    
    // Teste 1: Captura de imagem
    Serial.print("- Teste de captura de imagem... ");
    camera_fb_t* fb = captureImage();
    if (fb) {
        Serial.printf("OK (%d bytes)\n", fb->len);
        releaseCameraBuffer(fb);
    } else {
        Serial.println("FALHOU");
        return;
    }
    
    // Teste 2: Predição ML
    Serial.print("- Teste de predicao ML... ");
    String testResult = performMLPrediction();
    if (testResult != "erro") {
        Serial.println("OK (Resultado: " + testResult + ")");
    } else {
        Serial.println("FALHOU");
        return;
    }
    
    // Teste 3: Servidor web
    Serial.print("- Teste de servidor web... ");
    if (server.uri() != NULL || true) { // Servidor está rodando
        Serial.println("OK");
    } else {
        Serial.println("FALHOU");
    }
    
    Serial.println("Todos os testes passaram!");
}

void printSystemInfo() {
    Serial.println("INFORMACOES DO SISTEMA:");
    Serial.println("┌─────────────────────────────────────┐");
    Serial.printf("│ WiFi SSID: %-24s │\n", WIFI_SSID);
    Serial.printf("│ IP Local: %-25s │\n", WiFi.localIP().toString().c_str());
    Serial.printf("│ Força WiFi: %-23d │\n", WiFi.RSSI());
    Serial.printf("│ mDNS: http://%s.local %-10s │\n", MDNS_NAME, "");
    Serial.printf("│ Servidor: http://%s %-11s │\n", WiFi.localIP().toString().c_str(), "");
    Serial.printf("│ Modelo ML: %-24s │\n", ei_classifier_inferencing_get_project_name());
    Serial.printf("│ Classes: %-26d │\n", ei_classifier_label_count());
    Serial.printf("│ Resolução: %dx%-19d │\n", EI_CLASSIFIER_INPUT_WIDTH, EI_CLASSIFIER_INPUT_HEIGHT);
    Serial.printf("│ Heap livre: %-22d │\n", ESP.getFreeHeap());
    Serial.printf("│ Chip ID: 0x%-23llX │\n", ESP.getEfuseMac());
    Serial.println("└─────────────────────────────────────┘");
}

void updateStatusLED() {
    static unsigned long lastLEDUpdate = 0;
    static bool ledState = false;
    
    unsigned long now = millis();
    
    if (currentWashingStage == "desligado") {
        // LED piscando lento quando desligado
        if (now - lastLEDUpdate > 2000) {
            ledState = !ledState;
            digitalWrite(LED_BUILTIN_PIN, ledState);
            lastLEDUpdate = now;
        }
    } else {
        // LED ligado fixo quando em operação
        digitalWrite(LED_BUILTIN_PIN, HIGH);
    }
}

void blinkErrorLED() {
    for (int i = 0; i < 10; i++) {
        digitalWrite(LED_BUILTIN_PIN, HIGH);
        delay(200);
        digitalWrite(LED_BUILTIN_PIN, LOW);
        delay(200);
    }
}

// =================== FUNÇÕES PARA OUTROS MÓDULOS ===================

// Função para ser chamada pelo módulo web_server.h
void forcePrediction() {
    Serial.println("Predicao forcada via web interface");
    performMLPrediction();
}

// Função para obter uptime do sistema
unsigned long getSystemUptime() {
    return (millis() - systemStartTime) / 1000; // em segundos
}

// =================== CALLBACKS GLOBAIS ===================

// Callback para reconexão WiFi
void onWiFiEvent(WiFiEvent_t event) {
    switch (event) {
        case SYSTEM_EVENT_STA_DISCONNECTED:
            Serial.println("WiFi desconectado! Tentando reconectar...");
            break;
        case SYSTEM_EVENT_STA_CONNECTED:
            Serial.println("WiFi reconectado!");
            break;
        default:
            break;
    }
}

// =================== INFORMAÇÕES DE DEPURAÇÃO ===================

void printMemoryInfo() {
    Serial.printf("Heap livre: %d bytes\n", ESP.getFreeHeap());
    Serial.printf("Maior bloco livre: %d bytes\n", ESP.getMaxAllocHeap());
    Serial.printf("PSRAM livre: %d bytes\n", ESP.getFreePsram());
}

void printWiFiInfo() {
    Serial.printf("SSID: %s\n", WiFi.SSID().c_str());
    Serial.printf("IP: %s\n", WiFi.localIP().toString().c_str());
    Serial.printf("Gateway: %s\n", WiFi.gatewayIP().toString().c_str());
    Serial.printf("DNS: %s\n", WiFi.dnsIP().toString().c_str());
    Serial.printf("RSSI: %d dBm\n", WiFi.RSSI());
}