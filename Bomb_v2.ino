
/*
 * Parametros
 */
const int reset_pin = 19;

/*
 * Dispatch
 */
typedef enum B_STATES { ST_NORMAL, ST_MOVED, ST_CABLE, ST_DISARM, ST_BANG, ST_RESET } b_state;

typedef struct STATE_INFO state_info;

typedef void (*state_handler)  (state_info *);
typedef bool (*state_pre_check)(state_info *);
typedef void (*state_init)     (state_info *);

typedef struct DISPATCH_INFO {
  b_state state;
  state_init    init;
  state_handler handler;
  state_init    quit;
  state_pre_check * pre_checks;
} dispatch_info;

struct STATE_INFO {
  b_state prev_state;
  b_state curr_state;

  long millis;      // Comienzo del estado
  long sub_millis;  // Comienzo del subestado?

  /* Moved */
  int moved_times;
  int moved_ticks;

  /* Cable */
  byte cable_number;
  byte cable_missing;
  byte cable_cut;
  int  cable_ticks;
  bool cable_ooo;

  /* Disarm */
  byte disarm_state;

  /* Bang */
  byte bang_state;

  /* Reset */
  long reset_pressed_time;
  int  reset_time;
};

void transition_to(b_state, state_info * info);

void display_time() {
   if (fclock_tick_changed()) {
    fclock_reset_tick_change();
    fled_display_time(fclock_ticks());
    fbuzz_short();
  }
}


/* Precondiciones a chequear */
bool check_moved (state_info * info) {
  //return false;
  if(faccel_moved()) {
    transition_to(ST_MOVED, info);
    return true;
  }
  return false;
}

bool check_alarm (state_info * info) {
  return false;
}

bool check_bang  (state_info * info) {
  if(fclock_ticks() == 0) {
    transition_to(ST_BANG, info);
    return true;
  }
  return false;
}

/*
 * Normal state
 */
void st_normal_init(state_info * info) {
}

void st_normal_handle(state_info * info){
  //Serial.println("st_normal_handle");
  display_time();
}

void st_normal_quit(state_info * info) {
}

/*
 * Moved State
 */
typedef struct MOVED_MULT_INFO_STRUCT {
  int mult;
  int ticks;
} moved_mult_info_struct;

moved_mult_info_struct moved_mult_info[] = { {4, 12}, {4, 24}, {4, 48}, {8, 40}, {8, 60}, {10, 30}, {10, 60}, {10, 120}, {20, 60}, {20, 120} };
const int moved_mult_info_size = sizeof(moved_mult_info) / sizeof(moved_mult_info_struct);


void st_moved_init(state_info * info) {
  info->moved_ticks = 0;
  fclock_speed(moved_mult_info[info->moved_times].mult);
  //Serial.print("MovedInit para: ");
  //Serial.println(info->moved_times);
}

void st_moved_handle(state_info * info) {
  //Serial.println("st_moved_handle");
  if (fclock_tick_changed()) {
    display_time();    
    if (++info->moved_ticks >= moved_mult_info[info->moved_times].ticks)
      transition_to(ST_NORMAL, info);
  }
}

void st_moved_quit(state_info * info) {
  if (info->moved_times < (moved_mult_info_size - 1))
    info->moved_times++;
  fclock_speed(1);
  //Serial.println("moved_quit");  
}

/*
 * CABLES
 */

#define DEBOUNCE_DELAY 50

const int cable_leds_pins[] = {7, 14, 5, 4, 3, 15}; // Leds
const int cable_disarm[] = { 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32, 33 }; // Pares
#define CABLE_COUNT 2
#define _CABLE_COUNT (sizeof(cable_disarm)/sizeof(cable_disarm[0]))
int  cable_disarm_state[CABLE_COUNT];
int  cable_disarm_last_state[CABLE_COUNT];
long cable_last_debounced_time[CABLE_COUNT];

void cable_setup() {
  for(int ii=0; ii<CABLE_COUNT/2; ii++) {
    pinMode(cable_leds_pins[ii], OUTPUT);
    pinMode(cable_disarm[ii*2], INPUT_PULLUP);
    pinMode(cable_disarm[ii*2+1], INPUT_PULLUP);
    digitalWrite(cable_leds_pins[ii], HIGH); //OFF
  }

  for(int ii=0; ii<CABLE_COUNT/2; ii++) {
    byte p1 = digitalRead(cable_disarm[ii*2]);
    byte p2 = digitalRead(cable_disarm[ii*2+1]);
    /*
    Serial.print("Cable: ");
    Serial.print(cable_disarm[ii*2]);
    Serial.print(": ");
    Serial.print(p1);
    Serial.print(", ");
    Serial.print(cable_disarm[ii*2+1]);
    Serial.print(": ");
    Serial.println(p2);
    */
    if (p1 == LOW && p2 == LOW)
      digitalWrite(cable_leds_pins[ii], LOW); // ON
  }

  for(int ii=0; ii<CABLE_COUNT; ii++) {
    cable_disarm_state[ii] = LOW;
    cable_disarm_last_state[ii] = LOW;
  }

}


