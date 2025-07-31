#ifndef UTILS_H
#define UTILS_H

#include "config.h"

// =================== FUNÇÕES PÚBLICAS ===================
void indicateStageChange();
String formatUptime(unsigned long seconds);
void printSystemDiagnostics();
float getWiFiSignalQuality();
String getSystemStatusJSON();
void performSystemMaintenance();

// =================== IMPLEMENTAÇÃO ===================

void indicateStageChange() {
    // Piscar LED para indicar mudança de etapa
    for (int i = 0; i < 3; i++) {
        digitalWrite(LED_BUILTIN_PIN, LOW);
        delay(100);
        digitalWrite(LED_BUILTIN_PIN, HIGH);
        delay(100);
    }
}

String formatUptime(unsigned long seconds) {
    unsigned long days = seconds / 86400;
    seconds %= 86400;
    unsigned long hours = seconds / 3600;
    seconds %= 3600;
    unsigned long minutes = seconds / 60;
    seconds %= 60;
    
    String uptime = "";
    
    if (days > 0) {
        uptime += String(days) + "d ";
    }
    if (hours > 0 || days > 0) {
        uptime += String(hours) + "h ";
    }
    if (minutes > 0 || hours > 0 || days > 0) {
        uptime += String(minutes) + "m ";
    }
    uptime += String(seconds) + "s";
    
    return uptime;
}

void printSystemDiagnostics() {
    extern String currentWashingStage;
    extern float lastConfidence;
    extern unsigned long getSystemUptime();
    
    Serial.println("\n┌─────────────────────────────────────────┐");
    Serial.println("│           DIAGNOSTICO SISTEMA           │");
    Serial.println("├─────────────────────────────────────────┤");
    
    // Informações de memória
    Serial.printf("│ Heap livre: %-27d │\n", ESP.getFreeHeap());
    Serial.printf("│ Maior bloco: %-26d │\n", ESP.getMaxAllocHeap());
    Serial.printf("│ PSRAM livre: %-26d │\n", ESP.getFreePsram());
    
    // Informações de WiFi
    Serial.printf("│ WiFi RSSI: %-28d │\n", WiFi.RSSI());
    Serial.printf("│ WiFi Quality: %-23.1f │\n", getWiFiSignalQuality());
    
    // Informações de sistema
    Serial.printf("│ Uptime: %-31s │\n", formatUptime(getSystemUptime()).c_str());
    Serial.printf("│ CPU Freq: %-27d │\n", ESP.getCpuFreqMHz());
    
    // Status do sistema (sem ML)
    Serial.printf("│ Estado: %-29s │\n", currentWashingStage.c_str());
    Serial.printf("│ Confianca: %-26.1f │\n", lastConfidence * 100);
    Serial.printf("│ Modo: %-31s │\n", "Demonstracao");
    
    Serial.println("└─────────────────────────────────────────┘\n");
}

float getWiFiSignalQuality() {
    int rssi = WiFi.RSSI();
    float quality = 0;
    
    if (rssi <= -100) {
        quality = 0;
    } else if (rssi >= -50) {
        quality = 100;
    } else {
        quality = 2 * (rssi + 100);
    }
    
    return quality;
}

String getSystemStatusJSON() {
    extern String currentWashingStage;
    extern float lastConfidence;
    extern unsigned long getSystemUptime();
    
    String json = "{";
    json += "\"stage\":\"" + currentWashingStage + "\",";
    json += "\"confidence\":" + String(lastConfidence) + ",";
    json += "\"uptime\":" + String(getSystemUptime()) + ",";
    json += "\"heap_free\":" + String(ESP.getFreeHeap()) + ",";
    json += "\"wifi_rssi\":" + String(WiFi.RSSI()) + ",";
    json += "\"wifi_quality\":" + String(getWiFiSignalQuality()) + ",";
    json += "\"cpu_freq\":" + String(ESP.getCpuFreqMHz()) + ",";
    json += "\"mode\":\"demonstration\",";
    json += "\"timestamp\":" + String(millis());
    json += "}";
    
    return json;
}

void performSystemMaintenance() {
    extern String currentWashingStage;
    extern float lastConfidence;
    extern unsigned long getSystemUptime();
    
    static unsigned long lastMaintenance = 0;
    unsigned long now = millis();
    
    // Executar manutenção básica
    if (now - lastMaintenance > 3600000) { // A cada hora
        if (DEBUG_SYSTEM) {
            Serial.println("Executando manutencao do sistema...");
        }
        
        // 1. Verificar memória
        int freeHeap = ESP.getFreeHeap();
        if (freeHeap < 50000) {
            Serial.printf("AVISO: Memoria baixa: %d bytes\n", freeHeap);
        }
        
        // 2. Verificar qualidade WiFi
        if (getWiFiSignalQuality() < 30) {
            Serial.println("AVISO: Sinal WiFi fraco!");
        }
        
        // 3. Verificar conectividade WiFi
        if (WiFi.status() != WL_CONNECTED) {
            Serial.println("AVISO: WiFi desconectado, tentando reconectar...");
            WiFi.reconnect();
        }
        
        // 4. Verificar Sinric Pro
        if (!SinricPro.isConnected()) {
            Serial.println("AVISO: Sinric Pro desconectado!");
        }
        
        // 5. Verificar temperatura do chip (se disponível)
        float temp = temperatureRead();
        if (temp > 80) {
            Serial.printf("AVISO: Temperatura alta: %.1f°C\n", temp);
        }
        
        if (DEBUG_SYSTEM) {
            Serial.println("Manutencao concluida");
        }
        lastMaintenance = now;
    }
}

