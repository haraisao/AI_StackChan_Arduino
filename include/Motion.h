#pragma once

#include <Arduino.h>
#include <M5Unified.h>
#include <M5CoreS3.h>

#include <Stackchan_servo.h>
#include <Utils.h>


#ifndef MOTION_LIB
extern int isMoving;
#endif

#define NUM_MOTIONS 6

void refuseMotion(StackchanSERVO *servo);
void nodMotion(StackchanSERVO *servo, int count=2);
void upMotion(StackchanSERVO *servo, int sigV, int sigH);

void myMotion(StackchanSERVO *servo, int m);
