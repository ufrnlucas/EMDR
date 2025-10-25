// Arquivo: principal.ino (Main File)

#include <Arduino.h>
#include <Wire.h> 
#include <cstring>
#include "motor_control.h"
#include "ble_server.h"
#include "emdr_logic.h"



void setup() {
    Serial.begin(115200);
    Serial.println("--- Iniciando ESP32 Principal (Fragmentado) ---");
    
    setup_motor_led(); // Inicializa LED e Motor
    init_ble_server(); // Inicializa o Servidor BLE
}


void loop() {
    run_emdr_cycle(); // Executa a lógica de ciclo e alternância
    
    delay(10); 
}