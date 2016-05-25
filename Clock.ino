const int TICKS_BASE = 25000; // 0.1 segs.

bool clock_tick_change = false;
int  clock_ticks = 3600;
int  clock_ticks_dec = 10;

void display_clock_ticks() {
  int segs = clock_ticks % 60;
  int mins = clock_ticks / 60;
  int min1 = mins/10;
  int min2 = mins%10;
  int seg1 = segs/10;
  int seg2 = segs%10;

  Serial.print(min1);
  Serial.print(min2);
  Serial.print(":");
  Serial.print(seg1);
  Serial.println(seg2);
}

ISR(TIMER4_COMPA_vect) {          // timer compare interrupt service routine
  if(!--clock_ticks_dec) {
    if (clock_ticks) clock_ticks--;
    clock_tick_change = true;
    clock_ticks_dec = 10;
  }
}

// x = 1  -> Normal
// x = 2  -> Doble velocidad
// x = 5  -> 5 velocidad
// x = 10 -> 10 velocidad
void fclock_speed(int x) {
  OCR4A = TICKS_BASE / x; 
}

void fclock_reset(int ct) {
  clock_tick_change = false;
  clock_ticks = ct;
  clock_ticks_dec = 10;
}

void fclock_setup() {
  clock_tick_change = false;
  clock_ticks = 3600;
  clock_ticks_dec = 10;
  
  noInterrupts();

  TCCR4A = 0;
  TCCR4B = 0;
  TCNT4 = 0;

  //OCR1A = 15625; // 1 seg ?? ponele
  OCR4A = TICKS_BASE;   // 0.1 segs

  TCCR4B |= ((1<<CS11) | (1<<CS10)); // 1024 prescaler
  TCCR4B |= (1<<WGM12);              // CTC Mode 1

  TIMSK4 |= (1 << OCIE4A);
  
  interrupts();
}

inline bool fclock_tick_changed() {
  return clock_tick_change;
}

inline void fclock_reset_tick_change() {
  clock_tick_change = false;
}

inline int fclock_ticks() {
  return clock_ticks;
}

/*
void setup() {
  // put your setup code here, to run once:
  pinMode(13, OUTPUT);
  timer_setup();
  Serial.begin(9600);
}

void loop() {
  if (clock_tick_change) {
    digitalWrite(13, digitalRead(13) ^ 1);
    clock_tick_change = false;
    display_clock_ticks();
  }
  delay(4000);
  timer_speed(10);
}

*/
