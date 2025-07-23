#ifndef WEB_SERVER_H
#define WEB_SERVER_H

#include "WebServer.h"
#include "config.h"

// =================== VARIÁVEIS GLOBAIS ===================
extern WebServer server;
extern String currentWashingStage;
extern float lastConfidence;

// =================== FUNÇÕES PÚBLICAS ===================
void setupWebServer();
void handleWebServerRequests();

// =================== HANDLERS ===================
void handleRoot();
void handleStatus();
void handlePredict();
void handleNotFound();

// =================== IMPLEMENTAÇÃO ===================

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
    String html = generateMainHTML();
    server.send(200, "text/html", html);
}

void handleStatus() {
    String json = "{";
    json += "\"stage\":\"" + currentWashingStage + "\",";
    json += "\"confidence\":" + String(lastConfidence) + ",";
    json += "\"timestamp\":" + String(millis()) + ",";
    json += "\"uptime\":" + String(millis() / 1000);
    json += "}";
    
    server.send(200, "application/json", json);
}

void handlePredict() {
    // Força uma nova predição
    extern void forcePrediction();
    forcePrediction();
    
    String response = "{";
    response += "\"message\":\"Predicao executada\",";
    response += "\"stage\":\"" + currentWashingStage + "\",";
    response += "\"confidence\":" + String(lastConfidence);
    response += "}";
    
    server.send(200, "application/json", response);
}

void handleNotFound() {
    String message = "Pagina nao encontrada\n\n";
    message += "URI: " + server.uri() + "\n";
    message += "Metodo: " + (server.method() == HTTP_GET ? "GET" : "POST") + "\n";
    message += "Argumentos: " + String(server.args()) + "\n";
    
    for (uint8_t i = 0; i < server.args(); i++) {
        message += " " + server.argName(i) + ": " + server.arg(i) + "\n";
    }
    
    server.send(404, "text/plain", message);
}

