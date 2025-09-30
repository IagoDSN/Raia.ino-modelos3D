#include <Arduino.h>
#include <Adafruit_NeoPixel.h>

/*
 Conexões:
DI led porta digital 7

conexões motor Esquerdo
L1 porta digital 8
L2 porta digital 9
L3 porta digital 10
L4 porta digital 11

conexões motor Direito
R1 porta digital 2
R2 porta digital 3
R3 porta digital 4
R4 porta digital 5
 */

// ====== NeoPixel (MANTIDO no D7) ======
#define DATA_PIN    7
#define NUM_PIXELS  24
Adafruit_NeoPixel strip(NUM_PIXELS, DATA_PIN, NEO_GRB + NEO_KHZ800);

void set_all(uint8_t r, uint8_t g, uint8_t b) {
  for (uint16_t i = 0; i < strip.numPixels(); i++) {
    strip.setPixelColor(i, strip.Color(r, g, b));
  }
  strip.show();
}

// ====== Steppers 28BYJ-48 (ULN2003) ======
// Esquerdo (IN1..IN4)
const int L1 = 8;
const int L2 = 9;
const int L3 = 10;
const int L4 = 11;

// Direito (realocado para não conflitar com D7)
const int R1 = 2;
const int R2 = 3;
const int R3 = 4;
const int R4 = 5;

// Half-step sequence
const uint8_t SEQ[8][4] = {
  {1,0,0,1},
  {1,0,0,0},
  {1,1,0,0},
  {0,1,0,0},
  {0,1,1,0},
  {0,0,1,0},
  {0,0,1,1},
  {0,0,0,1}
};

long lpos = 0, rpos = 0;
int  lstep = 0, rstep = 0;
unsigned int stepDelay = 2; // ms (ajuste velocidade)

void applyPhase(int a, int b, int c, int d, bool isLeft) {
  if (isLeft) {
    digitalWrite(L1, a); digitalWrite(L2, b);
    digitalWrite(L3, c); digitalWrite(L4, d);
  } else {
    digitalWrite(R1, a); digitalWrite(R2, b);
    digitalWrite(R3, c); digitalWrite(R4, d);
  }
}

void doStep(int dir, bool isLeft) {
  if (isLeft) {
    lstep = (lstep + (dir > 0 ? 1 : -1) + 8) % 8;
    applyPhase(SEQ[lstep][0], SEQ[lstep][1], SEQ[lstep][2], SEQ[lstep][3], true);
    lpos += (dir > 0 ? 1 : -1);
  } else {
    rstep = (rstep + (dir > 0 ? 1 : -1) + 8) % 8;
    applyPhase(SEQ[rstep][0], SEQ[rstep][1], SEQ[rstep][2], SEQ[rstep][3], false);
    rpos += (dir > 0 ? 1 : -1);
  }
  delay(stepDelay);
}

void moveRel(long steps, bool isLeft) {
  int dir = (steps >= 0) ? +1 : -1;
  for (long i = 0; i < labs(steps); i++) doStep(dir, isLeft);
}

void moveAbs(long target, bool isLeft) {
  long &pos = isLeft ? lpos : rpos;
  moveRel(target - pos, isLeft);
}

void gestureHappy() {
  long up = -350;  // se sentido estiver invertido, troque para +350
  moveRel(up, true);
  moveRel(up, false);
  for (int k=0;k<2;k++) {
    moveRel(+60, true);  moveRel(-60, true);
    moveRel(-60, false); moveRel(+60, false);
  }
  moveAbs(0, true);
  moveAbs(0, false);
}

String readLine() {
  String s = "";
  unsigned long start = millis();
  while (true) {
    while (Serial.available()) {
      char c = Serial.read();
      if (c == '\n') { s.trim(); return s; }
      if (c == '\r') continue;
      s += c;
      if (s.length() > 200) { s.trim(); return s; }
    }
    if (millis() - start > 1000) { s.trim(); return s; }
  }
}

void setup() {
  Serial.begin(115200);
  delay(50);

  strip.begin();
  strip.clear();
  strip.setBrightness(150);
  set_all(0,0,0);

  pinMode(L1, OUTPUT); pinMode(L2, OUTPUT); pinMode(L3, OUTPUT); pinMode(L4, OUTPUT);
  pinMode(R1, OUTPUT); pinMode(R2, OUTPUT); pinMode(R3, OUTPUT); pinMode(R4, OUTPUT);
  applyPhase(0,0,0,0,true);
  applyPhase(0,0,0,0,false);

  Serial.println("LED+STEPPER pronto (NeoPixel D7)");
}

void loop() {
  if (!Serial.available()) return;
  String cmd = readLine();
  if (cmd.length() == 0) return;

  String up = cmd; up.toUpperCase();

  // === CORES ANEL ===
  if (up == "RED" || up == "R")       { set_all(255,0,0);     Serial.println("ACK:RED"); }
  else if (up == "GREEN" || up == "G"){ set_all(0,255,0);     Serial.println("ACK:GREEN"); }
  else if (up == "YELLOW" || up == "Y"){ set_all(255,200,0);  Serial.println("ACK:YELLOW"); }
  else if (up == "BLUE" || up == "B") { set_all(0,0,255);     Serial.println("ACK:BLUE"); }
  else if (up == "PURPLE" || up == "P"){set_all(128,0,128);   Serial.println("ACK:PURPLE"); }
  else if (up == "OFF" || up == "O")  { set_all(0,0,0);       Serial.println("ACK:OFF"); }

  // === STEPPERS ===
  else if (up.startsWith("LARM_REL ")) { long v = cmd.substring(9).toInt(); moveRel(v, true);  Serial.println("OK"); }
  else if (up.startsWith("RARM_REL ")) { long v = cmd.substring(9).toInt(); moveRel(v, false); Serial.println("OK"); }
  else if (up.startsWith("LARM_ABS ")) { long v = cmd.substring(9).toInt(); moveAbs(v, true);  Serial.println("OK"); }
  else if (up.startsWith("RARM_ABS ")) { long v = cmd.substring(9).toInt(); moveAbs(v, false); Serial.println("OK"); }
  else if (up == "LZERO")              { lpos = 0; Serial.println("OK"); }
  else if (up == "RZERO")              { rpos = 0; Serial.println("OK"); }
  else if (up == "HAPPY")              { gestureHappy(); Serial.println("OK"); }

  else {
    Serial.print("ERR:unknown=");
    Serial.println(cmd);
  }
}