bool check_disarm(state_info * info) {
  //return false;
  if(info->cable_number == CABLE_COUNT/2) {
    transition_to(ST_DISARM, info);
    return true;
  }
  
  return false;
}


void cable_set_led(int cn, bool state) {
  digitalWrite(cable_leds_pins[cn], state ? LOW : HIGH);
}

void cable_led_off(int cn) {
  cable_set_led(cn, false);
}

void cable_led_on(int cn) {
  cable_set_led(cn, true);  
}

bool fread_cables() {
  //Serial.println("readc");
  bool changed = false;
  for(int ii=0; ii<CABLE_COUNT; ii++) {
    // solo los cables pendientes
    if (cable_disarm_state[ii] == LOW) {
      int c_val = digitalRead(cable_disarm[ii]);
      if (c_val != cable_disarm_last_state[ii]) {
        cable_last_debounced_time[ii] = millis();
      }

      if ((millis() - cable_last_debounced_time[ii]) > DEBOUNCE_DELAY) {
        if (c_val == cable_disarm_last_state[ii] && c_val != cable_disarm_state[ii]) { //!!! Verrrr
          cable_disarm_state[ii] = c_val;
          changed = true;
        }
      }
      cable_disarm_last_state[ii] = c_val;
    }
  }
  return changed;
}

bool check_cable(state_info * info) {
  //return false;
  if (fread_cables()) {
    transition_to(ST_CABLE, info);
    return true;
  } 

  return false;
}

void st_cable_init(state_info * info) {
  info->cable_ticks = 0;
  info->cable_ooo = false;
  fclock_speed(5); 

  //Serial.println("Cable init");

  int ii;
  for (ii=CABLE_COUNT-1; ii>=0; ii--) {
    if (cable_disarm_state[ii] == HIGH) break;
  }

  if (ii/2 > info->cable_number) {
    info->cable_ooo = true;
    Serial.println("Cable ooo");
    return;
  }

  if (ii<0) {
    // No deberia pasar ...
    Serial.println("Cable < 0 ...");
    info->cable_missing = -1;
    return;
  }

  info->cable_cut = ii;
  info->cable_missing = ii + (ii % 2 == 0 ? 1 : -1);
  Serial.print("Cable Missing: ");
  Serial.println(info->cable_missing);
}

void st_cable_handle(state_info * info) {
  //Serial.print("Cable handle");

  if (info->cable_missing == -1) {
    transition_to(ST_NORMAL, info);
    return;
  }

  if (info->cable_ooo) {
    transition_to(ST_BANG, info);
    return;  
  }
  
  if (fclock_tick_changed()) {
    display_time();
    if (++info->cable_ticks >= 10)
      transition_to(ST_BANG, info);
      return;
  }

  fread_cables();

  //Serial.println("va bien");
  int ii;
  for (ii=CABLE_COUNT-1; ii>=0; ii--) {
    if (cable_disarm_state[ii] == HIGH && ii != info->cable_cut) break;
  }

  if (ii/2 > info->cable_number) {
    info->cable_ooo = true;
    return;
  }

  if (ii == info->cable_missing) {
    cable_led_off(ii/2);

    info->cable_number++;

    transition_to(check_disarm(info) ? ST_DISARM : ST_NORMAL, info);
    return;
  }
}

void st_cable_quit(state_info * info) {
  fclock_speed(1);  
}

/* 
 * Disarm 
 */
void st_disarm_init(state_info * info) {
  info->disarm_state = 0;
}

void st_disarm_handle(state_info * info){
  if ((millis() - info->millis) < 1000)
    return;

  switch(info->disarm_state) {
    case 0:
      fled_display_msg("BOMB");
      break;

    case 1:
      fled_display_msg("OFF ");
      break;
  }

  info->millis = millis();
  info->disarm_state = (info->disarm_state + 1) % 2;
}

void st_disarm_quit(state_info * info) {

}

/* 
 * Bang 
 */
void st_bang_init(state_info * info) {
  info->bang_state = 0;
}

