/**
 * @file Handlers.cpp
 * @author your name (you@domain.com)
 * @brief 
 * @version 0.1
 * @date 2026-04-30
 * 
 * @copyright Copyright (c) 2026
 * 
 */
#include "GoogleSpeech.h"
#include "Motion.h"

void resetVolume();
/**
 * @brief 
 * 
 * @param text 
 * @param avatar 
 */
static  SpiRamAllocator spiRamAllocatorTTS;

void speakGoogleTTS(String text,  m5avatar::Avatar *avatar, StackchanSERVO *servo) {
  M5.Speaker.begin();
  M5.Speaker.setVolume(200);
  M5_LOGI("Requesting TTS...");
  String apiKey = getApiKey("GOOGLE_SPEECH_KEY");
  SpiRamAllocator spiRamAllocator;

  WiFiClientSecure *client = new WiFiClientSecure;
  if (client) {
    String rootCA = readRootCA("google.pem");
    client->setCACert(rootCA.c_str());
    client->setInsecure();
    client->setTimeout(20);

    HTTPClient https;
    String url = "https://texttospeech.googleapis.com/v1/text:synthesize?key=" + apiKey;

    if (https.begin(*client, url)) {
      https.setTimeout(30000);

      https.useHTTP10(true);
      https.addHeader("Content-Type", "application/json; charset=utf-8");

      // リクエスト用のJSONを作成
      JsonDocument requestDoc(&spiRamAllocator);
      requestDoc["input"]["text"] = text;
      requestDoc["voice"]["languageCode"] = "ja-JP";
      requestDoc["voice"]["name"] = "ja-JP-Wavenet-B";
      requestDoc["voice"]["ssmlGender"] = "FEMALE";
      requestDoc["audioConfig"]["audioEncoding"] = "LINEAR16";
      requestDoc["audioConfig"]["speakingRate"] = "1.0";
      requestDoc["audioConfig"]["pitch"] = "1.0";
      requestDoc["audioConfig"]["volumeGainDb"] = "5";
      requestDoc["audioConfig"]["sampleRateHertz"] = "8000";

      size_t reqBody_size = 0;
      uint8_t *reqBody_buff = serializeJsonSpiram(requestDoc, &reqBody_size);

      if(servo) servo->moveDeltaXY(0, -10, 300);
      // POST送信
      int httpResponseCode = https.POST(reqBody_buff, reqBody_size);
      if(servo) servo->moveDeltaXY(0, 10, 300);
      if (httpResponseCode == 200) {
        //M5.Display.println("TTS Success! Decoding...");
        WiFiClient* stream = https.getStreamPtr();
        stream->setTimeout(10000);
        // 1. JSONの中から "audioContent" という文字列を探し出す
        if (stream->find("\"audioContent\"")) {
          // 2. コロンなどを読み飛ばし、値の開始（"）を探す
          while (stream->connected() || stream->available()) {
            if (stream->read() == '"') break;
          }
          // 3. PSRAMに、Base64を受け取るための巨大バッファ
          size_t max_b64_size = 1000000;
          char* b64_buffer = (char*)heap_caps_malloc(max_b64_size, MALLOC_CAP_SPIRAM);

          if (b64_buffer != nullptr) {
            // read encoded audio data
            size_t b64_len = 0;
            unsigned long timeout_start = millis();
            while (millis() - timeout_start < 15000) { 
              if (stream->available()) {
                int c = stream->read();
                if (c == '"') break;
                if (b64_len < max_b64_size - 1) {
                  b64_buffer[b64_len++] = (char)c;
                }
                timeout_start = millis(); 
              } else if (!stream->connected()) {
                break;
              } else {
                delay(1);
              }
            }
            b64_buffer[b64_len] = '\0';

            // Decode audio data
            size_t audio_len = 0;
            mbedtls_base64_decode(nullptr, 0, &audio_len, (const unsigned char*)b64_buffer, b64_len);
            uint8_t* audio_buf = (uint8_t*)heap_caps_malloc(audio_len, MALLOC_CAP_SPIRAM);

            if (audio_buf != nullptr) {
              mbedtls_base64_decode(audio_buf, audio_len, &audio_len, (const unsigned char*)b64_buffer, b64_len);
              free(b64_buffer);
              //M5_LOGI("Start speaking");
              //M5.Display.println("Playing Audio...");

              unsigned long start_time = millis();
              int16_t* pcm_data = (int16_t*)audio_buf+44;  // shift wav header
              size_t total_samples = audio_len-44;
              const float MAX_RMS = 9000.0;
              if (avatar){
                avatar->setSpeechText(text.c_str());
              }
              //float dsample = total_samples/num;
              M5.Speaker.playWav(audio_buf, audio_len);
              /// Spiking action...
              while (M5.Speaker.isPlaying()) {
                M5.update();
                //--------- Taking...
                unsigned long elapsed_ms = millis() - start_time;
                size_t current_sample = (elapsed_ms * 8000) / 1000;
                if (current_sample + 160 < total_samples) {
                  int64_t sum_sq = 0;
                  for (int i = 0; i < 160; i++) {
                    int16_t sample = pcm_data[current_sample + i];
                    sum_sq += sample * sample;
                  }
                  float ratio = sqrt(sum_sq / 160.0) / MAX_RMS;

                  if(avatar) avatar->setMouthOpenRatio(ratio);
                }
                delay(20);
              }
              if(avatar) {
                avatar->setMouthOpenRatio(0.0);
                avatar->setSpeechText("");
              }
              //----------------
              //M5_LOGI("Stop speaking");
              free(audio_buf);
              //M5.Display.println("Done.");
            } else {
              M5_LOGE("Fail to allocate audio buffer");
              free(b64_buffer);
            }
          } else {
            M5_LOGE("Fail to allocate recieved data");
          }
        } else {
          M5_LOGE("No 'audioContent' found...");
        }
      } else {
        M5_LOGE("HTTP Error: %d\n", httpResponseCode);
      }
      https.end();
      free(reqBody_buff);
    }
    M5.Speaker.stop();
    delay(50);
    resetVolume();
    delay(50);
    M5.Speaker.end();
    delete client;
  }
}