String generateMainHTML() {
    String html = R"(
<!DOCTYPE html>
<html lang="pt-BR">
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>Lavadora Inteligente - Monitor IoT</title>
    <style>
        * {
            box-sizing: border-box;
            margin: 0;
            padding: 0;
        }
        
        body {
            font-family: 'Segoe UI', Tahoma, Geneva, Verdana, sans-serif;
            background: linear-gradient(135deg, #667eea 0%, #764ba2 100%);
            color: #333;
            min-height: 100vh;
            padding: 20px;
        }
        
        .container {
            max-width: 800px;
            margin: 0 auto;
            background: rgba(255, 255, 255, 0.95);
            border-radius: 20px;
            box-shadow: 0 20px 40px rgba(0, 0, 0, 0.1);
            overflow: hidden;
        }
        
        .header {
            background: linear-gradient(135deg, #2c3e50, #34495e);
            color: white;
            padding: 30px;
            text-align: center;
        }
        
        .header h1 {
            font-size: 2.5em;
            margin-bottom: 10px;
            font-weight: 300;
        }
        
        .header p {
            opacity: 0.9;
            font-size: 1.1em;
        }
        
        .content {
            padding: 40px;
        }
        
        .status-card {
            background: #f8f9fa;
            border-radius: 15px;
            padding: 30px;
            margin-bottom: 30px;
            border-left: 5px solid #3498db;
            position: relative;
            overflow: hidden;
        }
        
        .status-card::before {
            content: '';
            position: absolute;
            top: 0;
            right: 0;
            width: 100px;
            height: 100px;
            background: linear-gradient(45deg, rgba(52, 152, 219, 0.1), transparent);
            border-radius: 50%;
            transform: translate(25%, -25%);
        }
        
        .status-label {
            font-size: 1.2em;
            color: #7f8c8d;
            margin-bottom: 15px;
            text-transform: uppercase;
            letter-spacing: 1px;
            font-weight: 600;
        }
        
        .current-stage {
            font-size: 3em;
            font-weight: bold;
            color: #2c3e50;
            margin-bottom: 15px;
            text-transform: capitalize;
            position: relative;
            z-index: 1;
        }
        
        .confidence {
            font-size: 1.3em;
            color: #27ae60;
            font-weight: 600;
        }
        
        .info-grid {
            display: grid;
            grid-template-columns: repeat(auto-fit, minmax(200px, 1fr));
            gap: 20px;
            margin-bottom: 30px;
        }
        
        .info-item {
            background: white;
            padding: 20px;
            border-radius: 10px;
            text-align: center;
            box-shadow: 0 5px 15px rgba(0, 0, 0, 0.08);
            border: 1px solid #ecf0f1;
        }
        
        .info-item h3 {
            color: #34495e;
            margin-bottom: 10px;
            font-size: 1.1em;
        }
        
        .info-item .value {
            font-size: 1.5em;
            font-weight: bold;
            color: #3498db;
        }
        
        .actions {
            display: flex;
            gap: 15px;
            flex-wrap: wrap;
            justify-content: center;
        }
        
        .btn {
            background: linear-gradient(135deg, #3498db, #2980b9);
            color: white;
            border: none;
            padding: 15px 30px;
            border-radius: 25px;
            font-size: 1.1em;
            font-weight: 600;
            cursor: pointer;
            transition: all 0.3s ease;
            text-decoration: none;
            display: inline-block;
            min-width: 150px;
        }
        
        .btn:hover {
            transform: translateY(-2px);
            box-shadow: 0 10px 25px rgba(52, 152, 219, 0.3);
        }
        
        .btn:active {
            transform: translateY(0);
        }
        
        .btn.secondary {
            background: linear-gradient(135deg, #95a5a6, #7f8c8d);
        }
        
        .btn.secondary:hover {
            box-shadow: 0 10px 25px rgba(149, 165, 166, 0.3);
        }
        
        .status-indicator {
            display: inline-block;
            width: 12px;
            height: 12px;
            border-radius: 50%;
            margin-right: 8px;
            background: #27ae60;
            animation: pulse 2s infinite;
        }
        
        @keyframes pulse {
            0% { opacity: 1; }
            50% { opacity: 0.5; }
            100% { opacity: 1; }
        }
        
        .footer {
            background: #ecf0f1;
            padding: 20px;
            text-align: center;
            color: #7f8c8d;
            font-size: 0.9em;
        }
        
        @media (max-width: 600px) {
            .container {
                margin: 10px;
                border-radius: 15px;
            }
            
            .header {
                padding: 20px;
            }
            
            .header h1 {
                font-size: 2em;
            }
            
            .content {
                padding: 20px;
            }
            
            .current-stage {
                font-size: 2em;
            }
            
            .actions {
                flex-direction: column;
            }
            
            .btn {
                width: 100%;
            }
        }
        
        .loading {
            opacity: 0.6;
            pointer-events: none;
        }
        
        .error {
            color: #e74c3c;
            background: #fff5f5;
            border-left-color: #e74c3c;
        }
    </style>
</head>
<body>
    <div class="container">
        <div class="header">
            <h1>Lavadora Inteligente</h1>
            <p>Monitoramento em tempo real com TinyML</p>
        </div>
        
        <div class="content">
            <div class="status-card" id="statusCard">
                <div class="status-label">
                    <span class="status-indicator"></span>
                    Status Atual
                </div>
                <div class="current-stage" id="currentStage">Carregando...</div>
                <div class="confidence" id="confidence">Confianca: --</div>
            </div>
            
            <div class="info-grid">
                <div class="info-item">
                    <h3>Tempo Online</h3>
                    <div class="value" id="uptime">--</div>
                </div>
                <div class="info-item">
                    <h3>Ultima Atualizacao</h3>
                    <div class="value" id="lastUpdate">--</div>
                </div>
                <div class="info-item">
                    <h3>Conexao</h3>
                    <div class="value" id="connection">WiFi</div>
                </div>
            </div>
            
            <div class="actions">
                <button class="btn" onclick="updateStatus()">Atualizar</button>
                <button class="btn secondary" onclick="forcePrediction()">Forcar Predicao</button>
            </div>
        </div>
        
        <div class="footer">
            <p>Powered by ESP32-CAM + TinyML | Desenvolvido com amor</p>
        </div>
    </div>

    <script>
        let isUpdating = false;
        
        function updateStatus() {
            if (isUpdating) return;
            
            isUpdating = true;
            const statusCard = document.getElementById('statusCard');
            statusCard.classList.add('loading');
            
            fetch('/status')
                .then(response => response.json())
                .then(data => {
                    document.getElementById('currentStage').textContent = 
                        data.stage.charAt(0).toUpperCase() + data.stage.slice(1);
                    document.getElementById('confidence').textContent = 
                        'Confianca: ' + Math.round(data.confidence * 100) + '%';
                    
                    // Atualizar tempo online
                    const uptimeSeconds = data.uptime;
                    const hours = Math.floor(uptimeSeconds / 3600);
                    const minutes = Math.floor((uptimeSeconds % 3600) / 60);
                    document.getElementById('uptime').textContent = 
                        hours + 'h ' + minutes + 'm';
                    
                    // Atualizar última atualização
                    document.getElementById('lastUpdate').textContent = 
                        new Date().toLocaleTimeString();
                    
                    // Remover classe de erro se existir
                    statusCard.classList.remove('error');
                })
                .catch(error => {
                    console.error('Erro:', error);
                    document.getElementById('currentStage').textContent = 'Erro de Conexao';
                    document.getElementById('confidence').textContent = 'Verificar rede';
                    statusCard.classList.add('error');
                })
                .finally(() => {
                    isUpdating = false;
                    statusCard.classList.remove('loading');
                });
        }
        
        function forcePrediction() {
            fetch('/predict')
                .then(response => response.json())
                .then(data => {
                    console.log('Predicao forcada:', data);
                    setTimeout(updateStatus, 1000); // Atualizar após 1 segundo
                })
                .catch(error => {
                    console.error('Erro na predicao:', error);
                });
        }
        
        // Atualização automática
        updateStatus();
        setInterval(updateStatus, 5000);
        
        // Adicionar indicador visual de conectividade
        window.addEventListener('online', () => {
            document.getElementById('connection').textContent = 'Online';
            document.getElementById('connection').style.color = '#27ae60';
        });
        
        window.addEventListener('offline', () => {
            document.getElementById('connection').textContent = 'Offline';
            document.getElementById('connection').style.color = '#e74c3c';
        });
    </script>
</body>
</html>
    )";
    
    return html;
}

#endif // WEB_SERVER_H