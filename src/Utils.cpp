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
#include <WiFiAP.h>

/**
 * @brief 
 * 
 */
void adjustTime(){
  //configTime(9 * 3600, 0, "pool.ntp.org");
  configTime(9 * 3600, 0, "ntp.nict.jp", "ntp.jst.mfeed.ad.jp");
  M5.Display.print("NTP Syncing");
  Serial.print("Waiting for NTP time sync...");
  time_t now = time(nullptr);
  while (now < 8 * 3600 * 2) {
    delay(500);
    M5.Display.print(".");
    Serial.print(".");
    now = time(nullptr);
  }
  
  m5::rtc_datetime_t currentTime;
  struct tm timeinfo;
  getLocalTime(&timeinfo);

  currentTime.date.year=timeinfo.tm_year+1900;
  currentTime.date.month=timeinfo.tm_mon+1;
  currentTime.date.date=timeinfo.tm_mday;
  currentTime.time.hours=timeinfo.tm_hour;
  currentTime.time.minutes=timeinfo.tm_min;
  currentTime.time.seconds=timeinfo.tm_sec;
  M5.Rtc.setDateTime(&currentTime);

  M5.Display.println("\nTime synced.");
  Serial.println("\nTime synchronized.");
}

String getCurrentTime(int style) {
  struct tm timeinfo;
  const char* week[7] = {"Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat"}; 
  if (!getLocalTime(&timeinfo)) {
    return "";
  }

  char date_buff[11];
  sprintf(date_buff, "%04d/%02d/%02d", \
    timeinfo.tm_year+1900, timeinfo.tm_mon+1, timeinfo.tm_mday, week[timeinfo.tm_wday]);
  char time_buff[6];
  sprintf(time_buff,"%02d:%02d", timeinfo.tm_hour, timeinfo.tm_min);
  if(style==0){
    return String(date_buff) +" "+ String(time_buff);
  }else if(style==1){
    return String(date_buff);
  }else if(style==2){
    return String(time_buff);
  }else{
    char time_buff2[9];
    sprintf(time_buff2,"%02d:%02d:%02d", timeinfo.tm_hour, timeinfo.tm_min, timeinfo.tm_sec);
    return String(time_buff2);
  }
}
/**
 * @brief 
 * 
 * @return true 
 * @return false 
 */
bool mountLitteFs(){
  if (!LittleFS.begin(true)) {
    M5.Display.println("LittleFS Error!");
    Serial.println("An Error has occurred while mounting LittleFS");
    return false;
  }
  return true;
}

/**
 * @brief 
 * 
 * @param fname 
 * @return String 
 */
String readRootCA(String fname){
  String path = "/rootCA/" + fname;
  File file = LittleFS.open(path.c_str());
  if (!file) {
    //M5.Display.println("Cert load failed!");
    Serial.println("Failed to open certificate file");
    return "";
  }
  String rootCACertificate = file.readString();
  file.close();
  //M5.Display.println("Cert loaded.");
  Serial.println("Certificate loaded from LittleFS.");
  return rootCACertificate;
}

bool mountSd(int trial) {
  int count = 0;
  while (false == SD.begin(GPIO_NUM_4, SPI, 25000000) && count < trial) {
    delay(200);
    M5_LOGI("Wait for SD card (%d)", count);
    count += 1;
  }
  if (count == trial) return false;
  return true;
}
/**
 * @brief 
 * 
 * @param fname 
 * @return String 
 */
String loadFile(String fname){
  File file;
  if(fname.startsWith("/sd")){
    String path2 = fname.substring(3);
    const char *filepath = path2.c_str();
    if (!SD.exists(filepath)) {
      M5_LOGE("File [%s] not found!(SD)", filepath);
      return "";
    }
    file = SD.open(filepath, FILE_READ);
  }else{
    const char *filepath = fname.c_str();
    if (!LittleFS.exists(filepath)) {
      M5_LOGE("File [%s] config not found!", filepath);
      return "";
    }
    file = LittleFS.open(filepath, FILE_READ);
  }
  String data = file.readString();
  file.close();
  return data;
}

