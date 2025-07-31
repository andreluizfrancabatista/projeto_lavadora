#ifndef ML_INFERENCE_H
#define ML_INFERENCE_H

#include <andreluiz-project-1_inferencing.h>
#include "config.h"
#include "camera_manager.h"

// =================== VARIÁVEIS GLOBAIS ===================
extern String currentWashingStage;
extern String lastWashingStage;
extern float lastConfidence;

// Buffer para imagem redimensionada
uint8_t resized_image[EI_CLASSIFIER_INPUT_WIDTH * EI_CLASSIFIER_INPUT_HEIGHT * EI_CLASSIFIER_RAW_SAMPLES_PER_FRAME];

// =================== DECLARAÇÕES DE FUNÇÕES ===================
bool initializeMLModel();
String performMLPrediction();
void processMLResult(ei_impulse_result_t* result);
int getSignalData(size_t offset, size_t length, float *out_ptr);
void printDetailedPrediction(ei_impulse_result_t* result);
void onStageChanged(String newStage, float confidence);
void logStageChange(String stage, float confidence);
void printMLStatistics();
bool validateModel();

// =================== IMPLEMENTAÇÃO ===================

bool initializeMLModel() {
    Serial.println("Inicializando modelo Edge Impulse...");
    
    // Verificar informações do modelo
    Serial.printf("Classes detectadas: %d\n", EI_CLASSIFIER_LABEL_COUNT);
    Serial.printf("Tamanho entrada: %dx%d\n", EI_CLASSIFIER_INPUT_WIDTH, EI_CLASSIFIER_INPUT_HEIGHT);
    Serial.printf("Samples por frame: %d\n", EI_CLASSIFIER_RAW_SAMPLES_PER_FRAME);
    Serial.printf("Frequencia: %.1f Hz\n", EI_CLASSIFIER_FREQUENCY);
    
    // Listar todas as classes disponíveis
    Serial.println("Classes disponíveis:");
    for (int i = 0; i < EI_CLASSIFIER_LABEL_COUNT; i++) {
        Serial.printf("  %d: %s\n", i, ei_classifier_inferencing_categories[i]);
    }
    
    // Verificar se o modelo está carregado corretamente
    if (EI_CLASSIFIER_LABEL_COUNT == 0) {
        Serial.println("ERRO: Modelo nao possui classes validas");
        return false;
    }
    
    if (EI_CLASSIFIER_INPUT_WIDTH == 0 || EI_CLASSIFIER_INPUT_HEIGHT == 0) {
        Serial.println("ERRO: Dimensoes de entrada invalidas");
        return false;
    }
    
    Serial.println("Modelo carregado e validado com sucesso!");
    return true;
}

String performMLPrediction() {
    static int predictionCount = 0;
    predictionCount++;
    
    if (DEBUG_PREDICTIONS && predictionCount % 5 == 0) {
        Serial.printf("--- Predicao #%d ---\n", predictionCount);
    }
    
    // 1. Capturar imagem da câmera
    camera_fb_t* fb = captureImage();
    if (!fb) {
        Serial.println("Erro ao capturar imagem para ML");
        return "erro";
    }
    
    // 2. Redimensionar imagem para entrada do modelo
    if (!resizeImageForML(fb->buf, fb->len, resized_image)) {
        Serial.println("Erro ao redimensionar imagem para ML");
        releaseCameraBuffer(fb);
        return "erro";
    }
    
    releaseCameraBuffer(fb);
    
    // 3. Preparar dados para inferência
    ei_impulse_result_t result = {0};
    signal_t features_signal;
    features_signal.total_length = EI_CLASSIFIER_INPUT_WIDTH * EI_CLASSIFIER_INPUT_HEIGHT * EI_CLASSIFIER_RAW_SAMPLES_PER_FRAME;
    features_signal.get_data = &getSignalData;
    
    // 4. Executar inferência
    unsigned long inference_start = millis();
    EI_IMPULSE_ERROR ei_error = run_classifier(&features_signal, &result, DEBUG_PREDICTIONS);
    unsigned long inference_time = millis() - inference_start;
    
    if (ei_error != EI_IMPULSE_OK) {
        Serial.printf("Erro na inferencia: %d\n", ei_error);
        return "erro";
    }
    
    if (DEBUG_PREDICTIONS && predictionCount % 5 == 0) {
        Serial.printf("Tempo de inferencia: %lu ms\n", inference_time);
    }
    
    // 5. Processar resultado
    processMLResult(&result);
    
    return currentWashingStage;
}

