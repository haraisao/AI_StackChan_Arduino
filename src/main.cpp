/**
 * 
 */
#include <Arduino.h>
#include <M5Unified.h>
#include <SD.h>
#include <Stackchan_system_config.h>
#include <Stackchan_servo.h>
//#include "M5AsyncWebServer.h"
#include "M5WebServer.h"
#include "TouchButton.h"
#include "Utils.h"
#include "GoogleSpeech.h"
#include "Gemini.h"

using namespace m5avatar;

/// Extern functions
void beep(int typ);

// --- avater
Avatar avatar;
const Expression expressions[] = {
  Expression::Neutral,
  Expression::Sleepy,
  Expression::Happy,
  Expression::Sad,
  Expression::Doubt,
  Expression::Angry,
};
static int face=0;
std::map<String, int> faceType = {{"normal", 0},{"look_d", 1}, {"smile", 2},
  {"unhappy", 3}, {"surprise", 4}, {"anger", 5}};

StackchanSERVO servo;
StackchanSystemConfig system_config;

static String interactionId="";

// ------ WebServer
//M5AsyncWebServer myServer(80, "/html");
M5WebServer myServer(80, "/html");
void handleHello() {
    myServer.response(200, "application/json", "{\"message\":\"Hello API\"}");
}

void handleFace() {
  String postBody=myServer.getBody();
  if(faceType.count(postBody)) {
    avatar.setExpression(expressions[faceType[postBody]]);
  }else{
    avatar.setExpression(expressions[0]);
  }
  myServer.response(200, "application/json", "{\"result\":\"OK\"}");
}

void handleTexttospeech() {
  String postBody=myServer.getBody();
  speakGoogleTTS(postBody, &avatar);
  myServer.response(200, "application/json", "{\"result\":\"OK\"}");
}

void handleMove() {
  String postBody=myServer.getBody();
  JsonDocument doc;
  DeserializationError error = deserializeJson(doc, postBody);
  int x = doc["x"];
  int y = doc["y"];
  int sp = doc["sp"];
  servo.moveXY(x+90, 90-y, sp);
  myServer.response(200, "application/json", "{\"result\":\"OK\"}");
}

void handleTalkGemini() {
  String postBody=myServer.getBody();
  //requestGemini(postBody, &avatar);

  if(postBody == "reset"){
    interactionId = "";
  } else {
    String msg = requestGeminiInteraction(postBody, interactionId, &avatar);
    executeGoogleTTS(msg, &avatar);
  }
  myServer.response(200, "application/json", "{\"result\":\"OK\"}");
}

void handleAsr() {
  String postBody=myServer.getBody();
  JsonDocument doc;
  DeserializationError error = deserializeJson(doc, postBody);
  int max_sec = doc["max_seconds"];

  String result = executeGoogleAsr(max_sec);
  if (result.length() > 0) {
    M5_LOGI("Recognize: %s", result.c_str());
    String msg = requestGeminiInteraction(result, interactionId, &avatar);
    executeGoogleTTS(msg, &avatar);
  }
  myServer.response(200, "application/json", "{\"result\":\"OK\"}");
}

void handleGetFileList() {
  String postBody=myServer.getBody();
  JsonDocument doc;
  DeserializationError error = deserializeJson(doc, postBody);
  String dirname = doc["dir_name"];
  std::map<String, std::vector<String>> flist;
  listDir(LittleFS, dirname.c_str(), flist);
  JsonDocument response;
  Serial.println("Dirs:");
  for(const String& s : flist["dir"]){
    response["dir_list"].add(s);
    Serial.println(s);
  }
  Serial.println("Files:");
  for(const String& s : flist["file"]){
    response["file_list"].add(s);
    Serial.println(s);
  }

  String responseData;
  serializeJson(response, responseData);
  myServer.response(200, "application/json", responseData.c_str());
}

void handleGetFile() {
  String postBody=myServer.getBody();
  JsonDocument doc;
  DeserializationError error = deserializeJson(doc, postBody);
  String fname = doc["file_name"];
  String content = loadFile(fname);

  JsonDocument response;
  response["data"] = content;
  String responseData;
  serializeJson(response, responseData);
  myServer.response(200, "application/json", responseData.c_str());
}

void handleSaveFile() {
  String postBody=myServer.getBody();
  JsonDocument doc;
  DeserializationError error = deserializeJson(doc, postBody);
  String fname = doc["file_name"];
  String data = doc["data"];
  saveFile(fname, data.c_str());

  JsonDocument response;
  response["result"] = "OK";
  String responseData;
  serializeJson(response, responseData);
  myServer.response(200, "application/json", responseData.c_str());
}