/**
 * @brief 
 * 
 * @param path 
 * @return true 
 * @return false 
 */
bool isFileExists(String path) {
  if(path.startsWith("/sd")){
    String path2 = path.substring(3);
    const char *filepath = path2.c_str();
    //M5_LOGI("File: %s", filepath);
    return SD.exists(filepath);
  }else{
    const char *filepath = path.c_str();
    return LittleFS.exists(filepath);
  }
}

/**
 * @brief Get the File Descriptor object
 * 
 * @param path 
 * @return File 
 */
File getFileDescriptor(String path) {
  if(path.startsWith("/sd")){
    String path2 = path.substring(3);
    const char *filepath = path2.c_str();
    //M5_LOGI("\nOpenFile: %s, %s", filepath, path2.c_str());
    return SD.open(filepath, FILE_READ);
  }else{
    const char *filepath = path.c_str();
    return LittleFS.open(filepath, FILE_READ);
  }
}

/**
 * @brief 
 * 
 * @param fs 
 * @param dirname 
 * @param flist 
 * @return true 
 * @return false 
 */
bool listDir(fs::FS &fs, const char* dirname, std::map<String, std::vector<String>> &flist){
  File root = fs.open(dirname);
  flist["dir"] = std::vector<String>();
  flist["file"] = std::vector<String>();

  if(!root){
    Serial.println("Error: fail to open directory.");
    return false;
  }
  if(!root.isDirectory()){
    Serial.printf("%s is not a directory.\n", dirname);
    return false;
  }
  File file = root.openNextFile();
  while(file){
    if(file.isDirectory()){
      flist["dir"].push_back(file.name());
    } else {
      flist["file"].push_back(file.name());
    }
    file = root.openNextFile();
  }
  return true;
}

/**
 * @brief 
 * 
 * @param fname 
 * @param contents 
 */
void saveFile(String fname, const char *contents){
  if(fname.startsWith("/sd")){
    M5_LOGI("Error: not support to save file on SD card.");
    return;
  }
  File file = LittleFS.open(fname.c_str(), FILE_WRITE);
  if(!file) {
    Serial.printf("Error: fail to open file: %s\n", fname.c_str());
    return;
  }
  if(file.print(contents)) {
    Serial.printf("Fail saved: %s\n", fname.c_str());
  }else{
    Serial.printf("Error: fail to save file: %s\n", fname.c_str());
  }
  file.close();
}
/**
 * @brief 
 * 
 * @param path 
 */
void removeFile(String path){
  if(path.startsWith("/sd")){
    M5_LOGI("Error: not support to remove file on SD card.");
    return;
  }
  if(LittleFS.remove(path)){
    Serial.printf("File deletec: %s\n", path.c_str());
  }else{
    Serial.printf("Error: fail to delete file: %s\n", path.c_str());
  }
}

/**
 * @brief 
 * 
 * @param src 
 * @param delims 
 * @param slist 
 */
void splitString(String src, std::vector<String>& delims, std::vector<String>& slist) {
  int n = 0;
  while(n < src.length()) {
    int m=src.length();
    for(int i=0;i < delims.size(); i++){
      int idx = src.indexOf(delims[i], n);
      if(idx > 0 && idx < m ){
        m = idx;
      }
    }
    String sub = String(src.substring(n, m));
    sub.trim();
    if(sub.length() > 0){
      slist.push_back(sub);
    }
    n=m+1;
  }
  return;
}

/**
 * @brief Get the Api Key object
 * 
 * @param key 
 * @return String 
 */
String getApiKey(String key) {
    String keyData = loadFile("/apikey.txt");
    int index=0;
    int datalen=keyData.length();
    int n = 0;
    if((n=keyData.indexOf(key,n)) < 0) return "";
    n += key.length()+1;
    int endOfKey = keyData.indexOf("\n", n);
    if (endOfKey > 0){
       return keyData.substring(n, endOfKey);
    }else{
       return keyData.substring(n);
    }
}

