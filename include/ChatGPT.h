/*
 */
#pragma once
#include <Arduino.h>
#include <M5Unified.h>
#include <time.h>

#include <Avatar.h>
#include <ArduinoJson.h>
#include <WiFiClientSecure.h>
#include <HTTPClient.h>

String requestChatGPT(String txt, m5avatar::Avatar *avatar=nullptr);
