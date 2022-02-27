#include <Wire.h>
#include <LCD.h>
#include <LiquidCrystal_I2C.h>
#include <EEPROM.h>

// ########## INIT DISPLAY ##########

// LiquidCrystal_I2C lcd(0x27, 2, 1, 0, 4, 5, 6, 7, 3, POSITIVE); // Set the LCD I2C address UNO
LiquidCrystal_I2C lcd(0x27, 2, 1, 0, 4, 5, 6, 7, 3, POSITIVE); // Set the LCD I2C address MEGA2560 ; PIN 20 SDA, PIN 21SCL 
//                    0x3F correct? see https://www.youtube.com/watch?v=B8DNokj9LnY

// HD44780 compatible LCD display special characters
#define UE "\365" // ü
#define AE "\341" // ä
#define OE "\357" // ö
#define SZ "\342" // ß

// EURO symbole
byte euro[8] = {
  B00110,
  B01001,
  B11100,
  B01000,
  B11100,
  B01001,
  B00110,
};

// ############ specific configurations ####################

// max = number of buttons (default: 5; even with 1,2 or 3 rows of 5 compartments); if you want to have a button for each compartment increase up to 15 or more
// don't forget to look at EEPROM structure when changing value of max
const int max = 5; 
// number of rows (1 = 5 compartments, 2 = 10 compartments, 3 = 15 compartments)
// if maxrow <=2 the button #5 is used to start refill programm otherwise for open 3rd row during programming
const int maxrow = 2;

unsigned long idlePeriod = 180000; // time in ms between idle messages or shutdown e.g. 180000  
// powersave = 0 show text when Idle; powersave = 1 shutdown when IdlePeriod reached

// debug modus: if you need 10 vendors and one additonal PIN (TX0 / Digital PIN 1 which should normally not be used) on UNO turn debug mode to 0 which disables serial 
// I use this pin to send a message to my smarthome system for any purchased product
bool debug = false;
 //          if (!debug) {
          const int homematic_pin = 1;   
 //          }
unsigned long idleTimerMillis = 0;
int randomNumber = 0;
// ########## INIT BUTTONS ##########

//const int selector[5] = {17, 16, 15, 14, 0 }; // input pins for selector buttons (A0=14, A1=15, A2=16, A3=17, Dig0=0) UNO
//const int selector[15] = {A1, A2, A3, A4, A5, A6, A7, A8, A9, A10, A11, A12, A13, A14, A15}; // input pins for selector buttons MEGA
const int selector[5] = {A1, A2, A3, A4, A5}; // input pins for selector buttons MEGA 5 buttons and 5/10 or 15 boxes

// ### don't uses dig1 PIN because it is related to the serial monitor and will causes issues with external periphery (UNO)
// PINS
const int configbutton = 5;   // 5 for Mega; 13 for Uno
const int refillbutton = 3;  // 3 for Mega; for Uno 12 (if only one row of compartments exists: maxrow = 1)
const int powersave_relais_pin = 4;  // 4 for Mega; for Uno 11
// config
const int powersave = 0;  // powersave = 1 -> turnoff power after idle threshold; run at least onetime with 0 to ensure correct data on eeprom

// #### EEPROM structure
//  0          -> 1           : version of this structure : 1 
//  2          -> 3           : max = number of buttons
// 
//  index * 2 + 4             : conveyorItems []
//  index * 2 + max * 2 + 4   : conveyorPrice []

const int EEPROM_version = 1;
 

// ########## INIT VALUES ##########
// #max
// #PREIS
//int conveyorPrice[15] = {600, 600, 600, 600, 600, 600, 600, 600, 600, 600, 600, 600, 600, 600, 600}; // default price  
int conveyorPrice[5] = {700, 700, 700, 700, 700}; // default price  
//int conveyorItems[15] = {1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1};
int conveyorItems[5] = {2, 2, 2, 2, 2};

// ########## INIT COIN ACCEPTOR ##########

