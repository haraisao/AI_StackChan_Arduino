#pragma once
#include <Arduino.h>
#include <M5Unified.h>
#include <ArduinoJson.h>
#include <Avatar.h>
#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <HTTPClient.h>
#include <time.h>
#include "mbedtls/base64.h"
#include "Utils.h"

//#define SAMPLE_RATE 16000
//#define FRAME_SIZE  320

void speakGoogleTTS(String text, m5avatar::Avatar *avatar=nullptr);
void executeGoogleTTS(String msg,  m5avatar::Avatar *avatar=nullptr);
String doGoogleASR(int16_t *audio_data, int audio_len);
String requestGoogleAsr(unsigned char* b64_buffer, size_t b64_size);
String executeGoogleAsr(int max_sec, m5avatar::Avatar *avatar=nullptr);