void speakGoogleTTS2(String text,  m5avatar::Avatar *avatar, StackchanSERVO *servo) {
  M5.Speaker.begin();
  M5.Speaker.setVolume(200);
  M5_LOGI("Requesting TTS...");
  String apiKey = getApiKey("GOOGLE_SPEECH_KEY");
  SpiRamAllocator spiRamAllocator;

  WiFiClientSecure *client = new WiFiClientSecure;
  if (client) {
    String rootCA = readRootCA("google.pem");
    client->setCACert(rootCA.c_str());
    client->setInsecure();
    client->setTimeout(20);

    //String url = "https://texttospeech.googleapis.com/v1/text:synthesize?key=" + apiKey;

    if (client->connect("texttospeech.googleapis.com", 443)) {

      // リクエスト用のJSONを作成
      JsonDocument requestDoc;
      requestDoc["input"]["text"] = text;
      requestDoc["voice"]["languageCode"] = "ja-JP";
      requestDoc["voice"]["name"] = "ja-JP-Wavenet-B";
      requestDoc["voice"]["ssmlGender"] = "FEMALE";
      requestDoc["audioConfig"]["audioEncoding"] = "LINEAR16";
      requestDoc["audioConfig"]["speakingRate"] = "1.0";
      requestDoc["audioConfig"]["pitch"] = "1.0";
      requestDoc["audioConfig"]["volumeGainDb"] = "5";
      requestDoc["audioConfig"]["sampleRateHertz"] = "8000";

      //size_t reqBody_size = 0;
      //uint8_t *reqBody_buff = serializeJsonSpiram(requestDoc, &reqBody_size);
      String request_buff;
      serializeJson(requestDoc, request_buff);
      int reqBody_size = request_buff.length();
      const char *reqBody_buff = request_buff.c_str();

      // --- Send Header
      String url = "/v1/text:synthesize?key=" + apiKey;
      client->println("POST " + url + " HTTP/1.0");
      client->println("Host: texttospeech.googleapis.com");
      client->println("Content-Type: application/json");
      client->print("Content-Length: ");
      client->println(reqBody_size);
      client->println();

      size_t bytes_sent= sendRequestBody(client, (unsigned char*)reqBody_buff, reqBody_size); 
      M5_LOGI("Waiting for response...(%d, %d)", bytes_sent, reqBody_size);
      //free(reqBody_buff);
      if(servo) servo->moveDeltaXY(0, -10, 100);
      int res = 0;
      int count = 0;
      while(res == 0) {
        res = client->available();
        delay(200);
      }
      if(servo) servo->moveDeltaXY(0, 10, 100);

      int contentLen = 1000000;
      bool chunk_flag = false;
      int httpResponseCode = 0;
      String resheader = readHttpHeader(client, &httpResponseCode, &contentLen, &chunk_flag);
      //M5_LOGI("%s", resheader.c_str());

      if (httpResponseCode == 200) {
        int buffLen = 0;
        uint8_t* buff = readResponseBody(client, contentLen, &buffLen);
        if(buff != nullptr) {
          String buff_str = String((char *)buff);
          int st = buff_str.indexOf("\"audioContent\"");
          if(st > 0) {
            st = buff_str.indexOf("\"",st+15);
            int ed = buff_str.indexOf("\"", st+1);
            int b64_len=ed-st-1;
            uint8_t* buffer = buff+st+1;
            // Decode audio data
            size_t audio_len = 0;
            mbedtls_base64_decode(nullptr, 0, &audio_len, (const unsigned char*)buffer, b64_len);
            //M5_LOGI("Audio: %d, %d", audio_len, b64_len);
            uint8_t* audio_buf = (uint8_t*)heap_caps_malloc(audio_len, MALLOC_CAP_SPIRAM);

            if (audio_buf != nullptr) {
              mbedtls_base64_decode(audio_buf, audio_len, &audio_len, (const unsigned char*)buffer, b64_len);

              unsigned long start_time = millis();
              int16_t* pcm_data = (int16_t*)audio_buf+44;  // shift wav header
              size_t total_samples = audio_len-44;
              const float MAX_RMS = 9000.0;
              if (avatar){
                avatar->setSpeechText(text.c_str());
              }
              //float dsample = total_samples/num;
              M5.Speaker.playWav(audio_buf, audio_len);
              /// Spiking action...
              while (M5.Speaker.isPlaying()) {
                M5.update();
                //--------- Taking...
                unsigned long elapsed_ms = millis() - start_time;
                size_t current_sample = (elapsed_ms * 8000) / 1000;
                if (current_sample + 160 < total_samples) {
                  int64_t sum_sq = 0;
                  for (int i = 0; i < 160; i++) {
                    int16_t sample = pcm_data[current_sample + i];
                    sum_sq += sample * sample;
                  }
                  float ratio = sqrt(sum_sq / 160.0) / MAX_RMS;

                  if(avatar) avatar->setMouthOpenRatio(ratio);
                }
                delay(20);
              }
              if(avatar) {
                avatar->setMouthOpenRatio(0.0);
                avatar->setSpeechText("");
              }
              free(audio_buf);
            }else{
              M5_LOGE("Memory allocation paser Error.");
            }
            free(buff);
          } else {
            M5_LOGE("No 'audioContent' found...");
            free(buff);
          }
        } else{
          M5_LOGE("HTTP Error: fail to allocate memory");
        }
      } else {
        M5_LOGE("HTTP Error: %d\n", httpResponseCode);
      }

    }
    M5.Speaker.stop();
    delay(50);
    resetVolume();
    delay(50);
    M5.Speaker.end();
    delete client;
  }
}

