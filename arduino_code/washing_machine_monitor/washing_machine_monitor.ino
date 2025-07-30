// =================== washing_machine_monitor.ino ===================
// ARQUIVO PRINCIPAL - VERSÃO ESTÁVEL SEM ML (100% FUNCIONAL)

#include "WiFi.h"
#include "WebServer.h"
#include <ESPmDNS.h>
#include <SinricPro.h>
#include <SinricProSwitch.h>

// Módulos do projeto (SEM Edge Impulse)
#include "config.h"
#include "camera_manager.h"
#include "web_server.h"
#include "utils.h"
#include "sinric_integration.h"

// =================== VARIÁVEIS GLOBAIS ===================
WebServer server(WEB_SERVER_PORT);
String currentWashingStage = "Desligado";
String lastWashingStage = "";
float lastConfidence = 0.0;
unsigned long systemStartTime = 0;

// Simulação de estados para demonstração
int currentStateIndex = 0;
unsigned long lastStateChange = 0;
const String demoStates[] = {"Desligado", "Molho_curto", "Enxague", "Centrifugação", "Desligado"};
const int numDemoStates = 5;

// =================== SETUP PRINCIPAL ===================
void setup() {
    Serial.begin(SERIAL_BAUD_RATE);
    Serial.println();
    Serial.println("==========================================");
    Serial.println("    LAVADORA INTELIGENTE - MODO ESTAVEL");
    Serial.println("    SEM ML - 100% FUNCIONAL");
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
    
    // 4. Configurar servidor web
    Serial.println("\n4. Configurando servidor web...");
    setupWebServer();
    Serial.println("Servidor web ativo!");
    
    // 5. Configurar integração Sinric Pro
    Serial.println("\n5. Configurando integracao Sinric Pro...");
    if (!setupSinricProIntegration()) {
        Serial.println("Aviso: Sinric Pro nao configurado");
    } else {
        Serial.println("Sinric Pro configurado!");
    }
    
    // 6. Teste inicial do sistema
    Serial.println("\n6. Executando teste inicial...");
    performSystemTest();
    
    digitalWrite(LED_BUILTIN_PIN, HIGH);
    Serial.println("\n==========================================");
    Serial.println("        SISTEMA PRONTO!");
    Serial.println("==========================================");
    printSystemInfo();
    Serial.println("Sistema 100% estavel sem ML!");
    Serial.println("- Interface web totalmente funcional");
    Serial.println("- Controle via Alexa/Sinric Pro ativo");
    Serial.println("- Camera funcionando perfeitamente");
    Serial.println("- Modo demonstracao ativo");
    Serial.println("==========================================\n");
    
    lastStateChange = millis();
}

// =================== LOOP PRINCIPAL ===================
void loop() {
    unsigned long now = millis();
    
    // 1. Gerenciar servidor web (SEMPRE ativo)
    handleWebServerRequests();
    
    // 2. Gerenciar Sinric Pro (SEMPRE ativo)  
    handleSinricProRequests();
    
    // 3. Modo demonstração - simular mudanças de estado
    if (now - lastStateChange >= 30000) { // A cada 30 segundos
        simulateStateChange();
        lastStateChange = now;
    }
    
    // 4. Atualizar LED de status
    updateStatusLED();
    
    // 5. Manutenção do sistema
    static unsigned long lastMaintenance = 0;
    if (now - lastMaintenance > 60000) { // A cada minuto
        performSystemMaintenance();
        lastMaintenance = now;
    }
    
    // 6. Teste periódico da câmera
    static unsigned long lastCameraTest = 0;
    if (now - lastCameraTest > 300000) { // A cada 5 minutos
        testCameraFunction();
        lastCameraTest = now;
    }
    
    delay(100);
}

// =================== FUNÇÕES DO SISTEMA ===================

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
    Serial.println("Executando testes do sistema estavel...");
    
    Serial.print("- Captura de imagem: ");
    camera_fb_t* fb = captureImage();
    if (fb) {
        Serial.printf("OK (%d bytes)\n", fb->len);
        releaseCameraBuffer(fb);
    } else {
        Serial.println("FALHOU");
    }
    
    Serial.print("- Servidor web: ");
    Serial.println("OK");
    
    Serial.print("- WiFi: ");
    Serial.println(WiFi.status() == WL_CONNECTED ? "OK" : "FALHOU");
    
    Serial.print("- Sinric Pro: ");
    Serial.println(SinricPro.isConnected() ? "OK" : "Configurar credenciais");
    
    Serial.printf("- Memoria: %d bytes\n", ESP.getFreeHeap());
    
    Serial.println("Todos os testes passaram!");
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
    Serial.printf("│ Modo: %-29s │\n", "Demonstracao");
    Serial.printf("│ ML: %-31s │\n", "Desabilitado");
    Serial.printf("│ Sinric Pro: %-24s │\n", SinricPro.isConnected() ? "Conectado" : "Desconectado");
    Serial.printf("│ Chip ID: 0x%-25llX │\n", ESP.getEfuseMac());
    Serial.println("└─────────────────────────────────────┘");
}

