#define BUZZER_PIN 38  
#define NUM_MOTORS 7
#define MOTOR_PINS {30, 31, 32, 33, 34, 35, 36}
// Pines
#include <Servo.h>
#include <EEPROM.h> 
#include <RTClib.h> 
#include <Wire.h> 
#include <LiquidCrystal_I2C.h>
#include <Keypad.h>

Servo motors[NUM_MOTORS];

RTC_DS1307 rtc;
// Configura la pantalla LCD I2C y el RTC
LiquidCrystal_I2C lcd(0x27, 16,2); 

const int ROW_NUM = 4;
const int COL_NUM = 4;
// Define el mapa de teclas para el teclado numérico 4x4
char keys[ROW_NUM][COL_NUM] = {
  {'1','2','3', 'A'},
  {'4','5','6', 'B'},
  {'7','8','9', 'C'},
  {'*','0','#', 'D'}
};

byte pin_rows[ROW_NUM] = {22,23,24,25};
byte pin_column[COL_NUM] = {26,27,28,29} ;
Keypad keypad = Keypad(makeKeymap(keys), pin_rows, pin_column, ROW_NUM, COL_NUM);
//configura el teclado

struct Toma{
  byte mes, dia, hora, minuto, nro_pila;

  bool operator==(const DateTime &otro){
    return ( otro.month() == this->mes && otro.day()==this->dia && otro.hour()==this->hora && otro.minute() == this->minuto);
  }
  
};
//creamos un struct para representar una dosis. Cada toma tiene un mes, dia, etc
// y sobrecargamos el operador == para poder compararlo con la hora del rtc


enum Conf_dosis{
  AGREGAR_MES, AGREGAR_DIA, AGREGAR_HORA, AGREGAR_MIN, AGREGAR_PILA
};
// esta es una enumeracion con cada paso en el proceso de cargar una dosis


//aqui colocamos algunas variables auxiliares
int cant_dosis = 0;
Toma *tomas = new Toma[0];

Toma nulo = {100, 100, 100, 100, 100};
Toma nuevo = nulo;

Conf_dosis etapa_conf_dosis = AGREGAR_MES;
int estado = 0;
int dosis_a_configurar = 0;
int selec_elim = 0;


void accionar_alarma() {
  for(int i = 0; i < 10; i++){
    digitalWrite(BUZZER_PIN, HIGH);
    delay(1000);
    digitalWrite(BUZZER_PIN, LOW);
  }
  
}


void cargar_tomas (Toma* &v, int &n){
  int cant_confirmada = 0;
  bool flag = true;
  int i = 0;
  while (flag) {
    Toma leer;
    EEPROM.get(i, leer);
    
    if(leer.mes == byte(255)) flag = false;
   
    else{

      Toma* temp = new Toma[cant_confirmada+1];
      for(int j = 0; j < cant_confirmada; j++) temp[j] = v[j];
      temp[cant_confirmada] = leer;
      delete[] v;
      v = temp;
      cant_confirmada++;
      i+=sizeof(Toma);
    }
  }
  n = cant_confirmada;
}

void cargar(byte &var, const char &key){
  switch(var)
  {
    case 100:
      var = key-48;
      
      break;
    default:
      var *= 10;
      var += key-48;
      delay(1000);
      lcd.clear();
      if (etapa_conf_dosis != AGREGAR_PILA) {
        etapa_conf_dosis = static_cast<Conf_dosis>(static_cast<int>(etapa_conf_dosis) + 1);  // Pasamos al siguiente paso
      }
      break;
  }
}

void elim_dosis(int it){
  EEPROM.put(it, nulo);
  cargar_tomas(tomas,cant_dosis);
}

void accionar_motor(byte nro_motor) {
  int nro_motor_int = int(nro_motor);
  if (nro_motor_int < 1 || nro_motor_int > NUM_MOTORS) {
    lcd.clear();
    lcd.print("Motor erroneo");
    delay(1000);
    lcd.clear();
    return;
  }


  motors[nro_motor_int - 1].write(90);
  delay(3000);

 
  motors[nro_motor_int - 1].write(0);
}

