#ifndef WEB_SERVER_H
#define WEB_SERVER_H

#include "WebServer.h"
#include "config.h"

extern WebServer server;
extern String currentWashingStage;
extern float lastConfidence;

void setupWebServer();
void handleWebServerRequests();
void handleRoot();
void handleStatus();
void handlePredict();
void handleNotFound();

void setupWebServer() {
    server.on("/", handleRoot);
    server.on("/status", handleStatus);
    server.on("/predict", handlePredict);
    server.onNotFound(handleNotFound);
    
    server.begin();
    Serial.println("Servidor web iniciado na porta " + String(WEB_SERVER_PORT));
}

void handleWebServerRequests() {
    server.handleClient();
}

void handleRoot() {
    String html = "<!DOCTYPE html>";
    html += "<html lang='pt-BR'>";
    html += "<head>";
    html += "<meta charset='UTF-8'>";
    html += "<meta name='viewport' content='width=device-width, initial-scale=1.0'>";
    html += "<title>Lavadora Inteligente - Monitor IoT</title>";
    html += "<style>";
    html += "* { box-sizing: border-box; margin: 0; padding: 0; }";
    html += "body { font-family: 'Segoe UI', Tahoma, Geneva, Verdana, sans-serif; background: linear-gradient(135deg, #667eea 0%, #764ba2 100%); color: #333; min-height: 100vh; padding: 20px; }";
    html += ".container { max-width: 800px; margin: 0 auto; background: rgba(255, 255, 255, 0.95); border-radius: 20px; box-shadow: 0 20px 40px rgba(0, 0, 0, 0.1); overflow: hidden; }";
    html += ".header { background: linear-gradient(135deg, #2c3e50, #34495e); color: white; padding: 30px; text-align: center; }";
    html += ".header h1 { font-size: 2.5em; margin-bottom: 10px; font-weight: 300; }";
    html += ".header p { opacity: 0.9; font-size: 1.1em; }";
    html += ".content { padding: 40px; }";
    html += ".status-card { background: #f8f9fa; border-radius: 15px; padding: 30px; margin-bottom: 30px; border-left: 5px solid #3498db; }";
    html += ".status-label { font-size: 1.2em; color: #7f8c8d; margin-bottom: 15px; text-transform: uppercase; letter-spacing: 1px; font-weight: 600; }";
    html += ".current-stage { font-size: 3em; font-weight: bold; color: #2c3e50; margin-bottom: 15px; text-transform: capitalize; }";
    html += ".confidence { font-size: 1.3em; color: #27ae60; font-weight: 600; }";
    html += ".info-grid { display: grid; grid-template-columns: repeat(auto-fit, minmax(200px, 1fr)); gap: 20px; margin-bottom: 30px; }";
    html += ".info-item { background: white; padding: 20px; border-radius: 10px; text-align: center; box-shadow: 0 5px 15px rgba(0, 0, 0, 0.08); border: 1px solid #ecf0f1; }";
    html += ".info-item h3 { color: #34495e; margin-bottom: 10px; font-size: 1.1em; }";
    html += ".info-item .value { font-size: 1.5em; font-weight: bold; color: #3498db; }";
    html += ".actions { display: flex; gap: 15px; flex-wrap: wrap; justify-content: center; }";
    html += ".btn { background: linear-gradient(135deg, #3498db, #2980b9); color: white; border: none; padding: 15px 30px; border-radius: 25px; font-size: 1.1em; font-weight: 600; cursor: pointer; transition: all 0.3s ease; text-decoration: none; display: inline-block; min-width: 150px; }";
    html += ".btn:hover { transform: translateY(-2px); box-shadow: 0 10px 25px rgba(52, 152, 219, 0.3); }";
    html += ".btn.secondary { background: linear-gradient(135deg, #95a5a6, #7f8c8d); }";
    html += ".footer { background: #ecf0f1; padding: 20px; text-align: center; color: #7f8c8d; font-size: 0.9em; }";
    html += ".status-indicator { display: inline-block; width: 12px; height: 12px; border-radius: 50%; background: #27ae60; margin-right: 8px; animation: pulse 2s infinite; }";
    html += ".status-indicator.offline { background: #e74c3c; }";
    html += "@keyframes pulse { 0% { transform: scale(1); opacity: 1; } 50% { transform: scale(1.1); opacity: 0.7; } 100% { transform: scale(1); opacity: 1; } }";
    html += "@media (max-width: 600px) { .container { margin: 10px; border-radius: 15px; } .header { padding: 20px; } .header h1 { font-size: 2em; } .content { padding: 20px; } .current-stage { font-size: 2em; } .actions { flex-direction: column; } .btn { width: 100%; } }";
    html += "</style>";
    html += "</head>";
    html += "<body>";
    html += "<div class='container'>";
    html += "<div class='header'>";
    html += "<h1>🏠 Lavadora Inteligente</h1>";
    html += "<p>Monitoramento IoT em tempo real</p>";
    html += "</div>";
    html += "<div class='content'>";
    html += "<div class='status-card'>";
    html += "<div class='status-label'><span class='status-indicator' id='statusIndicator'></span>Status Atual</div>";
    html += "<div class='current-stage' id='currentStage'>Carregando...</div>";
    html += "<div class='confidence' id='confidence'>Confianca: --</div>";
    html += "</div>";
    html += "<div class='info-grid'>";
    html += "<div class='info-item'><h3>⏱️ Tempo Online</h3><div class='value' id='uptime'>--</div></div>";
    html += "<div class='info-item'><h3>🔄 Última Atualização</h3><div class='value' id='lastUpdate'>--</div></div>";
    html += "<div class='info-item'><h3>📶 Conexão</h3><div class='value' id='connection'>WiFi</div></div>";
    html += "<div class='info-item'><h3>🧠 Modo</h3><div class='value' id='mode'>Demonstração</div></div>";
    html += "</div>";
    html += "<div class='actions'>";
    html += "<button class='btn' onclick='updateStatus()'>🔄 Atualizar</button>";
    html += "<button class='btn secondary' onclick='forcePrediction()'>⚡ Simular Mudança</button>";
    html += "</div>";
    html += "</div>";
    html += "<div class='footer'>";
    html += "<p>🚀 Powered by ESP32-CAM | Sistema 100% Estável | Desenvolvido com ❤️</p>";
    html += "</div>";
    html += "</div>";
    html += "<script>";
    html += "let isUpdating = false;";
    html += "function updateStatus() {";
    html += "if (isUpdating) return;";
    html += "isUpdating = true;";
    html += "const indicator = document.getElementById('statusIndicator');";
    html += "indicator.style.background = '#f39c12';";
    html += "fetch('/status')";
    html += ".then(response => response.json())";
    html += ".then(data => {";
    html += "document.getElementById('currentStage').textContent = data.stage.charAt(0).toUpperCase() + data.stage.slice(1);";
    html += "document.getElementById('confidence').textContent = 'Confiança: ' + Math.round(data.confidence * 100) + '%';";
    html += "const uptimeSeconds = data.uptime;";
    html += "const hours = Math.floor(uptimeSeconds / 3600);";
    html += "const minutes = Math.floor((uptimeSeconds % 3600) / 60);";
    html += "document.getElementById('uptime').textContent = hours + 'h ' + minutes + 'm';";
    html += "document.getElementById('lastUpdate').textContent = new Date().toLocaleTimeString();";
    html += "indicator.style.background = '#27ae60';";
    html += "indicator.classList.remove('offline');";
    html += "})";
    html += ".catch(error => {";
    html += "console.error('Erro:', error);";
    html += "document.getElementById('currentStage').textContent = 'Erro de Conexão';";
    html += "indicator.style.background = '#e74c3c';";
    html += "indicator.classList.add('offline');";
    html += "})";
    html += ".finally(() => { isUpdating = false; });";
    html += "}";
    html += "function forcePrediction() {";
    html += "const btn = event.target;";
    html += "const originalText = btn.textContent;";
    html += "btn.textContent = '⏳ Simulando...';";
    html += "btn.disabled = true;";
    html += "fetch('/predict')";
    html += ".then(response => response.json())";
    html += ".then(data => { ";
    html += "setTimeout(() => {";
    html += "updateStatus();";
    html += "btn.textContent = originalText;";
    html += "btn.disabled = false;";
    html += "}, 1000);";
    html += "})";
    html += ".catch(error => {";
    html += "console.error('Erro na simulação:', error);";
    html += "btn.textContent = originalText;";
    html += "btn.disabled = false;";
    html += "});";
    html += "}";
    html += "updateStatus();";
    html += "setInterval(updateStatus, 5000);";
    html += "document.addEventListener('DOMContentLoaded', function() {";
    html += "console.log('🏠 Lavadora Inteligente - Sistema Carregado!');";
    html += "console.log('📊 Atualizações automáticas a cada 5 segundos');";
    html += "});";
    html += "</script>";
    html += "</body>";
    html += "</html>";
    
    server.send(200, "text/html", html);
}

