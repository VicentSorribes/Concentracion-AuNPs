/*

Este código se ha construido a partir del del programa 'intensidad_fotodiodo.ino', ya que aplica el mismo procedimiento,
solo que con pasos adicionales. La toma de medidas se repite dos veces: la primera de ellas es para medir la intensidad de
la muestra sin NPs (I0), al acabar, la siguiente medida se guarda automáticamente como la de la intensidad de la muestra con
NPs (I1). A partir de estos valores, se calcula la absorbancia como Abs = -log10(I1 / I0) y su error por propagación, y a
partir de ella, se obtiene la concentración. Al terminar, el código se reinicia para volver a obtener una nueva medida de I0.

Nota: en verdad da igual si I0 es la muestra sin NPs e I1 la muestra con NPs o viceversa, ya que si ambos valores se
intercambian, simplemente se le cambia el signo a la absorbancia. Por eso el código calcula Abs directamente como el
valor absoluto del logaritmo, así matamos el signo y no nos importa el orden de las medidas.

*/


// LIBRERÍA PARA EL LCD

#include <LiquidCrystal.h>


// CONEXIONES DE ARDUINO E INICIALIZACIÓN DEL LCD

const int FOTODIODO = A0; // Pin analógico al que está conectado el fotodiodo.
const int LED = 9; // Pin digital al que está conectado el LED.

const int RS = 2; // Pin digital conectado al RS del LCD.
const int E  = 3; // Pin digital conectado al E  del LCD.
const int D4 = 4; // Pin digital conectado al D4 del LCD.
const int D5 = 5; // Pin digital conectado al D5 del LCD.
const int D6 = 6; // Pin digital conectado al D6 del LCD.
const int D7 = 7; // Pin digital conectado al D7 del LCD.

// El resto de conexiones del LCD:
// GND a tierra.
// VDD a la corriente.
// VO con una resistencia de 3.3 kΩ a tierra.
// RW a tierra.
// BLA con una resistencia de 220 Ω a la corriente.
// BLK a tierra.

LiquidCrystal lcd(RS, E, D4, D5, D6, D7);


// CONSTANTES DEL PROGRAMA

// Valores de la curva de calibrado para calcular la concentración a partir de la absorbancia según la recta
// Con = (Abs - x) / y [mg/L]. Los coeficientes y sus errores están proporcionados por un ajuste de MATLAB.
// Este funciona hasta llegar a una concentración de ~170 mg/L, a partir de ahí ya no es tan fiable.
const float x = -0.02;  const float ex = 0.03;
const float y = 0.0114; const float ey = 0.0003;

// Valor de intensidad a partir del cual el sensor indica que detecta luz. Si está muy oscuro y no fluctua demasiado,
// con 2 va bien (el valor de fondo en oscuridad total oscila entre 0 y 1). 
const int umbral = 10;

// Número máximo de medidas que se quieran guardar para hacer la media entre ellas. Cuando se llegue a este valor, el led
// parpadeará para avisar que se ha parado de tomar medias. No ponerlo demasiado alto para evitar que se llene la RAM del
// Arduino, aunque supongo que para que pase eso haría falta un valor excesivamente grande.
const int maxMedidas = 75;

// Tiempo que transcurre entre diferentes medidas (en milisegundos).
const int espera = 100;

// Estado del led mientras se están tomando medidas: HIGH = encendido, LOW = apagado.
const int estado = HIGH;


// INICIALIZACIÓN DE VARIABLES

float I0 = 0; float eI0 = 0; // Valor y error de la intensidad para la primera muestra.
float I1 = 0; float eI1 = 0; // Valor y error de la intensidad para la segunda muestra.

int valores[maxMedidas]; // Vector donde se guardan las medidas de la intensidad.
int indice; // Número de medida.
bool midiendo; // Controla si se detecta luz.
bool limiteAlcanzado; // Controla si se llena el vector "valores".

int fase = 0; // Fase del procedimiento.
// 0 = Esperando luz para medir I0.
// 1 = Midiendo I0.
// 2 = Esperando luz para medir I1.
// 3 = Midiendo I1.
// 4 = Mostrar resultados y resetar a fase 0.


// CÓDIGO

void setup() {
  pinMode(FOTODIODO, INPUT);
  pinMode(LED, OUTPUT);
  lcd.begin(16, 2);
  lcd.clear();
  lcd.print("Esperando luz.");
}

