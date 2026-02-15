/*
 * Power BI Dashboard Display - Phase 1: Skeleton
 *
 * Validates: Display, Touch, WiFi, NVS persistence
 *
 * On first boot (no saved settings):
 *   - Shows setup screen, prompts via Serial Monitor
 *   - Enter WiFi SSID, password, and Blob Storage URL
 *   - Saves to NVS flash for persistence
 *
 * On subsequent boots:
 *   - Loads credentials from flash automatically
 *   - Connects to WiFi
 *
 * Touch anywhere = shows touch coordinates on screen
 * Touch "RESET SETTINGS" button = clears NVS and reboots
 *
 * Serial Monitor: 115200 baud, line ending = Newline
 *
 * Required Libraries:
 *   - Arduino_GFX-master  (manufacturer version)
 *   - Touch_GT911         (manufacturer version)
 *
 * Board: ESP32S3 Dev Module
 * Flash: 16MB QIO 80MHz
 * PSRAM: OPI
 * Partition: 8MB APP
 * Board Package: ESP32 Arduino v2.0.14
 */

#include <Arduino_GFX_Library.h>
#include <Wire.h>
#include <Touch_GT911.h>
#include <WiFi.h>
#include <Preferences.h>

// ----- Display Setup -----
#define GFX_BL 38

Arduino_ESP32RGBPanel *bus = new Arduino_ESP32RGBPanel(
    39 /* CS */, 48 /* SCK */, 47 /* SDA */,
    18 /* DE */, 17 /* VSYNC */, 16 /* HSYNC */, 21 /* PCLK */,
    11 /* R0 */, 12 /* R1 */, 13 /* R2 */, 14 /* R3 */, 0 /* R4 */,
    8 /* G0 */, 20 /* G1 */, 3 /* G2 */, 46 /* G3 */, 9 /* G4 */, 10 /* G5 */,
    4 /* B0 */, 5 /* B1 */, 6 /* B2 */, 7 /* B3 */, 15 /* B4 */
);

Arduino_ST7701_RGBPanel *gfx = new Arduino_ST7701_RGBPanel(
    bus, GFX_NOT_DEFINED /* RST */, 0 /* rotation */,
    true /* IPS */, 480 /* width */, 480 /* height */,
    st7701_type1_init_operations, sizeof(st7701_type1_init_operations),
    true /* BGR */,
    10 /* hsync_front_porch */, 8 /* hsync_pulse_width */, 50 /* hsync_back_porch */,
    10 /* vsync_front_porch */, 8 /* vsync_pulse_width */, 20 /* vsync_back_porch */
);

// ----- Touch Setup -----
Touch_GT911 touch(19, 45, -1, -1, 480, 480);

// ----- NVS Storage -----
Preferences prefs;

// Settings
String saved_ssid;
String saved_pass;
String saved_blob_url;
bool settings_exist = false;
bool is_connected = false;

// Reset button bounds
#define BTN_X 140
#define BTN_Y 420
#define BTN_W 200
#define BTN_H 45

// Touch debounce
unsigned long last_touch_ms = 0;
#define TOUCH_DEBOUNCE_MS 300

// ----- Color helpers -----
#define COLOR_DARK_BG    0x1082
#define COLOR_GREY       0x7BEF
#define COLOR_DARK_GREY  0x4208

// ----- NVS Functions -----

void saveSettings(const String &ssid, const String &pass, const String &blobUrl) {
    prefs.begin("pbidash", false);
    prefs.putString("ssid", ssid);
    prefs.putString("pass", pass);
    prefs.putString("bloburl", blobUrl);
    prefs.putBool("configured", true);
    prefs.end();
    Serial.println("Settings saved to NVS.");
}

bool loadSettings() {
    prefs.begin("pbidash", true);
    bool configured = prefs.getBool("configured", false);
    if (configured) {
        saved_ssid = prefs.getString("ssid", "");
        saved_pass = prefs.getString("pass", "");
        saved_blob_url = prefs.getString("bloburl", "");
    }
    prefs.end();
    return configured;
}

