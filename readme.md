# Monitor Inteligente de Lavadora com ESP32-CAM

[![License: MIT](https://img.shields.io/badge/License-MIT-yellow.svg)](https://opensource.org/licenses/MIT)
[![Platform](https://img.shields.io/badge/Platform-ESP32-blue.svg)](https://espressif.com/)
[![Arduino IDE](https://img.shields.io/badge/Arduino%20IDE-2.3.6-green.svg)](https://www.arduino.cc/)
[![TensorFlow Lite](https://img.shields.io/badge/ML-TensorFlow%20Lite-orange.svg)](https://www.tensorflow.org/lite)
[![Alexa Compatible](https://img.shields.io/badge/Voice-Alexa%20Compatible-00CAFF.svg)](https://developer.amazon.com/alexa)
[![Build Status](https://img.shields.io/badge/Build-Passing-brightgreen.svg)]()

Transforma qualquer lavadora comum em um dispositivo IoT inteligente usando visão computacional e aprendizado de máquina embarcado. O sistema detecta as etapas do ciclo de lavagem analisando padrões de LEDs no painel de controle usando ESP32-CAM com inferência TinyML local.

## Descrição

Este projeto converte uma lavadora tradicional em um dispositivo IoT inteligente sem qualquer modificação de hardware no aparelho. Utiliza um ESP32-CAM posicionado para monitorar o painel de LEDs da máquina, executando um modelo de classificação TinyML localmente para identificar diferentes etapas de lavagem (centrifugação, enxágue, ciclos de molho, etc.).

O sistema integra com Amazon Alexa através do SinricPro, permitindo consultas por voz como "Alexa, qual a etapa da lavadora?" Todo o pipeline de classificação roda offline no ESP32, necessitando conectividade à internet apenas para integração com Alexa.

## Técnicas Web Interessantes

O projeto implementa várias técnicas web modernas para coleta de dados robusta e interação em tempo real:

- **[CSS Grid Layout](https://developer.mozilla.org/pt-BR/docs/Web/CSS/CSS_Grid_Layout)** com `repeat(auto-fit, minmax())` para layouts de botões responsivos que se adaptam automaticamente ao tamanho da viewport
- **[Fetch API](https://developer.mozilla.org/pt-BR/docs/Web/API/Fetch_API)** para comunicação assíncrona ESP32↔Navegador sem recarregamento de página
- **[Blob URLs](https://developer.mozilla.org/en-US/docs/Web/API/URL/createObjectURL)** para preview de câmera em tempo real diretamente do stream do ESP32-CAM
- **[Headers Content-Disposition](https://developer.mozilla.org/pt-BR/docs/Web/HTTP/Headers/Content-Disposition)** para downloads automáticos de imagens com nomes únicos durante coleta de dados
- **[CSS Transform](https://developer.mozilla.org/pt-BR/docs/Web/CSS/transform)** e **[Transition](https://developer.mozilla.org/pt-BR/docs/Web/CSS/transition)** para feedback visual instantâneo em interações do usuário
- **[Headers Cache-Control](https://developer.mozilla.org/pt-BR/docs/Web/HTTP/Headers/Cache-Control)** para prevenção de cache em streams de imagem em tempo real

## Tecnologias Não-Óbvias

O stack técnico combina tecnologias embarcadas e web de forma interessante:

### Aprendizado de Máquina Embarcado
- **[TensorFlow Lite para Microcontroladores](https://www.tensorflow.org/lite/microcontrollers)** - executa modelos de classificação diretamente no hardware ESP32
- **[Edge Impulse](https://edgeimpulse.com/)** - plataforma para treinamento de modelos TinyML otimizados para microcontroladores
- **MobileNetV2 0.35** - arquitetura de rede neural leve otimizada para dispositivos edge

### Drivers de Hardware
- **[Biblioteca ESP32-Camera](https://github.com/espressif/esp32-camera)** - driver oficial da Espressif para sensor de câmera OV2640
- **[ESP32-CAM-MB](https://github.com/Freenove/Freenove_ESP32_WROVER_Board)** - módulo programador USB com regulação automática de voltagem

### Integração IoT
- **[SDK SinricPro](https://github.com/sinricpro/esp8266-esp32-sdk)** - ponte ESP32↔Alexa sem necessidade de AWS Lambda ou servidores customizados
- **[ArduinoJson](https://github.com/bblanchon/ArduinoJson)** - parsing JSON otimizado para ambientes com limitação de memória

### Visão Computacional
- **[OpenMV](https://openmv.io/)** - algoritmos de processamento de imagem adaptados para ambientes de microcontroladores
- **Sensor de Câmera OV2640** - câmera 2MP com compressão JPEG e configurações de qualidade ajustáveis

## Project Structure

```
projeto_lavadora/
├── arduino_code/
├── data_collection/
│   ├── ligado_desligado/
│   ├── centrifugacao/
│   ├── enxague/
│   ├── molho_curto/
│   ├── molho_normal/
│   └── molho_longo/
├── model_training/
│   ├── dataset/
│   │   ├── train/
│   │   └── test/
│   └── models/
├── web_interface/
└── docs/
```

**[`data_collection/`](./data_collection/)** - Dataset organizado por classe de operação da lavadora. Cada subdiretório contém imagens rotuladas capturadas sob diferentes condições de iluminação e ângulos de câmera para treinamento robusto do modelo.

**[`model_training/`](./model_training/)** - Pipeline completo de treinamento TinyML incluindo scripts de pré-processamento, aumento de dados e utilitários de conversão para TensorFlow Lite.

**[`arduino_code/`](./arduino_code/)** - Firmware ESP32 com servidor web embarcado, motor de inferência TinyML local e integração SinricPro para conectividade Alexa.

**[`web_interface/`](./web_interface/)** - Interface de coleta de dados com preview de câmera em tempo real, download automático de imagens e rastreamento de estatísticas de classificação.

O sistema elimina dependências de computação em nuvem executando toda classificação localmente no ESP32, comunicando com serviços externos apenas para reportar atualizações de status para Alexa via SinricPro.

## Principais Funcionalidades

- **Processamento Local**: Toda classificação de imagem roda no dispositivo sem dependência de internet
- **Integração por Voz**: Consultas em linguagem natural através da Amazon Alexa
- **Não-Invasivo**: Não requer modificações na lavadora existente
- **Monitoramento em Tempo Real**: Detecção contínua de status com intervalos de polling configuráveis
- **Interface Web**: Coleta de dados e monitoramento do sistema baseado em navegador
- **Dataset Robusto**: Suporta múltiplas condições de iluminação e ângulos de visualização

## Requisitos de Hardware

- **Módulo ESP32-CAM AI-Thinker** com câmera OV2640
- **Placa programadora ESP32-CAM-MB** USB
- **Fonte de Alimentação 5V** (capacidade mínima de 1A)
- **Rede WiFi** (banda 2.4GHz)

## Dependências de Software

- **Arduino IDE 2.3.6** ou superior
- **Pacote de Placa ESP32** v2.0.11+
- **TensorFlowLite_ESP32** v0.9.0+
- **SinricPro** v2.10.1+
- **ArduinoJson** v6.21.3+

## Especificações Técnicas

- **Resolução de Imagem**: 640x480 (VGA) para equilíbrio ótimo qualidade/performance
- **Tamanho do Modelo**: <100KB (cabe na memória flash do ESP32)
- **Tempo de Inferência**: <500ms por classificação
- **Consumo de Energia**: ~250mA @ 5V durante operação
- **Alcance WiFi**: Padrão 802.11b/g/n (2.4GHz)
- **Classes de Classificação**: 6 estados da lavadora

## Licença

Este projeto está licenciado sob a Licença MIT - veja o arquivo [LICENSE](LICENSE) para detalhes.

## Contribuindo

Contribuições são bem-vindas! Sinta-se livre para enviar um Pull Request. Para mudanças importantes, abra uma issue primeiro para discutir o que gostaria de alterar.

## Agradecimentos

- **Espressif Systems** pelo hardware ESP32-CAM e drivers
- **Equipe TensorFlow** pelo suporte TensorFlow Lite para microcontroladores
- **SinricPro** pela integração simplificada com Alexa
- **Edge Impulse** pela plataforma acessível de treinamento TinyML