/**
 * @brief 
 * 
 * @param audioBuffer 
 * @param samplesCount 
 * @return float 
 */
float calcRms(byte *audioBuffer, int samplesCount) {
    int64_t sumSquares = 0;
    for (int i = 0; i < samplesCount; i++) {
        sumSquares += audioBuffer[i] * audioBuffer[i];
    }
    float rms = sqrt(sumSquares / samplesCount);
    return rms;
}

/**
 * @brief 
 * 
 * @param audio_frame 
 * @param threshold 
 * @return true 
 * @return false 
 */
bool myVad(int16_t *audio_frame, float threshold){
    // 1. DCオフセットの除去（波形のズレを直す）
    int64_t sum = 0;
    for (int i = 0; i < FRAME_SIZE; i++) {
      sum += audio_frame[i];
    }
    int16_t mean = sum / FRAME_SIZE; // 平均値を出す

    // 2. RMS（音量エネルギー）の計算
    int64_t sum_sq = 0;
    for (int i = 0; i < FRAME_SIZE; i++) {
      int16_t sample = audio_frame[i] - mean; 
      sum_sq += sample * sample;
    }
    float rms = sqrt(sum_sq / FRAME_SIZE);
    //M5_LOGI("RMS: %f", rms);
    // 3. 閾値判定 (VAD)
    return (rms > threshold);
}

/**
 * @brief 
 * 
 * @param audio_data 
 * @param audio_len 
 * @param outlen 
 * @return unsigned char* 
 */
unsigned char *b64EncodeAudio(int16_t* audio_data, int audio_len, int *outlen) {
  size_t b64_size=0;
  int buflen = audio_len*sizeof(uint16_t);
  mbedtls_base64_encode(nullptr, 0, &b64_size, (const unsigned char*)audio_data, buflen);
  unsigned char* base64_buffer = (unsigned char*)heap_caps_malloc(b64_size, MALLOC_CAP_SPIRAM);
  if (base64_buffer == nullptr){
    M5_LOGE("Fail to encode audio data");
    *outlen = 0;
  } else {
    size_t output_len = 0;
    int ret = mbedtls_base64_encode(base64_buffer, b64_size, &output_len, (const unsigned char*)audio_data, buflen);
    *outlen = output_len;
  }
  return base64_buffer;
}

/**
 * @brief Get the Speech From Mic object
 * 
 * @param audio_data 
 * @param max_sec 
 * @return int 
 */
int getSpeechFromMic(int16_t* audio_data, int max_sec) {
    int16_t audio_frame[FRAME_SIZE];

    int last_state = 0;
    int state = 0;
    //M5.Display.print("Start...");

    int idx = 0;
    int max_frame = max_sec * SAMPLE_RATE/FRAME_SIZE;

    //float threshold = 300;
    float threshold = 1000;
    M5.Mic.begin();
    M5.Mic.record(audio_frame, FRAME_SIZE, SAMPLE_RATE);
    while (M5.Mic.isRecording()) ;
    for(int i=0; i<max_frame; i++){
        M5.Mic.record(audio_frame, FRAME_SIZE, SAMPLE_RATE, false);
        while(M5.Mic.isRecording()) ;

        int is_speech = myVad(audio_frame, threshold);
        if (is_speech > 0) {
            if (state <= 0) {
                last_state = 1;
                idx = 1;
                memcpy(audio_data, audio_frame, FRAME_SIZE*2);
            }
            state = 30;
        }else{
            if(state > 0) {
                state -= 1;
                if(state <= 0){
                    last_state = -1;
                }
            }
        }
        if (last_state != 0) {
            M5.Display.setCursor(0, 0);
            if (state > 0) {
                //M5.Display.fillScreen(TFT_GREEN);
                //M5.Display.setTextColor(TFT_BLACK);
                //M5.Display.println("SPEECH DETECTED");
                M5_LOGI("SPEECH DETECTED");
            } else {
                //M5.Display.fillScreen(TFT_BLACK);
                //M5.Display.setTextColor(TFT_WHITE);
                //M5.Display.println("Silence...");
                M5_LOGI("Silence...");
                break;
            }
            last_state = 0;
        }else{
            if (idx > 0){
                memcpy(&audio_data[FRAME_SIZE*idx], audio_frame, FRAME_SIZE*2);
                idx += 1;
            }
        }

    }
    //M5.Display.println("...END");
    M5.Mic.end();
    return FRAME_SIZE*idx;
}