void clearSettings() {
    prefs.begin("pbidash", false);
    prefs.clear();
    prefs.end();
    Serial.println("Settings cleared from NVS.");
}

// ----- Display Helpers -----

void drawHeader() {
    gfx->fillRect(0, 0, 480, 40, COLOR_DARK_BG);
    gfx->setTextSize(2);
    gfx->setTextColor(WHITE);
    gfx->setCursor(10, 12);
    gfx->print("PBI Dashboard Display");
}

void drawField(int y, const char *label, const char *value, uint16_t color) {
    gfx->fillRect(0, y, 480, 28, BLACK);
    gfx->setTextSize(2);
    gfx->setTextColor(COLOR_GREY);
    gfx->setCursor(20, y + 4);
    gfx->print(label);
    gfx->setTextColor(color);
    gfx->setCursor(180, y + 4);
    gfx->print(value);
}

void drawStatus(const char *msg, uint16_t color) {
    gfx->fillRect(0, 50, 480, 30, BLACK);
    gfx->setTextSize(2);
    gfx->setTextColor(color);
    gfx->setCursor(20, 55);
    gfx->print(msg);
}

void drawResetButton() {
    gfx->fillRoundRect(BTN_X, BTN_Y, BTN_W, BTN_H, 8, RED);
    gfx->drawRoundRect(BTN_X, BTN_Y, BTN_W, BTN_H, 8, WHITE);
    gfx->setTextSize(2);
    gfx->setTextColor(WHITE);
    gfx->setCursor(BTN_X + 15, BTN_Y + 14);
    gfx->print("RESET SETTINGS");
}

String maskString(const String &s) {
    if (s.length() <= 3) return s;
    String masked = s.substring(0, 3);
    for (unsigned int i = 3; i < s.length() && i < 20; i++) masked += '*';
    return masked;
}

String truncateUrl(const String &url) {
    if (url.length() <= 30) return url;
    return url.substring(0, 27) + "...";
}

// ----- Test Pattern -----

void drawTestPattern() {
    // Draw color bars to validate display
    int barHeight = 60;
    uint16_t colors[] = {RED, GREEN, BLUE, YELLOW, CYAN, MAGENTA, WHITE, 0x7BEF};
    const char *names[] = {"RED", "GREEN", "BLUE", "YELLOW", "CYAN", "MAGENTA", "WHITE", "GREY"};

    gfx->fillRect(0, 90, 480, 480 - 90 - 60, BLACK);

    for (int i = 0; i < 8; i++) {
        int y = 90 + i * barHeight;
        if (y + barHeight > BTN_Y) break;
        gfx->fillRect(0, y, 480, barHeight - 2, colors[i]);
        // Draw label with contrasting outline
        gfx->setTextSize(2);
        gfx->setTextColor(i == 6 ? BLACK : WHITE);
        gfx->setCursor(200, y + 20);
        gfx->print(names[i]);
    }

    gfx->setTextSize(1);
    gfx->setTextColor(WHITE);
    gfx->setCursor(20, BTN_Y - 15);
    gfx->print("Touch anywhere to test. Touch RESET to clear settings.");
}

// ----- Serial Input -----

String readSerialLine() {
    String input = "";
    while (true) {
        if (Serial.available()) {
            char c = Serial.read();
            if (c == '\n' || c == '\r') {
                if (input.length() > 0) return input;
                continue;
            }
            input += c;
        }
        delay(10);
    }
}

