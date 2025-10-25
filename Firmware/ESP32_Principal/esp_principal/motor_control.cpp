// Arquivo: motor_control.cpp

#include "motor_control.h"

// Inicializa pinos e drivers
void setup_motor_led() {
    pinMode(LED_PIN, OUTPUT);
    digitalWrite(LED_PIN, LOW);
    Wire.begin(); // Garante que o I2C está ativo
    setupDRV(drv_m2, DRV_ADDR_M2);
}

void setupDRV(Adafruit_DRV2605& drv, uint8_t addr) {
    if (!drv.begin()) { 
        Serial.printf("ERRO: DRV2605L não encontrado em 0x%X\n", addr);
        return;
    }
    drv.selectLibrary(1); 
    drv.setMode(DRV2605_MODE_REALTIME);
    drv.setRealtimeValue(0);
}

void setMotorIntensity(Adafruit_DRV2605& drv, uint8_t intensity) {
    uint8_t duty = map(intensity, 0, 100, 0, 255); 
    drv.setRealtimeValue(duty);
}