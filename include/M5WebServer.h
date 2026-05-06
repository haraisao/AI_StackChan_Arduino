/**
 * 
 */
#pragma once
#include <WiFi.h>
#include <WebServer.h>
#include <SD.h>
#include <LittleFS.h>
#include <ArduinoJson.h>
#include <map>
#include <functional>


class M5WebServer {
private:
    WebServer server;
    String topDir; // "/html" など
    bool started = false;
    std::map<String, std::function<void()>> _registeredHandlers;

    String getContentType(String filename);

    void handleFileSystemFallback();

public:
    M5WebServer(int p = 80, String top = "/html") 
      : server(p), topDir(top) {
        server.onNotFound([this]() {
            this->handleFileSystemFallback();
        });
    }

    // API関数の登録用
    void registerHandler(String name, std::function<void()> func) {
        _registeredHandlers[name] = func;
    }

    void registerApi(String name, std::function<void()> func) {
        _registeredHandlers[name] = func;
        server.on("/"+name, HTTPMethod::HTTP_POST, func);
    }

    void registerGetApi(String name, std::function<void()> func) {
        _registeredHandlers[name] = func;
        server.on("/"+name, func);
    }

    void setDocumentRoot(String top){
        if(SD.exists(top)){
            topDir = "/sd" + top;
        }else{
            topDir = top;
        }
    }
    // api.yml を読み込んで URL パスと関数をバインド
    void loadApiConfig(const char* filepath);

    // WiFi接続 (SDの network.yml から読み込み)
    void connect_wlan_from_sd(const char* filepath);

    void start() { server.begin(); started = true; }
    void update() { if (started) server.handleClient(); }
    WebServer& getServer() { return server; }

    String getArg(const char* name);
    String getBody();
    void response(int code, const char* content_type, const char* data);

};