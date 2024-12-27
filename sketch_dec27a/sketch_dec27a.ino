#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <Adafruit_GFX.h>
#include <Adafruit_LEDBackpack.h>

// LCD 設定
LiquidCrystal_I2C lcd(0x27, 16, 2);

// LED Matrix 設定
Adafruit_8x8matrix matrix = Adafruit_8x8matrix();

// 遊戲參數
const int numRows = 8;              // LED 行數 (8x8 模組)
const int numCols = 8;              // LED 列數
int ledMatrix[numRows][numCols] = {0}; // LED 狀態 (0: 滅, 1: 亮)
int targetRow = 7;                  // 按鈕對應目標列 (8x8 最後一行)
int score = 0;                      // 玩家分數
int level = 1;                      // 遊戲等級 (難度)
unsigned long speed = 1000;         // 下落速度 (初始: 1000ms)
unsigned long lastUpdateTime = 0;   // 上次更新 LED 的時間

// 按鈕設定 (4 個按鈕分別接到 GPIO32~GPIO35)
const int buttonPins[4] = {32, 33, 34, 35};
bool buttonPressed[4] = {false, false, false, false};

// 初始化
void setup() {
  // 初始化 LCD
  lcd.init();
  lcd.backlight();
  lcd.setCursor(0, 0);
  lcd.print("Reaction Game");

  memset(ledMatrix, 0, sizeof(ledMatrix)); // 清空 LED 矩陣

  // 初始化 LED Matrix
  matrix.begin(0x70); // 預設 I2C 地址
  matrix.clear();
  matrix.writeDisplay();

  // 初始化按鈕
  for (int i = 0; i < 4; i++) {
    pinMode(buttonPins[i], INPUT_PULLUP);
  }

  delay(2000); // 顯示初始化畫面
  lcd.clear();
}

// 更新 LED Matrix 的顯示
void updateLEDs() {
  matrix.clear();
  for (int i = 0; i < numRows; i++) {
    for (int j = 0; j < numCols; j++) {
      if (ledMatrix[i][j] == 1) {
        matrix.drawPixel(j, i, LED_ON);
      }
    }
  }
  matrix.writeDisplay();
}

// LED 下落邏輯

void dropLEDs() {
  // 隨機生成新一層 LED（僅在第 1、3、5、7 列）
  for (int i = 0; i < numCols; i++) {
    if (i % 2 == 0) { // 只對第 1, 3, 5, 7 列操作 (列索引為偶數)
      ledMatrix[0][i] = random(0, 2); // 隨機亮或滅
    } else {
      ledMatrix[0][i] = 0; // 清空其他列
    }
  }

  // 將每層 LED 往下移
  for (int i = numRows - 1; i > 0; i--) {
    for (int j = 0; j < numCols; j++) {
      ledMatrix[i][j] = ledMatrix[i - 1][j];
    }
  }

  // 清空移動後上一層的 LED（原先的第一行）
  for (int i = 0; i < numCols; i++) {
    if (i % 2 == 0) { // 只清空第 1, 3, 5, 7 列
      ledMatrix[0][i] = 0;
    }
  }
  updateLEDs(); // 更新顯示
}

// 按鈕判斷邏輯
void checkButtonPress() {
  static unsigned long lastDebounceTime[4] = {0, 0, 0, 0}; // 記錄按鈕防抖時間
  const unsigned long debounceDelay = 50; // 防抖延遲 (50ms)
  int hit = 0;

  for (int i = 0; i < 4; i++) {
    int buttonState = digitalRead(buttonPins[i]);
    if (buttonState == LOW && (millis() - lastDebounceTime[i]) > debounceDelay) {
      buttonPressed[i] = true; // 記錄按鈕被按下
      lastDebounceTime[i] = millis(); // 更新防抖時間

      // 檢查按鈕對應的列是否有亮燈的 LED
      if (ledMatrix[targetRow][i * 2] == 1 || ledMatrix[targetRow][i * 2 + 1] == 1) {
        hit++;
        ledMatrix[targetRow][i * 2] = 0;     // 清除 LED
        ledMatrix[targetRow][i * 2 + 1] = 0; // 清除 LED
      }

      if (hit > 0) {
        score += hit;
        lcd.clear();
        lcd.setCursor(0, 0);
        lcd.print("Score:");
        lcd.print(score);
      } else {
        lcd.clear();
        lcd.setCursor(0, 0);
        lcd.print("Miss!");
      }

      buttonPressed[i] = false;
      updateLEDs();
    }
  }
}

// 遊戲難度提升
void increaseDifficulty() {
  if (score % 5 == 0 && level < 10) {
    level++;
    speed = max(200UL, speed - 100); // 修正型別衝突
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Level Up!");
    lcd.setCursor(0, 1);
    lcd.print("Level:");
    lcd.print(level);
    delay(1000);
    lcd.clear();
  }
}

// 遊戲結束檢查
bool checkGameOver() {
  for (int i = 0; i < numCols; i++) {
    if (ledMatrix[targetRow][i] == 1) { // 如果底部有亮燈的 LED
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print("Game Over!");
      lcd.setCursor(0, 1);
      lcd.print("Score: ");
      lcd.print(score);
      return true; // 遊戲結束
    }
  }
  return false;
}

void loop() {
  // 如果遊戲結束，等待玩家按下任意按鈕重新開始
  if (checkGameOver()) {
    while (true) {
      for (int i = 0; i < 4; i++) {
        if (digitalRead(buttonPins[i]) == LOW) { // 等待按下按鈕
          score = 0;      // 重置分數
          level = 1;      // 重置等級
          speed = 1000;   // 重置速度
          memset(ledMatrix, 0, sizeof(ledMatrix)); // 清空 LED 矩陣
          lcd.clear();
          lcd.print("Restarting...");
          delay(2000);
          lcd.clear();
          return; // 跳出等待重新開始遊戲
        }
      }
    }
  }

  // 檢查按鈕輸入
  checkButtonPress();

  // LED 下落控制
  unsigned long currentTime = millis();
  if (currentTime - lastUpdateTime > speed) { // 判斷是否需要更新 LED
    lastUpdateTime = currentTime;
    dropLEDs(); // LED 下落
    increaseDifficulty(); // 提升遊戲難度
  }
}