void loop() {
  int valorFD = analogRead(FOTODIODO);

  // Cuando detecta luz por primera vez comienza la fase 1.
  if (fase == 0 && valorFD >= umbral) {
    midiendo = true;
    indice = 0;
    limiteAlcanzado = false;

    fase = 1;
  }

  // Toma de medidas de I0.
  if (fase == 1 && valorFD >= umbral) {
    if (!limiteAlcanzado && indice < maxMedidas) {
      if (indice == 0) {
        lcd.clear();
        lcd.print("Midiendo I0...");
      }
      valores[indice++] = valorFD;
    }

    if (indice == maxMedidas) {
      limiteAlcanzado = true;
    }

    if (!limiteAlcanzado) {
      digitalWrite(LED, estado);
      delay(espera);
    }
  }

  // Cuando ya no detecta luz o se llena el vector de medidas, pasar a fase 2.
  if (fase == 1 && limiteAlcanzado) {
    I0 = Media(valores, indice);
    eI0 = ErrorDeLaMedia(valores, indice, I0);
    
    lcd.clear();
    lcd.print("Medicion de I0");
    lcd.setCursor(0,1);
    lcd.print("finalizada.");

    while (valorFD >= umbral) {
      digitalWrite(LED, HIGH);
      delay(250);
      digitalWrite(LED, LOW);
      delay(250);
      valorFD = analogRead(FOTODIODO);
    }

    midiendo = false;
    limiteAlcanzado = false;
    digitalWrite(LED, LOW);

    fase = 2;
  }
  if (fase == 1 && valorFD < umbral) {
    I0 = Media(valores, indice);
    eI0 = ErrorDeLaMedia(valores, indice, I0);
    
    lcd.clear();
    lcd.print("Medicion de I0");
    lcd.setCursor(0,1);
    lcd.print("finalizada.");

    midiendo = false;
    limiteAlcanzado = false;
    digitalWrite(LED, LOW);

    fase = 2;
  }

  // Cuando detecta luz por segunda vez, comienza la fase 3.
  if (fase == 2 && valorFD >= umbral) {
    midiendo = true;
    indice = 0;
    limiteAlcanzado = false;

    fase = 3;
  }

  // Toma de medidas de I1.
  if (fase == 3 && valorFD >= umbral) {
    if (!limiteAlcanzado && indice < maxMedidas) {
      if (indice == 0) {
        lcd.clear();
        lcd.print("Midiendo I1...");
      }
      valores[indice++] = valorFD;
    }

    if (indice == maxMedidas) {
      limiteAlcanzado = true;
    }

    if (!limiteAlcanzado) {
      digitalWrite(LED, estado);
      delay(espera);
    }
  }

  // Cuando ya no detecta luz o se llena el vector de medidas, pasar a fase 4.
  if (fase == 3 && limiteAlcanzado) {
    I1 = Media(valores, indice);
    eI1 = ErrorDeLaMedia(valores, indice, I1);
    
    lcd.clear();
    lcd.print("Medicion de I1");
    lcd.setCursor(0,1);
    lcd.print("finalizada.");

    while (valorFD >= umbral) {
      digitalWrite(LED, HIGH);
      delay(250);
      digitalWrite(LED, LOW);
      delay(250);
      valorFD = analogRead(FOTODIODO);
    }

    midiendo = false;
    limiteAlcanzado = false;
    digitalWrite(LED, LOW);

    fase = 4;
  }
  if (fase == 3 && valorFD < umbral) {
    I1 = Media(valores, indice);
    eI1 = ErrorDeLaMedia(valores, indice, I1);
    
    lcd.clear();
    lcd.print("Medicion de I1");
    lcd.setCursor(0,1);
    lcd.print("finalizada.");

    midiendo = false;
    limiteAlcanzado = false;
    digitalWrite(LED, LOW);

    fase = 4;
  }

  // Cálculo de los resultados.
  if (fase == 4) {
    float Abs = abs(log10(I1 / I0)); // Absorbancia.
    float eAbs = sqrt(pow(eI1 / I1, 2) + pow(eI0 / I0, 2)) / log(10); // Error por propagación de incertidumbres.

    float Con = (Abs - x) / y; // Concentración [mg/L].
    float eCon = sqrt(pow(eAbs / y, 2) + pow(ex / y, 2) + pow(Abs / y * ey, 2)); // Error por propagación de incertidumbres.

    lcd.clear();
    lcd.print("Conc: ");
    lcd.print(Con, 1);
    lcd.setCursor(0,1);
    lcd.print("  +/- ");
    lcd.print(eCon, 1);
    lcd.print(" mg/L ");

    // Resetear variables para una nueva medición.
    I0 = eI0 = I1 = eI1 = 0;
    fase = 0;
    midiendo = false;
    limiteAlcanzado = false;
  }
}

// Función para calcular la media
float Media(int *valor, int n) {
  float suma = 0;
  for (int i = 0; i < n; i++) {
    suma += valor[i];
  }
  return suma / n;
}

// Función para calcular el error de la media
float ErrorDeLaMedia(int *valor, int n, float media) {
  float sum_diff_2 = 0;
  for (int i = 0; i < n; i++) {
    float diff = valor[i] - media;
    sum_diff_2 += pow(diff, 2);
  } 
  float std = sqrt(sum_diff_2 / (n-1)); 
  return std / sqrt(n);
}