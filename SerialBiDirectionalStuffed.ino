

/****************************************************************/
/* 
 * Arduino Due has 64 byte RingBuffer on each USART and bytes are received under
 * interrupt. 
 * 
 * On the Arduino Mega in recent 1.5 versions TX also occurs under interrupt.
 * 
 * This 2 channel FIFO fills up with up to "maxread" bytes each loop.
 * Byte-stuffing allows us to place interesting flags that indicate a change of 
 * channel (or other significant event). We are still subject to the buffering 
 * under RX interrupt but it shouldn't matter too much
 * 
 * The user can opt to use different special bytes to avoid clashes with real data (TODO?)
 * 
 * Other magic data can be stuffed into the stream!
 *   
 */

#include "rb.h"

// Special bytes...
static uint8_t esc_next = 0x80;
static uint8_t cha = 0x81;
static uint8_t chb = 0x82;
static uint8_t bfull = 0x83;

static uint8_t rxc = 0;
static uint8_t chb_not_cha = 0;

// maxread is the controller
// before setting maxread the serial objects must be setup
static uint16_t maxread = 0; // 0 == disabled

// Statistical counters...
static uint32_t ctr_bfifo_full = 0;
static uint32_t ctr_rxa = 0;
static uint32_t ctr_rxb = 0;

/**
 * Add a new byte to the combined buffer.
 * We may need to add up to 3 bytes here!
 * A channel change, an escape byte, the original byte.
 */
static inline void newb(uint8_t b, uint8_t ch, uint8_t change_reqd) {
	uint8_t esc_reqd = (rxc == esc_next || rxc == cha || rxc == chb
			|| rxc == bfull);
	// How much space is there?
	uint16_t space = rb_space_available();
	// How many bytes do we need
	uint16_t breq = 1 + (change_reqd != 0) + (esc_reqd != 0);
	if(space < breq){
		ctr_bfifo_full++;
		if(space > 1)
			rb_add(bfull);
		return;
	}
	if(change_reqd)
		rb_add(ch);
	if(esc_reqd)
		rb_add(esc_next);
	rb_add(b);
}


// maxread is the controller
// before setting maxread the serial objects must be setup
static inline void fstuff(void) {
	for (uint16_t i = 0; i < maxread; i++) {
		if (Serial1.available()) {
			// data on channel A
			ctr_rxa++;
			rxc = (uint8_t) Serial1.read();
			Serial.print("*");
			// channel swap to channel A required if current channel is B
			newb(rxc, cha, chb_not_cha);
		} else if (Serial2.available()) {
			// data on channel B
			ctr_rxb++;
			rxc = (char) Serial2.read();
			// channel swap to channel B required if current channel is A
			newb(rxc, chb, (!chb_not_cha));
		} else
			return;
	}

}

String inputString = "";         // a string to hold incoming data
boolean inputStringComplete = false;  // whether the string is complete
unsigned long last;
static long collectPeriod = 1447;
boolean human_readable = true;

void setup() {
    pinMode(13, OUTPUT);
    digitalWrite(13, LOW);
    // initialize serial:
    Serial.begin(115200);
    Serial.println("SerialBiDirectionalStuffed v1.1");
    // reserve 200 bytes for the inputString:
    inputString.reserve(200);
    last = millis();
    channels_init(57600);
    digitalWrite(13, LOW);
    maxread = 100;
}

void channels_init(uint32_t baud) {
    // why does this hang?
    //if(Serial1 != NULL)
      //Serial1.end();
    //if(Serial2 != NULL)
      //Serial2.end();
    Serial1.begin(baud);
    Serial2.begin(baud);
}

void loop() {
  // print the string when a newline arrives:
  if (inputStringComplete) {
    processCommand();
    // clear the string:
    inputString = "";
    inputStringComplete = false;
  }
  // load byte-stuffed data into FIFO
  fstuff();
  // output buffered data
  // TODO this should wait for buffer filling or a quiet period if possible
  if(millis() - last > collectPeriod){
    if(human_readable)
      showBuffer();
    else
      mr_buffer();
    last = millis();
  }  
}

void serialEvent() {
  // Handle user input control...
  while (Serial.available()) {
    // get the new byte:
    char inChar = (char)Serial.read(); 
    // add it to the inputString:
    inputString += inChar;
    // if the incoming character is a newline, set a flag
    // so the main loop can do something about it:
    if (inChar == '\n') {
      inputStringComplete = true;
    } 
  }
}

void dumpbuf(uint8_t *b, uint16_t count){
  for(int i=0;i<count;i++){
    Serial.print(b[i], HEX);
    Serial.print(" "); 
  }
}

// machine readable buffer dump
void mr_buffer(void) {
  if(maxread){
    while(favailable()){
      uint8_t b = fremove();
      Serial.write(b);
    }
  }  
}
// verbose debug output routine...
void showBuffer(void){
  // Human-readable output: 
  // TODO: Machine-readable output
  Serial.print(last); 
  Serial.println(" ROUTINE... ");
  //
  Serial.print("DATA ");
  if(maxread){
    while(favailable()){
      uint8_t b = fremove();
      Serial.print(b, HEX);
      Serial.print(" "); 
    }
  }
  Serial.println(" #");
}

/*
 * Commands from PC
 */
void processCommand(void) {
    static char buf[] = {"0000000000"};
    Serial.print("USERINPUT: ");
    inputString.trim(); 
    Serial.println(inputString);
    char cmd = inputString[0];
    cmd = toupper(cmd);
    inputString = inputString.substring(1);
    inputString.trim();
    boolean setter = false;
    if(inputString[0] == '='){
        setter = true;
        inputString = inputString.substring(1);
        inputString.trim();
    }
    if(cmd == 'M'){
        if(setter){
            inputString.toCharArray(buf, sizeof(buf));
            uint32_t val = atol(buf);
            // TODO - validate
            maxread = val;
        }
        Serial.print("maxread = ");
        Serial.println(maxread);
        return;
    }
    if(cmd == 'C'){
        Serial.print("collectPeriod = ");
        Serial.println(collectPeriod);
        return;
    }
    if(cmd == 'R'){
        Serial.print("Received = ");
        Serial.print(ctr_rxa);
        Serial.print("|");
        Serial.println(ctr_rxb);
        return;
    }
    if(cmd == 'E'){
        if(Serial1)
          Serial1.end();
        if(Serial2)
          Serial2.end();
    }
    if(cmd == 'H'){
        if(setter) { 
            inputString.toCharArray(buf, sizeof(buf));
            uint32_t val = atol(buf);
            human_readable = (val != 0);
        }
        Serial.print("human_readable = ");
        Serial.println(human_readable);
    }
    // B = Baud
    if(cmd == 'B'){
        // Set baud with "B = nnnn"
        // 300, 1200, 2400, 4800, 9600, 14400, 19200, 28800, 38400, 57600, or 115200
        if(inputString[0] == '='){
            inputString = inputString.substring(1);
            inputString.trim();
            // expect a number here
            char buf[] = {"0000000000"};
            inputString.toCharArray(buf, sizeof(buf));
            uint32_t newbaud = atol(buf);
            switch(newbaud){
              case 300:
              case 1200:
              case 2400:
              case 4800:
              case 9600:
              case 14400:
              case 28800:
              case 38400:
              case 57600:
              case 115200:
                  Serial.println("new baud rate OK");
                  channels_init(newbaud);
                  break;
              default:
                  Serial.println("new baud rate bad");
                  break;
            }
            return;
        }
        Serial.println("TODO report current baud rate");
        return;
      }
  // TODO 
  // TODO enable 
  // TODO show counters
  // TODO 
  
}