const int coinInt = 0; // attach coinInt to interrupt pin 0 = digital pin 2 = digitalPinToInterrupt(2) . (interrupt pin 1 = digital pin 3 = digitalPinToInterrupt(3))

// set the coinsCurrentValue to a volatile float
// volatile as this variable changes any time the Interrupt is triggered
volatile int coinsCurrentValue = 0;
int coinsChange = 0; // a coin has been inserted flag

unsigned long currentMillis = 0;
unsigned long oldMillis = 0;

int pulsecount;

// const int relaisPin = xx; //
//const int relays[10] = {3, 4, 5, 6, 7, 8, 9, 10 , 11, 12}; //uno: 10 boxes
const int relays[15] = {22,24,26,28,30,32,34,36,38,40,42,44,46,48,50}; // mega: 15 boxes
const int poweroffrelais = 4;  // mega: 4; uno: only possible if less 10 compartments; in this case pin 12 recommended; remove this pin from relais list

// ########## END OF INIT ##########

// ########## SETUP ##########

void setup() {

if (debug) {
  Serial.begin(9600); // start serial communication
  Serial.println("Setup gestartet");
  }
 for (int index = 0; index < max; index++) {
   pinMode(selector[index], INPUT_PULLUP);
 }
 
  pinMode(configbutton, INPUT_PULLUP);
  pinMode(refillbutton, INPUT_PULLUP);  
  pulsecount = 0;
  digitalWrite(poweroffrelais, HIGH);
  pinMode (poweroffrelais, OUTPUT);

//           if (!debug) {  
            digitalWrite(homematic_pin, HIGH);
            pinMode (homematic_pin, OUTPUT);
           
//           }
   
  lcd.createChar(0, euro); // build € character

  lcd.begin(16, 2); // set up the LCD's number of columns and rows
  lcd.print("Bitte warten..."); // Print wait message to the LCD
   if (debug) {
  Serial.println("Warten auf Muenzpruefer");
  }
  delay(2000); // don't start main loop until we're sure that the coin selector has started

  // if coinInt goes HIGH (a Pulse), call the coinInserted function
  // an attachInterrupt will always trigger, even if your using delays
  //  attachInterrupt(digitalPinToInterrupt(2), coinInserted, RISING);
  attachInterrupt(coinInt, coinInserted, RISING);

// Relais initialisieren

 for (int index = 0; index < (max * maxrow); index++) {
  digitalWrite(relays[index], HIGH); // Turn OFF
  pinMode(relays[index], OUTPUT);
 }

digitalWrite(powersave_relais_pin,HIGH);
 pinMode(powersave_relais_pin, OUTPUT);

 if ((readEEPROM (0) != EEPROM_version) or (readEEPROM (2) != max))
 {
   if (debug) {
    Serial.print("Init EEPROM ");
    Serial.print("EEPROM_version ");
    Serial.print (readEEPROM (0)); 
    Serial.print("EEPROM_max ");
    Serial.print (readEEPROM (2)); 
    Serial.print("ProgVersion: ");
    Serial.print(EEPROM_version);  
    Serial.print("ProgMax: "); 
    Serial.println(max);   
   }    
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("INIT EEPROM ");
    lcd.setCursor(0, 1);    
    lcd.print(" v ");
    lcd.print( EEPROM_version);
  //  lcd.print(" max ");    
  //  lcd.print( max );   
  if (debug) {
      Serial.print("writeEEprom erforderlich.. ");     
  }      
   writeEEPROMcomplete();
   if (debug) {
    Serial.print("ende IF ");
   }    
 } 
 else
 {
// read EEPROM and update values
if (debug) {
  Serial.println("else:read EEPROM and update values ");
}  
 readEEPROMcomplete();
   
 }
   
 delay(200);
coinsCurrentValue = 0;
  lcd.print("Guthaben loeschen.");
delay(200);
if (debug) {
  Serial.println("Bereit");
}  
  lcd.clear();
  lcd.print("Bereit");
}

// ########## COIN INSERT ##########

