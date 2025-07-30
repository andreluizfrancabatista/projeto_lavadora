#ifndef SINRIC_INTEGRATION_H
#define SINRIC_INTEGRATION_H

#include <SinricPro.h>
#include <SinricProSwitch.h>
#include "config.h"

// =================== VARIÁVEIS GLOBAIS ===================
extern String currentWashingStage;
extern float lastConfidence;

// =================== DECLARAÇÕES DE FUNÇÕES ===================
bool setupSinricProIntegration();
void updateSinricProStatus(String stage, float confidence);
void handleSinricProRequests();
bool onPowerState(const String &deviceId, bool &state);
int mapStageToNumber(String stage);
String mapNumberToStage(int number);
void sendCustomWashingStageToAlexa(String stage);
String generateAlexaResponse(String stage, float confidence);
void printSinricProStatus();
bool testSinricProConnection();
void handleSinricProError(String error);
void trackUsageStatistics();
void onWashingCycleComplete();

// =================== IMPLEMENTAÇÃO ===================

bool setupSinricProIntegration() {
    Serial.println("Configurando integracao Sinric Pro...");
    
    // Verificar se as credenciais estão definidas
    if (String(SINRIC_APP_KEY) == "SUA_APP_KEY" || 
        String(SINRIC_APP_SECRET) == "SEU_APP_SECRET" || 
        String(SINRIC_DEVICE_ID) == "SEU_DEVICE_ID") {
        Serial.println("AVISO: Credenciais Sinric Pro nao configuradas!");
        Serial.println("Configure em config.h para habilitar integracao com Alexa");
        return false;
    }
    
    // Configurar dispositivo switch
    SinricProSwitch &myWashingMachine = SinricPro[SINRIC_DEVICE_ID];
    
    // Registrar callback para controle de energia
    myWashingMachine.onPowerState(onPowerState);
    
    // Configurar callbacks de conexão
    SinricPro.onConnected([]() {
        Serial.println("Sinric Pro conectado com sucesso!");
    });
    
    SinricPro.onDisconnected([]() {
        Serial.println("Sinric Pro desconectado!");
    });
    
    // Inicializar conexão
    SinricPro.begin(SINRIC_APP_KEY, SINRIC_APP_SECRET);
    
    Serial.println("Sinric Pro inicializado!");
    Serial.println("Alexa pode agora controlar a lavadora");
    
    // Aguardar conexão estabilizar
    delay(2000);
    
    return true;
}

void handleSinricProRequests() {
    SinricPro.handle();
}

void updateSinricProStatus(String stage, float confidence) {
    if (!SinricPro.isConnected()) {
        return; // Não fazer nada se não estiver conectado
    }
    
    SinricProSwitch &myWashingMachine = SinricPro[SINRIC_DEVICE_ID];
    
    // Mapear etapa para estado de energia (ligado/desligado)
    bool powerState = (stage != "desligado");
    
    // Enviar estado de energia
    myWashingMachine.sendPowerStateEvent(powerState);
    
    // Log da atualização
    Serial.printf("Sinric Pro atualizado: %s (%.1f%%)\n", stage.c_str(), confidence * 100);
}

// =================== CALLBACKS DO SINRIC PRO ===================

bool onPowerState(const String &deviceId, bool &state) {
    Serial.printf("Alexa solicitou: %s\n", state ? "LIGAR" : "DESLIGAR");
    
    // Responder com estado atual real
    state = (currentWashingStage != "desligado");
    
    // Enviar resposta personalizada dependendo do estado
    if (state) {
        Serial.println("Resposta: Lavadora esta em operacao");
        Serial.printf("Etapa atual: %s\n", currentWashingStage.c_str());
    } else {
        Serial.println("Resposta: Lavadora esta desligada");
    }
    
    return true; // Confirma que o comando foi processado
}

// =================== FUNÇÕES AUXILIARES ===================

int mapStageToNumber(String stage) {
    // Mapear etapas para números para facilitar integração
    if (stage == "desligado") return 0;
    else if (stage == "molho_curto") return 1;
    else if (stage == "molho_normal") return 2;
    else if (stage == "molho_longo") return 3;
    else if (stage == "enxague") return 4;
    else if (stage == "centrifugacao") return 5;
    else return 0; // Default para desligado
}