int getSignalData(size_t offset, size_t length, float *out_ptr) {
    // Converter uint8 para float normalizado (0-1)
    size_t total_samples = EI_CLASSIFIER_INPUT_WIDTH * EI_CLASSIFIER_INPUT_HEIGHT * EI_CLASSIFIER_RAW_SAMPLES_PER_FRAME;
    
    for (size_t i = 0; i < length; i++) {
        size_t sample_index = offset + i;
        
        if (sample_index < total_samples) {
            // Normalizar de 0-255 para 0.0-1.0
            out_ptr[i] = (float)resized_image[sample_index] / 255.0f;
        } else {
            // Preencher com zeros se ultrapassar o buffer
            out_ptr[i] = 0.0f;
        }
    }
    
    return 0;
}

void processMLResult(ei_impulse_result_t* result) {
    // Encontrar classe com maior confiança
    float max_confidence = 0;
    int predicted_class = 0;
    
    for (int i = 0; i < EI_CLASSIFIER_LABEL_COUNT; i++) {
        if (result->classification[i].value > max_confidence) {
            max_confidence = result->classification[i].value;
            predicted_class = i;
        }
    }
    
    // Usar as classes reais do modelo
    String newStage = String(ei_classifier_inferencing_categories[predicted_class]);
    
    // Verificar se a confiança é suficiente
    if (max_confidence >= MIN_CONFIDENCE) {
        // Só atualizar se mudou de estado
        if (newStage != lastWashingStage) {
            // Validar mudança de estado (evitar oscilações)
            static String pendingStage = "";
            static int pendingCount = 0;
            
            if (newStage == pendingStage) {
                pendingCount++;
                if (pendingCount >= 2) { // Confirmar com 2 predições consecutivas
                    currentWashingStage = newStage;
                    lastWashingStage = newStage;
                    lastConfidence = max_confidence;
                    pendingCount = 0;
                    
                    // Log da mudança
                    Serial.println("=======================================");
                    Serial.printf("  NOVA ETAPA: %-19s\n", newStage.c_str());
                    Serial.printf("  Confianca: %.1f%%\n", max_confidence * 100);
                    Serial.printf("  Timestamp: %-18lu\n", millis());
                    Serial.println("=======================================");
                    
                    // Notificar outros módulos
                    onStageChanged(newStage, max_confidence);
                }
            } else {
                pendingStage = newStage;
                pendingCount = 1;
            }
        } else {
            // Atualizar confiança mesmo se for a mesma etapa
            lastConfidence = max_confidence;
        }
    }
    
    // Debug detalhado a cada 10 predições
    static int debug_counter = 0;
    if (DEBUG_PREDICTIONS && ++debug_counter % 10 == 0) {
        printDetailedPrediction(result);
    }
}

void printDetailedPrediction(ei_impulse_result_t* result) {
    Serial.println("┌─── PREDICOES DETALHADAS ───┐");
    
    for (int i = 0; i < EI_CLASSIFIER_LABEL_COUNT; i++) {
        float confidence = result->classification[i].value * 100;
        const char* className = ei_classifier_inferencing_categories[i];
        
        // Destacar a classe atual
        if (String(className) == currentWashingStage) {
            Serial.printf("│ > %-12s: %5.1f%% < │\n", className, confidence);
        } else {
            Serial.printf("│   %-12s: %5.1f%%   │\n", className, confidence);
        }
    }
    
    Serial.println("├─────────────────────────────┤");
    Serial.printf("│ Atual: %-12s (%4.1f%%) │\n", currentWashingStage.c_str(), lastConfidence * 100);
    Serial.printf("│ Tempo: %-18lu │\n", millis());
    Serial.println("└─────────────────────────────┘");
}