// Touch Buttons
TouchButton touchButton;
void callbackBtnA(){
  if(avatar.isDrawing()) {
    beep(0);
    avatar.stop();
    delay(300);
    M5.Display.clearDisplay();
    M5.Display.setCursor(0,0);
  }else{
    beep(1);
    avatar.init();
  }
}

void callbackBtnB(){
  if(avatar.isDrawing()) {
    M5_LOGI("Press BtnB");
    face = (face+1) % 6;
    avatar.setExpression(expressions[face]);
  }else{
    M5.Display.println("音声認識開始...");
    String result = executeGoogleAsr(10);
    M5.Display.printf("[%s]\n", result.c_str());
    M5_LOGI("Recognized:%s", result.c_str());
  }
}

void callbackBtnC(){
  if(avatar.isDrawing()) { return; }
  if (WiFi.status() == WL_CONNECTED) {
    M5.Display.setTextColor(GREEN);
    M5.Display.printf("\nIP: %s\n", WiFi.localIP().toString().c_str());
    M5.Display.setTextColor(WHITE);
    //requestGemini("こんにちは");
  }else{
    myServer.connect_wlan_from_sd("/wlan.json");
    if (WiFi.status() == WL_CONNECTED) {
      beep(1);
      adjustTime();
    }
  }

}

/**
 * @brief 
 * 
 */
void setup() {
  auto cfg = M5.config();
  M5.begin(cfg);
  //M5.Log.setLogLevel(m5::log_target_display, ESP_LOG_INFO);
  M5.Log.setLogLevel(m5::log_target_serial, ESP_LOG_INFO);
  M5.Log.setEnableColor(m5::log_target_serial, true);

  M5.Display.setFont(&fonts::lgfxJapanGothic_12);
  M5.Display.setTextSize(1);
  M5.Display.setBrightness(100);

  int count=0;
  while (false == SD.begin(GPIO_NUM_4, SPI, 25000000) && count < 3) {
    delay(200);
    M5_LOGI("Wait for SD card (%d)", count);
    count += 1;
  }

  mountLitteFs();
  system_config.loadConfig(LittleFS, "/yaml/SC_BasicConfig.yaml");
  
  // servo
  servo.begin(system_config.getServoInfo(AXIS_X)->pin,
              system_config.getServoInfo(AXIS_X)->start_degree,
              system_config.getServoInfo(AXIS_X)->offset,
              system_config.getServoInfo(AXIS_Y)->pin,
              system_config.getServoInfo(AXIS_Y)->start_degree,
              system_config.getServoInfo(AXIS_Y)->offset,
              (ServoType)system_config.getServoType());
  delay(2000);

  
  servo_interval_s* servo_interval = system_config.getServoInterval(AvatarMode::NORMAL);
  servo_interval_s* servo_interval_sing = system_config.getServoInterval(AvatarMode::SINGING);

  // WebServer
  myServer.connect_wlan_from_sd("/wlan.json");

  /// register REST-API
  myServer.registerApi("hello_api", handleHello);
  myServer.registerApi("face", handleFace);
  myServer.registerApi("tts", handleTexttospeech);
  myServer.registerApi("gemini", handleTalkGemini);
  myServer.registerApi("move", handleMove);
  myServer.registerApi("asr", handleAsr);
  myServer.registerApi("get_file_list", handleGetFileList);
  myServer.registerApi("get_file", handleGetFile);
  myServer.registerApi("save_file", handleSaveFile);
  
  //myServer.loadApiConfig("/api.yml");
  myServer.start();

  /// Touch buttons
  touchButton.createButton(0, 200, 100, 40, callbackBtnA);
  touchButton.createButton(105, 200, 100, 40, callbackBtnB);
  touchButton.createButton(210, 200, 100, 40, callbackBtnC);

  if (WiFi.status() == WL_CONNECTED) {
    beep(1);
    adjustTime();
  } else {
    beep(2);
  }

  /// Mic
  auto mic_cfg = M5.Mic.config();
  mic_cfg.sample_rate = SAMPLE_RATE;
  mic_cfg.stereo = false;
  mic_cfg.noise_filter_level = (mic_cfg.noise_filter_level + 8) & 255;
  mic_cfg.magnification = 24;
  M5.Mic.config(mic_cfg);

  // Avatar and interaction_id
  interactionId=loadFile("/gemini_interaction");
  avatar.init();
  avatar.setSpeechFont(&fonts::lgfxJapanGothic_12);
}

void loop() {
  M5.update();
  myServer.update();
  touchButton.update();
}
