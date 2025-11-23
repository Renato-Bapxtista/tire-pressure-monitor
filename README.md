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
Tire-pressure-monitor/
│   ├── components/
│   │   ├── bmp280_driver/
|   |   |    ├── include/
|   |   |    ├── src/
|   |   ├── button_drive/
|   |   |    ├── include/
|   |   |    ├── src/
|   |   ├── i2c_maneger/
|   |   |    ├── include/
|   |   |    ├── src/
|   |   ├── oled_display/
|   |   |    ├── include/
|   |   |    ├── src/
|   |   ├──smp3011_driver/
|   |   |    ├── include/
|   |   |    ├── src/
|   |   ├── sistem_controller/
|   |   |    ├── include/
|   |   |    ├── src/
|   |   ├── task_maneger/
|   |   |    ├── include/
|   |   |    ├── src/
├── main/
|   ├── include/
│   ├── main.cpp             # Ponto de entrada do FreeRTOS e inicialização
|   
├── CMakeLists.txt
├── README.md
├── sdkconfig.defaults       # Configurações padrão do ESP-IDF (LVGL, C++, I2C)
```

## Resumo da Implementação

A arquitetura do sistema foi simplificada e centralizada em um **Controlador de Sistema** (`SystemController`), que orquestra as operações de hardware e a lógica de negócio.

1.  **Inicialização Centralizada:**
    *   O `main.cpp` inicializa dois barramentos I2C (`i2c0_bus` e `i2c1_bus`) e todas as instâncias de hardware (Display, Sensores, Botões) como objetos globais.
    *   O `SystemController` é instanciado e recebe referências para todos os drivers de hardware, estabelecendo a comunicação entre as camadas.

2.  **Loop de Execução Único (Single-Task):**
    *   Ao contrário de uma arquitetura multi-task distribuída, o sistema opera em um **loop principal de alta frequência (20Hz)** dentro da task principal do FreeRTOS (`app_main`).
    *   A cada iteração, o loop chama o método `system_controller.process_events()`.

3.  **Orquestração pelo `SystemController`:**
    *   O `SystemController` é responsável por:
        *   **Leitura de Sensores:** Acionar a leitura dos sensores BMP280 e SMP3011.
        *   **Processamento de Dados:** Realizar a compensação de pressão (usando o BMP280) e a conversão de unidades (bar/psi).
        *   **Gerenciamento de Eventos:** Verificar o estado dos botões e aplicar as mudanças de modo ou unidade.
        *   **Atualização da GUI:** Enviar os dados processados para o `OLEDDisplay` para atualização da interface gráfica via LVGL.

Esta nova estrutura, baseada em um loop de controle centralizado, simplifica a sincronização e o gerenciamento de recursos, mantendo a modularidade através dos drivers em C++.