// Callback para notificar mudança de etapa
void onStageChanged(String newStage, float confidence) {
    // Notificar módulo Sinric Pro
    extern void updateSinricProStatus(String stage, float confidence);
    updateSinricProStatus(newStage, confidence);
    
    // Piscar LED para indicar mudança
    extern void indicateStageChange();
    indicateStageChange();
    
    // Log para análise posterior
    logStageChange(newStage, confidence);
}

void logStageChange(String stage, float confidence) {
    static unsigned long lastLogTime = 0;
    unsigned long now = millis();
    
    // Log a cada mudança com timestamp
    Serial.printf("LOG: %lu,%s,%.3f\n", now, stage.c_str(), confidence);
    
    // Estatísticas de tempo entre mudanças
    if (lastLogTime > 0) {
        unsigned long timeDiff = now - lastLogTime;
        Serial.printf("Tempo desde ultima mudanca: %lu ms (%.1f min)\n", 
                     timeDiff, timeDiff / 60000.0);
    }
    lastLogTime = now;
}

// Função para obter estatísticas do modelo
void printMLStatistics() {
    static int totalPredictions = 0;
    
    totalPredictions++;
    
    Serial.println("=== ESTATISTICAS ML ===");
    Serial.printf("Total de predicoes: %d\n", totalPredictions);
    Serial.printf("Etapa atual: %s\n", currentWashingStage.c_str());
    Serial.printf("Confianca atual: %.1f%%\n", lastConfidence * 100);
    Serial.printf("Classes do modelo: %d\n", EI_CLASSIFIER_LABEL_COUNT);
    Serial.printf("Resolucao: %dx%d\n", EI_CLASSIFIER_INPUT_WIDTH, EI_CLASSIFIER_INPUT_HEIGHT);
    Serial.println("======================");
}

// Função para validar integridade do modelo
bool validateModel() {
    // Verificações básicas usando constantes disponíveis
    if (EI_CLASSIFIER_LABEL_COUNT == 0) {
        Serial.println("ERRO CRITICO: Modelo sem classes");
        return false;
    }
    if (EI_CLASSIFIER_INPUT_WIDTH == 0) {
        Serial.println("ERRO CRITICO: Largura invalida");
        return false;
    }
    if (EI_CLASSIFIER_INPUT_HEIGHT == 0) {
        Serial.println("ERRO CRITICO: Altura invalida");
        return false;
    }
    if (EI_CLASSIFIER_RAW_SAMPLES_PER_FRAME == 0) {
        Serial.println("ERRO CRITICO: Samples por frame invalido");
        return false;
    }
    
    // Verificar se as classes esperadas existem
    bool hasExpectedClasses = true;
    for (int i = 0; i < 6; i++) {
        bool found = false;
        for (int j = 0; j < EI_CLASSIFIER_LABEL_COUNT; j++) {
            if (String(CLASS_NAMES[i]) == String(ei_classifier_inferencing_categories[j])) {
                found = true;
                break;
            }
        }
        if (!found) {
            Serial.printf("AVISO: Classe esperada '%s' nao encontrada no modelo\n", CLASS_NAMES[i]);
            hasExpectedClasses = false;
        }
    }
    
    if (hasExpectedClasses) {
        Serial.println("Todas as classes esperadas encontradas no modelo!");
    } else {
        Serial.println("AVISO: Algumas classes esperadas nao foram encontradas");
        Serial.println("O sistema funcionara com as classes do modelo treinado");
    }
    
    return true; // Retornar true se verificações básicas passaram
}

#endif // ML_INFERENCE_H