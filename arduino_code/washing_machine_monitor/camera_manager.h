#ifndef CAMERA_MANAGER_H
#define CAMERA_MANAGER_H

#include "esp_camera.h"
#include "config.h"

// =================== FUNÇÕES PÚBLICAS ===================
bool initializeCamera();
camera_fb_t* captureImage();
void releaseCameraBuffer(camera_fb_t* fb);
bool resizeImageForML(uint8_t* input_buf, size_t input_len, uint8_t* output_buf);
void optimizeCameraSettings();

// =================== IMPLEMENTAÇÃO ===================

bool initializeCamera() {
    Serial.println("Inicializando camera ESP32-CAM...");
    
    camera_config_t config;
    config.ledc_channel = LEDC_CHANNEL_0;
    config.ledc_timer = LEDC_TIMER_0;
    config.pin_d0 = Y2_GPIO_NUM;
    config.pin_d1 = Y3_GPIO_NUM;
    config.pin_d2 = Y4_GPIO_NUM;
    config.pin_d3 = Y5_GPIO_NUM;
    config.pin_d4 = Y6_GPIO_NUM;
    config.pin_d5 = Y7_GPIO_NUM;
    config.pin_d6 = Y8_GPIO_NUM;
    config.pin_d7 = Y9_GPIO_NUM;
    config.pin_xclk = XCLK_GPIO_NUM;
    config.pin_pclk = PCLK_GPIO_NUM;
    config.pin_vsync = VSYNC_GPIO_NUM;
    config.pin_href = HREF_GPIO_NUM;
    config.pin_sscb_sda = SIOD_GPIO_NUM;
    config.pin_sscb_scl = SIOC_GPIO_NUM;
    config.pin_pwdn = PWDN_GPIO_NUM;
    config.pin_reset = RESET_GPIO_NUM;
    config.xclk_freq_hz = 20000000;
    config.pixel_format = PIXFORMAT_RGB565; // RGB para o modelo ML
    config.frame_size = FRAMESIZE_QVGA;     // 320x240 - bom compromisso
    config.jpeg_quality = 4;                // Alta qualidade
    config.fb_count = 2;                    // Double buffering
    
    // Inicializar câmera
    esp_err_t err = esp_camera_init(&config);
    if (err != ESP_OK) {
        Serial.printf("ERRO: Falha ao inicializar camera: 0x%x\n", err);
        return false;
    }
    
    Serial.println("Camera inicializada com sucesso!");
    
    // Aplicar configurações otimizadas
    optimizeCameraSettings();
    
    return true;
}

camera_fb_t* captureImage() {
    camera_fb_t* fb = esp_camera_fb_get();
    if (!fb) {
        Serial.println("ERRO: Falha ao capturar imagem da camera");
        return nullptr;
    }
    
    // Verificar se a imagem tem conteúdo válido
    if (fb->len == 0) {
        Serial.println("ERRO: Imagem capturada esta vazia");
        esp_camera_fb_return(fb);
        return nullptr;
    }
    
    return fb;
}

void releaseCameraBuffer(camera_fb_t* fb) {
    if (fb) {
        esp_camera_fb_return(fb);
    }
}

void optimizeCameraSettings() {
    Serial.println("Aplicando configuracoes otimizadas da camera...");
    
    sensor_t* s = esp_camera_sensor_get();
    if (!s) {
        Serial.println("ERRO: Nao foi possivel obter sensor da camera");
        return;
    }
    
    // Configurações otimizadas para detecção de LEDs
    s->set_brightness(s, 0);        // Brilho neutro
    s->set_contrast(s, 2);          // Contraste alto (melhor para LEDs)
    s->set_saturation(s, 0);        // Saturação normal
    s->set_special_effect(s, 0);    // Sem efeitos especiais
    s->set_whitebal(s, 1);          // White balance automático
    s->set_awb_gain(s, 1);          // AWB gain ativo
    s->set_wb_mode(s, 0);           // Auto white balance
    s->set_exposure_ctrl(s, 1);     // Controle de exposição ativo
    s->set_aec2(s, 1);              // AEC2 ativo (melhor exposição)
    s->set_ae_level(s, 0);          // Nível de exposição neutro
    s->set_aec_value(s, 600);       // Exposição moderada
    s->set_gain_ctrl(s, 1);         // Controle de ganho ativo
    s->set_agc_gain(s, 10);         // Ganho moderado
    s->set_gainceiling(s, (gainceiling_t)4); // Teto de ganho
    s->set_bpc(s, 1);               // Correção de bad pixel
    s->set_wpc(s, 1);               // Correção de white pixel
    s->set_raw_gma(s, 1);           // Gamma correction
    s->set_lenc(s, 1);              // Correção de lente
    s->set_hmirror(s, 0);           // Sem espelho horizontal
    s->set_vflip(s, 0);             // Sem flip vertical
    s->set_dcw(s, 1);               // Downsize enable
    s->set_colorbar(s, 0);          // Sem barra de cores
    
    Serial.println("Configuracoes da camera aplicadas!");
    Serial.printf("Resolucao: %dx%d\n", 320, 240);
    Serial.printf("Formato: RGB565\n");
    Serial.printf("Qualidade: Otimizada para LEDs\n");
}