// WiFi接続 
bool connect_wlan(const char* filepath) {
  Serial.printf("\nConnect Wifi from: %s\n", filepath);
    if(!isFileExists(String(filepath))) {
      Serial.printf("WiFi config not found!: %s\n", filepath);
      return false;
    }
    File file = getFileDescriptor(String(filepath));
    JsonDocument doc;
    DeserializationError error = deserializeJson(doc, file);
    file.close();

    if (error) {
      Serial.printf("JSON Parse Error: %s\n", error.c_str());
      return false;
    }

    JsonObject networks = doc.as<JsonObject>();
    bool connected = false;

    // JSON内の各エントリ（Home, Work, Mobile等）を順番に試行
    for (JsonPair p : networks) {
        String profileName = p.key().c_str();
        const char* ssid = p.value()["essid"];
        const char* pass = p.value()["passwd"];

        Serial.printf("\nConnecting to %s...\n", profileName.c_str());
        WiFi.begin(ssid, pass);
        // タイムアウト判定
        int timeout_sec = 5;
        unsigned long startAttemptTime = millis();
        while (WiFi.status() != WL_CONNECTED && millis() - startAttemptTime < timeout_sec * 1000) {
            delay(500);
        }

        if (WiFi.status() == WL_CONNECTED) {
          connected = true;
          Serial.printf("Success IP: %s",  WiFi.localIP().toString().c_str());
          return true;
        } else {
          Serial.println("\nTimeout / Failed.");
          WiFi.disconnect();
          delay(100);
        }
    }

    if (connected == false) {
      Serial.println("All Wifi attempts failed.");
    }
    return false;
}

void setupWifi(String conf_file){
  if(connect_wlan(conf_file.c_str())) return;
  conf_file = "/sd"+conf_file;
  if(connect_wlan(conf_file.c_str())) return;
  // setup access point
  const char *ssid = "M5StackAP";
  const char *password = "stack-chan";
  WiFi.softAP(ssid, password);
  IPAddress myIP = WiFi.softAPIP();
  Serial.print("AP IP address: ");
  Serial.println(myIP);
  return;
}


int convertToInt(uint8_t *buff) {
  int val = buff[0] | buff[1] << 8 | buff[2] << 16 | buff[3] << 24;
  return val;
}

int convertToShort(uint8_t *buff) {
  int val = buff[0] | buff[1] << 8;
  return val;
}

void talkWav(m5avatar::Avatar *avatar, uint8_t *wav_data,
           unsigned long start_time, int duration_ms, float MAX_RMS) {
  //--------- Taking...
  unsigned long elapsed_ms = millis() - start_time;
  int16_t *pcm_data = (int16_t *)(wav_data+44);
  int total_samples = convertToInt(wav_data+4) - 36;
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
  //----------------
}



/**
 * @brief 
 * 
 * @param doc 
 * @param size 
 * @return uint8_t* 
 */
uint8_t *serializeJsonSpiram(JsonDocument doc,  size_t *size){
  size_t doc_size = measureJson(doc);
  M5_LOGI("Doc size: %d", doc_size);
  uint8_t *buff = (uint8_t *)heap_caps_malloc(doc_size +1, MALLOC_CAP_SPIRAM);
  if(buff == nullptr) {
    M5_LOGE("Fail to alloc memorty");
    return nullptr;
  }
  memset(buff, 0, doc_size +1);
  serializeJson(doc, buff, doc_size+1);
  *size = doc_size;
  return buff;
}

/**
 * @brief 
 * 
 * @param client 
 * @param code 
 * @param contentLen 
 * @param chunk_flag 
 * @return String 
 */
