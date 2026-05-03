#include <M5Unified.h>
#include "M5WebServer.h"
#include "Utils.h"

String M5WebServer::getContentType(String filename) {
    if (filename.endsWith(".html") || filename.endsWith(".htm")) return "text/html";
    if (filename.endsWith(".css")) return "text/css";
    if (filename.endsWith(".js"))  return "application/javascript";
    if (filename.endsWith(".png")) return "image/png";
    if (filename.endsWith(".jpg") || filename.endsWith(".jpeg")) return "image/jpeg";
    if (filename.endsWith(".ico")) return "image/x-icon";
    if (filename.endsWith(".json")) return "application/json";
    return "text/plain";
}

// --- 【重要】BluePrintのようにファイルを自動検索して応答するハンドラ ---
void M5WebServer::handleFileSystemFallback() {
    String path = server.uri();
    M5_LOGI("Access Request: %s", path.c_str());

    // 1. ルート（/）アクセスの場合は index.html に読み替え
    if (path.endsWith("/")) path += "index.html";

    // 2. フルパスを作成 (/html/path)
    String fullPath = topDir + path;

    if(isFileExists(fullPath)){
        File file = getFileDescriptor(fullPath);
        server.streamFile(file, getContentType(fullPath));
        file.close();
        return; 
    }

    // 4. ファイルもAPIも見つからなかった場合に初めて404を返す
    M5_LOGE("File Not Found: %s", fullPath.c_str());
    server.send(404, "text/plain", "404: Not Found");
}


// api.yml を読み込んで URL パスと関数をバインド
void M5WebServer::loadApiConfig(const char* filepath) {
    if(!isFileExists(String(filepath))) return;
    File file = getFileDescriptor(String(filepath));

    while (file.available()) {
        String line = file.readStringUntil('\n');
        line.trim();
        if (line.length() == 0 || line.startsWith("#")) continue;

        int sep = line.indexOf(':');
        if (sep != -1) {
            String urlPath = line.substring(0, sep);
            String funcName = line.substring(sep + 1);
            urlPath.trim(); funcName.trim();

            if (_registeredHandlers.count(funcName)) {
                server.on(urlPath, HTTPMethod::HTTP_POST, _registeredHandlers[funcName]);
                M5_LOGI("API Bind: %s -> %s", urlPath.c_str(), funcName.c_str());
            }
        }
    }
    file.close();
}

// WiFi接続 (SDの network.yml から読み込み)
void M5WebServer::connect_wlan_from_sd(const char* filepath) {
    if(!isFileExists(String(filepath))) {
        M5.Display.setTextColor(RED);
        M5.Display.println("WiFi config not found!");
        M5.Display.setTextColor(WHITE);
        return;
    }
    File file = getFileDescriptor(String(filepath));
    
    JsonDocument doc;
    DeserializationError error = deserializeJson(doc, file);
    file.close();

    if (error) {
        M5.Display.printf("JSON Parse Error: %s\n", error.c_str());
        return;
    }

    JsonObject networks = doc.as<JsonObject>();
    bool connected = false;

    // JSON内の各エントリ（Home, Work, Mobile等）を順番に試行
    for (JsonPair p : networks) {
        String profileName = p.key().c_str();
        const char* ssid = p.value()["essid"];
        const char* pass = p.value()["passwd"];

        M5.Display.printf("Connecting to %s...\n", profileName.c_str());
        M5_LOGI("Profile: %s, SSID: %s", profileName.c_str(), ssid);

        WiFi.begin(ssid, pass);

        // 10秒のタイムアウト判定
        unsigned long startAttemptTime = millis();
        while (WiFi.status() != WL_CONNECTED && millis() - startAttemptTime < 10000) {
            delay(500);
            //M5.Display.print(".");
            //M5_LOGI("Wait for connection: %s", profileName.c_str());
        }

        if (WiFi.status() == WL_CONNECTED) {
            connected = true;
            M5.Display.setTextColor(GREEN);
            M5.Display.printf("\nSuccess! IP: %s\n", WiFi.localIP().toString().c_str());
            M5.Display.setTextColor(WHITE);
            M5_LOGI("Success IP: %s",  WiFi.localIP().toString().c_str());
            break; // 接続できたらループを抜ける
        } else {
            M5.Display.println("\nTimeout / Failed.");
            WiFi.disconnect();
            delay(100);
        }
    }

    if (connected == false) {
        M5_LOGE("All Wifi attempts failed.");
        M5.Display.setTextColor(RED);
        M5.Display.println("All WiFi attempts failed.");
        M5.Display.setTextColor(WHITE);
    }   
}


String M5WebServer::getArg(const char* name) {
    if(server.hasArg(name)){
        return server.arg(name);
    }
    return "";
}

String M5WebServer::getBody() {
    if(server.hasArg("plain")){
        return server.arg("plain");
    }
    return "";
}

void M5WebServer::response(int code, const char* content_type, const char* content) {
    server.send(code, content_type, content);
}

#if 0
M5WebServer myServer(80, "/html");

void handleHello() {
    myServer.getServer().send(200, "application/json", "{\"message\":\"Hello API\"}");
}

void handleGetFile() {
    if (!mServer.getServer().hasArg("plain")){
        myServer.getServer().send(500, "application/json", "{\"res\":\"error\"}");
        return;
    }
    String postData = myServer.getServer().arg("plain");
    JsonDocument doc;
    deserializeJson(doc, postData);
    const char* fileName = doc["file_name"];
    if (SD.exists(fileName)) {
        File f = SD.open(fileName, FILE_READ);
        String content = f.readString();
        f.close();
        JsonDocument res;
        res["data"] = content;
        String output;
        serializeJson(res, output);
        myServer.getServer().send(200, "application/json", output);
    } else {
        myServer.getServer().send(404, "application/json", "{\"res\":\"error\"}");
    }
}

//  状態切替API
void handleTerminate() {
    myServer.getServer().send(200, "text/plain", "Server Terminating...");
    delay(500);
    ESP.restart(); // ArduinoでのTerminate例として再起動
}

// --- メインルーチン ---
void setup() {
    auto cfg = M5.config();
    M5.begin(cfg);
    SD.begin();

    // 1. API処理ロジックの登録（名前と関数の紐付け）
    myServer.registerHandler("hello_api", handleHello);

    // 2. WiFi接続
    myServer.connect_wlan_from_sd("/network.yml");

    // 3. api.yml からURLパスを動的に割り当て
    // 例: "/api/test: hello_api" と書いてあれば /api/test が有効になる
    myServer.loadApiConfig("/api.yml");

    // 4. サーバー開始
    myServer.start();
    M5.Display.println("WebServer Running...");
}

void loop() {
    M5.update();
    myServer.update();
}
#endif