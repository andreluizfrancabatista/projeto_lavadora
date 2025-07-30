#ifndef ML_INFERENCE_H
#define ML_INFERENCE_H

#include <andreluiz-project-1_inferencing.h>
#include "config.h"
#include "camera_manager.h"

// =================== VARIÁVEIS GLOBAIS ===================
extern String currentWashingStage;
extern String lastWashingStage;
extern float lastConfidence;

// Buffer para imagem redimensionada (com proteção extra)
static uint8_t* resized_image = nullptr;
static bool ml_buffer_allocated = false;
static size_t allocated_buffer_size = 0;

// Flags de segurança
static bool ml_system_stable = true;
static int critical_errors = 0;

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
void safeCleanupML();
bool allocateMLBuffers();
bool performSafeInferenceTest();
bool isMemorySafeForML();
void handleCriticalMLError(String error);

// =================== IMPLEMENTAÇÃO ===================

bool isMemorySafeForML() {
    size_t free_heap = ESP.getFreeHeap();
    size_t largest_block = ESP.getMaxAllocHeap();
    
    // Verificações rigorosas de memória
    if (free_heap < 80000) {
        Serial.printf("Memoria insuficiente: %d bytes\n", free_heap);
        return false;
    }
    
    if (largest_block < 50000) {
        Serial.printf("Maior bloco insuficiente: %d bytes\n", largest_block);
        return false;
    }
    
    // Verificar fragmentação
    if (largest_block < (free_heap / 2)) {
        Serial.println("Memoria muito fragmentada");
        return false;
    }
    
    return true;
}

void handleCriticalMLError(String error) {
    critical_errors++;
    ml_system_stable = false;
    
    Serial.printf("ERRO CRITICO ML #%d: %s\n", critical_errors, error.c_str());
    
    // Cleanup imediato
    safeCleanupML();
    
    // Se muitos erros, desabilitar permanentemente
    if (critical_errors > 3) {
        Serial.println("Muitos erros criticos - ML desabilitado permanentemente");
        ml_system_stable = false;
    }
    
    // Forçar garbage collection
    delay(1000);
}

bool allocateMLBuffers() {
    if (ml_buffer_allocated && resized_image != nullptr) {
        return true; // Já alocado
    }
    
    if (!isMemorySafeForML()) {
        Serial.println("Condicoes de memoria nao seguras para ML");
        return false;
    }
    
    allocated_buffer_size = EI_CLASSIFIER_INPUT_WIDTH * EI_CLASSIFIER_INPUT_HEIGHT * EI_CLASSIFIER_RAW_SAMPLES_PER_FRAME;
    
    Serial.printf("Tentando alocar buffer ML: %d bytes\n", allocated_buffer_size);
    Serial.printf("Heap antes da alocacao: %d bytes\n", ESP.getFreeHeap());
    
    // Alocar com margem de segurança (50% extra)
    size_t safe_buffer_size = allocated_buffer_size + (allocated_buffer_size / 2);
    resized_image = (uint8_t*)calloc(safe_buffer_size, 1); // calloc zera a memória
    
    if (resized_image == nullptr) {
        handleCriticalMLError("Falha ao alocar buffer ML");
        return false;
    }
    
    // Verificar se a alocação foi realmente bem-sucedida
    if (ESP.getFreeHeap() < 50000) {
        Serial.println("Heap muito baixo após alocação - liberando buffer");
        free(resized_image);
        resized_image = nullptr;
        return false;
    }
    
    ml_buffer_allocated = true;
    Serial.printf("Buffer ML alocado: %d bytes (safe size). Heap restante: %d bytes\n", 
                 safe_buffer_size, ESP.getFreeHeap());
    return true;
}

void safeCleanupML() {
    if (resized_image != nullptr) {
        free(resized_image);
        resized_image = nullptr;
    }
    ml_buffer_allocated = false;
    allocated_buffer_size = 0;
    Serial.println("Buffer ML liberado com seguranca");
}

