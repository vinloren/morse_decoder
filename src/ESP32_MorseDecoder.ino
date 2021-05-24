/*
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy of
 * this software and associated documentation files (the "Software"), to deal in
 * the Software without restriction, including without limitation the rights to
 * use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies
 * of the Software, and to permit persons to whom the Software is furnished to do
 * so, subject to the following conditions:
 * 
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 * 
 */
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

const byte interruptPin = 25;
volatile int pininterruptCounter = 0;
int pinnumberOfInterrupts = 0;

const byte startPin = 26;
volatile int startinterruptCounter = 0;
int startnumberOfInterrupts = 0;
bool RUNNING = false;

#define AUDIO_IN_PIN 35
#define soglia 2140    // soglia adc per eliminare rumore di fondo microfono

const int markPin = 17;
const int spacePin = 16;

#define OLED_SDA 21
#define OLED_SCL 22

int passo = 80000;    // passo 15wpm
int analogValue;

char print_buf[300];
char rcvBuf[32] = {0};
char rcvChar = 0;
int bytecount = 0;
int lastPush = 0;
char decode_Char;
char CodeBuffer[8] = {0};

portMUX_TYPE mux = portMUX_INITIALIZER_UNLOCKED;
portMUX_TYPE mux1 = portMUX_INITIALIZER_UNLOCKED;

void blink (){
    digitalWrite (markPin, LOW);  
    delay(500); 
    digitalWrite (markPin, HIGH); 
    digitalWrite (spacePin, LOW);  
    delay(500); 
    digitalWrite (spacePin, HIGH); 
}

void handleInterrupt() {
  portENTER_CRITICAL_ISR(&mux);
  pininterruptCounter++;
  portEXIT_CRITICAL_ISR(&mux);
}

void handleStart() {
  portENTER_CRITICAL_ISR(&mux1);
  startinterruptCounter++;
  portEXIT_CRITICAL_ISR(&mux1);
} 

Adafruit_SSD1306 display(128, 32, &Wire, -1);
int count = 0;

void initializeDisplay() {
  pinMode (markPin, OUTPUT);
  pinMode (spacePin, OUTPUT);
  blink();
  Serial.println("Initializing display...");

  Serial.println ("Scanning I2C device...");
  Wire.begin();
  for (byte i = 0; i <64; i++)
  {
    Wire.beginTransmission (i);
    if (Wire.endTransmission () == 0)
    {
      Serial.print ("Address found->");
      Serial.print (" (0x");
      Serial.print (i, HEX);
      Serial.println (")"); 
    }
  }
  Wire.begin(OLED_SDA, OLED_SCL);
  if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
    Serial.println("Failed to initialize the dispaly");
    for (;;);
  }
  sprintf(print_buf,"WPM: %d", 6000000/passo/5);
  showOled(print_buf);
  delay(2000);
  Serial.println("Display initialized");
}

void showOled(char* s) {
  display.clearDisplay();
  display.setTextSize(2); 
  display.setTextColor(WHITE);
  display.setCursor(0, 0);
  display.println(s);
  display.display();  
}

void showData(char c) {
  rcvBuf[bytecount] = c;
  bytecount++;
  if(bytecount > 20){
    for(int i=0;i<20;i++)
      rcvBuf[i] = rcvBuf[i+1];
    bytecount--;
  }
  rcvBuf[20] = 0;
  showOled(rcvBuf);
}

int testMark() {
  while(analogRead(AUDIO_IN_PIN)<soglia);
  long int inizio = micros();
  while(true) {
    int cnt = 0;
    for(int i=0;i<50;i++) {
      if(analogRead(AUDIO_IN_PIN)>soglia)
        cnt++;  
    }
    if(cnt>0)
      digitalWrite (markPin, LOW); 
    else {
      digitalWrite (markPin, HIGH);
      break;
    } 
  }
  return (int)(micros()-inizio);
}

int testSpace() {
  long int inizio = micros();
  while(true) {
    int cnt = 0;
    for(int i=0;i<50;i++) {
      if(analogRead(AUDIO_IN_PIN)>soglia)
        cnt++;  
    }
    if(micros()>(inizio+8*passo)) // time out per ultimo char a chiusura trasmissione
      break;
    if(cnt<1)
      digitalWrite (spacePin, LOW);  
    else {
      digitalWrite (spacePin, HIGH);  
      break;
    }
  }
  return (int)(micros()-inizio);
}