// This function is called by interrupt every time we receives a pulse from the coin acceptor
void coinInserted() {
coinsChange = 1; // flag that there has been a coin inserted

  currentMillis = millis();
  int difference = currentMillis - oldMillis;

//  Serial.print("difference: ");
if (debug) {
  Serial.println(difference); 
}
  oldMillis = currentMillis;
  pulsecount++;

// new coin? start to count from beginning....   
if (difference > 134 or difference < 50 ) {
    if (pulsecount > 1) {
       pulsecount = 1;
     }
   }

if (difference < 135) {
switch (pulsecount) {
case 2: coinsCurrentValue = coinsCurrentValue + 10;  
        break; 
case 3: coinsCurrentValue = coinsCurrentValue + 10;  
        break; 
case 4: coinsCurrentValue = coinsCurrentValue + 30;  
        break; 
case 5: coinsCurrentValue = coinsCurrentValue + 50;  
        break; 
case 6: coinsCurrentValue = coinsCurrentValue + 100;  
        break;         
   } // switch
  } // if
    idleTimerMillis = millis();
if (debug) {    
  Serial.print ("(/coinsInserted)neuer Wert: ");
  Serial.println (  coinsCurrentValue);
}
}

// ########## Open Compartement ##########

// Fach Öffnen
void CompartementOpen(int j) {

  
 if (debug) {
  Serial.print("Fachnummer ");
  Serial.print(j+1);
  Serial.println(" öffnen:");
 }
  //hier relais ansteuern
  
  digitalWrite(relays[j], LOW); // Fach öffnen
  delay(1000);
  digitalWrite(relays[j], HIGH); // Relais wieder aus
}

// ########## LCD IDLE ##########

// messages on LCD while idle
void idle() {
  // only run when idlePeriod has been reached since last run and no coins have been thrown
  
  if ((millis() > idleTimerMillis + idlePeriod) && (coinsCurrentValue > 1) ) {
   
     idleTimerMillis = millis();     
     coinsCurrentValue = 0;
     lcd.print("Guthaben loeschen...");
     delay(200);
     lcd.clear();
     lcd.print("Bereit..");
     
  }
  
  
  if ((millis() > idleTimerMillis + idlePeriod) && (coinsCurrentValue < 1) ) {
  // idle display text messages only if not in powersave mode
   if (powersave == 0)
 {   

    idleTimerMillis = millis();
    randomNumber = random(0, 5); // random number from 0 to 4
    lcd.clear();
    if (randomNumber == 0) {
      lcd.setCursor(0, 0);
 // #PREIS
      lcd.print("Preis");
      lcd.setCursor(0, 1);
      lcd.print("");
    } else if (randomNumber == 1) {
      lcd.setCursor(0, 0);
      lcd.print("Bitte passend ");
      lcd.setCursor(0, 1);
      lcd.print("zahlen. ");
    } else if (randomNumber == 2) {
      lcd.setCursor(0, 0);
      lcd.print("Geld einwerfen, dann");
      lcd.setCursor(0, 1);
      lcd.print("Fach waehlen");
    } else if (randomNumber == 3) {
      lcd.setCursor(0, 0);
  // #PREIS
      lcd.print("Willkommen!");
      lcd.setCursor(0, 1);
      lcd.print("   ");
    } else if (randomNumber == 4) {
      lcd.setCursor(0, 0);
      lcd.print("Leergut bitte");
      lcd.setCursor(0, 1);
      lcd.print("zurueck. Danke.");
    }
 
   // clear lcd 1000 msec before idle periode expires and no coins have been thrown
  if ((millis() > idleTimerMillis + idlePeriod - 1000) && (coinsCurrentValue < 1)) {
    lcd.clear();
  }
 
 }
else
 {
// shutdown when powersave == 1
  if (debug) {
  Serial.print("power off..");
  }  
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("power off...");
  delay(2000);
  digitalWrite(poweroffrelais, LOW);
  // power relais not available?... continue..
  idleTimerMillis = millis();
  lcd.clear();
    digitalWrite(poweroffrelais, HIGH);
  lcd.print("Bereit..");
  delay(2000);
 }
  }

}