bool performSafeInferenceTest() {
    if (!ml_buffer_allocated || resized_image == nullptr) {
        handleCriticalMLError("Buffer ML nao alocado para teste");
        return false;
    }
    
    if (!isMemorySafeForML()) {
        handleCriticalMLError("Memoria nao segura para teste");
        return false;
    }
    
    // Inicializar buffer com padrão seguro
    memset(resized_image, 128, allocated_buffer_size); // Cinza médio ao invés de preto
    
    ei_impulse_result_t result = {0};
    signal_t features_signal;
    features_signal.total_length = EI_CLASSIFIER_INPUT_WIDTH * EI_CLASSIFIER_INPUT_HEIGHT * EI_CLASSIFIER_RAW_SAMPLES_PER_FRAME;
    features_signal.get_data = &getSignalData;
    
    Serial.println("Iniciando teste de inferencia seguro...");
    
    // Verificar heap antes do teste
    size_t heap_before = ESP.getFreeHeap();
    
    // Executar inferência de teste com timeout rigoroso
    unsigned long start_time = millis();
    EI_IMPULSE_ERROR ei_error = run_classifier(&features_signal, &result, false);
    unsigned long inference_time = millis() - start_time;
    
    // Verificar heap após o teste
    size_t heap_after = ESP.getFreeHeap();
    
    if (ei_error != EI_IMPULSE_OK) {
        handleCriticalMLError("Erro no teste de inferencia: " + String(ei_error));
        return false;
    }
    
    if (inference_time > 8000) { // 8 segundos máximo
        handleCriticalMLError("Teste de inferencia muito lento: " + String(inference_time) + "ms");
        return false;
    }
    
    // Verificar se houve vazamento de memória
    if (heap_after < (heap_before - 5000)) {
        handleCriticalMLError("Possivel vazamento de memoria detectado");
        return false;
    }
    
    Serial.printf("Teste de inferencia OK (%lu ms, heap: %d->%d)\n", 
                 inference_time, heap_before, heap_after);
    return true;
}

bool initializeMLModel() {
    Serial.println("Inicializando modelo Edge Impulse com seguranca maxima...");
    
    // Reset de flags de segurança
    ml_system_stable = true;
    critical_errors = 0;
    
    // Verificação rigorosa de memória
    if (!isMemorySafeForML()) {
        handleCriticalMLError("Memoria insuficiente para inicializacao");
        return false;
    }
    
    // Alocar buffers com proteção
    if (!allocateMLBuffers()) {
        return false;
    }
    
    // Verificar informações do modelo
    Serial.printf("Classes detectadas: %d\n", EI_CLASSIFIER_LABEL_COUNT);
    Serial.printf("Tamanho entrada: %dx%d\n", EI_CLASSIFIER_INPUT_WIDTH, EI_CLASSIFIER_INPUT_HEIGHT);
    Serial.printf("Samples por frame: %d\n", EI_CLASSIFIER_RAW_SAMPLES_PER_FRAME);
    Serial.printf("Buffer necessario: %d bytes\n", allocated_buffer_size);
    
    // Listar classes
    Serial.println("Classes disponíveis:");
    for (int i = 0; i < EI_CLASSIFIER_LABEL_COUNT; i++) {
        Serial.printf("  %d: %s\n", i, ei_classifier_inferencing_categories[i]);
    }
    
    // Validações críticas
    if (EI_CLASSIFIER_LABEL_COUNT == 0) {
        handleCriticalMLError("Modelo sem classes validas");
        return false;
    }
    
    if (EI_CLASSIFIER_INPUT_WIDTH == 0 || EI_CLASSIFIER_INPUT_HEIGHT == 0) {
        handleCriticalMLError("Dimensoes de entrada invalidas");
        return false;
    }
    
    if (allocated_buffer_size > 100000) { // Buffer muito grande
        handleCriticalMLError("Buffer ML muito grande: " + String(allocated_buffer_size));
        return false;
    }
    
    // Teste rigoroso de inferência
    Serial.println("Executando testes de seguranca...");
    if (!performSafeInferenceTest()) {
        return false;
    }
    
    // Teste adicional com dados diferentes
    delay(500);
    if (!performSafeInferenceTest()) {
        return false;
    }
    
    Serial.println("Modelo ML inicializado com seguranca maxima!");
    return true;
}