bool resizeImageForML(uint8_t* input_buf, size_t input_len, uint8_t* output_buf) {
    // Configurações de entrada e saída
    const int input_width = 320;
    const int input_height = 240;
    const int output_width = EI_CLASSIFIER_INPUT_WIDTH;
    const int output_height = EI_CLASSIFIER_INPUT_HEIGHT;
    const int bytes_per_pixel_input = 2;   // RGB565 = 2 bytes
    const int bytes_per_pixel_output = 3;  // RGB888 = 3 bytes
    
    // Verificar se o buffer de entrada tem tamanho suficiente
    size_t expected_input_size = input_width * input_height * bytes_per_pixel_input;
    if (input_len < expected_input_size) {
        Serial.printf("ERRO: Buffer de entrada muito pequeno. Esperado: %d, Recebido: %d\n", 
                     expected_input_size, input_len);
        return false;
    }
    
    // Calcular região central para crop
    int start_x = (input_width - output_width) / 2;
    int start_y = (input_height - output_height) / 2;
    
    // Garantir que não saia dos limites
    if (start_x < 0) start_x = 0;
    if (start_y < 0) start_y = 0;
    if (start_x + output_width > input_width) start_x = input_width - output_width;
    if (start_y + output_height > input_height) start_y = input_height - output_height;
    
    // Fazer crop e conversão RGB565 -> RGB888
    for (int y = 0; y < output_height; y++) {
        for (int x = 0; x < output_width; x++) {
            // Calcular índices
            int src_x = start_x + x;
            int src_y = start_y + y;
            int input_idx = (src_y * input_width + src_x) * bytes_per_pixel_input;
            int output_idx = (y * output_width + x) * bytes_per_pixel_output;
            
            // Verificar limites
            if (input_idx + 1 < input_len) {
                // Ler pixel RGB565 (little endian)
                uint16_t pixel = (input_buf[input_idx + 1] << 8) | input_buf[input_idx];
                
                // Converter RGB565 para RGB888
                uint8_t r = ((pixel >> 11) & 0x1F) << 3;  // 5 bits -> 8 bits
                uint8_t g = ((pixel >> 5) & 0x3F) << 2;   // 6 bits -> 8 bits  
                uint8_t b = (pixel & 0x1F) << 3;          // 5 bits -> 8 bits
                
                // Aplicar dithering para melhor qualidade
                r |= (r >> 5);
                g |= (g >> 6);
                b |= (b >> 5);
                
                // Armazenar no buffer de saída
                output_buf[output_idx] = r;
                output_buf[output_idx + 1] = g;
                output_buf[output_idx + 2] = b;
            } else {
                // Pixel fora dos limites - preencher com preto
                output_buf[output_idx] = 0;
                output_buf[output_idx + 1] = 0;
                output_buf[output_idx + 2] = 0;
            }
        }
    }
    
    return true;
}

void printCameraInfo() {
    sensor_t* s = esp_camera_sensor_get();
    if (s) {
        Serial.println("=== INFORMACOES DA CAMERA ===");
        Serial.printf("ID do sensor: 0x%02X\n", s->id.PID);
        Serial.printf("Resolucao: %dx%d\n", 320, 240);
        Serial.printf("Formato: RGB565\n");
        Serial.printf("Buffers: 2 (double buffering)\n");
        Serial.println("============================");
    }
}

// Função para ajustar configurações em tempo real
void adjustCameraForLighting(bool brightEnvironment) {
    sensor_t* s = esp_camera_sensor_get();
    if (!s) return;
    
    if (brightEnvironment) {
        // Ambiente claro - reduzir exposição
        s->set_aec_value(s, 400);
        s->set_agc_gain(s, 5);
        Serial.println("Camera ajustada para ambiente claro");
    } else {
        // Ambiente escuro - aumentar exposição
        s->set_aec_value(s, 800);
        s->set_agc_gain(s, 15);
        Serial.println("Camera ajustada para ambiente escuro");
    }
}

#endif // CAMERA_MANAGER_H