#define LED 2

void setup() {
  // Set pin mode
  pinMode(LED,OUTPUT);
}

void loop() {
  delay(50);
// you can set the delay time by adjusting the parameter of delay();
  digitalWrite(LED,HIGH);
  delay(50);
  digitalWrite(LED,LOW);
}