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

#include <Motion.h>
#include <GoogleSpeech.h>


String requestGemini(String txt, m5avatar::Avatar *avatar=nullptr);
String requestGeminiInteraction(String txt, String& interactionId,
        m5avatar::Avatar *avatar=nullptr, StackchanSERVO *servo=nullptr);
String responseGeminiMpc(String result, String& interactionId, String func,
        String callId, m5avatar::Avatar *avatar, StackchanSERVO *servo);