void insertPuntoLinea(char x) {
  int i; 
  for(i=0;i<7;i++) {
    if(CodeBuffer[i] == 0)
       break; 
  }
  CodeBuffer[i] = x;
  CodeBuffer[i+1] = 0; 
}

void CodeToChar() { // translate cw code to ascii character//
  char decode_char = '{';
  if (strcmp(CodeBuffer,".-") == 0)      decode_char = char('a');
  if (strcmp(CodeBuffer,"-...") == 0)    decode_char = char('b');
  if (strcmp(CodeBuffer,"-.-.") == 0)    decode_char = char('c');
  if (strcmp(CodeBuffer,"-..") == 0)     decode_char = char('d'); 
  if (strcmp(CodeBuffer,".") == 0)       decode_char = char('e'); 
  if (strcmp(CodeBuffer,"..-.") == 0)    decode_char = char('f'); 
  if (strcmp(CodeBuffer,"--.") == 0)     decode_char = char('g'); 
  if (strcmp(CodeBuffer,"....") == 0)    decode_char = char('h'); 
  if (strcmp(CodeBuffer,"..") == 0)      decode_char = char('i');
  if (strcmp(CodeBuffer,".---") == 0)    decode_char = char('j');
  if (strcmp(CodeBuffer,"-.-") == 0)     decode_char = char('k'); 
  if (strcmp(CodeBuffer,".-..") == 0)    decode_char = char('l'); 
  if (strcmp(CodeBuffer,"--") == 0)      decode_char = char('m'); 
  if (strcmp(CodeBuffer,"-.") == 0)      decode_char = char('n'); 
  if (strcmp(CodeBuffer,"---") == 0)     decode_char = char('o'); 
  if (strcmp(CodeBuffer,".--.") == 0)    decode_char = char('p'); 
  if (strcmp(CodeBuffer,"--.-") == 0)    decode_char = char('q'); 
  if (strcmp(CodeBuffer,".-.") == 0)     decode_char = char('r'); 
  if (strcmp(CodeBuffer,"...") == 0)     decode_char = char('s'); 
  if (strcmp(CodeBuffer,"-") == 0)       decode_char = char('t'); 
  if (strcmp(CodeBuffer,"..-") == 0)     decode_char = char('u'); 
  if (strcmp(CodeBuffer,"...-") == 0)    decode_char = char('v'); 
  if (strcmp(CodeBuffer,".--") == 0)     decode_char = char('w'); 
  if (strcmp(CodeBuffer,"-..-") == 0)    decode_char = char('x'); 
  if (strcmp(CodeBuffer,"-.--") == 0)    decode_char = char('y'); 
  if (strcmp(CodeBuffer,"--..") == 0)    decode_char = char('z'); 
  
  if (strcmp(CodeBuffer,".----") == 0)   decode_char = char('1'); 
  if (strcmp(CodeBuffer,"..---") == 0)   decode_char = char('2'); 
  if (strcmp(CodeBuffer,"...--") == 0)   decode_char = char('3'); 
  if (strcmp(CodeBuffer,"....-") == 0)   decode_char = char('4'); 
  if (strcmp(CodeBuffer,".....") == 0)   decode_char = char('5'); 
  if (strcmp(CodeBuffer,"-....") == 0)   decode_char = char('6'); 
  if (strcmp(CodeBuffer,"--...") == 0)   decode_char = char('7'); 
  if (strcmp(CodeBuffer,"---..") == 0)   decode_char = char('8'); 
  if (strcmp(CodeBuffer,"----.") == 0)   decode_char = char('9'); 
  if (strcmp(CodeBuffer,"-----") == 0)   decode_char = char('0'); 

  if (strcmp(CodeBuffer,"..--..") == 0)  decode_char = char('?'); 
  if (strcmp(CodeBuffer,".-.-.-") == 0)  decode_char = char('.'); 
  if (strcmp(CodeBuffer,"--..--") == 0)  decode_char = char(','); 
  if (strcmp(CodeBuffer,"-.-.--") == 0)  decode_char = char('!'); 
  if (strcmp(CodeBuffer,".--.-.") == 0)  decode_char = char('@'); 
  if (strcmp(CodeBuffer,"---...") == 0)  decode_char = char(':'); 
  if (strcmp(CodeBuffer,"-....-") == 0)  decode_char = char('-'); 
  if (strcmp(CodeBuffer,"-..-.") == 0)   decode_char = char('/'); 

  if (strcmp(CodeBuffer,"-.--.") == 0)   decode_char = char('('); 
  if (strcmp(CodeBuffer,"-.--.-") == 0)  decode_char = char(')'); 
  if (strcmp(CodeBuffer,".-...") == 0)   decode_char = char('_'); 
  if (strcmp(CodeBuffer,"...-..-") == 0) decode_char = char('$'); 
  if (strcmp(CodeBuffer,"...-.-") == 0)  decode_char = char('>'); 
  if (strcmp(CodeBuffer,".-.-.") == 0)   decode_char = char('<'); 
  if (strcmp(CodeBuffer,"...-.") == 0)   decode_char = char('~'); 
  if (strcmp(CodeBuffer,".-.-") == 0)    decode_char = char('a'); // a umlaut
  if (strcmp(CodeBuffer,"---.") == 0)    decode_char = char('o'); // o accent
  if (strcmp(CodeBuffer,".--.-") == 0)   decode_char = char('a'); // a accent
  if (decode_char != '{') {
    showData(decode_char);
    Serial.print(decode_char);
  }
}

