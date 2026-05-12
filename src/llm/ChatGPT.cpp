/**
 * 
 */
#include "Utils.h"
#include "ChatGPT.h"

/**
 * @brief 
 * 
 * @param txt 
 * @param avatar 
 */

#if 0
String requestChatGPT(String txt, m5avatar::Avatar *avatar) {
  String apikey =  getApiKey("OPENAI_KEY");
  WiFiClientSecure *client = new WiFiClientSecure;
  String result = "";

  if (client) {
    String rootCA = readRootCA("openai.pem");
    if(rootCA == ""){
      client->setInsecure();
    }else{
      client->setCACert(rootCA.c_str());
    }
    client->setTimeout(20);

    String model = "gpt-5";
    JsonDocument doc;
    doc["model"] = model;

    //JsonObject prompt_top = doc.createNestedObject("system_instruction");
    JsonObject prompt = doc["input"].add<JsonObject>(); 
    prompt["content"] = "あなたは、小さなスーパーロボット スタックチャンです。音声合成で応答するので、返答は20字以内でまとめてください。";
    prompt["role"] = "developer";

    String role="user";
    JsonObject contnet = doc["input"].add<JsonObject>();
    contnet["content"] = txt;
    contnet["role"] = role;

    // Web search
    JsonObject tool1 = doc["tools"].add<JsonObject>();
    tool1["type"]="web_search";

    //doc["contents"].add(chatContent.c_str());
    String postData;
    serializeJson(doc, postData);
    M5_LOGI("==>[%s]", postData.c_str());

    if (client->connect("api.openai.com", 443)) {
      size_t total_length = postData.length();
      // --- 1. HTTPヘッダーを手動で送信 ---
      String url = "/v1/responses";
      client->println("POST " + url + " HTTP/1.0");
      client->println("Host: api.openai.com");
      client->println("Content-Type: application/json");
      client->println("Authorization: Bearer " + apikey);
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
          JsonArray conts = response["output"];
          for(JsonVariant v: conts){
            JsonObject obj = v.as<JsonObject>();
            String typ = obj["type"];
            Serial.printf("-- %s\r\n", typ.c_str());
            if( typ == "message"){
              JsonArray res = obj["content"];
              for(JsonVariant v2: res){
                JsonObject obj2 = v2.as<JsonObject>();
                String typ2 = obj2["type"];
                Serial.printf(">>> %s\r\n", typ2.c_str());
                if(typ2 == "output_text"){
                  String txt = obj2["text"];
                  M5_LOGI("Res: %s", txt.c_str());
                }

              }

            }
          }
        }else{
          M5_LOGE("Parse error");
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
#else
String requestChatGPT(String txt, m5avatar::Avatar *avatar) {
  String apikey =  getApiKey("OPENAI_KEY");
  WiFiClientSecure *client = new WiFiClientSecure;
  String result = "";
  String responseId = loadFile("/chatgpt_interaction");
  if (client) {
    String rootCA = readRootCA("openai.pem");
    if(rootCA == ""){
      client->setInsecure();
    }else{
      client->setCACert(rootCA.c_str());
    }
    client->setTimeout(20);

    String model = "gpt-5.4-mini";
    JsonDocument doc;
    doc["model"] = model;
    if(responseId.length() > 0){
      doc["previous_response_id"] = responseId;
    }else{
      doc["instructions"] = "あなたは、小さなスーパーロボット スタックチャンです。音声合成で応答するので、返答は20字以内でまとめてください。";
    }
    doc["input"] = txt;
    doc["store"] = true;

    // Web search
    JsonObject tool1 = doc["tools"].add<JsonObject>();
    tool1["type"]="web_search";

    //doc["contents"].add(chatContent.c_str());
    String postData;
    serializeJson(doc, postData);
    //M5_LOGI("==>[%s]", postData.c_str());
    String info="考え中";
    if (client->connect("api.openai.com", 443)) {
      if(avatar) avatar->setInfoText(info.c_str(), TFT_BLACK, TFT_YELLOW);
      size_t total_length = postData.length();
      // --- 1. HTTPヘッダーを手動で送信 ---
      String url = "/v1/responses";
      client->println("POST " + url + " HTTP/1.0");
      client->println("Host: api.openai.com");
      client->println("Content-Type: application/json");
      client->println("Authorization: Bearer " + apikey);
      client->print("Content-Length: ");
      client->println(total_length);
      client->println();

      unsigned char *reqBuff = (unsigned char *)postData.c_str();
      size_t bytes_sent= sendRequestBody(client, reqBuff, total_length);
      M5_LOGI("Waiting for response...");

      int res = 0;
      int count = 0;
      while(res == 0) {
        res = client->available();

        count += 1;
        if(count > 10){
          //upMotion(servo, -1,-1);
          info = info + ".";
          if(avatar) avatar->setInfoText(info.c_str(), TFT_BLACK, TFT_YELLOW);
          count = 0;
        }else{
          delay(200);
        }
      }

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
          //M5_LOGI("Response: %s", buffer);
        }
        
        JsonDocument response;
        DeserializationError error = deserializeJson(response, buffer);
        if(!error){
          const char *response_id = response["id"];
          saveFile("/chatgpt_interaction", response_id);
          JsonArray conts = response["output"];
          for(JsonVariant v: conts){
            JsonObject obj = v.as<JsonObject>();
            String typ = obj["type"];
            //Serial.printf("-- %s\r\n", typ.c_str());
            if( typ == "message"){
              JsonArray res = obj["content"];
              for(JsonVariant v2: res){
                JsonObject obj2 = v2.as<JsonObject>();
                String typ2 = obj2["type"];
                //Serial.printf(">>> %s\r\n", typ2.c_str());
                if(typ2 == "output_text"){
                  String txt = obj2["text"];
                  result += txt;
                  //M5_LOGI("Res: %s", txt.c_str());
                }

              }

            }
          }
        }else{
          M5_LOGE("Parse error");
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

#endif