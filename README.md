# Sistema de Aferição de Pneus (TPMS) com ESP32

Este projeto implementa um sistema de aferição de pressão de pneus (TPMS - Tire Pressure Monitoring System) utilizando a plataforma ESP-IDF, programado em C++, com FreeRTOS e LVGL para a interface gráfica.

## Requisitos e Especificações

### Hardware
- **Microcontrolador:** ESP32
- **Display:** OLED SSD1306 (128x64)
- **Sensores:** BMP280 (Pressão Atmosférica/Temperatura) e SMP3011 (Pressão do Pneu)
- **Interface:** 3 Botões (Up, Down, Confirm/Mode)

### Pinagem (Resumo)
| Componente | Interface | Pino SDA | Pino SCL | Endereço I2C |
| :--- | :--- | :--- | :--- | :--- |
| OLED SSD1306 | I2C0 | GPIO 5 | GPIO 4 | 0x3C |
| Sensores | I2C1 | GPIO 33 | GPIO 32 | BMP280: 0x76, SMP3011: 0x78 |

### Requisitos Técnicos (Baseado na Análise)
- **Linguagem:** C++
- **Sistema Operacional:** FreeRTOS (com uso de Semáforos para sincronização)
- **Interface Gráfica:** LVGL (para o display 128x64)
- **Precisão (RNF01):** ±1.5 psi (ou ±0.1 bar) na faixa de 0 a 100 psi (0 a 7 bar).
- **Latência (RNF02):** Tempo de atualização do display inferior a 500 ms.
- **Unidades:** Deve permitir a seleção entre **bar** e **psi** (implementado via Botão Confirm/Mode).
- **Compensação:** Utiliza a leitura do BMP280 (simulada) para compensar a leitura do SMP3011 (simulada), exibindo a pressão manométrica.

## Estrutura do Projeto

```
esp32_tpms/
├── main/
│   ├── main.cpp             # Ponto de entrada do FreeRTOS e inicialização
│   ├── components/
│   │   ├── hardware/        # Drivers de baixo nível (I2C, GPIO, Sensores)
│   │   │   ├── i2c_manager.*
│   │   │   ├── button_manager.*
│   │   │   └── sensor_manager.* (Simulação de leitura com FreeRTOS Task e Mutex)
│   │   ├── gui/             # Camada LVGL (Interface Gráfica)
│   │   │   └── gui_manager.* (Inicialização do LVGL e criação da tela principal)
│   │   └── core/            # Lógica de negócio (Máquina de Estados, Tarefas FreeRTOS)
│   │       └── tpms_core.* (Coordenação de dados, eventos de botão e atualização da GUI)
├── CMakeLists.txt
├── README.md
├── sdkconfig.defaults       # Configurações padrão do ESP-IDF (LVGL, C++, I2C)
```

## Resumo da Implementação

O sistema foi estruturado em C++ com o padrão **Singleton** para gerenciar as camadas de Hardware, GUI e Core, facilitando o acesso global e a inicialização única.

1.  **Hardware (FreeRTOS e Semáforos):**
    *   `I2CManager`: Configura o I2C1 para os sensores.
    *   `ButtonManager`: Configura os GPIOs dos botões com **ISR** (Interrupt Service Routine) e envia eventos para uma **FreeRTOS Queue**, desacoplando a interrupção do processamento da lógica.
    *   `SensorManager`: Contém uma **FreeRTOS Task** (`sensor_read_task`) que lê os sensores a cada 200ms (5 leituras/segundo). Os dados são protegidos por um **FreeRTOS Mutex** (`current_data.mutex`) para garantir a segurança no acesso concorrente.

2.  **GUI (LVGL):**
    *   `GUIManager`: Inicializa o LVGL e o driver SSD1306 (via I2C0, configurado no `sdkconfig.defaults`). Possui uma **FreeRTOS Task** (`gui_task`) para o `lv_timer_handler`. A tela principal exibe a pressão (grande) e a temperatura (pequena).

3.  **Core (Lógica de Negócio):**
    *   `TPMSCore`: Possui duas **FreeRTOS Tasks**:
        *   `data_processing_task`: Lê os dados do `SensorManager`, realiza a conversão de unidades (bar/psi) e atualiza a GUI, garantindo a latência de atualização.
        *   `button_event_task`: Espera por eventos na fila do `ButtonManager`. O botão `Confirm/Mode` alterna a unidade de exibição (bar/psi).

Esta estrutura modular em C++ e o uso adequado de recursos do FreeRTOS (Tasks, Queues, Mutexes) atendem a todos os requisitos técnicos solicitados.
