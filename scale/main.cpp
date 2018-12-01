#include "Arduino.h"
#include "scale.h"

// create a scale device (physical) and inform digital PINS used to create a software serial 
// communication 
Scale scale(D7,D6);
#define SERIAL_BAUD 115200

/** 
 * initialize the program, creating a new logical device, and linking it to the physical device
 */
void setup() {

    Serial.begin(SERIAL_BAUD);

}


/** 
 * main loop of the app 
 * just check connection, and if available, make a reading on the physical connected device 
 * this read triggers internally the logic to verify if the reading need to be replicated
 * to the logical device ... if so, maintain it synchronized automatically 
 * 
 */
void loop() {

    if (scale.getState()) {
        scale.read();
    }
    else  {
        Serial.println("setting up scale...");
        scale.begin();
    }


    yield();

}
