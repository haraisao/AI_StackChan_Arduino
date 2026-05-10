/**
 * @file TouchButton.cpp
 * @author your name (you@domain.com)
 * @brief 
 * @version 0.1
 * @date 2026-05-02
 * 
 * @copyright Copyright (c) 2026
 * 
 */
#include "TouchButton.h"

/**
 * @brief 
 * 
 * @param x 
 * @param y 
 * @param w 
 * @param h 
 * @param func 
 */
void TouchButton::createButton(int x, int y, int w, int h, std::function<void()> func){
  auto btn = RectArea(x, y, w, h);
  btn.registerCallback(func);
  buttons.push_back(btn);
}

/**
 * @brief 
 * 
 */
void TouchButton::update() {
  if(M5.Touch.isEnabled()) {
    auto dt = M5.Touch.getDetail();
    if(dt.isPressed()){
      state=0;
      //M5_LOGI("Touch (%d, %d)", dt.x, dt.y);
      for (std::vector<RectArea>::const_iterator it = buttons.begin(), e = buttons.end(); it != e; ++it) {
        RectArea area = *it;
        if(area.update(dt.x, dt.y)) { 
          delay(delayTime);
          break; 
        }
      }
    }

    if(dt.wasFlickStart()){
      //M5.Display.print("Flick Start");
      M5_LOGI("FlickStart(%d, %d)", dt.x, dt.y);
      if(dt.y < 200){
        flickFlag = 1;
      }
    }
    /*
    if(dt.isFlicking()){
      M5_LOGI("Flicking(%d, %d)(%d, %d)", dt.x, dt.y, dt.distanceX(), dt.distanceY());
    }
      */
    if(flickFlag && dt.wasFlicked()){
      //M5.Display.print("Flick");
      M5_LOGI("Flicked(%d, %d)", dt.distanceX(), dt.distanceY());
      int dx = dt.distanceX();
      int dy = dt.distanceY();
      if (dy < 50 && dy > -50){
        if (dx > 100) {
          M5_LOGI("Flick Right.");
          state = FlickMotion::Right;
        }else if(dx < -100){
          M5_LOGI("Flick Left");
          state = FlickMotion::Left;
        }
      } else if (dx < 30 && dx > -30){
          if (dy > 60) {
          M5_LOGI("Flick Down.");
          state = FlickMotion::Down;
        }else if(dy < -60){
          M5_LOGI("Flick Up");
          state = FlickMotion::Up;
        }
      }
      flickFlag=0;
    }
  }
}


/**
 * @brief 
 * 
 */
void TouchButton::updateOld(){
  if(M5.Touch.isEnabled()) {
    auto dt = M5.Touch.getDetail();
    if(dt.isPressed()){
      //M5_LOGI("Touch (%d, %d), (%d, %d)", dt.x, dt.y, btnWidth, btnHeight);
      if (dt.y > btnHeight){
        if (dt.x < btnWidth){
            M5_LOGI("Call BtnA  (%d, %d)", dt.x, dt.y);
            if (_registeredHandlers.count("BtnA")) {
              _registeredHandlers["BtnA"]();
            }
        }else if(dt.x > M5.Display.width() - btnWidth){
            M5_LOGI("Call BtnC  (%d, %d)", dt.x, dt.y);
            if (_registeredHandlers.count("BtnC")) {
              _registeredHandlers["BtnC"]();
            }
        }else{
            M5_LOGI("Call BtnB (%d, %d), (%d, %d)", dt.x, dt.y, btnWidth, btnHeight);
            if (_registeredHandlers.count("BtnB")) {
              _registeredHandlers["BtnB"]();
            }
        }
        delay(delayTime);
      }
    }
  }
}