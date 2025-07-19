#include "esp_camera.h"
#include "WiFi.h"
#include "WebServer.h"

// =================== CONFIGURAÇÕES ===================
// WiFi - ALTERE AQUI SUAS CREDENCIAIS
const char* ssid = "SEU_WIFI_AQUI";          // Nome da sua rede WiFi
const char* password = "SUA_SENHA_AQUI";      // Senha da sua rede WiFi

// Configuração da câmera ESP32-CAM AI-Thinker
#define PWDN_GPIO_NUM     32
#define RESET_GPIO_NUM    -1
#define XCLK_GPIO_NUM      0
#define SIOD_GPIO_NUM     26
#define SIOC_GPIO_NUM     27
#define Y9_GPIO_NUM       35
#define Y8_GPIO_NUM       34
#define Y7_GPIO_NUM       39
#define Y6_GPIO_NUM       36
#define Y5_GPIO_NUM       21
#define Y4_GPIO_NUM       19
#define Y3_GPIO_NUM       18
#define Y2_GPIO_NUM        5
#define VSYNC_GPIO_NUM    25
#define HREF_GPIO_NUM     23
#define PCLK_GPIO_NUM     22

// =================== VARIÁVEIS GLOBAIS ===================
WebServer server(80);
int imageCountTotal = 0;
int imageCountClass = 0;
String currentClass = "selecione_classe";

// Classes disponíveis
String classes[] = {
  "ligado_desligado",
  "centrifugacao", 
  "enxague",
  "molho_curto",
  "molho_normal",
  "molho_longo"
};

// =================== SETUP ===================
void setup() {
  Serial.begin(115200);
  Serial.println();
  Serial.println("=== INICIANDO SISTEMA DE COLETA ===");
  
  // Configurar câmera
  Serial.println("1. Configurando câmera...");
  if (!initCamera()) {
    Serial.println("❌ ERRO: Falha ao configurar câmera!");
    Serial.println("Verifique as conexões e reinicie o ESP32-CAM");
    while(1);
  }
  Serial.println("✅ Câmera configurada com sucesso!");
  
  // Conectar WiFi
  Serial.println("2. Conectando ao WiFi...");
  connectToWiFi();
  
  // Configurar servidor web
  Serial.println("3. Configurando servidor web...");
  setupWebServer();
  
  Serial.println("=== SISTEMA PRONTO! ===");
  Serial.println("🌐 Acesse: http://" + WiFi.localIP().toString());
  Serial.println("📱 Use seu celular ou computador para capturar imagens");
  Serial.println();
}

// =================== LOOP PRINCIPAL ===================
void loop() {
  server.handleClient();
  delay(10);
}

// =================== FUNÇÕES ===================

bool initCamera() {
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
  config.pixel_format = PIXFORMAT_JPEG;
  
  // ===== MELHOR QUALIDADE =====
  config.frame_size = FRAMESIZE_VGA;    // 640x480 (era QVGA 320x240)
  config.jpeg_quality = 4;              // 4 = alta qualidade (era 12)
  config.fb_count = 2;                  // 2 buffers (era 1)
  
  esp_err_t err = esp_camera_init(&config);
  if (err != ESP_OK) {
    Serial.printf("Erro camera: 0x%x\n", err);
    return false;
  }
  
  // ===== CONFIGURAÇÕES OTIMIZADAS PARA LEDs =====
  sensor_t * s = esp_camera_sensor_get();
  if (s) {
    s->set_brightness(s, 0);        // 0 = brilho neutro
    s->set_contrast(s, 2);          // 2 = contraste alto (melhor para LEDs)
    s->set_saturation(s, 0);        // 0 = saturação normal
    s->set_special_effect(s, 0);    // 0 = sem efeitos
    s->set_whitebal(s, 1);          // 1 = white balance ativo
    s->set_awb_gain(s, 1);          // 1 = AWB gain ativo
    s->set_wb_mode(s, 0);           // 0 = auto white balance
    s->set_exposure_ctrl(s, 1);     // 1 = controle exposição ativo
    s->set_aec2(s, 1);              // 1 = AEC2 ativo (melhor exposição)
    s->set_ae_level(s, 0);          // 0 = nível exposição neutro
    s->set_aec_value(s, 600);       // 600 = exposição moderada
    s->set_gain_ctrl(s, 1);         // 1 = controle ganho ativo
    s->set_agc_gain(s, 10);         // 10 = ganho moderado
    s->set_gainceiling(s, (gainceiling_t)4); // 4 = teto de ganho
    s->set_bpc(s, 1);               // 1 = correção bad pixel
    s->set_wpc(s, 1);               // 1 = correção white pixel
    s->set_raw_gma(s, 1);           // 1 = gamma correction
    s->set_lenc(s, 1);              // 1 = correção de lente
    s->set_hmirror(s, 0);           // 0 = sem espelho horizontal
    s->set_vflip(s, 0);             // 0 = sem flip vertical
    s->set_dcw(s, 1);               // 1 = downsize enable
    s->set_colorbar(s, 0);          // 0 = sem barra de cores
  }
  
  Serial.println("Camera configurada para ALTA QUALIDADE!");
  Serial.println("Resolucao: 640x480, Qualidade JPEG: 4/63");
  
  return true;
}

