#include <Arduino.h>
#include <M5Unified.h>

static int Volume=40;

void setVolume(int v){
    Volume=v;
}

void resetVolume() {
    M5.Speaker.setVolume(Volume);
}

void beep(int typ){
    M5.Speaker.begin();
    M5.Speaker.setVolume(Volume);
    switch(typ){
        case 0:
            M5.Speaker.tone(1500, 200);
            delay(200);
            M5.Speaker.tone(1000, 200);
            delay(200);
            break;
        case 1:
            M5.Speaker.tone(1000, 200);
            delay(200);
            M5.Speaker.tone(1500, 200);
            delay(200);
            break;
        case 2:
            M5.Speaker.tone(1500, 200);
            delay(400);
            M5.Speaker.tone(1500, 200);
            delay(200);
            break;
        default:
            M5.Speaker.tone(1500, 200);
            delay(200);
            break;
    }
    //M5.Speaker.stop();
    M5.Speaker.end();
}