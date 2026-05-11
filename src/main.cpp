/**
 * 
 */
#include <StackChan.h>

using namespace m5avatar;

// --- avater
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

std::map<String, Expression> faceType2 = {{"Neutral", Expression::Neutral},
  {"Sleepy", Expression::Sleepy}, {"Happy", Expression::Happy},
  {"Sad", Expression::Sad}, {"Duobt", Expression::Doubt}, {"Angry", Expression::Angry}};

/** Static */
int motion_id=0;
static String interactionId="";
static int randNum = random(1, 51);

/* Instances  */
Avatar avatar;
StackchanSERVO servo;
StackchanSystemConfig system_config;
M5WebServer myServer(80, "/html");

// POST
void handleFace(void *arg) {
  M5WebServer *srv = reinterpret_cast<M5WebServer *>(arg);
  String postBody = srv->getBody();
  if(faceType.count(postBody)) {
    avatar.setExpression(expressions[faceType[postBody]]);
  }else{
    avatar.setExpression(expressions[0]);
  }
  srv->response(200, "application/json", "{\"result\":\"OK\"}");
}

void handleTexttospeech(void *arg) {
  M5WebServer *srv = reinterpret_cast<M5WebServer *>(arg);
  String postBody=srv->getBody();
  speakGoogleTTS2(postBody, &avatar, &servo);
  srv->response(200, "application/json", "{\"result\":\"OK\"}");
}

void handleMove(void *arg) {
  M5WebServer *srv = reinterpret_cast<M5WebServer *>(arg);
  JsonDocument doc;
  DeserializationError error = srv->requestJson(doc);
  int x = doc["x"];
  int y = doc["y"];
  int sp = doc["sp"];

#ifdef RT_VERSION
  /// for RT vesion.
  servo.moveXY(x+180, 90+y, sp);
#else
  //// for SG90
  servo.moveXY(x+90, 90-y, sp);
#endif
  srv->response(200, "application/json", "{\"result\":\"OK\"}");
}

void handleTalkGemini(void *arg) {
  M5WebServer *srv = reinterpret_cast<M5WebServer *>(arg);
  String postBody=srv->getBody();

  if(postBody == "reset"){
    interactionId = "";
  } else {
    String msg = requestGeminiInteraction(postBody, interactionId, &avatar, &servo);
    //M5_LOGI(">>> %s",msg.c_str()); 
    avatar.setInfoText("応答中",TFT_BLACK, TFT_GREEN);
    JsonDocument doc;
    DeserializationError error = deserializeJson(doc, msg);
    if (!error){
      if(doc["type"] == "text"){
        executeGoogleTTS(doc["text"], &avatar, &servo);
      } else {
        //M5_LOGI(">>%s",msg.c_str());
        String result="{\"result\": \"Finished\"}";
        String func = doc["name"]; 
        String callId = doc["id"];
        if(func == "stackchan:face"){
          String exp = doc["arguments"]["expression"];
          //M5_LOGI("Face: %s", exp.c_str());
          avatar.setExpression(faceType2[exp]);
        }
        msg = responseGeminiMpc(result, interactionId, func, callId, &avatar, &servo);
        //M5_LOGI(">>> %s",msg.c_str());
        error = deserializeJson(doc, msg);
        if (!error){
          executeGoogleTTS(doc["text"], &avatar, &servo);
        }
      }
    }else{

    }
  }
  avatar.setInfoText("");
  srv->response(200, "application/json", "{\"result\":\"OK\"}");
}