void setup() {
  Serial.begin(9600); 
  
  lcd.begin(16, 2);
  lcd.clear();
  lcd.print("MARQUE 1, 2 o 3");
  if (! rtc.begin()) {
    Serial.println("No se pudo encontrar RTC");
    while (1);
  }
  if (! rtc.isrunning()) {
    Serial.println("RTC no está funcionando!");
    // La siguiente línea ajusta la hora del RTC al momento de la compilación
    rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
  }
  cargar_tomas(tomas, cant_dosis);

  int motorPins[NUM_MOTORS] = MOTOR_PINS;
  for (int i = 0; i < NUM_MOTORS; i++) {
    motors[i].attach(motorPins[i]); 
    motors[i].write(0); 
  }
  pinMode(BUZZER_PIN, OUTPUT);
}


void loop() {
  
  DateTime now = rtc.now();
  
  Toma* it = tomas;
  for (; it != tomas + cant_dosis; ++it) {
      if (*it == now) {
          break;
      }
  }

  if(it != tomas + cant_dosis){
    Serial.println("La alarma suena..." );
      byte nro_motor = (*it).nro_pila;
      int dist = it - tomas; 
      elim_dosis(dist);
      
      accionar_motor(nro_motor);
      accionar_alarma();
  }
  


  char key = keypad.getKey();
  // si se presiona una tecla...
  if (key) {
    

    switch(estado)
    {
      case 0:
        lcd.clear();
        
        switch(key)
        {
          case '1': 
           
            estado = 11;
            lcd.print("¿Cuantas dosis?"); //no sé si el salto de linea funciona o no
            break;
          case '2':
            
            for (int i = 0; i < cant_dosis*sizeof(Toma); i++){
              EEPROM.write(i,byte(255));
            }
            lcd.print("Conf eliminada");
            delay(1000);
            lcd.clear();
            delete [] tomas;
            //aca puedo seguir usando el pto no?
            lcd.print("MARQUE 1, 2 o 3");
            break;
          case '3':
            estado = 20;
            lcd.print("Cual eliminas?");
            break;
           default:
            lcd.setCursor(0,0);
            lcd.print("OTRO BOTON");
            delay(1000);
            lcd.clear();
            lcd.print("MARQUE 1, 2 o 3");
            break;
        }
        break;
      //VOY A TENER QUE SETEAR A DOSISCONFIGURAR EN 0 !!!!!!!
      case 11:
          
          if(key == '*'){
              estado = 12;
              lcd.clear();
              lcd.print("Conf nro: ");
              lcd.println(dosis_a_configurar);
             
          }

         else if(dosis_a_configurar == 0){
              
              lcd.clear();
              dosis_a_configurar = key-48;
              lcd.print(key);
          }

          else{
              dosis_a_configurar = dosis_a_configurar * 10;
              dosis_a_configurar+= key-48;
              lcd.print(key);
          }

          break;
      
      case 12:
        lcd.print(key);
        switch(etapa_conf_dosis)
        { 
          
          case AGREGAR_MES:
            cargar(nuevo.mes, key);
            break;

          case AGREGAR_DIA: 
            cargar(nuevo.dia, key);
            break;

          case AGREGAR_HORA:
            cargar(nuevo.hora, key);
            break;

          case AGREGAR_MIN:
            cargar(nuevo.minuto, key);
            break;
    
          case AGREGAR_PILA:

            nuevo.nro_pila = key-48;
            etapa_conf_dosis = AGREGAR_MES;

            int direccion = cant_dosis * sizeof(Toma);
            EEPROM.put(direccion, nuevo);
            Toma leer;

            
            cargar_tomas(tomas,cant_dosis);
            
            nuevo = nulo;
            delay(1000);
            lcd.clear();
            dosis_a_configurar--;
            if(dosis_a_configurar == 0){
              estado = 0;
              lcd.print("Carga terminada");
              delay(1000);
              lcd.clear();
              lcd.print("MARQUE 1, 2 o 3");
            }
            else{
              
              lcd.print("Conf nro: ");
              lcd.print(dosis_a_configurar);
              lcd.print(" ");
            }
            
            break;
          
        }
      break;
      case 20:
      
          if(key == '*'){
              
              elim_dosis(selec_elim-1);
              
              lcd.clear();
              
              lcd.print("Dosis eliminada");
              delay(1000);
              lcd.clear();
              selec_elim = 0;
              estado = 0;
              lcd.print("MARQUE 1, 2 o 3");
          }

         else if(selec_elim == 0){
              
              lcd.clear();
              selec_elim = key-48;
              lcd.print(key);
          }

          else{
              selec_elim *= 10;
              selec_elim+= key-48;
              lcd.print(key);
          }
        break;
     
    }
  }
 
}