String readHttpHeader(WiFiClientSecure *client, int *code, int *contentLen, bool *chunk_flag){
  String response = "";
  while (client->connected()) {
    String line = client->readStringUntil('\n');
    response += line + "\n";
    if (line.startsWith("HTTP/")){
      int n = line.indexOf(" ")+1;
      String res_code = line.substring(n, n+3);
      //M5_LOGI("===>%s", res_code.c_str());
      *code = res_code.toInt();
    }
    if (line.startsWith("Content-Length:")){
      *contentLen = line.substring(16).toInt();
    }
    if (line.startsWith("Transfer-Encoding: chunked")){
      *chunk_flag = true;
    }
    if (line == "\r") {
      break;
    }
  }
  return response;
}

/**
 * @brief 
 * 
 * @param client 
 * @param buffer 
 * @param total_length 
 * @return size_t 
 */
size_t sendRequestBody(WiFiClientSecure *client, unsigned char *buffer, int total_length) { 
  size_t bytes_sent = 0;
  size_t chunk_size = 4096; 
  
  while (bytes_sent < total_length) {
    size_t to_send = total_length - bytes_sent;
    if (to_send > chunk_size) to_send = chunk_size;
    client->write(buffer + bytes_sent, to_send);
    bytes_sent += to_send;
    delay(2); 
  }
  return bytes_sent;
}

/**
 * @brief 
 * 
 * @param client 
 * @param contentLen 
 * @param len 
 * @return uint8_t* 
 */
uint8_t *readResponseBody(WiFiClientSecure *client, int contentLen, int *len) {
  uint8_t* buffer = (uint8_t*)heap_caps_malloc(contentLen, MALLOC_CAP_SPIRAM);
  memset(buffer, 0, contentLen);
    
  if (buffer != nullptr) {
    size_t buffLen = 0;
    int size = 4096;
    unsigned long timeout_start = millis();
    while (client->connected() && (millis() - timeout_start < 1000)) {
      if((size=client->available())){
        size_t count = client->readBytes(buffer + buffLen, size);
        if (count > 0){
          buffLen += count;
        }
        timeout_start = millis();
      } else if (!client->connected()) {
        break; 
      } else {
        delay(1);
      }
    }
    *len = buffLen;
  }
  return buffer;
}

/**
 * @brief 
 * 
 */
void showRAM() {
  M5_LOGI("PSRAM Total: %d bytes\n", ESP.getPsramSize());
  M5_LOGI("PSRAM Free : %d bytes\n", ESP.getFreePsram());
}


/**
 * @brief 
 * 
 * @param url 
 * @param postData 
 * @param apikey 
 */
void sendHttpPostRequest(String url, String postData, String apikey) {
  WiFiClientSecure *client = new WiFiClientSecure;

  if (client) {
    client->setInsecure();
    client->setTimeout(20);

    HTTPClient https;
    
    if (https.begin(*client, url)) {
      //M5.Display.println("Sending POST...");
      Serial.println("Starting HTTPS POST...");

      https.addHeader("Content-Type", "application/json");
      https.addHeader("Authorization", "Bearer "+apikey);

      int httpResponseCode = https.POST(postData);

      if (httpResponseCode > 0) {
        //M5.Display.printf("HTTP Code: %d\n", httpResponseCode);
        Serial.printf("HTTP Response code: %d\n", httpResponseCode);
        String payload = https.getString();
        Serial.println("Response Payload: ");
        Serial.println(payload);
        //M5.Display.println("Check Serial for payload");
      } else {
        //M5.Display.printf("Error: %d\n", httpResponseCode);
        Serial.printf("Error code: %d\n", httpResponseCode);
        Serial.println(https.errorToString(httpResponseCode).c_str());
      }

      https.end();
    } else {
      //M5.Display.println("Connection failed.");
      Serial.println("Unable to connect to the server.");
    }
    delete client;
  } else {
    Serial.println("Unable to create client.");
  }
}
