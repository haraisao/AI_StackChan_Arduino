/**
 * @file Vosk.cpp
 * @author Isao Hara (isao@hara-jp.com)
 * @brief 
 * @version 0.1
 * @date 2026-05-16
 * 
 * @copyright Copyright (c) 2026
 * 
 */
#include <Vosk.h>

/**
 * @brief 
 * 
 * @param audio_data 
 * @param audio_len 
 * @return String 
 */
String doVoskASR(String host, int16_t *audio_data, int audio_len) {
  // base64 encode...
  int output_len = 0;
  unsigned char*base64_buffer = b64EncodeAudio(audio_data, audio_len, &output_len);
  if (base64_buffer == nullptr) { return ""; }
  String payload = requestVoskAsr(host, base64_buffer, output_len);
  free(base64_buffer);

  if (payload.length() > 0){
    JsonDocument responseDoc;
    DeserializationError error = deserializeJson(responseDoc, payload);
    if (!error) {
      const char* result = responseDoc["results"];
      if (result){
        return String(result);
      }else{
        M5_LOGI("Fail to get Result");
      }
    } else {
      //M5.Display.printf("deserialize Error: %s\n", error.c_str());
      M5_LOGE("deserialize Error: %s\n", error.c_str());
    }
  }
  return "";
}

/**
 * @brief 
 * 
 * @param b64_buffer 
 * @param b64_size 
 * @return String 
 */
String requestVoskAsr(String url, unsigned char* b64_buffer, size_t b64_size) {
  //WiFiClientSecure *client = new WiFiClientSecure;
  HttpRequest http = HttpRequest(url);
  String payload = "{\"audio\": \"_B64_AUDIO_\"}";

  http.postRequestAsr(payload, b64_buffer, b64_size);
  http.waitResponse();
  int len=50000;
  char *buffer = (char *)http.recvResponse(&len);
  String result = buffer;
  free(buffer); 
  return result;
}

#if 0
String requestVoskAsrOrg(String host, unsigned char* b64_buffer, size_t b64_size) {
  WiFiClientSecure *client = new WiFiClientSecure;
  String payload = ""; 
  String hostname = host.substring(0, host.indexOf(":"));
  int port = host.substring(host.indexOf(":")+1).toInt();
  if (client) {
    client->setTimeout(20);
    if (client->connect(hostname.c_str(), port)) {
      String json_start = "{\"audio\": ";
      String json_end = "\"}";
      
      size_t total_length = json_start.length() + b64_size + json_end.length();

      // --- 1. HTTPヘッダーを手動で送信 ---
      String url = "/vosk";
      client->println("POST " + url + " HTTP/1.0");
      client->print("Host: ");
      client->println(hostname.c_str());
      client->println("Content-Type: application/json");
      client->print("Content-Length: ");
      client->println(total_length);
      client->println();

      // --- 2. Send Payload  ---
      //client->print(json_start);
      sendRequestBody(client, b64_buffer, b64_size);
      //client->print(json_end);

      //M5.Display.println("Waiting for response...");
      M5_LOGI("Waiting for response...");

      int responseCode=0;
      int contentLen=0;
      bool chunk_flag=false;
      String response = readHttpHeader(client, &responseCode, &contentLen, &chunk_flag);

      if (responseCode == 200) {
        payload = client->readString();
        payload = payload.substring(payload.indexOf("{"));
      } else {
        M5_LOGE("HTTP Error Code: %d", responseCode);
      }
    } else {
      M5_LOGE("Connection to Google failed.");
    }
    client->stop();
    delete client;
  }
  return payload;
}
#endif

String executeVoskAsr(String host, int max_sec, m5avatar::Avatar *avatar) {
  int silence = 5;
  int buff_len = SAMPLE_RATE*(max_sec+silence)*sizeof(int16_t);
  M5_LOGI("Try to alloc memory.(%d)", buff_len);
  int16_t* audio_data = (int16_t*)heap_caps_malloc(buff_len, MALLOC_CAP_SPIRAM);
  M5_LOGI("Finish to alloc memory.(%d)", buff_len);
  String result="";
  if(audio_data){
    memset(audio_data, 0, buff_len);
    int audiolen = getSpeechFromMic(audio_data, max_sec);
    M5_LOGI("Finish to capture audio.(%d)", audiolen);
    if (audiolen > SAMPLE_RATE) { // over 1sec
      if(avatar) avatar->setInfoText("音声認識中",TFT_BLACK, TFT_GREEN);
      result = doVoskASR(host, audio_data, audiolen + SAMPLE_RATE*2);
      if(avatar) avatar->setInfoText("");
    } else {
      M5_LOGI("Fail to recognize.");
    }
    free(audio_data);
  }else{
    M5_LOGE("Fail to alloc memory.(%d)", buff_len);
  }
  return result;
}