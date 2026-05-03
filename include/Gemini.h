#pragma once
#include <Arduino.h>
#include <M5Unified.h>
#include <Avatar.h>
#include <ArduinoJson.h>
#include <WiFiClientSecure.h>
#include <HTTPClient.h>
#include <time.h>
#include "GoogleSpeech.h"

String requestGemini(String txt, m5avatar::Avatar *avatar=nullptr);
String requestGeminiInteraction(String txt, String& interactionId, m5avatar::Avatar *avatar=nullptr);