void handleAsr(void *arg) {
  avatar.setInfoText("音声取得中",TFT_BLACK, TFT_ORANGE);
  M5WebServer *srv = reinterpret_cast<M5WebServer *>(arg);
  JsonDocument doc;
  DeserializationError error = srv->requestJson(doc);
  int max_sec = doc["max_seconds"];

  String result = executeGoogleAsr(max_sec, &avatar);
  if (result.length() > 0) {
    M5_LOGI("Recognize: %s", result.c_str());
  
    String msg = requestGeminiInteraction(result, interactionId, &avatar, &servo);
    avatar.setInfoText("応答中",TFT_BLACK, TFT_GREEN);
    JsonDocument res;
    DeserializationError error = deserializeJson(res, msg);
    if (!error){
      if(res["type"] == "text"){
        executeGoogleTTS(res["text"], &avatar);
      }
    }
  }
  avatar.setInfoText("");
  srv->response(200, "application/json", "{\"result\":\"OK\"}");
}

void handleCommand(void *arg) {
  M5WebServer *srv = reinterpret_cast<M5WebServer *>(arg);
  JsonDocument doc;
  DeserializationError error = srv->requestJson(doc);
  String cmd = doc["cmd"];
  String data = doc["data"];
  if (cmd == "message"){
    avatar.setSpeechText(data.c_str());
  }
  JsonDocument response;
  response["result"] = "OK";
  srv->response(200, "application/json", response);
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
    M5.Display.println("モード1");
  }else{
    beep(1);
    avatar.init(8);
  }
}

void callbackBtnB(){
  if(avatar.isDrawing()) {
    M5_LOGI("Press BtnB");
    face = (face+1) % 6;
    avatar.setExpression(expressions[face]);
  }else{
    /**
    M5.Display.println("音声認識開始...");
    String result = executeGoogleAsr(10, &avatar);
    M5.Display.printf("[%s]\n", result.c_str());
    M5_LOGI("Recognized:%s", result.c_str());
    */
    if (CoreS3.Camera.get()) {
      CoreS3.Display.pushImage(0, 0, 320, 240, (uint16_t *)CoreS3.Camera.fb->buf);
      CoreS3.Camera.free();
    } else {
      Serial.println("Camera capture failed");
    }
  }
}

void callbackBtnC(){
  if(avatar.isDrawing()) {
    motion_id = (motion_id+1) % NUM_MOTIONS;
    M5_LOGI("Start Motion %d", motion_id);
    myMotion(&servo, motion_id);
    M5_LOGI("End Motion %d", motion_id);
    return;
  } else {
    if (WiFi.status() == WL_CONNECTED) {
      M5.Display.setTextColor(GREEN);
      M5.Display.printf("\nIP: %s\n", WiFi.localIP().toString().c_str());
      M5.Display.setTextColor(WHITE);
    }else{
      setupWifi("/wlan.json");

      if (WiFi.status() == WL_CONNECTED) {
        beep(1);
        adjustTime();
      }
    }
  }
}

/*** Initialize servo motors */
void initServoMotors() {
  if(SD.exists("/yaml/SC_BasicConfig.yaml")){
    system_config.loadConfig(SD, "/yaml/SC_BasicConfig.yaml");
  }else{
    system_config.loadConfig(LittleFS, "/yaml/SC_BasicConfig.yaml");
  }
  // servo
  servo.begin(system_config.getServoInfo(AXIS_X)->pin,
              system_config.getServoInfo(AXIS_X)->start_degree,
              system_config.getServoInfo(AXIS_X)->offset,
              system_config.getServoInfo(AXIS_Y)->pin,
              system_config.getServoInfo(AXIS_Y)->start_degree,
              system_config.getServoInfo(AXIS_Y)->offset,
              (ServoType)system_config.getServoType());
  delay(2000);

  system_config.getServoInfo(AXIS_X)->start_degree;
  system_config.getServoInfo(AXIS_Y)->start_degree;
}

