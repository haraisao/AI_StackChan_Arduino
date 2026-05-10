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

    //JsonObject prompt_top = doc.createNestedObject("system_instruction");
    JsonObject prompt_top = doc["system_instruction"].to<JsonObject>(); 
    JsonObject p_txt = prompt_top["parts"].add<JsonObject>();
    p_txt["text"] = "あなたは、小さなスーパーロボット スタックチャンです。音声合成で応答するので、返答は20字以内でまとめてください。";
    prompt_top["role"] = "model";

    JsonObject tool = doc["tools"].add<JsonObject>();
    JsonObject tool1 = tool["url_context"].to<JsonObject>();
    JsonObject tool2 = tool["google_search"].to<JsonObject>();

    //doc["contents"].add(chatContent.c_str());
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
          //String payload = client->readString();
          //payload = payload.substring(payload.indexOf("{"));
          //M5_LOGI("Response: %s", payload.c_str());
          M5_LOGI("Response: %s", buffer);
        }
        
        JsonDocument response;
        DeserializationError error = deserializeJson(response, buffer);
        if(!error){
          const char *msg=response["candidates"][0]["content"]["parts"][0]["text"];
          result = String(msg);
        }
        //M5.Display.printf("%s\n", msg);
        free(buffer);
        //speakGoogleTTS(msg, avatar);
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
String requestGeminiInteraction(String txt, String& interactionId, m5avatar::Avatar *avatar, StackchanSERVO *servo) {
  String apikey = getApiKey("GEMINI_KEY");
  WiFiClientSecure *client = new WiFiClientSecure;
  String interId = interactionId;
  SpiRamAllocator spiRamAllocator;
  String result="";

  String model = loadFile("/gemini-model");
  String prompt = loadFile("/gemini-prompt");

  if (client) {
    if(avatar) avatar->setInfoText("考え中", TFT_BLACK, TFT_YELLOW);
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

    JsonObject gen_conf = doc["generation_config"].to<JsonObject>();
    gen_conf["thinking_level"] = "low";

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

    JsonDocument fn1;
    if(loadJson("/mcp/function1.json", fn1) == 0){
      doc["tools"].add(fn1);
    }
    JsonDocument fn2;
    if(loadJson("/mcp/function2.json", fn2) == 0){
      doc["tools"].add(fn2);
    } 

    String postData;
    serializeJson(doc, postData);
    //M5_LOGI("==>%s<==", postData.c_str());
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
      //M5_LOGI("%s", reqBuff);
      M5_LOGI("Waiting for response...(%d, %d)", bytes_sent, total_length);

      int res = 0;
      int count = 0;
      while(res == 0) {
        res = client->available();

        count += 1;
        if(count > 10){
          upMotion(servo, -1,-1);
          count = 0;
        }else{
          delay(200);
        }
      }

      int contentLen = 400000;
      bool chunk_flag = false;
      int return_code = 0;
      String resheader = readHttpHeader(client, &return_code, &contentLen, &chunk_flag);
      //M5_LOGI("%s", resheader.c_str());

      if (return_code == 200){
        int buffLen = 0;
        uint8_t* buffer = readResponseBody(client, contentLen, &buffLen);

        if (buffer != nullptr) {
          JsonDocument filter;
          filter["id"] = true;
          filter["outputs"][0]["text"]=true;
          filter["outputs"][0]["type"]=true;
          filter["outputs"][0]["id"]=true;
          filter["outputs"][0]["name"]=true;
          filter["outputs"][0]["arguments"]=true;

          JsonDocument response(&spiRamAllocator);
          DeserializationError error = deserializeJson(response,
                              (char *)buffer, buffLen+1,
                              DeserializationOption::Filter(filter));
          if (!error) {
            const char *id=response["id"];
            //M5_LOGI("ID: %s", id);
            interId = String(id);
            interactionId=interId;
            saveFile("/gemini_interaction", id);
            JsonArray obj = response["outputs"];
            size_t n = obj.size();
            int idx=0;

            for(;idx<n;idx++){
              String type_ = response["outputs"][idx]["type"];
              // M5_LOGI("Type: [%d,%d]  %s", idx, n, type_.c_str());
              if (type_ == "text" || type_ == "function_call"){
                break;
              }
            }
            if (idx < n) {
              String tmp;
              serializeJson(response["outputs"][idx], tmp);
              result=tmp;
            }

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
  if(avatar) avatar->setInfoText("");
  //M5_LOGI("RES2: %s",result.c_str() );
  return result;
}

String responseGeminiMpc(String result, String& interactionId, String func, String callId,
   m5avatar::Avatar *avatar, StackchanSERVO *servo) {
  String apikey = getApiKey("GEMINI_KEY");
  WiFiClientSecure *client = new WiFiClientSecure;
  String interId = interactionId;
  SpiRamAllocator spiRamAllocator;

  String model = loadFile("/gemini-model");
  String prompt = loadFile("/gemini-prompt");

  if (client) {
    if(avatar) avatar->setInfoText("考え中", TFT_BLACK, TFT_YELLOW);
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
    JsonObject res = doc["input"].add<JsonObject>();
    res["type"] = "function_result";
    res["name"] = func;
    res["call_id"] = callId;
    JsonObject func_res = res["result"].add<JsonObject>();
    func_res["type"] = "text";
    func_res["text"] = result.c_str();

    doc["previous_interaction_id"] = interactionId;

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
      //M5_LOGI("%s", reqBuff);
      M5_LOGI("Waiting for response...(%d, %d)", bytes_sent, total_length);

      int res = 0;
      int count = 0;
      while(res == 0) {
        res = client->available();

        count += 1;
        if(count > 10){
          upMotion(servo, -1,-1);
          count = 0;
        }else{
          delay(200);
        }
      }

      int contentLen = 400000;
      bool chunk_flag = false;
      int return_code = 0;
      String resheader = readHttpHeader(client, &return_code, &contentLen, &chunk_flag);
      //M5_LOGI("%s", resheader.c_str());

      if (return_code == 200){
        int buffLen = 0;
        uint8_t* buffer = readResponseBody(client, contentLen, &buffLen);

        if (buffer != nullptr) {
          JsonDocument filter;
          filter["id"] = true;
          filter["outputs"][0]["text"]=true;
          filter["outputs"][0]["type"]=true;
          filter["outputs"][0]["id"]=true;
          filter["outputs"][0]["name"]=true;
          filter["outputs"][0]["arguments"]=true;

          JsonDocument response(&spiRamAllocator);
          DeserializationError error = deserializeJson(response,
                              (char *)buffer, buffLen+1,
                              DeserializationOption::Filter(filter));
          if (!error) {
            const char *id=response["id"];
            //M5_LOGI("ID: %s", id);
            interId = String(id);
            interactionId=interId;
            saveFile("/gemini_interaction", id);
            JsonArray obj = response["outputs"];
            size_t n = obj.size();
            int idx=0;

            for(;idx<n;idx++){
              String type_ = response["outputs"][idx]["type"];
              // M5_LOGI("Type: [%d,%d]  %s", idx, n, type_.c_str());
              if (type_ == "text" || type_ == "function_call"){
                break;
              }
            }
            if (idx < n) {
              String tmp;
              serializeJson(response["outputs"][idx], tmp);
              result=tmp;
            }

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
  if(avatar) avatar->setInfoText("");
  //M5_LOGI("RES2: %s",result.c_str() );
  return result;
}