String mapNumberToStage(int number) {
    switch (number) {
        case 0: return "desligado";
        case 1: return "molho_curto";
        case 2: return "molho_normal";
        case 3: return "molho_longo";
        case 4: return "enxague";
        case 5: return "centrifugacao";
        default: return "desligado";
    }
}

// =================== FUNÇÕES AVANÇADAS ===================

void sendCustomWashingStageToAlexa(String stage) {
    // Funcionalidade simplificada para v3.5.1
    updateSinricProStatus(stage, lastConfidence);
    Serial.printf("Enviando etapa para Alexa: %s\n", stage.c_str());
}

String generateAlexaResponse(String stage, float confidence) {
    String response = "";
    
    if (stage == "desligado") {
        response = "A lavadora esta desligada";
    } else if (stage == "centrifugacao") {
        response = "A lavadora esta centrifugando as roupas";
    } else if (stage == "enxague") {
        response = "A lavadora esta enxaguando as roupas";
    } else if (stage.startsWith("molho")) {
        response = "A lavadora esta no ciclo de molho";
    } else {
        response = "A lavadora esta em operacao";
    }
    
    // Adicionar informação de confiança se for alta
    if (confidence > 0.9) {
        response += " com alta precisao";
    } else if (confidence > 0.7) {
        response += " com boa precisao";
    }
    
    return response;
}

// =================== DIAGNÓSTICO E MONITORAMENTO ===================

void printSinricProStatus() {
    Serial.println("=== STATUS SINRIC PRO ===");
    Serial.printf("Conectado: %s\n", SinricPro.isConnected() ? "Sim" : "Nao");
    Serial.printf("Device ID: %s\n", SINRIC_DEVICE_ID);
    Serial.printf("Ultimo estado enviado: %s\n", currentWashingStage.c_str());
    Serial.printf("Ultima confianca: %.1f%%\n", lastConfidence * 100);
    Serial.println("========================");
}

bool testSinricProConnection() {
    Serial.println("Testando conexao Sinric Pro...");
    
    if (!SinricPro.isConnected()) {
        Serial.println("Nao conectado ao Sinric Pro");
        return false;
    }
    
    // Enviar um estado de teste
    SinricProSwitch &myWashingMachine = SinricPro[SINRIC_DEVICE_ID];
    bool testResult = myWashingMachine.sendPowerStateEvent(true);
    
    if (testResult) {
        Serial.println("Teste de envio bem-sucedido");
        delay(1000);
        myWashingMachine.sendPowerStateEvent(currentWashingStage != "desligado");
        return true;
    } else {
        Serial.println("Falha no teste de envio");
        return false;
    }
}

// =================== TRATAMENTO DE ERROS ===================

void handleSinricProError(String error) {
    Serial.printf("Erro Sinric Pro: %s\n", error.c_str());
    
    // Tentar reconectar em caso de erro
    static unsigned long lastReconnectAttempt = 0;
    unsigned long now = millis();
    
    if (now - lastReconnectAttempt > 30000) { // Tentar a cada 30 segundos
        Serial.println("Tentando reconectar Sinric Pro...");
        SinricPro.begin(SINRIC_APP_KEY, SINRIC_APP_SECRET);
        lastReconnectAttempt = now;
    }
}

// =================== ESTATÍSTICAS DE USO ===================

void trackUsageStatistics() {
    static unsigned long totalOnTime = 0;
    static unsigned long lastStateChange = 0;
    static String lastState = "desligado";
    
    unsigned long now = millis();
    
    if (currentWashingStage != lastState) {
        if (lastState != "desligado") {
            totalOnTime += (now - lastStateChange);
        }
        lastStateChange = now;
        lastState = currentWashingStage;
        
        Serial.printf("Tempo total de operacao: %.1f horas\n", totalOnTime / 3600000.0);
    }
}

// =================== COMANDOS DE VOZ PERSONALIZADOS ===================

void onWashingCycleComplete() {
    // Função chamada quando um ciclo de lavagem é concluído
    if (currentWashingStage == "centrifugacao") {
        // Assumir que centrifugação é a última etapa
        Serial.println("Ciclo de lavagem concluido!");
        
        // Enviar notificação simples via mudança de estado
        SinricProSwitch &myWashingMachine = SinricPro[SINRIC_DEVICE_ID];
        myWashingMachine.sendPowerStateEvent(false); // Sinalizar conclusão
        delay(1000);
        myWashingMachine.sendPowerStateEvent(true);
    }
}

#endif // SINRIC_INTEGRATION_H