void handleStatus() {
    extern unsigned long getSystemUptime();
    String json = "{";
    json += "\"stage\":\"" + currentWashingStage + "\",";
    json += "\"confidence\":" + String(lastConfidence) + ",";
    json += "\"timestamp\":" + String(millis()) + ",";
    json += "\"uptime\":" + String(getSystemUptime()) + ",";
    json += "\"heap\":" + String(ESP.getFreeHeap()) + ",";
    json += "\"wifi_rssi\":" + String(WiFi.RSSI()) + ",";
    json += "\"mode\":\"demonstration\"";
    json += "}";
    
    server.send(200, "application/json", json);
}

void handlePredict() {
    // Simular predição forçada
    extern void forcePrediction();
    forcePrediction();
    
    String response = "{";
    response += "\"message\":\"Simulacao de predicao executada\",";
    response += "\"stage\":\"" + currentWashingStage + "\",";
    response += "\"confidence\":" + String(lastConfidence) + ",";
    response += "\"mode\":\"demonstration\"";
    response += "}";
    
    server.send(200, "application/json", response);
}

void handleNotFound() {
    String message = "Pagina nao encontrada\\n\\n";
    message += "URI: " + server.uri() + "\\n";
    message += "Metodo: ";
    message += (server.method() == HTTP_GET) ? "GET" : "POST";
    message += "\\n";
    message += "Argumentos: " + String(server.args()) + "\\n";
    
    for (uint8_t i = 0; i < server.args(); i++) {
        message += " " + server.argName(i) + ": " + server.arg(i) + "\\n";
    }
    
    server.send(404, "text/plain", message);
}

#endif // WEB_SERVER_H