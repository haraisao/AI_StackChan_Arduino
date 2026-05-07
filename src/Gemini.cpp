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
#include "Utils.h"
#include "Gemini.h"

/**
 * @brief 
 * 
 * @param txt 
 * @param avatar 
 */
String requestGemini(String txt, m5avatar::Avatar *avatar) {
  String apikey =  getApiKey("GEMINI_KEY");
  WiFiClientSecure *client = new WiFiClientSecure;
  String result = "";

  if (client) {
    String rootCA = readRootCA("google.pem");
    if(rootCA == ""){
      client->setInsecure();
    }else{
      client->setCACert(rootCA.c_str());
    }
    client->setTimeout(20);

    String role="user";
    JsonDocument doc;
    JsonObject p1 = doc["contents"].add<JsonObject>();
    JsonObject txt1 = p1["parts"].add<JsonObject>();
    txt1["text"] = txt;
    p1["role"] = role;

    JsonObject prompt_top = doc["system_instruction"].to<JsonObject>(); 
    JsonObject p_txt = prompt_top["parts"].add<JsonObject>();
    p_txt["text"] = "あなたは、小さなスーパーロボット スタックチャンです。音声合成で応答するので、返答は20字以内でまとめてください。";
    prompt_top["role"] = "model";

    JsonObject tool = doc["tools"].add<JsonObject>();
    JsonObject tool1 = tool["url_context"].to<JsonObject>();
    JsonObject tool2 = tool["google_search"].to<JsonObject>();

    String postData;
    serializeJson(doc, postData);
    M5_LOGI("==>[%s]", postData.c_str());

    if (client->connect("generativelanguage.googleapis.com", 443)) {
      size_t total_length = postData.length();

      // --- 1. HTTPヘッダーを手動で送信 ---
      String model = "gemini-3-flash-preview";
      String url = "/v1beta/models/" + model + ":generateContent";
      client->println("POST " + url + " HTTP/1.0");
      client->println("Host: generativelanguage.googleapis.com");
      client->println("Content-Type: application/json");
      client->println("x-goog-api-key:" + apikey);
      client->print("Content-Length: ");
      client->println(total_length);
      client->println();

      unsigned char *reqBuff = (unsigned char *)postData.c_str();
      size_t bytes_sent= sendRequestBody(client, reqBuff, total_length);
      M5_LOGI("Waiting for response...");

      int contentLen = 400000;
      bool chunk_flag = false;
      int return_code = 0;
      String response = readHttpHeader(client, &return_code, &contentLen, &chunk_flag);

      if (return_code == 200) {
        uint8_t* buffer = (uint8_t*)heap_caps_malloc(contentLen, MALLOC_CAP_SPIRAM);

        if (buffer != nullptr) {
          size_t buffLen = 0;
          while(client->available() && buffLen < contentLen){
            size_t count = client->readBytes(buffer + buffLen, 4096);
            if (count >= 0){
              buffLen += count;
            }
            delay(2); 
          }
          M5_LOGI("Response: %s", buffer);
        }
        
        JsonDocument response;
        DeserializationError error = deserializeJson(response, buffer);
        if(!error){
          const char *msg=response["candidates"][0]["content"]["parts"][0]["text"];
          result = String(msg);
        }
        free(buffer);
      } else {
        M5_LOGI("HTTP Error Code: %d", return_code);
      }
    } else {
      M5_LOGE("Connection to Google failed.");
    }
    client->stop();
    delete client;
  } else {
    M5_LOGI("Unable to create client.");
  }
  return result;
}

/**
 * @brief 
 * 
 * @param txt 
 * @param interactionId 
 * @param avatar 
 * @return String 
 */
