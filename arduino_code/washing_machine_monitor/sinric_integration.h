#ifndef SINRIC_INTEGRATION_H
#define SINRIC_INTEGRATION_H

#include <SinricPro.h>
#include <SinricProSwitch.h>
#include "config.h"

// =================== FUNÇÕES AVANÇADAS ===================

void sendCustomWashingStageToAlexa(String stage) {
    if (!SinricPro.isConnected()) return;
    
    // Criar payload customizado para enviar etapa específica
    DynamicJsonDocument doc(1024);
    doc["deviceId"] = SINRIC_DEVICE_ID;
    doc["action"] = "setRangeValue";
    doc["value"]["rangeValue"] = mapStageToNumber(stage);
    doc["value"]["rangeName"] = "WashingStage";
    
    String payload;
    serializeJson(doc, payload);
    
    // Enviar mensagem customizada
    // Nota: Esta funcionalidade pode requerer configuração adicional no Sinric Pro
    Serial.printf("Enviando etapa customizada: %s\n", stage.c_str());
}

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

// =================== COMANDOS DE VOZ PERSONALIZADOS ===================

void setupCustomVoiceCommands() {
    // Configurar respostas personalizadas para diferentes comandos
    Serial.println("Configurando comandos de voz personalizados...");
    
    // Estas funções podem ser expandidas para responder a comandos específicos
    // Por exemplo: "Alexa, qual a etapa da lavadora?"
    // Resposta: "A lavadora está na etapa de centrifugação com 95% de confiança"
}

String generateAlexaResponse(String stage, float confidence) {
    String response = "";
    
    if (stage == "desligado") {
        response = "A lavadora está desligada";
    } else if (stage == "centrifugacao") {
        response = "A lavadora está centrifugando as roupas";
    } else if (stage == "enxague") {
        response = "A lavadora está enxaguando as roupas";
    } else if (stage.startsWith("molho")) {
        response = "A lavadora está no ciclo de molho";
    } else {
        response = "A lavadora está em operação";
    }
    
    // Adicionar informação de confiança se for alta
    if (confidence > 0.9) {
        response += " com alta precisão";
    } else if (confidence > 0.7) {
        response += " com boa precisão";
    }
    
    return response;
}

// =================== DIAGNÓSTICO E MONITORAMENTO ===================

void printSinricProStatus() {
    Serial.println("=== STATUS SINRIC PRO ===");
    Serial.printf("Conectado: %s\n", SinricPro.isConnected() ? "Sim" : "Não");
    Serial.printf("Device ID: %s\n", SINRIC_DEVICE_ID);
    Serial.printf("Último estado enviado: %s\n", currentWashingStage.c_str());
    Serial.printf("Última confiança: %.1f%%\n", lastConfidence * 100);
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

// =================== CONFIGURAÇÕES AVANÇADAS ===================

void configureSinricProSettings() {
    // Configurações adicionais que podem ser úteis
    SinricPro.restoreDeviceStates(true); // Restaurar estados após reconexão
    
    // Configurar intervalo de heartbeat
    // SinricPro.setResponseMessage("Lavadora IoT respondendo");
}

// =================== INTEGRAÇÃO COM OUTROS SERVIÇOS ===================

void sendNotificationToAlexa(String message) {
    // Função para enviar notificações proativas
    // Útil para avisar quando um ciclo termina
    
    if (!SinricPro.isConnected()) return;
    
    Serial.printf("Enviando notificacao: %s\n", message.c_str());
    
    // Implementar envio de notificação personalizada
    // Esta funcionalidade pode requerer configuração adicional no Sinric Pro
}

void onWashingCycleComplete() {
    // Função chamada quando um ciclo de lavagem é concluído
    if (currentWashingStage == "centrifugacao") {
        // Assumir que centrifugação é a última etapa
        sendNotificationToAlexa("O ciclo de lavagem foi concluído");
        Serial.println("Ciclo de lavagem concluido!");
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

#endif // SINRIC_INTEGRATION_H= VARIÁVEIS GLOBAIS ===================
extern String currentWashingStage;
extern float lastConfidence;

// =================== FUNÇÕES PÚBLICAS ===================
bool setupSinricProIntegration();
void updateSinricProStatus(String stage, float confidence);
void handleSinricProRequests();

// =================== CALLBACKS ===================
bool onPowerState(const String &deviceId, bool &state);
bool onBrightnessState(const String &deviceId, int &brightness);

// =================== IMPLEMENTAÇÃO ===================

bool setupSinricProIntegration() {
    Serial.println("Configurando integração Sinric Pro...");
    
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
    
    // Registrar callbacks
    myWashingMachine.onPowerState(onPowerState);
    myWashingMachine.onBrightnessState(onBrightnessState);
    
    // Configurar Sinric Pro
    SinricPro.onConnected([]() {
        Serial.println("Sinric Pro conectado!");
    });
    
    SinricPro.onDisconnected([]() {
        Serial.println("Sinric Pro desconectado!");
    });
    
    // Inicializar conexão
    bool success = SinricPro.begin(SINRIC_APP_KEY, SINRIC_APP_SECRET);
    
    if (success) {
        Serial.println("Sinric Pro inicializado com sucesso!");
        Serial.println("Alexa pode agora controlar a lavadora");
        
        // Enviar estado inicial
        delay(2000); // Aguardar conexão estabilizar
        updateSinricProStatus(currentWashingStage, lastConfidence);
        
        return true;
    } else {
        Serial.println("Falha ao inicializar Sinric Pro");
        return false;
    }
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
    
    // Mapear confiança para brilho (0-100)
    int brightness = (int)(confidence * 100);
    
    // Enviar atualizações
    myWashingMachine.sendPowerStateEvent(powerState);
    myWashingMachine.sendBrightnessStateEvent(brightness);
    
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

bool onBrightnessState(const String &deviceId, int &brightness) {
    // Usar brightness para reportar confiança da predição
    brightness = (int)(lastConfidence * 100);
    
    Serial.printf("Alexa solicitou brilho: %d%% (confianca atual)\n", brightness);
    
    return true;
}

// ==================