/*
 */
#pragma once
#include <M5WebServer.h>
#include <Utils.h>

/** POST */
void handleHello(void *arg);
void handleGetFileList(void *arg);
void handleGetFile(void *arg);
void handleSaveFile(void *arg);

/** GET */
void handleCameraImage(void *arg);
void handleStreamPath(void *arg);
