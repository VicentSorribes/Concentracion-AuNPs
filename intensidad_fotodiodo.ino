/*

Cuando el fotodiodo detecta luz, este código enciende un led y comienza a tomar medidas de la intensidad recibida. Va
guardando todas esas medidas mientras siga habiendo luz o se llegue a un máximo de medidas (especificado más adelante),
lo que ocurra primero. Cuando se detiene la toma de datos, devuelve la media de todas las intensidades guardadas junto
con su error y se prepara para tomar una nueva medida.

La intensidad máxima que nos devuelve es de 1023 (el fotodiodo satura), y la mínima de 0 (no detecta nada). A mayor
resistencia entre el fotodiodo y tierra, la sensibilidad es también mayor (hay más diferencia entre las intensidades
detectadas con luz directa vs con algo de por medio), pero también antes satura el fotodiodo con la misma luz.

La máxima resistencia que he podido poner para evitar que sature con luz directa es de 432 kΩ (dos resistencias de 68 kΩ
en paralelo conectadas en serie con otra de 68 kΩ y una de 330 kΩ), donde la luz del láser de 405 nm llega a un máximo de
intensidad desde 890 hasta 990, supongo que será por razones como la luz ambiente y que debido a mi pulso la posición de
incidencia del haz baile alrededor del detector.

*Actualización*: después de medirlo ya en oscuridad y con un soporte para el láser, se obtiene un máximo de ~980.

*/


// CONEXIONES DE ARDUINO

const int FOTODIODO = A0; // Pin analógico al que está conectado el fotodiodo.
const int LED = 4; // Pin digital al que está conectado el LED.


// CONSTANTES DEL PROGRAMA

// Valor de intensidad a partir del cual el sensor indica que detecta luz.
const int umbral = 50;

// Número máximo de medidas que se quieran guardar para hacer la media entre ellas. Cuando se llegue a este valor, el led
// parpadeará para avisar que se ha parado de tomar medias. No ponerlo demasiado alto para evitar que se llene la RAM del
// Arduino, aunque supongo que para que pase eso haría falta un valor excesivamente grande.
const int maxMedidas = 75;

// Tiempo que transcurre entre diferentes medidas (en milisegundos).
const int espera = 100;

// Estado del led mientras se están tomando medidas: HIGH = encendido, LOW = apagado.
const int estado = HIGH;


// INICIALIZACIÓN DE VARIABLES

float I = 0; float eI = 0; // Valor y error de la intensidad registrada por el fotodiodo.

int valores[maxMedidas]; // Vector donde se guardan las medidas de la intensidad.
int indice; // Número de medida.
bool midiendo; // Controla si se detecta luz.
bool limiteAlcanzado; // Controla si se llena el vector "valores".

int fase = 0; // Fase del procedimiento.
// 0 = Esperando luz para medir la intensidad.
// 1 = Midiendo la intensidad.
// 2 = Mostrar el valor y resetar a fase 0.


// CÓDIGO

void setup() {
  pinMode(FOTODIODO, INPUT);
  pinMode(LED, OUTPUT);
  Serial.begin(9600);
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

  // Toma de medidas de la intensidad.
  if (fase == 1 && valorFD >= umbral) {
    if (!limiteAlcanzado && indice < maxMedidas) {
      if (indice == 0) {
        Serial.println();
        Serial.println("Midiendo... ");
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
    while (valorFD >= umbral) {
      digitalWrite(LED, HIGH);
      delay(250);
      digitalWrite(LED, LOW);
      delay(250);
      valorFD = analogRead(FOTODIODO);
    }

    digitalWrite(LED, LOW);
    fase = 2;
  }
  if (fase == 1 && valorFD < umbral) {
    digitalWrite(LED, LOW);
    fase = 2;
  }

  // Mostrar los resultados.
  if (fase == 2) {
    I = Media(valores, indice);
    eI = ErrorDeLaMedia(valores, indice, I);
    
    Serial.print("Medición finalizada. Intensidad media I = ");
    Serial.print(I, 2);
    Serial.print(" +/- ");
    Serial.println(eI, 2);

    // Resetear variables para una nueva medición.
    I = eI = 0;
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