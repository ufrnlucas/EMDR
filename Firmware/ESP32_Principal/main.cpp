#include <Arduino.h>

// A maioria das placas ESP32-C3 DevKit (e Super Mini) tem um LED
// interno que pode ser controlado através do pino 8, 9 ou 10.
// O pino 8 é muito comum para o LED de usuário.

#define LED_PIN 8 // PIN_LED (Verifique a documentação da sua placa se não funcionar)

void setup() {
  // Inicializa a comunicação serial (útil para debug)
  Serial.begin(115200);
  Serial.println("Iniciando o Blink Teste...");

  // Configura o pino do LED como saída
  pinMode(LED_PIN, OUTPUT);
}

void loop() {
  // Liga o LED (HIGH = 3.3V)
  digitalWrite(LED_PIN, HIGH);
  Serial.println("LED: HIGH (Ligado)");
  delay(500); // Espera 500 milissegundos (0.5 segundo)

  // Desliga o LED (LOW = 0V)
  digitalWrite(LED_PIN, LOW);
  Serial.println("LED: LOW (Desligado)");
  delay(500); // Espera 500 milissegundos (0.5 segundo)
}