// ########## SET VALUES ##########

void set_values() {
  if (debug) {
  Serial.println("Gerätekonfiguration gestartet");
  }
  for (int index = 0; index < max; index++) {
     idleTimerMillis = millis();
      // set conveyor items
    int scope = 0;
    int pass = index + 1;
   if (debug) {
    Serial.print("Anzahl");
    Serial.println(pass);
   }    
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Fachnr. ");
    lcd.print(pass);
    lcd.print(" Anzahl ");
    lcd.setCursor(0, 1);
    lcd.print(conveyorItems[index]);
    lcd.print("  ");


    // Setup Warenmenge Button 1 = runterzählen, Button 2 = raufzählen, Button 3 = weiter, Button 4 = alle öffnen, Button 5 = refill (öffne alle leeren & setzte Werte)
if (debug) {
    Serial.print("scope ");
    Serial.println(scope);
    Serial.print("Setup Warenmenge");

    Serial.print("digitalRead(selector[0] ");
    Serial.println (digitalRead(selector[0])) ;
    Serial.print("digitalRead(selector[1] ");
    Serial.println (digitalRead(selector[1])) ;
    Serial.print("digitalRead(selector[2] ");
    Serial.println (digitalRead(selector[2])) ;
    Serial.print("digitalRead(selector[3] ");
    Serial.println (digitalRead(selector[3])) ;
    Serial.print("digitalRead(selector[4] ");
    Serial.println (digitalRead(selector[4])) ;

    Serial.print("while schleife Anzahl def beginnt");
 }    
    while (scope == 0) {
      if (digitalRead(selector[0]) == LOW && conveyorItems[index] > 0) {
        conveyorItems[index] = conveyorItems[index] - 1;
        lcd.setCursor(0, 1);
        lcd.print(conveyorItems[index]);
        lcd.print("   ");
        delay(200);
      } //selector [0]
      if (digitalRead(selector[1]) == LOW && conveyorItems[index] < 10) {
        conveyorItems[index] = conveyorItems[index] + 1;
        lcd.setCursor(0, 1);
        lcd.print(conveyorItems[index]);
        lcd.print("   ");
        delay(200);
      } //selector [1]
      
      if (digitalRead(selector[3]) == LOW) {
if (debug) {
        Serial.print("alle Faecher öffnen ");
}
        //if (maxrow == 1) {
           CompartementOpen(index);          
           delay(500);           
        //   } //maxrow == 1
        if (maxrow > 1) {
           CompartementOpen(index+max);          
           delay(500);
           } //maxrow ==2
        if (maxrow > 2) {
           CompartementOpen(index+max+max);          
           delay(500);           
           } // maxrow ==3
      } //selector [3]
       
      if (digitalRead(selector[4]) == LOW) {
 if (debug) {
        Serial.print("refill.. ");
 }        
           refill();  
           scope = max;           
           delay(200);
      } //selector [4]


      if (digitalRead(selector[2]) == LOW) {
if (debug) {        
        Serial.print("Neue Anzahl: ");
        Serial.println(conveyorItems[index]);
}
        scope++;
        delay(200);
      } //selector [2]
    } // while (scope == 0)

  } //for (int index = 0;

  // Setup Preise Button 1 = runterzählen, Button 2 = raufzählen in 10er Schritten, Button 3 = Ende und Anzeige
if (debug) {
  Serial.print("while schleife Preise beginnt");
}  
  for (int index = 0; index < max; index++) {
    // sest conveyor prices
    int scope = 0;
    int pass = index + 1;
 if (debug) {
    Serial.print("Preis fuer Fach ");
    Serial.println(pass);
 }    
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Fach ");
    lcd.print(pass);
    lcd.print(" Preis");
    lcd.setCursor(0, 1);
    lcd.write(byte(0)); // display EURO symbol
    lcd.print(" ");
    lcd.print(conveyorPrice[index] / 100.00);
    lcd.print("     ");
    while (scope == 0) {
      if (digitalRead(selector[0]) == LOW && conveyorPrice[index] >= 10) {
        conveyorPrice[index] = conveyorPrice[index] - 10;
        displayConfPrice(index);
        delay(200);
      }
      if (digitalRead(selector[0]) == LOW && conveyorPrice[index] > 0 && conveyorPrice[index] < 10) {
        conveyorPrice[index] = 0;
        displayConfPrice(index);
        delay(200);
      }
      if (digitalRead(selector[1]) == LOW && conveyorPrice[index] < 1000) {
        conveyorPrice[index] = conveyorPrice[index] + 10;
        displayConfPrice(index);
        delay(200);
      }
      if (digitalRead(selector[2]) == LOW) {
  if (debug) {
        Serial.print("Neuer Preis:  ");
        Serial.println(conveyorPrice[index] / 100.00);
  }        
        scope++;
        delay(200);
      }
    }
  }

 coinsCurrentValue = 0;
 //displayBalance();
if (debug) {  
  Serial.println("Neue Konfiguration ins Eeprom schreiben...");
}  
  writeEEPROMcomplete();
if (debug) {
  Serial.println("Gerätekonfiguration beendet");
}  
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Gespeichert");
  delay(200);

  displayBalance();
}