/**
 * @brief 
 * 
 * @param msg 
 * @param avatar 
 */
void executeGoogleTTS(String msg,  m5avatar::Avatar *avatar, StackchanSERVO *servo) {
  std::vector<String> msg_list;
  std::vector<String> delims={ "。", "\n", "　", "  " };
  splitString(msg, delims, msg_list);
  for(int i=0; i<msg_list.size();i++){
    if(msg_list[i].length() > 2){
      M5_LOGI("Message: %s", msg_list[i].c_str());
      speakGoogleTTS2(msg_list[i], avatar, servo);
    }else{
      M5_LOGI("Invalid String");
    }
    delay(3);
  }
  if(servo) upMotion(servo, 1, 0);
}

/**
 * @brief 
 * 
 * @param audio_data 
 * @param audio_len 
 * @return String 
 */
String doGoogleASR(int16_t *audio_data, int audio_len) {
  // base64 encode...
  int output_len = 0;
  unsigned char*base64_buffer = b64EncodeAudio(audio_data, audio_len, &output_len);
  if (base64_buffer == nullptr) { return ""; }
  String payload = requestGoogleAsr(base64_buffer, output_len);
  free(base64_buffer);

  if (payload.length() > 0){
    JsonDocument responseDoc;
    DeserializationError error = deserializeJson(responseDoc, payload);
    if (!error) {
      const char* result = responseDoc["results"][0]["alternatives"][0]["transcript"];
      if (result){
        //M5.Display.printf("[%s]\n", result);
        //M5_LOGI("%s", result);
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
String requestGoogleAsr(unsigned char* b64_buffer, size_t b64_size) {
  String apiKey = getApiKey("GOOGLE_SPEECH_KEY");
  WiFiClientSecure *client = new WiFiClientSecure;
  String payload = ""; 
  
  if (client) {
    String rootCA = readRootCA("google.pem");
    client->setCACert(rootCA.c_str());
    client->setInsecure();
    client->setTimeout(20);
    if (client->connect("speech.googleapis.com", 443)) {
      String json_start = "{\"config\":{\"encoding\":\"LINEAR16\",\"sampleRateHertz\":16000,\"languageCode\":\"ja-JP\"},\"audio\":{\"content\":\"";
      String json_end = "\"}}";
      
      size_t total_length = json_start.length() + b64_size + json_end.length();

      // --- 1. HTTPヘッダーを手動で送信 ---
      String url = "/v1/speech:recognize?key=" + String(apiKey);
      client->println("POST " + url + " HTTP/1.1");
      client->println("Host: speech.googleapis.com");
      client->println("Content-Type: application/json");
      client->print("Content-Length: ");
      client->println(total_length);
      client->println();

      // --- 2. Send Payload  ---
      client->print(json_start);
      sendRequestBody(client, b64_buffer, b64_size);
      client->print(json_end);

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

String executeGoogleAsr(int max_sec, m5avatar::Avatar *avatar) {
  int silence = 2;
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
      result = doGoogleASR(audio_data, audiolen + SAMPLE_RATE*2);
      if(avatar) avatar->setInfoText("");
    } else {
      M5_LOGI("Fain to recognize.");
    }
    free(audio_data);
  }else{
    M5_LOGE("Fain to alloc memory.(%d)", buff_len);
  }
  return result;
}