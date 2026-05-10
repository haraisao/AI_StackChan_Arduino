/*

*/
#define MOTION_LIB
#include "Motion.h"

static int isMoving=0;

void refuseMotion(StackchanSERVO *servo) {
  if (!servo) return;
  if (isMoving) return;
  isMoving=1;
  servo->moveXY(90, 90, 500);
  servo->moveX(80, 200);
  servo->moveX(100, 400);
  servo->moveX(80, 400);
  servo->moveX(100, 400);
  servo->moveXY(90, 90, 200);
  delay(500);
  isMoving=0;
}

void nodMotion(StackchanSERVO *servo, int count) {
  if (!servo) return;
  if (isMoving) return;
  isMoving=1;
#if 1
  servo->moveXY(90, 90, 500);

  for(int i=0; i< count; i++){
    servo->moveY(80, 400);
    servo->moveY(100, 800);
    delay(100);
  }
  servo->moveXY(90, 90, 500);
#endif
  delay(500);
  isMoving=0;
}

void upMotion(StackchanSERVO *servo, int sigV, int sigH) {
  if (!servo) return;
  if (isMoving) return;
  isMoving=1;
  servo->moveDeltaXY(0,5*sigV, 200);
  servo->moveDeltaXY(10*sigH,5*sigV, 200);
  delay(300);
  servo->moveDeltaXY(-10*sigH, -10*sigV, 300);
  //delay(500);
  isMoving=0;
}

void myMotion(StackchanSERVO *servo, int m) {
  if (!servo) return;
  switch(m){
    case 0:
      refuseMotion(servo);
      break;
    case 1:
      nodMotion(servo);
      break;
    case 2:
      upMotion(servo, -1, -1);
      break;
    case 3:
      upMotion(servo, 1, -1);
      break;
    case 4:
      upMotion(servo, -1, 1);
      break;
    case 5:
      upMotion(servo, 1, 1);
      break;
    default:
      servo->moveXY(90,90, 500);
  }
  return;
}