// ########## Refill ##########
// open all empty boxes and set their value to maxrow

void refill() {
 if (debug) {
  Serial.print(" Refill gestartet ");
 }  
  lcd.clear();
if (debug) {
      Serial.print("maxrow ");
      Serial.print(maxrow);
}
   
 for (int rindex = 0; rindex < max; rindex++) {
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print("Refill Column ");
     if (debug) {
      Serial.print("Refill Column ");
      Serial.print(rindex+1);
      Serial.print("conyeyorItems[rindex]: ");
      Serial.print(conveyorItems[rindex]);
     } 

      
      lcd.print(rindex+1);

    if ( conveyorItems[rindex] == 0) {
      if (debug) {
          Serial.print("Öffnen ");
      }          
          lcd.setCursor(0, 1);
          lcd.print (rindex+1); 
          CompartementOpen(rindex); 
          conveyorItems[rindex] ++;
          delay(500);
                 }
          
    if (( conveyorItems[rindex] == 1) and (maxrow > 1)) {
       if (debug) {
          Serial.print("rindex plus max öffnen ");
       }          
          lcd.setCursor(0, 1);
          CompartementOpen(rindex + max); 
          lcd.print (rindex+max +1); 
          conveyorItems[rindex] ++;          
          delay(500);
          }

    if (( conveyorItems[rindex] == 2) and (maxrow > 2)) {
        if (debug) {
          Serial.print("rindex plus 10 öffnen ");
        }
          lcd.setCursor(0, 1);
          lcd.print (rindex+max + max +1);
          CompartementOpen(rindex + max + max); 
          delay(500);
          conveyorItems[rindex] ++;
          }
         
    delay(500);
    } 
    
    lcd.clear();
    lcd.setCursor(0, 0);
    if (debug) {
    Serial.println(" Refill beendet. ");
    lcd.print("Refill beendet.");
    }    
    delay(2000);
    lcd.clear();
    displayBalance();
    }


// ########## LCD MESSAGES ##########

void displayBalance() {
  if (debug) {
  Serial.print("Guthaben:  ");
  Serial.println(coinsCurrentValue / 100.00);
  }  
  lcd.clear(); // reset LCD
  lcd.setCursor(0, 0);
  lcd.print("Guthaben");
  lcd.setCursor(0, 1); // set cursor to LCD row 2 column 1 (starting with 0)
  lcd.write(byte(0)); // display EURO symbol
  lcd.print(" ");
  lcd.print(coinsCurrentValue / 100.00); // display current balance
}

