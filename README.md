# このリポジトリについて
これは、RT版およびタカオ版ｽﾀｯｸﾁｬﾝのサンプルプログラムです。

[m5stack-avatar](https://github.com/stack-chan/m5stack-avatar)と[stackchan-arduino](https://github.com/stack-chan/stackchan-arduino)をベースに使用して、GoogleAPIを利用した対話機能を付加したものです。

RT版ｽﾀｯｸﾁｬﾝのArduino版のベースとして開発しています。

プログラムのビルドは、PlatformIOを使って下さい。

## 主な機能

- Webサーバー機能（REST-APIによる制御、コンテンツ作成 etc.）
- GoogleSpeechAPIを使った音声認識、音声合成
- Gemini InteractionAPIを使った対話生成
- 内蔵カメラによるストリーミング

## 使用しているライブラリ
:::note info
<font color="red">注意：</font>
このプログラムを利用するには、ArduinoJson var.7 を使用しているため m5stack-avatarとstackchan-arduinoを修正したものを使用しています。オフィシャルのライブラリでは、不具合が発生する可能性があります。
:::

- M5Unified
- ArduinoJson@7.2.2
- M5CoreS3@1.0.1
- [stachchan-arduino](https://github.com/haraisao/stackchan-arduino/tree/dev)
    - ESP32Servo
    - ServoEasing
    - ArduinoJson
    - Dynamixel2Arduino
    - YAMLDuino
    - SCServo
    - M5GFX
- [m5stack-avatar](https://github.com/haraisao/m5stack-avatar/tree/dev)