// =================== FUNÇÕES DE LOGGING ===================

void logEvent(String event, String details = "") {
    unsigned long timestamp = millis();
    Serial.printf("[%lu] %s", timestamp, event.c_str());
    if (details.length() > 0) {
        Serial.printf(" - %s", details.c_str());
    }
    Serial.println();
}

void logError(String error, String function = "") {
    Serial.print("ERRO");
    if (function.length() > 0) {
        Serial.printf(" em %s", function.c_str());
    }
    Serial.printf(": %s\n", error.c_str());
}

void logWarning(String warning) {
    Serial.printf("AVISO: %s\n", warning.c_str());
}

void logInfo(String info) {
    Serial.printf("INFO: %s\n", info.c_str());
}

// =================== FUNÇÕES DE VALIDAÇÃO ===================

bool validateSystemState() {
    bool isValid = true;
    
    // Verificar WiFi
    if (WiFi.status() != WL_CONNECTED) {
        logError("WiFi desconectado", "validateSystemState");
        isValid = false;
    }
    
    // Verificar memória
    if (ESP.getFreeHeap() < 30000) {
        logWarning("Memoria baixa: " + String(ESP.getFreeHeap()) + " bytes");
        isValid = false;
    }
    
    // Verificar estado do sistema
    extern String currentWashingStage;
    if (currentWashingStage == "") {
        logError("Estado do sistema nao inicializado", "validateSystemState");
        isValid = false;
    }
    
    return isValid;
}

// =================== FUNÇÕES DE REDE ===================

bool checkInternetConnectivity() {
    WiFiClient client;
    bool connected = client.connect("google.com", 80);
    if (connected) {
        client.stop();
        return true;
    }
    return false;
}

void printNetworkInfo() {
    Serial.println("=== INFORMACOES DE REDE ===");
    Serial.printf("SSID: %s\n", WiFi.SSID().c_str());
    Serial.printf("IP Local: %s\n", WiFi.localIP().toString().c_str());
    Serial.printf("Gateway: %s\n", WiFi.gatewayIP().toString().c_str());
    Serial.printf("DNS: %s\n", WiFi.dnsIP().toString().c_str());
    Serial.printf("MAC: %s\n", WiFi.macAddress().c_str());
    Serial.printf("RSSI: %d dBm\n", WiFi.RSSI());
    Serial.printf("Qualidade: %.1f%%\n", getWiFiSignalQuality());
    Serial.printf("Internet: %s\n", checkInternetConnectivity() ? "OK" : "Falha");
    Serial.println("===========================");
}

// =================== FUNÇÕES DE FORMATO ===================

String formatFileSize(size_t bytes) {
    if (bytes < 1024) return String(bytes) + " B";
    else if (bytes < 1024 * 1024) return String(bytes / 1024.0, 1) + " KB";
    else if (bytes < 1024 * 1024 * 1024) return String(bytes / (1024.0 * 1024.0), 1) + " MB";
    else return String(bytes / (1024.0 * 1024.0 * 1024.0), 1) + " GB";
}

String formatPercentage(float value) {
    return String(value * 100, 1) + "%";
}

String getCurrentTimeString() {
    return String(millis() / 1000) + "s";
}

// =================== FUNÇÕES DE RESET ===================

void softReset() {
    logInfo("Executando soft reset do sistema");
    delay(1000);
    ESP.restart();
}

void factoryReset() {
    logInfo("Executando factory reset");
    
    // Limpar configurações salvas se implementado
    delay(1000);
    ESP.restart();
}

// =================== FUNÇÕES ESPECÍFICAS DO MODO DEMONSTRAÇÃO ===================

void logDemoStateChange(String oldState, String newState, float confidence) {
    logEvent("Mudanca de estado demo", oldState + " -> " + newState + " (" + String(confidence * 100, 1) + "%)");
}

void validateDemoState(String state) {
    // Verificar se o estado é válido
    bool validState = false;
    for (int i = 0; i < NUM_DEMO_STATES; i++) {
        if (String(DEMO_STATES[i]) == state) {
            validState = true;
            break;
        }
    }
    
    if (!validState) {
        logWarning("Estado demo invalido: " + state);
    }
}

String getRandomDemoState() {
    int randomIndex = random(NUM_DEMO_STATES);
    return String(DEMO_STATES[randomIndex]);
}

#endif // UTILS_H