String performMLPrediction() {
    static int predictionCount = 0;
    static unsigned long lastFailure = 0;
    
    predictionCount++;
    
    // Verificar estabilidade do sistema
    if (!ml_system_stable) {
        return currentWashingStage;
    }
    
    // Cooldown após falha
    if (millis() - lastFailure < 60000) { // 1 minuto
        if (predictionCount % 20 == 0) {
            Serial.println("ML em cooldown apos erro");
        }
        return currentWashingStage;
    }
    
    // Verificações de segurança rigorosas
    if (!isMemorySafeForML()) {
        handleCriticalMLError("Memoria nao segura para predicao");
        lastFailure = millis();
        return "erro";
    }
    
    if (!ml_buffer_allocated || resized_image == nullptr) {
        handleCriticalMLError("Buffer ML nao disponivel");
        lastFailure = millis();
        return "erro";
    }
    
    if (DEBUG_PREDICTIONS && predictionCount % 5 == 0) {
        Serial.printf("--- Predicao Segura #%d ---\n", predictionCount);
        Serial.printf("Heap antes: %d bytes\n", ESP.getFreeHeap());
    }
    
    // 1. Capturar imagem com verificação
    camera_fb_t* fb = captureImage();
    if (!fb) {
        handleCriticalMLError("Falha na captura de imagem");
        lastFailure = millis();
        return "erro";
    }
    
    // Verificações rigorosas da imagem
    if (fb->len == 0 || fb->buf == nullptr) {
        releaseCameraBuffer(fb);
        handleCriticalMLError("Imagem capturada invalida");
        lastFailure = millis();
        return "erro";
    }
    
    if (fb->len > 500000) { // Imagem muito grande
        releaseCameraBuffer(fb);
        handleCriticalMLError("Imagem muito grande: " + String(fb->len));
        lastFailure = millis();
        return "erro";
    }
    
    // 2. Redimensionar com verificação de bounds
    bool resize_success = false;
    
    // Verificar se temos memória suficiente durante o redimensionamento
    if (ESP.getFreeHeap() > 60000) {
        resize_success = resizeImageForML(fb->buf, fb->len, resized_image);
    }
    
    releaseCameraBuffer(fb);
    
    if (!resize_success) {
        handleCriticalMLError("Falha no redimensionamento de imagem");
        lastFailure = millis();
        return "erro";
    }
    
    // 3. Verificar heap antes da inferência
    size_t heap_before_inference = ESP.getFreeHeap();
    if (heap_before_inference < 50000) {
        handleCriticalMLError("Heap insuficiente para inferencia: " + String(heap_before_inference));
        lastFailure = millis();
        return "erro";
    }
    
    // 4. Preparar dados para inferência
    ei_impulse_result_t result = {0};
    signal_t features_signal;
    features_signal.total_length = EI_CLASSIFIER_INPUT_WIDTH * EI_CLASSIFIER_INPUT_HEIGHT * EI_CLASSIFIER_RAW_SAMPLES_PER_FRAME;
    features_signal.get_data = &getSignalData;
    
    // 5. Executar inferência com monitoramento rigoroso
    unsigned long inference_start = millis();
    EI_IMPULSE_ERROR ei_error = run_classifier(&features_signal, &result, false);
    unsigned long inference_time = millis() - inference_start;
    
    // Verificar heap após inferência
    size_t heap_after_inference = ESP.getFreeHeap();
    
    if (ei_error != EI_IMPULSE_OK) {
        handleCriticalMLError("Erro na inferencia: " + String(ei_error));
        lastFailure = millis();
        return "erro";
    }
    
    if (inference_time > 12000) { // 12 segundos máximo
        handleCriticalMLError("Inferencia muito lenta: " + String(inference_time) + "ms");
        lastFailure = millis();
        return "erro";
    }
    
    // Verificar vazamento de memória
    if (heap_after_inference < (heap_before_inference - 10000)) {
        handleCriticalMLError("Vazamento de memoria detectado");
        lastFailure = millis();
        return "erro";
    }
    
    if (DEBUG_PREDICTIONS && predictionCount % 5 == 0) {
        Serial.printf("Inferencia OK: %lu ms, heap: %d->%d\n", 
                     inference_time, heap_before_inference, heap_after_inference);
    }
    
    // 6. Processar resultado
    processMLResult(&result);
    
    return currentWashingStage;
}

