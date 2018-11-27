#include "config.h"

#define __MODULE_NAME__ "MEASUREMENTS"

#include "measurement.h"

void Measurement::copy(Measurement *m) {
    if (m) {
        this->ts = m->ts;
        this->weight_raw = m->weight_raw;
        this->weight_net = m->weight_net;
        this->tare = m->tare;
        this->plate = m->plate;
        this->bag = m->bag;
        this->person = m->person;
        this->update = m->update;
    } else {
        LOGDEV("invalid M");
    }
}

Measurement::Measurement(Measurement *m) {
    this->copy(m);
}

Measurement::Measurement() {
    this->reset();
}

void Measurement::reset() {
    this->ts = millis();
    this->weight_raw = -1.0; // -1 is a start value used to check if the value on the scale is real or reset by program
    this->weight_net = -1.0;
    this->tare = -1.0;
    // flags
    this->plate = 0;   // this measurement is related to the disposal of a new plate
    this->bag = 0;     // this measurement is related to removal of a trash bag
    this->person = 0;  // this measurement is related to measure the mass of a person (or heavy item)
    this->update = 0;  // some other distinct event that should trigger saving on the platform 
}

Measurement::~Measurement() {

}

/**
 * indicate if this sample measurement has some active flag 
 * that indicates that it should be send to the cloud
 */
boolean Measurement::isFlagged() {
    return (this->plate || this->bag || this->person || this->update); 
}

/**
 * parse a dataframe directly read from serial line 
 * into a new object of this structure, returning a
 * new Measurement object when OK or NULL when something 
 * was wrong on processing
 * 
 */
Measurement* Measurement::parse(char *buffer, Measurement *obj) {
    // parse data on a new measurement 
    Measurement* m = NULL; 

    if (strlen(buffer) < 36) {
        // wrong size of buffer
        LOG("size of buffer is %d", strlen(buffer));    
        ERROR("size of buffer %d ... expected 36 or 37", strlen(buffer));
        m = NULL;
    } else {
        LOG("reformatting dataframe to parse");
        for (byte i=0; i < strlen(buffer); i++) {
            if (buffer[i] == ',')
                buffer[i] = '.';
            //yield();
        }

        if (obj)  {
            m = obj; // reuse previously created object // to avoid memory fragmentation
            LOGDEV("reuse previously created object");
            m->reset();
        } else {
            m = new Measurement();
            LOGDEV("create new object");
        }

        // just an adjustement to read partial information from the device when just one character is missing
        // the point here is that when a weight is placed over the scale, it takes some time to adjust 
        // the final read (stabilize) and sometimes when this happens, one byte is "lost" in communication,
        // since the previously data is been send to the platform (there's just one core in the ESP)
        // so no multithreading is possible ...
        // possible solutions are use the more advanced ESP32 (multicore) or other more potent device
        //
        if (strlen(buffer) == 36) 
            sscanf(buffer, PROTOCOL_FORMAT2, &(m->weight_raw), &(m->weight_net), &(m->tare));
        else
            sscanf(buffer, PROTOCOL_FORMAT, &(m->weight_raw), &(m->weight_net), &(m->tare));

        LOGDEV("parsed");
    }

    return m;
}

void Measurement::dump() {
    char aux[255];
    snprintf(aux, 255, "W-RAW:%.2f W-NET:%.2f TARE:%.2f PLATES:%d BAGS:%d PERSON:%d UPDATE:%d", 
        this->weight_raw, this->weight_net, this->tare, this->plate, this->bag, this->person, this->update);
    aux[254] = '\0';
    Serial.println(aux);
}

void Measurement::payload(char* aux, int size) {
    snprintf(aux, size, "{\"raw\":%.2f, \"net\":%.2f, \"tare\":%.2f,  \"flag_plate\":%d, \"flag_bag\":%d, \"flag_person\":%d}", 
        this->weight_raw, this->weight_net, this->tare, this->plate, this->bag, this->person);
    aux[size-1] = '\0';
}

float Measurement::weight() {
    float w = 0.0;
    w = (weight_net ? weight_net : weight_raw);
    if (tare) 
        w = w + tare;
    return w;
}