void runSerialSetup() {
    drawStatus("Setup required - open Serial Monitor", YELLOW);

    gfx->setTextSize(2);
    gfx->setTextColor(WHITE);
    gfx->setCursor(20, 110);
    gfx->print("No saved settings found.");

    gfx->setTextColor(CYAN);
    gfx->setCursor(20, 150);
    gfx->print("Open Arduino Serial Monitor");
    gfx->setCursor(20, 175);
    gfx->print("(115200 baud, Newline ending)");

    gfx->setTextColor(YELLOW);
    gfx->setCursor(20, 220);
    gfx->print("Follow the prompts to enter");
    gfx->setCursor(20, 245);
    gfx->print("WiFi & blob storage settings.");

    gfx->setTextColor(COLOR_GREY);
    gfx->setCursor(20, 295);
    gfx->print("Waiting for Serial input...");

    // Prompt 1: SSID
    Serial.println();
    Serial.println("=== PBI Dashboard Setup ===");
    Serial.println();
    Serial.println("Step 1/3: Enter your WiFi SSID:");
    saved_ssid = readSerialLine();
    Serial.printf("  SSID: %s\n", saved_ssid.c_str());
    drawField(330, "SSID:", saved_ssid.c_str(), GREEN);

    // Prompt 2: Password
    Serial.println();
    Serial.println("Step 2/3: Enter your WiFi password:");
    saved_pass = readSerialLine();
    Serial.println("  Password set.");
    drawField(360, "Pass:", maskString(saved_pass).c_str(), GREEN);

    // Prompt 3: Blob URL
    Serial.println();
    Serial.println("Step 3/3: Enter your Azure Blob Storage URL:");
    Serial.println("  (e.g. https://myaccount.blob.core.windows.net/container/dashboard.rgb565)");
    saved_blob_url = readSerialLine();
    Serial.printf("  Blob URL: %s\n", saved_blob_url.c_str());
    drawField(390, "URL:", truncateUrl(saved_blob_url).c_str(), GREEN);

    // Save to NVS
    saveSettings(saved_ssid, saved_pass, saved_blob_url);

    Serial.println();
    Serial.println("All settings saved to flash!");
    Serial.println("These persist across power cycles.");
    Serial.println();

    settings_exist = true;
}

// ----- WiFi Connection -----

void connectWifi() {
    drawStatus("Connecting to WiFi...", YELLOW);
    Serial.printf("Connecting to '%s'...\n", saved_ssid.c_str());

    WiFi.mode(WIFI_STA);
    WiFi.begin(saved_ssid.c_str(), saved_pass.c_str());
    WiFi.setAutoReconnect(true);

    int attempts = 0;
    while (!WiFi.isConnected() && attempts < 40) {
        delay(500);
        Serial.print(".");
        int dotX = 20 + (attempts % 20) * 22;
        gfx->fillCircle(dotX, 80, 4, (attempts % 2) ? YELLOW : COLOR_DARK_GREY);
        attempts++;
    }
    gfx->fillRect(0, 73, 480, 16, BLACK);
    Serial.println();

    is_connected = WiFi.isConnected();
}

void drawConnectedScreen() {
    gfx->fillRect(0, 50, 480, 40, BLACK);

    if (is_connected) {
        drawStatus("WiFi Connected!", GREEN);

        char buf[64];

        snprintf(buf, sizeof(buf), "%s", WiFi.localIP().toString().c_str());
        drawField(90, "IP:", buf, GREEN);

        int rssi = WiFi.RSSI();
        const char *quality;
        uint16_t color;
        if (rssi > -50)      { quality = "Excellent"; color = GREEN; }
        else if (rssi > -60) { quality = "Good";      color = GREEN; }
        else if (rssi > -70) { quality = "Fair";      color = YELLOW; }
        else                 { quality = "Weak";      color = RED; }
        snprintf(buf, sizeof(buf), "%d dBm (%s)", rssi, quality);
        drawField(120, "Signal:", buf, color);

        drawField(150, "SSID:", saved_ssid.c_str(), WHITE);
        drawField(180, "Blob:", truncateUrl(saved_blob_url).c_str(), CYAN);

        Serial.printf("Connected! IP: %s, RSSI: %d dBm\n",
                      WiFi.localIP().toString().c_str(), rssi);
        Serial.printf("Blob URL: %s\n", saved_blob_url.c_str());
    } else {
        drawStatus("WiFi Connection FAILED", RED);
        drawField(110, "SSID:", saved_ssid.c_str(), RED);
        drawField(150, "Tip:", "Touch RESET to re-enter", YELLOW);
        Serial.println("WiFi connection failed!");
    }
}

