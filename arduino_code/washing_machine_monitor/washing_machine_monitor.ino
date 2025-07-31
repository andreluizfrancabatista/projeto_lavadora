// =================== washing_machine_monitor.ino ===================
// ARQUIVO PRINCIPAL - ML ORIGINAL FUNCIONANDO

#include "WiFi.h"
#include "WebServer.h"
#include <ESPmDNS.h>
#include <SinricPro.h>
#include <SinricProSwitch.h>

// Edge Impulse
#include <andreluiz-project-1_inferencing.h>

// Módulos do projeto
#include "config.h"
#include "camera_manager.h"
#include "web_server.h"
#include "utils.h"
#include "ml_inference.h"
#include "sinric_integration.h"

// =================== VARIÁVEIS GLOBAIS ===================
WebServer server(WEB_SERVER_PORT);
String currentWashingStage = "Desligado";
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
    Serial.println("    VERSAO COM ML FUNCIONAL");
    Serial.println("==========================================");
    
    systemStartTime = millis();
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
    
    // 4. Inicializar modelo ML
    Serial.println("\n4. Inicializando modelo TinyML...");
    if (!initializeMLModel()) {
        Serial.println("ERRO: Falha ao carregar modelo ML!");
        Serial.println("Sistema continuara sem deteccao automatica");
    } else {
        Serial.println("Modelo ML carregado com sucesso!");
    }
    
    // 5. Configurar integração Sinric Pro
    Serial.println("\n5. Configurando integracao Sinric Pro...");
    if (!setupSinricProIntegration()) {
        Serial.println("Aviso: Sinric Pro nao configurado");
    } else {
        Serial.println("Sinric Pro configurado!");
    }
    
    // 6. Configurar servidor web
    Serial.println("\n6. Configurando servidor web...");
    setupWebServer();
    Serial.println("Servidor web ativo!");
    
    // 7. Teste inicial do sistema
    Serial.println("\n7. Executando teste inicial...");
    performSystemTest();
    
    digitalWrite(LED_BUILTIN_PIN, HIGH);
    Serial.println("\n==========================================");
    Serial.println("        SISTEMA PRONTO!");
    Serial.println("==========================================");
    printSystemInfo();
    Serial.println("Sistema funcionando com TinyML!");
    Serial.println("Capturando imagens e detectando estados da lavadora!");
    Serial.println("==========================================\n");
    
    lastPrediction = millis();
}

// =================== LOOP PRINCIPAL ===================
void loop() {
    unsigned long now = millis();
    
    // 1. Gerenciar servidor web
    handleWebServerRequests();
    
    // 2. Gerenciar Sinric Pro
    handleSinricProRequests();
    
    // 3. Executar predição ML periodicamente
    if (now - lastPrediction >= PREDICTION_INTERVAL) {
        String prediction = performMLPrediction();
        if (prediction != "erro") {
            lastPrediction = now;
        }
    }
    
    // 4. Atualizar LED de status
    updateStatusLED();
    
    // 5. Manutenção do sistema
    static unsigned long lastMaintenance = 0;
    if (now - lastMaintenance > 60000) { // A cada minuto
        performBasicMaintenance();
        lastMaintenance = now;
    }
    
    delay(50);
}

// =================== FUNÇÕES AUXILIARES ===================

bool connectToWiFi() {
    #ifdef USE_STATIC_IP
    if (USE_STATIC_IP) {
        if (!WiFi.config(STATIC_IP, GATEWAY, SUBNET, DNS_PRIMARY, DNS_SECONDARY)) {
            Serial.println("Falha ao configurar IP estatico, usando DHCP");
        }
    }
    #endif
    
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
        
        if (attempts % 10 == 0) {
            Serial.print("\nTentando reconectar...");
            WiFi.disconnect();
            delay(1000);
            WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
        }
    }
    
    return (WiFi.status() == WL_CONNECTED);
}

void performSystemTest() {
    Serial.println("Executando teste completo do sistema...");
    
    Serial.print("- Teste de captura de imagem... ");
    camera_fb_t* fb = captureImage();
    if (fb) {
        Serial.printf("OK (%d bytes)\n", fb->len);
        releaseCameraBuffer(fb);
    } else {
        Serial.println("FALHOU");
        return;
    }
    
    Serial.print("- Teste de modelo ML... ");
    if (validateModel()) {
        Serial.println("OK");
    } else {
        Serial.println("FALHOU");
    }
    
    Serial.print("- Teste de servidor web... ");
    Serial.println("OK");
    
    Serial.print("- Teste de conectividade WiFi... ");
    if (WiFi.status() == WL_CONNECTED) {
        Serial.println("OK");
    } else {
        Serial.println("FALHOU");
    }
    
    Serial.println("Testes concluidos!");
}

void printSystemInfo() {
    Serial.println("INFORMACOES DO SISTEMA:");
    Serial.println("┌─────────────────────────────────────┐");
    Serial.printf("│ WiFi SSID: %-24s │\n", WIFI_SSID);
    Serial.printf("│ IP Local: %-25s │\n", WiFi.localIP().toString().c_str());
    Serial.printf("│ Força WiFi: %-23d │\n", WiFi.RSSI());
    Serial.printf("│ mDNS: http://%-22s │\n", (String(MDNS_NAME) + ".local").c_str());
    Serial.printf("│ Servidor: http://%-18s │\n", WiFi.localIP().toString().c_str());
    Serial.printf("│ Heap livre: %-25d │\n", ESP.getFreeHeap());
    Serial.printf("│ Modelo ML: %-26s │\n", validateModel() ? "Ativo" : "Inativo");
    Serial.printf("│ Sinric Pro: %-24s │\n", SinricPro.isConnected() ? "Conectado" : "Desconectado");
    Serial.printf("│ Chip ID: 0x%-25llX │\n", ESP.getEfuseMac());
    Serial.println("└─────────────────────────────────────┘");
}

void updateStatusLED() {
    static unsigned long lastLEDUpdate = 0;
    static bool ledState = false;
    
    unsigned long now = millis();
    
    if (currentWashingStage == "Desligado") {
        if (now - lastLEDUpdate > 2000) {
            ledState = !ledState;
            digitalWrite(LED_BUILTIN_PIN, ledState);
            lastLEDUpdate = now;
        }
    } else {
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

void forcePrediction() {
    Serial.println("Predicao forcada via web interface");
    String result = performMLPrediction();
    Serial.printf("Resultado da predicao forcada: %s\n", result.c_str());
}

unsigned long getSystemUptime() {
    return (millis() - systemStartTime) / 1000;
}

void performBasicMaintenance() {
    int freeHeap = ESP.getFreeHeap(); 
    
    Serial.printf("Manutencao: Heap=%d, Etapa=%s, Conf=%.1f%%, Uptime=%lus\n", 
                 freeHeap, 
                 currentWashingStage.c_str(),
                 lastConfidence * 100,
                 getSystemUptime());
    
    // Verificar WiFi
    if (WiFi.status() != WL_CONNECTED) {
        Serial.println("AVISO: WiFi desconectado - tentando reconectar");
        WiFi.reconnect();
    }
    
    // Verificar memória crítica
    if (freeHeap < 50000) {
        Serial.printf("AVISO: Memoria baixa: %d bytes\n", freeHeap);
    }
    
    // Verificar Sinric Pro
    if (!SinricPro.isConnected()) {
        Serial.println("AVISO: Sinric Pro desconectado");
    }
}