void connectToWiFi() {
  WiFi.begin(ssid, password);
  Serial.print("Conectando");
  
  int attempts = 0;
  while (WiFi.status() != WL_CONNECTED && attempts < 30) {
    delay(1000);
    Serial.print(".");
    attempts++;
  }
  
  if (WiFi.status() == WL_CONNECTED) {
    Serial.println();
    Serial.println("✅ WiFi conectado!");
    Serial.print("📡 IP: ");
    Serial.println(WiFi.localIP());
  } else {
    Serial.println();
    Serial.println("❌ ERRO: Não foi possível conectar ao WiFi");
    Serial.println("Verifique SSID e senha, depois reinicie o ESP32-CAM");
    while(1);
  }
}

void setupWebServer() {
  server.on("/", handleRoot);
  server.on("/preview", handlePreview);
  server.on("/capture", handleCapture);
  server.on("/setclass", handleSetClass);
  server.on("/stats", handleStats);
  server.begin();
  Serial.println("✅ Servidor web ativo!");
}

// =================== HANDLERS WEB ===================

void handleRoot() {
  String html = R"(
<!DOCTYPE html>
<html>
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>📸 Coleta de Dados - Lavadora IoT</title>
    <style>
        body { 
            font-family: Arial, sans-serif; 
            margin: 0; 
            padding: 20px; 
            background: linear-gradient(135deg, #667eea 0%, #764ba2 100%);
            color: #333;
            min-height: 100vh;
        }
        .container { 
            max-width: 800px; 
            margin: 0 auto; 
            background: rgba(255,255,255,0.95); 
            padding: 30px; 
            border-radius: 15px; 
            box-shadow: 0 10px 30px rgba(0,0,0,0.3);
        }
        h1 { 
            text-align: center; 
            color: #2c3e50; 
            margin-bottom: 30px;
            font-size: 2.5em;
        }
        .status-panel {
            background: #ecf0f1;
            padding: 20px;
            border-radius: 10px;
            margin-bottom: 30px;
            border-left: 5px solid #3498db;
        }
        .class-buttons {
            display: grid;
            grid-template-columns: repeat(auto-fit, minmax(200px, 1fr));
            gap: 15px;
            margin-bottom: 30px;
        }
        .class-btn {
            padding: 15px;
            border: none;
            border-radius: 8px;
            font-size: 16px;
            cursor: pointer;
            transition: all 0.3s ease;
            font-weight: bold;
        }
        .class-btn:hover { transform: translateY(-2px); box-shadow: 0 5px 15px rgba(0,0,0,0.2); }
        .btn-ligado { background: #e74c3c; color: white; }
        .btn-centrifugacao { background: #9b59b6; color: white; }
        .btn-enxague { background: #3498db; color: white; }
        .btn-molho-curto { background: #1abc9c; color: white; }
        .btn-molho-normal { background: #f39c12; color: white; }
        .btn-molho-longo { background: #34495e; color: white; }
        
        .active-class { border: 4px solid #27ae60 !important; box-shadow: 0 0 20px rgba(39,174,96,0.5); }
        
        .preview-section {
            text-align: center;
            margin: 30px 0;
        }
        .preview-img {
            max-width: 100%;
            height: auto;
            border-radius: 10px;
            box-shadow: 0 5px 15px rgba(0,0,0,0.2);
            margin: 20px 0;
        }
        .capture-btn {
            background: #27ae60;
            color: white;
            padding: 20px 40px;
            border: none;
            border-radius: 10px;
            font-size: 20px;
            font-weight: bold;
            cursor: pointer;
            transition: all 0.3s ease;
            margin: 10px;
        }
        .capture-btn:hover { 
            background: #229954; 
            transform: scale(1.05);
        }
        .capture-btn:disabled {
            background: #95a5a6;
            cursor: not-allowed;
            transform: none;
        }
        .stats { 
            font-size: 18px; 
            text-align: center; 
            margin: 20px 0;
        }
        .current-class { 
            font-weight: bold; 
            font-size: 24px; 
            color: #e74c3c;
        }
        .tips {
            background: #fff3cd;
            border: 1px solid #ffeaa7;
            border-radius: 8px;
            padding: 15px;
            margin: 20px 0;
        }
        .tips h3 { color: #d68910; margin-top: 0; }
    </style>
</head>
<body>
    <div class="container">
        <h1>📸 Coleta de Dados - Lavadora IoT</h1>
        
        <div class="status-panel">
            <div class="stats">
                <p>🎯 <strong>Classe Atual:</strong> <span class="current-class" id="currentClass">Selecione uma classe</span></p>
                <p>📊 <strong>Imagens desta classe:</strong> <span id="classCount">0</span></p>
                <p>📈 <strong>Total de imagens:</strong> <span id="totalCount">0</span></p>
            </div>
        </div>

        <div class="tips">
            <h3>💡 Dicas para boa coleta:</h3>
            <ul>
                <li>Mantenha a ESP32-CAM na mesma posição durante toda a coleta</li>
                <li>Capture imagens em diferentes horários (manhã, tarde, noite)</li>
                <li>Capture pelo menos 100 imagens por classe</li>
                <li>Varie ligeiramente o ângulo (±5 graus) entre capturas</li>
            </ul>
        </div>
        
        <h2>🏷️ Selecionar Classe:</h2>
        <div class="class-buttons">
            <button class="class-btn btn-ligado" onclick="setClass('ligado_desligado')">🔴 Ligado/Desligado</button>
            <button class="class-btn btn-centrifugacao" onclick="setClass('centrifugacao')">🌀 Centrifugação</button>
            <button class="class-btn btn-enxague" onclick="setClass('enxague')">💧 Enxágue</button>
            <button class="class-btn btn-molho-curto" onclick="setClass('molho_curto')">⏱️ Molho Curto</button>
            <button class="class-btn btn-molho-normal" onclick="setClass('molho_normal')">⏰ Molho Normal</button>
            <button class="class-btn btn-molho-longo" onclick="setClass('molho_longo')">⏳ Molho Longo</button>
        </div>
        
        <div class="preview-section">
            <h2>📷 Preview e Captura:</h2>
            <button class="capture-btn" onclick="updatePreview()">🔄 Atualizar Preview</button>
            <button class="capture-btn" id="captureBtn" onclick="captureImage()" disabled>📸 CAPTURAR IMAGEM</button>
            <div id="previewContainer">
                <p>Clique em "Atualizar Preview" para ver a imagem atual da câmera</p>
            </div>
        </div>
    </div>

    <script>
        let currentClassSelected = '';
        
        function setClass(className) {
            currentClassSelected = className;
            
            // Remover classe ativa de todos os botões
            document.querySelectorAll('.class-btn').forEach(btn => {
                btn.classList.remove('active-class');
            });
            
            // Adicionar classe ativa ao botão clicado
            event.target.classList.add('active-class');
            
            // Atualizar display
            document.getElementById('currentClass').textContent = className.replace('_', ' ');
            
            // Habilitar botão de captura
            document.getElementById('captureBtn').disabled = false;
            
            // Enviar para ESP32
            fetch('/setclass?class=' + className)
                .then(response => response.text())
                .then(data => {
                    console.log('Classe definida:', className);
                    updateStats();
                })
                .catch(error => console.error('Erro:', error));
        }
        
        function updatePreview() {
            const container = document.getElementById('previewContainer');
            container.innerHTML = '<p>🔄 Carregando preview...</p>';
            
            const img = new Image();
            img.onload = function() {
                container.innerHTML = '';
                img.className = 'preview-img';
                container.appendChild(img);
            };
            img.onerror = function() {
                container.innerHTML = '<p style="color: red;">❌ Erro ao carregar preview</p>';
            };
            img.src = '/preview?t=' + new Date().getTime();
        }
        
        function captureImage() {
            if (!currentClassSelected) {
                alert('Selecione uma classe primeiro!');
                return;
            }
            
            const btn = document.getElementById('captureBtn');
            btn.disabled = true;
            btn.textContent = '📸 Capturando...';
            
            fetch('/capture')
                .then(response => response.text())
                .then(data => {
                    console.log('Imagem capturada:', data);
                    updateStats();
                    updatePreview(); // Atualizar preview após captura
                    
                    // Feedback visual
                    btn.style.background = '#27ae60';
                    btn.textContent = '✅ Capturada!';
                    
                    setTimeout(() => {
                        btn.disabled = false;
                        btn.textContent = '📸 CAPTURAR IMAGEM';
                        btn.style.background = '#27ae60';
                    }, 1000);
                })
                .catch(error => {
                    console.error('Erro:', error);
                    btn.disabled = false;
                    btn.textContent = '❌ Erro!';
                    btn.style.background = '#e74c3c';
                    
                    setTimeout(() => {
                        btn.textContent = '📸 CAPTURAR IMAGEM';
                        btn.style.background = '#27ae60';
                    }, 2000);
                });
        }
        
        function updateStats() {
            fetch('/stats')
                .then(response => response.json())
                .then(data => {
                    document.getElementById('classCount').textContent = data.classCount;
                    document.getElementById('totalCount').textContent = data.totalCount;
                })
                .catch(error => console.error('Erro ao atualizar stats:', error));
        }
        
        // Atualizar stats ao carregar a página
        updateStats();
        
        // Auto-refresh stats a cada 5 segundos
        setInterval(updateStats, 5000);
    </script>
</body>
</html>
  )";
  
  server.send(200, "text/html", html);
}

void handlePreview() {
  camera_fb_t * fb = esp_camera_fb_get();
  if (!fb) {
    server.send(500, "text/plain", "Erro ao capturar preview");
    return;
  }
  
  server.sendHeader("Content-Type", "image/jpeg");
  server.sendHeader("Cache-Control", "no-cache");
  server.send_P(200, "image/jpeg", (const char*)fb->buf, fb->len);
  
  esp_camera_fb_return(fb);
}

void handleCapture() {
  if (currentClass == "selecione_classe") {
    server.send(400, "text/plain", "Selecione uma classe primeiro");
    return;
  }
  
  camera_fb_t * fb = esp_camera_fb_get();
  if (!fb) {
    server.send(500, "text/plain", "Erro ao capturar imagem");
    return;
  }
  
  // Incrementar contadores
  imageCountTotal++;
  imageCountClass++;
  
  // Criar nome do arquivo
  String filename = currentClass + "_" + String(imageCountClass) + "_" + String(millis()) + ".jpg";
  
  // Log no serial
  Serial.println("📸 IMAGEM CAPTURADA:");
  Serial.println("   Classe: " + currentClass);
  Serial.println("   Arquivo: " + filename);
  Serial.println("   Tamanho: " + String(fb->len) + " bytes");
  Serial.println("   Total classe: " + String(imageCountClass));
  Serial.println("   Total geral: " + String(imageCountTotal));
  Serial.println();
  
  // Enviar imagem como download
  server.sendHeader("Content-Disposition", "attachment; filename=\"" + filename + "\"");
  server.sendHeader("Content-Type", "image/jpeg");
  server.send_P(200, "image/jpeg", (const char*)fb->buf, fb->len);
  
  esp_camera_fb_return(fb);
}

void handleSetClass() {
  if (server.hasArg("class")) {
    String newClass = server.arg("class");
    
    // Se mudou de classe, resetar contador da classe
    if (newClass != currentClass) {
      imageCountClass = 0;
      currentClass = newClass;
      
      Serial.println("🏷️ CLASSE ALTERADA PARA: " + currentClass);
      Serial.println("   Contador da classe resetado");
      Serial.println();
    }
    
    server.send(200, "text/plain", "OK");
  } else {
    server.send(400, "text/plain", "Parâmetro 'class' não encontrado");
  }
}

void handleStats() {
  String json = "{";
  json += "\"currentClass\":\"" + currentClass + "\",";
  json += "\"classCount\":" + String(imageCountClass) + ",";
  json += "\"totalCount\":" + String(imageCountTotal);
  json += "}";
  
  server.send(200, "application/json", json);
}