void displayPrice(int currentPrice) {
  if (debug) {
  Serial.print("Warenpreis:  ");
  Serial.println(currentPrice / 100.00);
  }  
  lcd.clear(); // reset LCD
  lcd.setCursor(0, 0);
  lcd.print("Preis");
  lcd.setCursor(0, 1); // set cursor to LCD row 2 column 1 (starting with 0)
  lcd.write(byte(0)); // display EURO symbol
  lcd.print(" ");
  lcd.print(currentPrice / 100.00);
  if (coinsCurrentValue > 0) {
    delay(1000);
    displayBalance();
  }
}

void displayEmpty() {
  if (debug) {
  Serial.println("Gewähltes Fach leer");
  }  
  lcd.clear(); // reset LCD
  lcd.setCursor(0, 0);
  lcd.print("Leider leer :(");
  lcd.setCursor(0, 1); // set cursor to LCD row 2 column 1 (starting with 0)
  lcd.print("Bitte neu w" AE "hlen");
  if (coinsCurrentValue > 0) {
    delay(1000);
    displayBalance();
  }
}

void displayConfPrice(int con) {
  lcd.setCursor(0, 1);
  lcd.write(byte(0)); // display EURO symbol
  lcd.print(" ");
  lcd.print(conveyorPrice[con] / 100.00);
  lcd.print("     ");
}

// ########## write eeprom ########## see https://www.youtube.com/watch?v=5x0nUy36Trs
void writeEPROM (int address, int number)
{
     
   EEPROM.write(address, (number >> 8) & 0xFF);
   EEPROM.write(address + 1, number & 0xFF);
}

// ########## read eeprom ##########
int readEEPROM (int address)
 {  
   return (EEPROM.read(address) << 8) + EEPROM.read(address + 1);
  } 

// #########  write eeprom complete #########
void writeEEPROMcomplete ()
 { 
   if (debug) {
    Serial.print("writeEEPROMcomplete..");
    Serial.print("version ");
    Serial.print(EEPROM_version);  
    Serial.print("max "); 
    Serial.print(max);   
   }
   writeEPROM (0, EEPROM_version);
   writeEPROM (2, max);
   if (debug) {
   Serial.print("write eeprom version und max done.");
   }
    for (int index = 0; index < max; index++) {
//    3          -> 4 + 2 * index : conveyorItems []
//    4          -> index * 2 + max * 2 + 4 : conveyorPrice []
   
   if (debug) {
     Serial.print("index * 2 + 4  = ");
     Serial.print(index * 2 + 4);     
     Serial.print("Items [index]: "); 
     Serial.print(conveyorItems[index]);
   }
     writeEPROM (index * 2 + 4, conveyorItems[index]);

if (debug) {
     Serial.print("index * 2 + 2 * max + 4 = ");
     Serial.print(index * 2 + 2 * max + 4);     
     Serial.print("Price [index]: "); 
     Serial.print(conveyorPrice[index]);
}
     writeEPROM (index * 2 + max * 2 + 4, conveyorPrice[index]);
   
    }  

    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("saved. ");
    delay (200);
   

  }

// #########  read eeprom complete #########
void readEEPROMcomplete ()
 { 
 if (debug) {
    Serial.print("readEEPROMcomplete..");
 }    

 //   EEPROM_version = readEEPROM (0);
 //   max            = readEEPROM (2);

if (debug) {    
    Serial.print("version ");
    Serial.print(EEPROM_version);  
    Serial.print("max "); 
    Serial.print(max);   
}

    for (int index = 0; index < max; index++) {
//    3          -> 4 + 2 * index : conveyorItems []
//    4          -> index * 2 + max * 2 + 4 : conveyorPrice []
//        delay (500);
 if (debug) {
     Serial.print("index = ");
     Serial.println(index);     
 
     Serial.print(" index * 2 + 4  = ");
     Serial.print(index * 2 + 4);     
     Serial.print(" Items [index]: "); 
     Serial.print(conveyorItems[index]);
 }     
     conveyorItems[index] = readEEPROM(index * 2 + 4);
  if (debug) {
     Serial.print(" Items new [index]: "); 
     Serial.print(conveyorItems[index]);

     Serial.print(" index * 2 + 2 * max + 4 = ");
     Serial.print(index * 2 + 2 * max + 4);     
     Serial.print(" Price old [index]: "); 
     Serial.print(conveyorPrice[index]);
  }
     conveyorPrice[index] = readEEPROM(index * 2 + max * 2 + 4);
     if (debug) {
     Serial.print(" Price new [index]: "); 
     Serial.print(conveyorPrice[index]);
     Serial.println(" ----- "); 
     }
    }  

    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("loaded. ");
    if (debug) {
    Serial.print(" loaded from EEPROM. "); 
    }    
    delay (200);
   

  }


