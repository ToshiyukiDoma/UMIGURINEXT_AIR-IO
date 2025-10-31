// Copyright (c) 2024 inonote
// Use of this source code is governed by the MIT License
// that can be found at https://github.com/inonote/umgrio/blob/main/LICENSE

#include <Adafruit_NeoPixel.h>

typedef unsigned char uchar;

Adafruit_NeoPixel LED1 = Adafruit_NeoPixel(3, 5, NEO_GRB + NEO_KHZ800);
Adafruit_NeoPixel LED2 = Adafruit_NeoPixel(3, 6, NEO_GRB + NEO_KHZ800);

#define SLIDER_COMMAND_SLIDER_REPORT        0x01
#define SLIDER_COMMAND_LED_BRIGHTNESS       0x02
#define SLIDER_COMMAND_SUBSCRIBE_SLIDER     0x03
#define SLIDER_COMMAND_UNSUBSCRIBE_SLIDER   0x04
#define SLIDER_COMMAND_RESET                0x10
#define SLIDER_COMMAND_EXCEPTION            0xEE
#define SLIDER_COMMAND_BOARD_INFO           0xF0

#define SLIDER_BD_NUMBER "99330   "
#define SLIDER_BD_DEV_CLASS 0xA1
#define SLIDER_BD_CHIP_NUM "00000"
#define SLIDER_BD_FW_VER 1

#define JVS_SYN 0xE0
#define JVS_ESC 0xD0

// 受信データバッファ
uchar receivedBuf[0xFE];
int receivedBufLength = 0;
bool receivedBufInvalid = false;

bool subscribed = false;

int ignoredTimer = 0; // LED輝度報告が送られてこなかった時間

void setup() {
  LED1.begin();
  LED2.begin();
  LED1.show();
  LED2.show();
  Serial.begin(115200);
   pinMode(8, INPUT_PULLUP);
   pinMode(9, INPUT_PULLUP);
   pinMode(10, INPUT_PULLUP);
   pinMode(18, INPUT_PULLUP);
   pinMode(19, INPUT_PULLUP);
   pinMode(20, INPUT_PULLUP);

  onReset();
}

void loop() {
  recieveData();

  if (subscribed) {
    uchar frame[5];
    frame[0] = JVS_SYN;
    frame[1] = SLIDER_COMMAND_SLIDER_REPORT;
    frame[2] = 2;
    frame[3] = 0;
    frame[4] = 0;
    
    for(int i = 0; i < 6; ++i) {
      frame[4] |= analogRead(i) > 40;
      frame[4] <<= 1;
    }
   
    sendData(frame, sizeof(frame) / sizeof(frame[0]));
  }

  if (ignoredTimer == 281) {
    onReset();
  }
  if (ignoredTimer < 281)
    ++ignoredTimer;
  delay(16);
}

void onData() {
  if (receivedBufLength < 4)
    return;
  
  switch(receivedBuf[1]) {
    case SLIDER_COMMAND_SUBSCRIBE_SLIDER:
      subscribed = true;
      break;
    case SLIDER_COMMAND_UNSUBSCRIBE_SLIDER:
      subscribed = false;
      break;
    
    // 色設定
    case SLIDER_COMMAND_LED_BRIGHTNESS:
      if (receivedBufLength < 4 + 19)
        break;
      ignoredTimer = 0;
      LED1.setBrightness(receivedBuf[3]);
      LED2.setBrightness(receivedBuf[3]);
      for(int i = 0; i < 3; ++i) {      
          LED1.setPixelColor(i, receivedBuf[4 + i * 3 + 2], receivedBuf[4 + i * 3 + 1], receivedBuf[4 + i * 3]);
      }
      for(int i = 3; i < 6; ++i) {      
          LED2.setPixelColor(i - 3, receivedBuf[4 + i * 3 + 2], receivedBuf[4 + i * 3 + 1], receivedBuf[4 + i * 3]);
      }
      LED1.show();
      LED2.show();
      break;
    
    case SLIDER_COMMAND_RESET:
      onReset();
      {
        uchar frame[3];
        frame[0] = JVS_SYN;
        frame[1] = SLIDER_COMMAND_RESET;
        frame[2] = 0;
        sendData(frame, sizeof(frame) / sizeof(frame[0]));
      }
      break;
    
    case SLIDER_COMMAND_BOARD_INFO:
      {
        uchar frame[21];
        frame[0] = JVS_SYN;
        frame[1] = SLIDER_COMMAND_BOARD_INFO;
        frame[2] = 18;
        memcpy(frame + 3, SLIDER_BD_NUMBER, 8);
        frame[11] = SLIDER_BD_DEV_CLASS;
        memcpy(frame + 12, SLIDER_BD_CHIP_NUM, 5);
        frame[17] = 0xFF;
        frame[18] = SLIDER_BD_FW_VER;
        frame[19] = 0x00;
        frame[20] = 0x64;
        sendData(frame, sizeof(frame) / sizeof(frame[0]));
      }
      break;
  }
}

void onReset() {
  LED1.clear();
  LED2.clear();
  LED1.show();
  LED2.show();
}

void recieveData() {
  while(Serial.available() > 0) {
    int rawByte = Serial.read();
    if (rawByte == JVS_SYN) {
      if (receivedBufLength > 0 && !receivedBufInvalid)
        onData();
      receivedBufLength = 0;
      receivedBufInvalid = false;
    }

    if (rawByte != -1) {
      if (receivedBufLength >= 0xFE)
        receivedBufInvalid = true;
      if (!receivedBufInvalid) {
        receivedBuf[receivedBufLength] = (uchar)rawByte;
        ++receivedBufLength;
      }
    }
  }
  
  if (receivedBufLength >= 4 && receivedBuf[2] == receivedBufLength - 4 && !receivedBufInvalid) {
    onData();
    receivedBufLength = 0;
    receivedBufInvalid = false;
  }
}

void sendData(uchar frame[], int frameLength) {
  uchar sum = JVS_SYN;
  for(int i = 0; i < frameLength; ++i) {
    sum -= frame[i];
    if (i && (frame[i] == JVS_SYN || frame[i] == JVS_ESC)) {
      Serial.write(JVS_ESC);
      Serial.write(frame[i] - 1);
    }
    else {
      Serial.write(frame[i]);
    }
  }
  sum = (sum - 0xFF) & 0xFF;
  if (sum == JVS_SYN || sum == JVS_ESC) {
    Serial.write(JVS_ESC);
    Serial.write(sum - 1);
  }
  else {
    Serial.write(sum);
  }
}

uchar calcCheckSum(uchar frame[], int frameLength) {
  int sum = JVS_SYN;
  for(int i = 0; i < frameLength; ++i)
    sum -= frame[i];
  return (sum - 0xFF) & 0xFF;
}

