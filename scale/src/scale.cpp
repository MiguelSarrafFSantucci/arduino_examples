#include "config.h"

#define __MODULE_NAME__ "SCALE"

#include "scale.h"
#include <cmath> 

#define FLASH_STATE 
#define FLASH_ERROR 

Scale::Scale() {
    LOGDEV("initializing scale internal structure");
    for (int i =0; i < MEASUREMENT_BUFFER_SIZE; i++) {
        this->measurements[i] = new Measurement();
        if (!this->measurements[i]) {
            LOGDEV("invalid measurement created");
        }
    }
    this->index = 0; // no data 

    _lastUpdate = 0;
    _lastReadFromDevice = 0;
    // _mx = new Measurement();
}

// default pins D7 = RX, D8 = TX
//
Scale::Scale(int _rx, int _tx) : Scale() {

    this->serial_conn = new SoftwareSerial(_rx,_tx,true,256);
    this->index = 0;
}

Scale::~Scale() {
    for (int i =0; i < MEASUREMENT_BUFFER_SIZE; i++) {
        if (this->measurements[i]) {
            delete this->measurements[i];
        }
        this->measurements[i] = NULL;
    }

    // remove serial object
    if (this->serial_conn) {
        delete this->serial_conn;
        this->serial_conn = NULL;
    }
}


/** 
 * push  a new measurement to the cache, cycling when needed
 */
int Scale::add(Measurement* m) {

    int retvalue = 0 ;

    if (!m || (m && m->weight_raw == -1 && m->weight_net == -1 && m->tare == -1)) {
        // invalid entry 
        LOGDEV("invalid entry ... discarding");
        if (m) {
            m->dump();
        }
    } else {
        this->samples++;

        if (m) {
            LOG("T:%.2f N:%.2f TARE:%.2f -- CALC WEIGHT:%.2f", m->weight_raw, m->weight_net, m->tare, m->weight());
        } else {
            LOG("m is NULL");
        }

        this->index++;
        // check where to put next read on the buffer
        if (this->index >= MEASUREMENT_BUFFER_SIZE) { 
            this->index = 0;
        } 
    }

    return retvalue;

}

Measurement* Scale::getCurrent() {
    if (this->index >= 0) {
        return this->measurements[this->index];
    }
    return NULL;
}

/** 
 * process read temperature string and create a new object
 * to hold this sample, adding it to the scale buffer 
 */
int Scale::process(char *buffer) {    
    Measurement *m = this->measurements[this->index];
    if (m) {
        LOGDEV("parsing ...");
        m->parse(buffer, m);
        return this->add(m);
    }
    else {
        LOGDEV("invalid read from the device = %s", buffer);
    }

    return 0;
}

/**
 * shows the content of read buffer in ASC and HEX
 */
void Scale::dump(char* buffer, int size) {
    // dump output 
    Serial.print(buffer);
    // calculate space to col 80 
    for (int i=size; i <= 45; i++) {
        Serial.print(" ");
    }
    Serial.print(" | ");
    // dump hex code 
    for (int i=0; i < size; i++) { 
        byte b = buffer[i];
        Serial.print(b < 0x10 ? " 0" : " ");
        Serial.print(b, HEX);
    }
    Serial.println("");
    yield();
}

/** 
 * read serial line and fill the read buffer to be processed
 * looking for dataframe finish.
 * possible the first dataframe is junked and should be ignored 
 * since the scale is in continous transmission mode
 */ 
int Scale::read() {    
    unsigned long ts = millis();
    int ret = 0 ;
    if (this->state && this->serial_conn->available() > 0) { 
        // read serial input while there's any data 
        char b;
        
        _buffer[0] = '\0';
        int i=0;
        bool packet = false; 
        while (i < 39 && !packet) {
            for (; (b = this->serial_conn->read()) != 0xFF && i < (DATA_BUFFER_SIZE-1) && !packet;i++) {
                _buffer[i] = b;
                packet =  (b == 0x0A);
            }
            if (b == 0xFF) {
                yield();
            }
        }
        if ((i-1) >= 0 && _buffer[(i-1)] == 0x0A) {
            _buffer[(i-1)] = '\0';
            i--;
        }
        if ((i-1) >= 0 && _buffer[(i-1)] == 0x0D) {
            _buffer[(i-1)] = '\0';
            i--;
        }        
        _buffer[i] = '\0';

#ifdef DEBUG
        this->dump(_buffer, i);
#endif
        yield();
        ret = this->process(_buffer);

        LOG("[ %d ]", i);
        _lastReadFromDevice = ts; 
    } else {
        if (!this->state) {
            LOGDEV("need to initialize the serial first");
        } else  {
            // too long time that doesn't receive information from serial 
            if ((ts - _lastReadFromDevice) > (TIMEOUT_READING_FROM_SCALE)) 
                FLASH_ERROR;
        }
    }

    return ret;
}

void Scale::begin() {
    this->serial_conn->begin(9600);
    this->state = 1;
}

int Scale::getState() {
    return this->state;
}