void st_bang_handle(state_info * info){
  if ((millis() - info->millis) < 1000)
    return;

  switch(info->bang_state) {
    case 0:
      fled_display_msg("RUN ");
      break;

    case 1:
      fled_display_msg("LIKE");
      break;

    case 2:
      fled_display_msg("HELL");
      break;
  }

  info->millis = millis();
  info->bang_state = (info->bang_state + 1) % 3;
}

void st_bang_quit(state_info * info) {

}

/*
 * Reset state
 */
int reset_last_state = HIGH;
long reset_last_debounced_time;
int reset_state = HIGH;

/* Precondiciones a chequear */
bool check_reset (state_info * info) {
  //return false;
  if(fread_reset()) {
    transition_to(ST_RESET, info);
    return true;
  }
  return false;
}

bool fread_reset() {
  //Serial.println("readc");
  bool changed = false;
  int c_val = digitalRead(reset_pin);
  //Serial.println(c_val);
  if (c_val != reset_last_state) {
    reset_last_debounced_time = millis();
  }

  if ((millis() - reset_last_debounced_time) > DEBOUNCE_DELAY) {
    if (c_val == reset_last_state) {
      reset_state = c_val;
      changed = true;
    }
  }
  reset_last_state = c_val;
  return !reset_state;
}

void st_reset_init(state_info * info) {
  info->reset_pressed_time = millis();
  info->reset_time = 3600;
  fbuzz_off();
  fclock_reset(info->reset_time);
  fled_display_time(fclock_ticks());
}

void st_reset_handle(state_info * info){
  // cada segundo bajo 5 minutos
  if (!fread_reset()) {
    reset();
    transition_to(ST_NORMAL, info);
    return;
  }

  long m = millis();
  if ((m - info->reset_pressed_time) > 1000) {
    info->reset_pressed_time = m;
    info->reset_time -= 600;
    if (info->reset_time < 60) info->reset_time = 60;
    fclock_reset(info->reset_time);
    fled_display_time(fclock_ticks());
  }
}

void st_reset_quit(state_info * info) {
  Serial.println(fclock_ticks());
}

dispatch_info dispatch_info_table[] = {
  { ST_NORMAL, st_normal_init, st_normal_handle, st_normal_quit, (state_pre_check[]){check_reset, check_alarm, check_bang, check_disarm, check_moved, check_cable, check_reset, NULL} },
  { ST_MOVED , st_moved_init, st_moved_handle, st_moved_quit, (state_pre_check[]){check_bang, check_disarm, check_cable, NULL} },
  { ST_CABLE , st_cable_init, st_cable_handle, st_cable_quit, (state_pre_check[]){check_bang, check_disarm, NULL} },
  { ST_DISARM , st_disarm_init, st_disarm_handle, st_disarm_quit, (state_pre_check[]){check_reset, NULL} },
  { ST_BANG , st_bang_init, st_bang_handle, st_bang_quit, (state_pre_check[]){check_reset, NULL} },
  { ST_RESET , st_reset_init, st_reset_handle, st_reset_quit, (state_pre_check[]){NULL} }
};

void transition_to(b_state state, state_info * info) {
  if (dispatch_info_table[info->curr_state].quit)
    dispatch_info_table[info->curr_state].quit(info);

  info->prev_state = info->curr_state;
  info->curr_state = state;
  info->millis = millis();

  if (dispatch_info_table[info->curr_state].init)
    dispatch_info_table[info->curr_state].init(info);
}

state_info info;

void reset() {
  info.moved_times = 0;
  info.cable_number = 0;
  cable_setup();

  //fled_setup(13);
  //fclock_setup();
  fled_display_time(fclock_ticks());
  
  faccel_setup(A0, A1, A2, 5);
  faccel_adjust();
}

void setup() {
  Serial.begin(9600);

  pinMode(reset_pin, INPUT_PULLUP);
  
  Serial.println("Begin");
  info.prev_state  = ST_NORMAL;
  info.curr_state  = ST_NORMAL;
  
  info.moved_times = 0;
  
  info.cable_number = 0;

  cable_setup();

  fled_setup(13);
  //fclock_setup();
  fled_display_time(fclock_ticks());
  
  faccel_setup(A0, A1, A2, 5);
  faccel_adjust();

  fclock_setup();

  st_normal_init(&info);
}

void loop() {
  for(int ii=0; dispatch_info_table[info.curr_state].pre_checks[ii]; ii++)
    if (dispatch_info_table[info.curr_state].pre_checks[ii](&info))
      return; //transision
  dispatch_info_table[info.curr_state].handler(&info);
  //delay(500);
}