// ########## MAIN LOOP ##########

void loop() {
  if (digitalRead(configbutton) == LOW) {
if (debug) {
   Serial.print("DigitalRead(configbutton): LOW");
}
   set_values();
   delay(1000);
  }

 if (digitalRead(refillbutton) == LOW) {
if (debug) {   
   Serial.print("DigitalRead(refillbutton): LOW");
}   
   refill();
   delay(1000);
  }  

  idle();

  // check if a coin has been inserted
 if (coinsChange == 1) {
   coinsChange = 0; // unflag that a coin has been inserted
   displayBalance(); // display current balance
  
  }

  // ********** BUTTON PRESSED **********


 // wenn nur 5 Button, dann Index kleiner max
 // for (int index = 0; index <= 4; index++) {
   for (int index = 0; index < max; index++) {    
    if (digitalRead(selector[index]) == LOW) { // button pressed
if (debug) {
      Serial.print("Spalte ");
      Serial.print(index + 1);
      Serial.println(" gedrueckt ");
      Serial.println(" items: ");
      Serial.print(conveyorItems[index]);
      Serial.println("  ");
}      
      if (conveyorItems[index] > 0) {
        if (coinsCurrentValue < conveyorPrice[index]) { // insufficient balance - display price
          displayPrice(conveyorPrice[index]);
          idleTimerMillis = millis();
          delay(200);
        }
        if ((coinsCurrentValue > conveyorPrice[index] - 1) ) { // sufficient balance
          coinsCurrentValue = coinsCurrentValue - conveyorPrice[index]; // reduce balance
          conveyorItems[index]--; // reduce items
    if (debug) {
    Serial.print("update EEPROM conveyorItem ");
    Serial.print(index * 2 + 4, conveyorItems[index]);          
    }
          writeEPROM (index * 2 + 4, conveyorItems[index]);
          if (coinsCurrentValue < 0) {
            coinsCurrentValue = 0;  // correct float rounding error
          }
          delay(200);
   
//          hier gehts weiter - welches Fach öffnen: oben oder unten, der Zähler wurde schon reduziert
//          ist nicht interessant, wenn es für jedes Fach einen Knopf gibt; 
if (debug) {
    Serial.print("conveyorItems[index] ");
    Serial.print(conveyorItems[index]);
    Serial.print("index ");
    Serial.print(index);
    }
          if ( conveyorItems[index] == 0) {
           if (debug) {
          Serial.print("index öffnen ");
           }
           CompartementOpen(index); }
          
          if ( conveyorItems[index] == 1) {
          if (debug) {
          Serial.print("index plus max öffnen ");
          }          
          CompartementOpen(index + max); }

          if ( conveyorItems[index] == 2) {
           if (debug) {
          Serial.print("index plus 2 * max öffnen ");
           }
          CompartementOpen(index + max + max); }

          displayBalance(); // display current balance
          
          idleTimerMillis = millis(); // reset idle timer

         if (!debug) {
//          inform homematic about sold item only if serial.print not used (using digital pin1 which is dedicaded to serial) (issue with uno only)
            delay(1000);
            digitalWrite(homematic_pin, LOW); 
            delay(1000);
            digitalWrite(homematic_pin, HIGH); 
            delay(200);
           }
        }
      } else {
        displayEmpty();
        idleTimerMillis = millis(); // reset idle timer
        delay(200);
      }
    }
    
  }

}