// ----- Touch Handler -----

void drawTouchFeedback(int x, int y) {
    // Show coordinates in a bar at the bottom of screen
    gfx->fillRect(0, BTN_Y - 30, 480, 25, COLOR_DARK_BG);
    gfx->setTextSize(2);
    gfx->setTextColor(CYAN);
    char buf[32];
    snprintf(buf, sizeof(buf), "Touch: X=%d  Y=%d", x, y);
    gfx->setCursor(130, BTN_Y - 25);
    gfx->print(buf);

    // Draw a small crosshair at touch point
    gfx->drawLine(x - 10, y, x + 10, y, CYAN);
    gfx->drawLine(x, y - 10, x, y + 10, CYAN);
}

// ----- Main -----

void setup() {
    Serial.begin(115200);
    Serial.println("PBI Dashboard Display - Phase 1");

    // Init display
    gfx->begin(16000000);
    gfx->fillScreen(BLACK);
    pinMode(GFX_BL, OUTPUT);
    digitalWrite(GFX_BL, HIGH);

    // Init touch
    Wire.begin(19, 45);
    touch.begin();
    touch.setRotation(ROTATION_NORMAL);

    drawHeader();

    // Try to load settings from NVS
    settings_exist = loadSettings();

    if (settings_exist && saved_ssid.length() > 0) {
        Serial.println("Settings loaded from NVS:");
        Serial.printf("  SSID: %s\n", saved_ssid.c_str());
        Serial.printf("  Blob URL: %s\n", saved_blob_url.c_str());
    } else {
        settings_exist = false;
        runSerialSetup();
    }

    // Connect to WiFi
    connectWifi();
    drawConnectedScreen();

    // Draw the test pattern below the status fields
    drawTestPattern();

    // Draw reset button
    drawResetButton();

    Serial.println();
    Serial.println("Phase 1 Ready!");
    Serial.println("- Touch anywhere to see coordinates");
    Serial.println("- Touch RESET SETTINGS to clear NVS and reboot");
}

void loop() {
    touch.read();

    if (touch.isTouched) {
        unsigned long now = millis();
        if (now - last_touch_ms < TOUCH_DEBOUNCE_MS) return;
        last_touch_ms = now;

        // Invert coordinates as required by this hardware
        int x = map(touch.points[0].x, 480, 0, 0, 479);
        int y = map(touch.points[0].y, 480, 0, 0, 479);

        Serial.printf("Touch: x=%d, y=%d\n", x, y);

        // Check reset button
        if (x >= BTN_X && x <= BTN_X + BTN_W &&
            y >= BTN_Y && y <= BTN_Y + BTN_H) {
            Serial.println("Reset button pressed!");

            // Visual feedback
            gfx->fillRoundRect(BTN_X, BTN_Y, BTN_W, BTN_H, 8, 0x7800);
            delay(200);

            clearSettings();

            gfx->fillRect(0, 50, 480, 370, BLACK);
            drawStatus("Settings CLEARED!", YELLOW);
            drawField(150, "Action:", "NVS wiped", RED);
            drawField(190, "Next:", "Rebooting in 3s...", YELLOW);

            delay(3000);
            ESP.restart();
        }

        // Show touch feedback
        drawTouchFeedback(x, y);
    }

    // Monitor WiFi state changes
    static bool prev_connected = is_connected;
    bool now_connected = WiFi.isConnected();
    if (now_connected != prev_connected) {
        is_connected = now_connected;
        drawConnectedScreen();
        drawTestPattern();
        drawResetButton();
        prev_connected = now_connected;
    }

    delay(50);
}