int getSignalData(size_t offset, size_t length, float *out_ptr) {
    // Verificações de segurança rigorosas
    if (!ml_buffer_allocated || resized_image == nullptr || out_ptr == nullptr) {
        Serial.println("ERRO: getSignalData com parametros invalidos");
        return -1;
    }
    
    size_t total_samples = EI_CLASSIFIER_INPUT_WIDTH * EI_CLASSIFIER_INPUT_HEIGHT * EI_CLASSIFIER_RAW_SAMPLES_PER_FRAME;
    
    // Verificar bounds
    if (offset >= total_samples) {
        Serial.printf("ERRO: offset invalido: %d >= %d\n", offset, total_samples);
        return -1;
    }
    
    if (offset + length > total_samples) {
        Serial.printf("ERRO: acesso fora dos limites: %d + %d > %d\n", offset, length, total_samples);
        return -1;
    }
    
    // Converter com verificação de bounds
    for (size_t i = 0; i < length; i++) {
        size_t sample_index = offset + i;
        
        if (sample_index < total_samples && sample_index < allocated_buffer_size) {
            // Normalizar de 0-255 para 0.0-1.0
            out_ptr[i] = (float)resized_image[sample_index] / 255.0f;
        } else {
            // Preencher com valor seguro
            out_ptr[i] = 0.5f;
        }
    }
    
    return 0;
}

void processMLResult(ei_impulse_result_t* result) {
    if (result == nullptr) {
        Serial.println("ERRO: resultado ML nulo");
        return;
    }
    
    // Encontrar classe com maior confiança
    float max_confidence = 0;
    int predicted_class = 0;
    
    for (int i = 0; i < EI_CLASSIFIER_LABEL_COUNT; i++) {
        if (result->classification[i].value > max_confidence) {
            max_confidence = result->classification[i].value;
            predicted_class = i;
        }
    }
    
    // Verificar se o índice é válido
    if (predicted_class < 0 || predicted_class >= EI_CLASSIFIER_LABEL_COUNT) {
        Serial.printf("ERRO: classe predita invalida: %d\n", predicted_class);
        return;
    }
    
    String newStage = String(ei_classifier_inferencing_categories[predicted_class]);
    
    // Verificar se a confiança é suficiente
    if (max_confidence >= MIN_CONFIDENCE) {
        if (newStage != lastWashingStage) {
            static String pendingStage = "";
            static int pendingCount = 0;
            
            if (newStage == pendingStage) {
                pendingCount++;
                if (pendingCount >= 3) { // 3 predições consecutivas para maior estabilidade
                    currentWashingStage = newStage;
                    lastWashingStage = newStage;
                    lastConfidence = max_confidence;
                    pendingCount = 0;
                    
                    Serial.println("=======================================");
                    Serial.printf("  NOVA ETAPA: %-19s\n", newStage.c_str());
                    Serial.printf("  Confianca: %.1f%%\n", max_confidence * 100);
                    Serial.printf("  Timestamp: %-18lu\n", millis());
                    Serial.println("=======================================");
                    
                    onStageChanged(newStage, max_confidence);
                }
            } else {
                pendingStage = newStage;
                pendingCount = 1;
            }
        } else {
            lastConfidence = max_confidence;
        }
    }
}

