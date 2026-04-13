#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <WiFi.h>
#include <time.h>
#include <Arduino.h>

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_SDA 8
#define OLED_SCL 9
#define OLED_ADDR 0x3C

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_ADDR);

float ballX = SCREEN_WIDTH / 2.0;
float ballY = SCREEN_HEIGHT / 2.0;
float ballRadius = 5;
float speedX = 2;
float speedY = 1.5;
int bounceCount = 0;
bool timeSynced = false;

// 震动参数
int shakeOffsetX = 0;
int shakeOffsetY = 0;
int shakeDuration = 0;
unsigned long shakeStartTime = 0;
#define SHAKE_INTENSITY 2
#define SHAKE_DURATION_MS 300

// WiFi 配置
const char* ssid = "ICT-Monitor4";
const char* password = "C402-402";

// 进度条参数
#define PROGRESS_BAR_X 14
#define PROGRESS_BAR_Y 40
#define PROGRESS_BAR_WIDTH 100
#define PROGRESS_BAR_HEIGHT 8

void drawLoadingScreen(const char* title, int progress, int maxProgress) {
    display.clearDisplay();
    display.setTextSize(1);
    display.setTextColor(SSD1306_WHITE);

    int titleLen = strlen(title);
    int titleX = (SCREEN_WIDTH - titleLen * 6) / 2;
    display.setCursor(titleX, 15);
    display.print(title);

    display.drawRect(PROGRESS_BAR_X, PROGRESS_BAR_Y, PROGRESS_BAR_WIDTH, PROGRESS_BAR_HEIGHT, SSD1306_WHITE);

    int fillWidth = (progress * (PROGRESS_BAR_WIDTH - 2)) / maxProgress;
    display.fillRect(PROGRESS_BAR_X + 1, PROGRESS_BAR_Y + 1, fillWidth, PROGRESS_BAR_HEIGHT - 2, SSD1306_WHITE);

    display.display();
}

void drawSuccessScreen(const char* message) {
    display.clearDisplay();
    display.setTextSize(1);
    display.setTextColor(SSD1306_WHITE);

    int msgLen = strlen(message);
    int msgX = (SCREEN_WIDTH - msgLen * 6) / 2;
    display.setCursor(msgX, 28);
    display.print(message);

    display.display();
    delay(500);
}

void setup() {
    Serial.begin(115200);
    Wire.begin(OLED_SDA, OLED_SCL);

    if (!display.begin(SSD1306_SWITCHCAPVCC, OLED_ADDR)) {
        Serial.println("SSD1306 init failed!");
        while (1);
    }

    display.clearDisplay();
    display.setTextSize(1);
    display.setTextColor(SSD1306_WHITE);
    display.display();

    // WiFi 连接阶段
    WiFi.begin(ssid, password);
    Serial.print("Connecting to WiFi");

    for (int i = 0; i < 30; i++) {
        if (WiFi.status() == WL_CONNECTED) break;
        delay(500);
        Serial.print(".");
        drawLoadingScreen("Connecting...", i + 1, 30);
    }

    if (WiFi.status() == WL_CONNECTED) {
        Serial.println("\nWiFi connected!");
        drawSuccessScreen("WiFi OK!");

        // NTP 同步阶段
        configTime(8 * 3600, 0, "pool.ntp.org", "time.nist.gov");

        Serial.print("Syncing time");
        time_t now;
        for (int i = 0; i < 15; i++) {
            if (time(&now) > 8 * 3600 * 2) break;
            delay(500);
            Serial.print(".");
            drawLoadingScreen("Syncing Time", i + 1, 15);
        }

        if (time(&now) > 8 * 3600 * 2) {
            Serial.println("\nTime synced!");
            timeSynced = true;
            drawSuccessScreen("Time OK!");
        } else {
            Serial.println("\nTime sync failed, using uptime");
            drawSuccessScreen("No NTP!");
        }
    } else {
        Serial.println("\nWiFi connection failed!");
        drawLoadingScreen("WiFi Failed!", 0, 1);
        delay(2000);
    }
}

void loop() {
    ballX += speedX;
    ballY += speedY;

    if (ballX - ballRadius <= 0 || ballX + ballRadius >= SCREEN_WIDTH) {
        speedX = -speedX;
        ballX = constrain(ballX, ballRadius, SCREEN_WIDTH - ballRadius);
        bounceCount++;
        shakeStartTime = millis();
        shakeDuration = SHAKE_DURATION_MS;
    }

    if (ballY - ballRadius <= 0 || ballY + ballRadius >= SCREEN_HEIGHT) {
        speedY = -speedY;
        ballY = constrain(ballY, ballRadius, SCREEN_HEIGHT - ballRadius);
        bounceCount++;
        shakeStartTime = millis();
        shakeDuration = SHAKE_DURATION_MS;
    }

    // 更新震动偏移
    if (shakeDuration > 0) {
        unsigned long elapsed = millis() - shakeStartTime;
        if (elapsed < shakeDuration) {
            shakeOffsetX = random(-SHAKE_INTENSITY, SHAKE_INTENSITY + 1);
            shakeOffsetY = random(-SHAKE_INTENSITY, SHAKE_INTENSITY + 1);
        } else {
            shakeDuration = 0;
            shakeOffsetX = 0;
            shakeOffsetY = 0;
        }
    }

    display.clearDisplay();

    // 左上角：反弹次数
    display.setCursor(shakeOffsetX, shakeOffsetY);
    display.print("Hits: ");
    display.println(bounceCount);

    // 右下角：时间
    char timeStr[9];
    if (timeSynced) {
        time_t now = time(nullptr);
        struct tm* t = localtime(&now);
        sprintf(timeStr, "%02d:%02d:%02d", t->tm_hour, t->tm_min, t->tm_sec);
    } else {
        unsigned long secs = millis() / 1000;
        int h = (secs / 3600) % 24;
        int m = (secs / 60) % 60;
        int s = secs % 60;
        sprintf(timeStr, "%02d:%02d:%02d", h, m, s);
    }

    display.setCursor(SCREEN_WIDTH - 50 + shakeOffsetX, SCREEN_HEIGHT - 8 + shakeOffsetY);
    display.print(timeStr);

    // 绘制小球本体
    int drawX = (int)ballX + shakeOffsetX;
    int drawY = (int)ballY + shakeOffsetY;
    display.fillCircle(drawX, drawY, (int)ballRadius, SSD1306_WHITE);

    display.display();
    delay(16);
}