void setupRestAPIs() {
  /// register REST-API
  myServer.registerPostApi("hello_api", handleHello);
  myServer.registerPostApi("face", handleFace);
  myServer.registerPostApi("tts", handleTexttospeech);
  myServer.registerPostApi("gemini", handleTalkGemini);
  myServer.registerPostApi("move", handleMove);
  myServer.registerPostApi("asr", handleAsr);
  myServer.registerPostApi("get_file_list", handleGetFileList);
  myServer.registerPostApi("get_file", handleGetFile);
  myServer.registerPostApi("save_file", handleSaveFile);
  myServer.registerPostApi("command", handleCommand);
  myServer.registerGetApi("camera_image", handleCameraImage);
  myServer.registerGetApi("stream", handleStreamPath); 
  //myServer.loadApiConfig("/api.yml");
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

  M5.Display.setFont(&fonts::lgfxJapanGothic_16);
  M5.Display.setTextSize(1);
  M5.Display.setBrightness(100);

  if (!M5.Rtc.isEnabled()) {
    Serial.println("RTC not found.");
    for (;;) { vTaskDelay(500);}
  }

  Serial.begin(115200);

  mountSd();
  mountLitteFs();

  initServoMotors();
  /*
  servo_interval_s* servo_interval = system_config.getServoInterval(AvatarMode::NORMAL);
  servo_interval_s* servo_interval_sing = system_config.getServoInterval(AvatarMode::SINGING);
  */
  // Camera
  CoreS3.Camera.begin();
  CoreS3.Camera.sensor->set_framesize(CoreS3.Camera.sensor, FRAMESIZE_QVGA);
  delay(500);

  // Wifi
  setupWifi("/wlan.json");
  if (WiFi.status() == WL_CONNECTED) {
    beep(1);
    adjustTime();
  } else {
    beep(2);
  }

  // Start Web service
  // WebServer
  myServer.setDocumentRoot("/html");
  setupRestAPIs();
  myServer.start();

  /// Touch buttons
  touchButton.createButton(0, 200, 100, 40, callbackBtnA);
  touchButton.createButton(105, 200, 100, 40, callbackBtnB);
  touchButton.createButton(210, 200, 100, 40, callbackBtnC);

  /// Mic
  auto mic_cfg = M5.Mic.config();
  mic_cfg.sample_rate = SAMPLE_RATE;
  mic_cfg.stereo = false;
  mic_cfg.noise_filter_level = (mic_cfg.noise_filter_level + 8) & 255;
  mic_cfg.magnification = 24;
  M5.Mic.config(mic_cfg);

  // Avatar and interaction_id
  interactionId = loadFile("/gemini_interaction");
  avatar.setBatteryIcon(true);
  avatar.init(8);
  avatar.setSpeechFont(&fonts::lgfxJapanGothic_12);

  // Timer event
  //M5.Rtc.setAlarmIRQ(120+randNum);
}

void loop() {
  M5.update();
  myServer.update();
  touchButton.update();

  // Check IRQ event
  if(M5.Rtc.getIRQstatus()){
    randNum = random(1, 51);
    int face = randNum % 6;    
    avatar.setExpression(expressions[face]);

    int m_id = randNum % 10;
    if (m_id > 1 && m_id < 6) { myMotion(&servo, m_id); }

    // Set next
    M5.Rtc.setAlarmIRQ(120+randNum);
  }

  // Update battery level
  avatar.setBatteryStatus( M5.Power.isCharging(),
          M5.Power.getBatteryLevel());

  // Flick actions
  int state = touchButton.getState();
  switch(state){
    case FlickMotion::Right:
      touchButton.resetState();
      servo.moveDeltaXY(10, 0, 500);
      break;
    case FlickMotion::Left:
      servo.moveDeltaXY(-10, 0, 500);
      touchButton.resetState();
      break;
    case FlickMotion::Down:
      touchButton.resetState();
#if RT_VERSION
      servo.moveDeltaXY(0, -5, 500);
#else
      servo.moveDeltaXY(0, 5, 500);
#endif
      break;
    case FlickMotion::Up:
#if RT_VERSION
      servo.moveDeltaXY(0, 5, 500);
#else
      servo.moveDeltaXY(0, -5, 500);
#endif
      touchButton.resetState();
      break;
    default:
      break;
  }
  
}