void printDetailedPrediction(ei_impulse_result_t* result) {
    if (result == nullptr) return;
    
    Serial.println("┌─── PREDICOES DETALHADAS ───┐");
    
    for (int i = 0; i < EI_CLASSIFIER_LABEL_COUNT; i++) {
        float confidence = result->classification[i].value * 100;
        const char* className = ei_classifier_inferencing_categories[i];
        
        if (String(className) == currentWashingStage) {
            Serial.printf("│ > %-12s: %5.1f%% < │\n", className, confidence);
        } else {
            Serial.printf("│   %-12s: %5.1f%%   │\n", className, confidence);
        }
    }
    
    Serial.println("├─────────────────────────────┤");
    Serial.printf("│ Atual: %-12s (%4.1f%%) │\n", currentWashingStage.c_str(), lastConfidence * 100);
    Serial.printf("│ Heap: %-27d │\n", ESP.getFreeHeap());
    Serial.printf("│ Erros: %-26d │\n", critical_errors);
    Serial.println("└─────────────────────────────┘");
}

void onStageChanged(String newStage, float confidence) {
    extern void updateSinricProStatus(String stage, float confidence);
    updateSinricProStatus(newStage, confidence);
    
    extern void indicateStageChange();
    indicateStageChange();
    
    logStageChange(newStage, confidence);
}

void logStageChange(String stage, float confidence) {
    static unsigned long lastLogTime = 0;
    unsigned long now = millis();
    
    Serial.printf("LOG: %lu,%s,%.3f,%d\n", now, stage.c_str(), confidence, ESP.getFreeHeap());
    
    if (lastLogTime > 0) {
        unsigned long timeDiff = now - lastLogTime;
        Serial.printf("Tempo desde ultima mudanca: %lu ms (%.1f min)\n", 
                     timeDiff, timeDiff / 60000.0);
    }
    lastLogTime = now;
}

void printMLStatistics() {
    Serial.println("=== ESTATISTICAS ML SEGURAS ===");
    Serial.printf("Etapa atual: %s\n", currentWashingStage.c_str());
    Serial.printf("Confianca atual: %.1f%%\n", lastConfidence * 100);
    Serial.printf("Classes do modelo: %d\n", EI_CLASSIFIER_LABEL_COUNT);
    Serial.printf("Resolucao: %dx%d\n", EI_CLASSIFIER_INPUT_WIDTH, EI_CLASSIFIER_INPUT_HEIGHT);
    Serial.printf("Buffer alocado: %s (%d bytes)\n", ml_buffer_allocated ? "Sim" : "Nao", allocated_buffer_size);
    Serial.printf("Heap livre: %d bytes\n", ESP.getFreeHeap());
    Serial.printf("Sistema estavel: %s\n", ml_system_stable ? "Sim" : "Nao");
    Serial.printf("Erros criticos: %d\n", critical_errors);
    Serial.println("===============================");
}

bool validateModel() {
    if (EI_CLASSIFIER_LABEL_COUNT == 0) {
        Serial.println("ERRO CRITICO: Modelo sem classes");
        return false;
    }
    if (EI_CLASSIFIER_INPUT_WIDTH == 0 || EI_CLASSIFIER_INPUT_HEIGHT == 0) {
        Serial.println("ERRO CRITICO: Dimensoes invalidas");
        return false;
    }
    if (EI_CLASSIFIER_RAW_SAMPLES_PER_FRAME == 0) {
        Serial.println("ERRO CRITICO: Samples por frame invalido");
        return false;
    }
    
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
            Serial.printf("AVISO: Classe esperada '%s' nao encontrada\n", CLASS_NAMES[i]);
            hasExpectedClasses = false;
        }
    }
    
    if (hasExpectedClasses) {
        Serial.println("Todas as classes esperadas encontradas!");
    }
    
    return true;
}

#endif // ML_INFERENCE_H