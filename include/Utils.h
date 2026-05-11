/*
 */
#pragma once
#include <Arduino.h>
#include <M5Unified.h>
#include <M5CoreS3.h>
#include <esp_camera.h>

#include <Avatar.h>
#include <map>
#include <time.h>

#include <WiFi.h>
#include <LittleFS.h>
#include <SD.h>

#include <WiFiClientSecure.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <mbedtls/base64.h>

#define SAMPLE_RATE 16000
#define FRAME_SIZE  320


void adjustTime();
String getCurrentTime(int style);
bool mountLitteFs();
bool mountSd(int trial=3);
String readRootCA(String fname);
String loadFile(String fname);
bool isFileExists(String path);
File getFileDescriptor(String path);

bool listDir(fs::FS &fs, const char* dirname, std::map<String, std::vector<String>> &flist);
int createDir(fs::FS &fs, const char *path);
void saveFile(String fname, const char *contents);
void removeFile(String path);
void splitString(String src, std::vector<String>& delims, std::vector<String>& slist);
int cutString(String src, int len, std::vector<String>& slist);

int loadJson(String fname, JsonDocument& doc);

void setVolume(int v);
void resetVolume();
void beep(int typ);

void setupWifi(String conf_file);

int convertToInt(uint8_t *buff);
int convertToShort(uint8_t *buff);
void talkWav(m5avatar::Avatar *avatar, uint8_t *wav_data,
           unsigned long start_time, int duration_ms, float MAX_RMS);

String getApiKey(String key);
int getSpeechFromMic(int16_t* audio_data, int max_frame);
void showRAM();
void sendHttpPostRequest(String url, String rootCA, String postData, String apikey);

unsigned char *b64EncodeAudio(int16_t* audio_data, int audio_len, int *outlen);

uint8_t *serializeJsonSpiram(JsonDocument doc, size_t *size);

int checkClientRead(WiFiClientSecure *client, int timeout);
String readHttpHeader(WiFiClientSecure *client, int *code, int *contentLen, bool *chunk_flag);
size_t sendRequestBody(WiFiClientSecure *client, unsigned char *buffer, int total_length);
uint8_t *readResponseBody(WiFiClientSecure *client, int contentLen, int *len);

struct SpiRamAllocator: public ArduinoJson::Allocator {
  void* allocate(size_t size) override {
    void *ptr = heap_caps_malloc(size, MALLOC_CAP_SPIRAM);
    if (!ptr) {
        Serial.printf("PSRAM allocation failed: %d bytes\n", size);
        vTaskDelay(pdMS_TO_TICKS(100));
        ptr = heap_caps_malloc(size, MALLOC_CAP_SPIRAM);
        if (!ptr) {
            Serial.printf("PSRAM allocation failed: %d bytes, again\n", size);
        }
    }
    return ptr;
  }
  void deallocate(void* ptr) override {
    heap_caps_free(ptr);
  }
  void* reallocate(void* ptr, size_t new_size) override {
    return heap_caps_realloc(ptr, new_size, MALLOC_CAP_SPIRAM);
  }
};