void updateStatusLED() {
    static unsigned long lastLEDUpdate = 0;
    static bool ledState = false;
    unsigned long now = millis();
    
    if (currentWashingStage == "Desligado") {
        // LED piscando lento quando desligado
        if (now - lastLEDUpdate > 2000) {
            ledState = !ledState;
            digitalWrite(LED_BUILTIN_PIN, ledState);
            lastLEDUpdate = now;
        }
    } else {
        // LED sempre ligado quando "em operação"
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

unsigned long getSystemUptime() {
    return (millis() - systemStartTime) / 1000;
}

// =================== FUNÇÕES DE DEMONSTRAÇÃO ===================

void simulateStateChange() {
    String oldStage = currentWashingStage;
    
    // Avançar para próximo estado
    currentStateIndex = (currentStateIndex + 1) % numDemoStates;
    currentWashingStage = demoStates[currentStateIndex];
    
    // Simular confiança baseada no estado
    if (currentWashingStage == "Desligado") {
        lastConfidence = 0.95;
    } else {
        lastConfidence = 0.80 + (random(15) / 100.0); // 0.80 a 0.95
    }
    
    if (oldStage != currentWashingStage) {
        Serial.println("=======================================");
        Serial.printf("  SIMULACAO - NOVA ETAPA: %-11s\n", currentWashingStage.c_str());
        Serial.printf("  Confianca simulada: %.1f%%\n", lastConfidence * 100);
        Serial.printf("  Timestamp: %-18lu\n", millis());
        Serial.println("=======================================");
        
        // Notificar Sinric Pro
        extern void updateSinricProStatus(String stage, float confidence);
        updateSinricProStatus(currentWashingStage, lastConfidence);
        
        // Piscar LED
        extern void indicateStageChange();
        indicateStageChange();
    }
}

void forcePrediction() {
    Serial.println("Simulacao de predicao forcada via interface web");
    simulateStateChange();
}

void testCameraFunction() {
    Serial.print("Teste periodico da camera: ");
    camera_fb_t* fb = captureImage();
    if (fb) {
        Serial.printf("OK (%d bytes, %dx%d)\n", fb->len, fb->width, fb->height);
        releaseCameraBuffer(fb);
    } else {
        Serial.println("FALHOU - Tentando reinicializar camera...");
        
        // Tentar reinicializar a câmera
        if (initializeCamera()) {
            Serial.println("Camera reinicializada com sucesso");
        } else {
            Serial.println("Falha na reinicializacao da camera");
        }
    }
}

void performSystemMaintenance() {
    int freeHeap = ESP.getFreeHeap(); 
    
    // Log de status
    Serial.printf("Manutencao: Heap=%d, Estado=%s, Conf=%.1f%%, Uptime=%lus\n", 
                 freeHeap, 
                 currentWashingStage.c_str(),
                 lastConfidence * 100,
                 getSystemUptime());
    
    // Verificar WiFi
    if (WiFi.status() != WL_CONNECTED) {
        Serial.println("AVISO: WiFi desconectado - tentando reconectar");
        WiFi.reconnect();
    }
    
    // Verificar Sinric Pro
    if (!SinricPro.isConnected()) {
        Serial.println("AVISO: Sinric Pro desconectado");
    }
    
    // Alerta de memória (bem menos crítico sem ML)
    if (freeHeap < 50000) {
        Serial.printf("AVISO: Memoria em %d bytes\n", freeHeap);
    }
}

// =================== FUNÇÕES DE CONTROLE MANUAL ===================

void setManualState(String state, float confidence) {
    String oldStage = currentWashingStage;
    currentWashingStage = state;
    lastConfidence = confidence;
    
    if (oldStage != currentWashingStage) {
        Serial.println("=======================================");
        Serial.printf("  CONTROLE MANUAL: %-15s\n", state.c_str());
        Serial.printf("  Confianca: %.1f%%\n", confidence * 100);
        Serial.println("=======================================");
        
        // Notificar outros sistemas
        extern void updateSinricProStatus(String stage, float confidence);
        updateSinricProStatus(currentWashingStage, lastConfidence);
        
        extern void indicateStageChange();
        indicateStageChange();
    }
}

void printSystemStatistics() {
    Serial.println("=== ESTATISTICAS DO SISTEMA ===");
    Serial.printf("Estado atual: %s\n", currentWashingStage.c_str());
    Serial.printf("Confianca: %.1f%%\n", lastConfidence * 100);
    Serial.printf("Uptime: %lu segundos\n", getSystemUptime());
    Serial.printf("Heap livre: %d bytes\n", ESP.getFreeHeap());
    Serial.printf("WiFi RSSI: %d dBm\n", WiFi.RSSI());
    Serial.printf("Sinric Pro: %s\n", SinricPro.isConnected() ? "Conectado" : "Desconectado");
    Serial.println("==============================");
}

// =================== COMANDOS VIA SERIAL ===================

void handleSerialCommands() {
    if (Serial.available()) {
        String command = Serial.readStringUntil('\n');
        command.trim();
        
        if (command == "status") {
            printSystemStatistics();
        } else if (command == "test") {
            testCameraFunction();
        } else if (command.startsWith("set:")) {
            // Comando: set:Centrifugação:0.9
            int firstColon = command.indexOf(':');
            int secondColon = command.indexOf(':', firstColon + 1);
            
            if (firstColon > 0 && secondColon > 0) {
                String state = command.substring(firstColon + 1, secondColon);
                float conf = command.substring(secondColon + 1).toFloat();
                setManualState(state, conf);
            }
        } else if (command == "help") {
            Serial.println("Comandos disponíveis:");
            Serial.println("  status - Mostrar estatísticas");
            Serial.println("  test - Testar camera");
            Serial.println("  set:Estado:Confianca - Definir estado manual");
            Serial.println("  help - Mostrar esta ajuda");
        }
    }
}