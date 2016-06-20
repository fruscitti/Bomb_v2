// Rutinas para el acelerometro
//

const int SAMPLES = 10;
int aPins[3] = {0, 0, 0};
int sensibility = 5;
int promedio[3] = {0, 0, 0};

int samples[3][SAMPLES];
int total[3] = {0, 0, 0};
int sampleIndex  = 0;
int sampleNumber = 0;


void faccel_setup(int xpin, int ypin, int zpin, int sens) {
  aPins[0] = xpin;
  aPins[1] = ypin;
  aPins[2] = zpin;
  sensibility = sens;
}
      

// Ajusta los valores de referencia
// Llamar cuando esta todo quieto.
//
void faccel_adjust() {

  // Inicializo
  for(int jj=0; jj<3; jj++) {
    for(int ii=0; ii<SAMPLES; ii++) {
      samples[jj][ii] = 0;
    }
    total[jj] = 0;
    promedio[jj] = 0;
  }
  sampleIndex = 0;

  
  // Dos segundos para que se quede quieto
  fled_blink(4, 250);

  // Promedio de SAMPLES mediciones de los 3 ejes
  for (int ii=0; ii<40; ii++) {
    for(int jj=0; jj<3; jj++) {
      total[jj] -= samples[jj][sampleIndex];
      samples[jj][sampleIndex] = analogRead(aPins[jj]);
      total[jj] += samples[jj][sampleIndex];
      sampleIndex++;
      if (sampleIndex >= SAMPLES)
        sampleIndex = 0;
    }
    if (ii%5)
      fled_blink(1, 50);
  }

  for(int jj=0; jj<3; jj++) {
    promedio[jj] = total[jj] / SAMPLES;
  }

  fled_blink(1, 500);
}

void faccel_readjust() {

  // Inicializo
  for(int jj=0; jj<3; jj++) {
    for(int ii=0; ii<SAMPLES; ii++) {
      samples[jj][ii] = 0;
    }
    total[jj] = 0;
    promedio[jj] = 0;
  }
  sampleIndex = 0;

  
  // Promedio de SAMPLES mediciones de los 3 ejes
  for (int ii=0; ii<SAMPLES; ii++) {
    for(int jj=0; jj<3; jj++) {
      total[jj] -= samples[jj][sampleIndex];
      samples[jj][sampleIndex] = analogRead(aPins[jj]);
      total[jj] += samples[jj][sampleIndex];
      sampleIndex++;
      if (sampleIndex >= SAMPLES)
        sampleIndex = 0;
    }
    delay(2);
  }

  for(int jj=0; jj<3; jj++) {
    promedio[jj] = total[jj] / SAMPLES;
  }
}

// Lee n veces los valores y los promedia
//
void faccel_read(int n, int * results) {
  long totals[3] = {0L, 0L, 0L};

  for(int ii=0; ii<n; ii++) {
    for(int jj=0; jj<3; jj++) {
      int v =  analogRead(aPins[jj]);
      totals[jj] += v;
    }
    delay(2);
  }

  for(int jj=0; jj<3; jj++) {
    results[jj] = (int)(totals[jj] / (long)n);
  }
}

#define _FACCEL_TRACE 1

bool faccel_moved() {
  int readings[] = {0, 0, 0};

  faccel_read(20, readings);

#ifdef _FACCEL_TRACE
  Serial.print("Sens: ");
  Serial.print(sensibility);
  Serial.print(", xProm: ");
  Serial.print(promedio[0]);
  Serial.print(", xRead: ");
  Serial.print(readings[0]);
  Serial.print(", yProm: ");
  Serial.print(promedio[1]);
  Serial.print(", yRead: ");
  Serial.print(readings[1]);
  Serial.print(", zProm: ");
  Serial.print(promedio[2]);
  Serial.print(", zRead: ");
  Serial.print(readings[2]);
#endif

  for(int jj=0; jj<3; jj++) {
    if ((readings[jj] >  (promedio[jj] + sensibility)) || (readings[jj] <  (promedio[jj] - sensibility)))  {
#ifdef _FACCEL_TRACE
      Serial.print(", true for: ");
      Serial.println(jj);
#endif
      return true;
    }
  }
#ifdef _FACCEL_TRACE
  Serial.println(", false");
#endif      
  return false;
}