void setup() {
  Serial.begin(115200);
  initializeDisplay();
  
  pinMode(interruptPin, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(interruptPin), handleInterrupt, FALLING);
  pinMode(startPin, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(startPin), handleStart, FALLING);
  Serial.println("Pin 25 set speed/shift");
  Serial.println("Pin 26 push to start");
  
}

void loop() {
    if(startinterruptCounter>0) {
      portENTER_CRITICAL(&mux1);
      startinterruptCounter--;
      portEXIT_CRITICAL(&mux1);
      // debounce push button
      if(millis()-lastPush > 350) {
        //Serial.println("Premuto run"); 
        startnumberOfInterrupts++;
        lastPush = millis();
        if(!RUNNING) {
          blink();
          RUNNING = true;
        }     
        else
          RUNNING = false;
      }
    }

    if(pininterruptCounter>0){
      portENTER_CRITICAL(&mux);
      pininterruptCounter--;
      portEXIT_CRITICAL(&mux);

      // debounce push button
      if(millis()-lastPush > 350) {
        pinnumberOfInterrupts++;
        lastPush = millis();
        // in base a pininterruptCounter vai a impostare shift mark freq, space freq, speed (45.45, 50, 75 baud)
        // in modo che abbiamo counter timer 22000 -> 45.45, 20000 -> 50, 13333 -> 75 bauds
        switch (pinnumberOfInterrupts%3) {
          case 0:
            passo = 80000;  // 15wpm
            break;
          case 1:
            passo = 48000;  // 25wpm
            break;
          case 2:
            passo = 30000;  // 40wpm
            break;
      }
      int bs = 6000000/passo/5;
      sprintf(print_buf,"WPM: %d", bs);
      showOled(print_buf);
      Serial.println("Pemuto speed");
     }
    }
    if(!RUNNING)
      return;

    /**** decode morse code in RUNNING state only ****/     
    int markL = testMark();
    //sprintf(print_buf,"Mark len: %d",markL);
    //Serial.println(print_buf);
    float res = (float)markL/(float)(passo*3);
    if(res > 0.75) {
      insertPuntoLinea('-'); 
    }
    else {
      insertPuntoLinea('.');
    }
    //Serial.print(CodeBuffer);
    int spaceL = testSpace();

    //sprintf(print_buf,"Space len: %d",spaceL);
    //Serial.println(print_buf);
    res = (float)spaceL/(float)(passo*7);
    if(res > 0.94) {
      // fine parola
      CodeToChar();
      CodeBuffer[0] = 0;
      showData(' ');
      Serial.print(" ");
    }
    else {
      res = (float)spaceL/(float)(passo*3);
      if(res > 0.94) {
          // fine carattere
        CodeToChar();
        CodeBuffer[0] = 0;
      }
    }
}
    
