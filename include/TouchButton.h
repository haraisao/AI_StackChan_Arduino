/*
 */
#pragma once
#include <Arduino.h>
#include <M5Unified.h>

#include <map>
#include <vector>
#include <functional>


class RectArea {
private:
    int x0 = 0;
    int y0 = 0;
    int width = 100;
    int height = 40;
    std::function<void()> _callback;
public:
    RectArea(int x=0, int y=0, int w=100, int h=40):
      x0(x), y0(y),width(w), height(h){}

    bool isInside(int x, int y){
        //M5_LOGI("Touch (%d, %d) (%d, %d, %d, %d)", x, y, x0, y0, x0+width, y0+height);
        return (x > x0 && x < x0+width && y > y0 && y < y0+height);
    }

    bool hasCallback(){
        if(_callback) { return true; }
        return false;
    }

    void registerCallback(std::function<void()> func){
        _callback = func;
    }

    bool update(int x, int y){
        if (hasCallback() && isInside(x, y) ){
            _callback();
            return true;
        }
        return false;
    }
};

enum FlickMotion {
    Right = 1,
    Left, Down, Up
};

class TouchButton {
private:
   int btnHeight = 200;
   int btnWidth = 107;
   int delayTime = 200;
   std::map<String, std::function<void()>> _registeredHandlers;
   std::vector<RectArea> buttons;
   int flickFlag = 0;
   int state;

public:
    TouchButton() {};

    void init(int height=40){
      btnWidth = M5.Display.width() * 0.33;
      btnHeight = M5.Display.height() - height;
    }

    void setDelayTime(int tm) { delayTime = tm; }
    void registerHandler(const char *name, std::function<void()> func) {
        _registeredHandlers[name] = func;
    }

    void createButton(int x, int y, int w, int h, std::function<void()> func);
    void updateOld();
    void update();

    int getState(){ return state; }
    void resetState(){ state = 0; }
};