String requestGeminiInteraction(String txt, String& interactionId, m5avatar::Avatar *avatar) {
  String apikey = getApiKey("GEMINI_KEY");
  WiFiClientSecure *client = new WiFiClientSecure;
  String interId = interactionId;
  SpiRamAllocator spiRamAllocator;
  String result="";

  String model = loadFile("/gemini-model");
  String prompt = loadFile("/gemini-prompt");


  if (client) {
    avatar->setInfoText("考え中", TFT_BLACK, TFT_YELLOW);
    String rootCA = readRootCA("google.pem");
    if(rootCA == ""){
      client->setInsecure();
    }else{
      client->setCACert(rootCA.c_str());
    }
    client->setTimeout(20);

    String role="user";
    JsonDocument doc;
    if(model != "") {
      doc["model"] = model;
    }else{
      // gemini-3.1-flash-lite-preview
      // gemini-3-flash-preview
      // gemini-2.5-flash
      // gemini-2.5-flash-lite
      //doc["model"] = "gemini-2.5-flash-lite";
      doc["model"] = "gemini-3.1-flash-lite-preview";
    }
    doc["input"] = txt;
    if(interactionId.length() > 0){
      doc["previous_interaction_id"] = interactionId;
    }else{
      if (prompt == "") {
        doc["system_instruction"] = "あなたは、小さなスーパーロボット スタックチャンです。応答は音声合成で出力するので、返答は ＃ や ＊ の飾り文字をつけずに３０字以内でまとめてください。また、応答の最後に「以上です」と付け加えてください。";
      } else {
        doc["system_instruction"] = prompt;
      }
    }
    JsonObject tool = doc["tools"].add<JsonObject>();
    tool["type"] = "google_search";

    String postData;
    serializeJson(doc, postData);
    M5_LOGI("==>%s<==", postData.c_str());
    size_t total_length = postData.length();
    if (client->connect("generativelanguage.googleapis.com", 443)) {
      // --- Send Header
      String url = "/v1beta/interactions";
      client->println("POST " + url + " HTTP/1.0");
      client->println("Host: generativelanguage.googleapis.com");
      client->println("Content-Type: application/json");
      client->println("x-goog-api-key:" + apikey);
      client->print("Content-Length: ");
      client->println(total_length);
      client->println();

      unsigned char *reqBuff = (unsigned char *)postData.c_str();
      size_t bytes_sent= sendRequestBody(client, reqBuff, total_length); 
      M5_LOGI("Waiting for response...(%d, %d)", bytes_sent, total_length);

      int contentLen = 400000;
      bool chunk_flag = false;
      int return_code = 0;
      String response = readHttpHeader(client, &return_code, &contentLen, &chunk_flag);

      if (return_code == 200){
        int buffLen = 0;
        uint8_t* buffer = readResponseBody(client, contentLen, &buffLen);

        if (buffer != nullptr) {
          JsonDocument filter;
          filter["id"] = true;
          filter["outputs"][0]["text"]=true;

          JsonDocument response(&spiRamAllocator);
          DeserializationError error = deserializeJson(response,
                              (char *)buffer, buffLen+1,
                              DeserializationOption::Filter(filter));
          if (!error) {
            const char *id=response["id"];
            interId = String(id);
            interactionId=interId;
            saveFile("/gemini_interaction", id);
            JsonArray obj = response["outputs"];
            size_t n = obj.size();
            int idx=0;
            for(;idx<n;idx++){
              const char *tmp_msg = response["outputs"][idx]["text"];
              if (tmp_msg){ break; }
            }

            if (idx == n){ idx=n-1; }
            String msg = response["outputs"][idx]["text"];
            result=msg;
          }else{
            M5_LOGE("Json paser Error. %s", error.c_str());
            free(buffer);
          }
          free(buffer);
        }
      } else {
        M5_LOGI("HTTP Error Code: %d", return_code);
      }
    } else {
      M5_LOGE("Connection to Google failed.");
    }
    client->stop();
    delete client;
  } else {
    M5_LOGI("Unable to create client.");
  }
  avatar->setInfoText("");
  return result;
}
