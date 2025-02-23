/*********
  Rui Santos
  Complete project details at https://RandomNerdTutorials.com/esp32-cam-take-photo-display-web-server/
  
  The above copyright notice and this permission notice shall be included in all
  copies or substantial portions of the Software.
*********/
//#define SerialPrintEnable // Easy way to toggle Serial Print message

#include <pgmspace.h>
#include "website_html.h"
#include "WiFi.h"
#include "esp_camera.h"
#include "esp_timer.h"
#include "img_converters.h"
#include "Arduino.h"
#include "soc/soc.h"           // Disable brownout problems
#include "soc/rtc_cntl_reg.h"  // Disable brownout problems
#include "driver/rtc_io.h"
#include <ESPAsyncWebServer.h>
#include <SPIFFS.h>
#include <FS.h>

// Your network credentials
const char* ssid = "Sanjith";
const char* password = "awfq3737";
IPAddress local_IP(192, 168, 232, 232);
IPAddress gateway_address(192, 168, 232, 68);
IPAddress subnet_mask(255, 255, 255, 0);
IPAddress DNS_1(192, 168, 232, 68);
IPAddress DNS_2(0, 0, 0, 0);
// ESP32 CAM's MAC Address = 2C:BC:BB:85:36:F4

// Create AsyncWebServer object on port 80
AsyncWebServer server(80);

boolean takeNewPhoto = false;

// Photo File Name to save in SPIFFS
#define FILE_PHOTO "/photo.jpg"

// Button Interrupt
boolean isButtonPressed = false;
// Flash
boolean toggleFlash = false;
uint8_t isFlashOn = 1;
// Video Mode
boolean enableVideo = false;

// OV2640 camera module pins (CAMERA_MODEL_AI_THINKER)
#define PWDN_GPIO_NUM     32
#define RESET_GPIO_NUM    -1
#define XCLK_GPIO_NUM      0
#define SIOD_GPIO_NUM     26
#define SIOC_GPIO_NUM     27
#define Y9_GPIO_NUM       35
#define Y8_GPIO_NUM       34
#define Y7_GPIO_NUM       39
#define Y6_GPIO_NUM       36
#define Y5_GPIO_NUM       21
#define Y4_GPIO_NUM       19
#define Y3_GPIO_NUM       18
#define Y2_GPIO_NUM        5
#define VSYNC_GPIO_NUM    25
#define HREF_GPIO_NUM     23
#define PCLK_GPIO_NUM     22

#define FLASH_PIN         4
#define BUTTON_PIN        13  // Labelled IO13 on the board


// Interrupt Service Routine (ISR)
// Use IRAM_ATTR so that the ISR is placed in IRAM (a requirement for ESP32)
void IRAM_ATTR onButtonPress() {
  takeNewPhoto = true;
  detachInterrupt(digitalPinToInterrupt(BUTTON_PIN)); // temporarily disable interrupt
}


void setup() {
  // Serial port for debugging purposes
  Serial.begin(115200);

  // Assign a static IP Address
  if (!WiFi.config(local_IP, gateway_address, subnet_mask, DNS_1, DNS_2)) {
    Serial.println("Static wifi address configuration failed!");
  }
  // Connect to Wi-Fi using the Static IP Address
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.println("Connecting to WiFi...");
  }
  if (!SPIFFS.begin(true)) {
    Serial.println("An Error has occurred while mounting SPIFFS");
    ESP.restart();
  }
  else {
    delay(500);
    Serial.println("SPIFFS mounted successfully");
  }

  // Print ESP32's Static Local IP Address
  Serial.print("IP Address: http://");
  Serial.println(WiFi.localIP());

  // Turn-off the 'brownout detector'
  WRITE_PERI_REG(RTC_CNTL_BROWN_OUT_REG, 0);

  // Flash Setup
  pinMode(FLASH_PIN, OUTPUT);
  digitalWrite(FLASH_PIN, LOW);

  // Attach an interrupt to the button pin
  // Trigger the ISR on a falling edge (when the button is pressed)
  attachInterrupt(digitalPinToInterrupt(BUTTON_PIN), onButtonPress, RISING);

  // OV2640 camera module
  camera_config_t config;
  config.ledc_channel = LEDC_CHANNEL_0;
  config.ledc_timer = LEDC_TIMER_0;
  config.pin_d0 = Y2_GPIO_NUM;
  config.pin_d1 = Y3_GPIO_NUM;
  config.pin_d2 = Y4_GPIO_NUM;
  config.pin_d3 = Y5_GPIO_NUM;
  config.pin_d4 = Y6_GPIO_NUM;
  config.pin_d5 = Y7_GPIO_NUM;
  config.pin_d6 = Y8_GPIO_NUM;
  config.pin_d7 = Y9_GPIO_NUM;
  config.pin_xclk = XCLK_GPIO_NUM;
  config.pin_pclk = PCLK_GPIO_NUM;
  config.pin_vsync = VSYNC_GPIO_NUM;
  config.pin_href = HREF_GPIO_NUM;
  config.pin_sccb_sda = SIOD_GPIO_NUM;
  config.pin_sccb_scl = SIOC_GPIO_NUM;
  config.pin_pwdn = PWDN_GPIO_NUM;
  config.pin_reset = RESET_GPIO_NUM;
  //config.xclk_freq_hz = 20000000; //Base Freq
  config.xclk_freq_hz = 16000000; // Ideal Freq
  config.pixel_format = PIXFORMAT_JPEG;

  if (psramFound()) {
    config.frame_size = FRAMESIZE_UXGA;
    config.jpeg_quality = 10;
    config.fb_count = 2;
  } else {
    config.frame_size = FRAMESIZE_SVGA;
    config.jpeg_quality = 12;
    config.fb_count = 1;
  }
  // Camera init
  esp_err_t err = esp_camera_init(&config);
  if (err != ESP_OK) {
    Serial.printf("Camera init failed with error 0x%x", err);
    ESP.restart();
  }

  // Route for root / web page
  server.on("/", HTTP_GET, [](AsyncWebServerRequest * request) {
    request->send(200, "text/html", index_html);
  });

  server.on("/capture", HTTP_GET, [](AsyncWebServerRequest * request) {
    takeNewPhoto = true;
    request->send(200, "text/plain", "Taking Photo");
  });

  server.on("/saved-photo", HTTP_GET, [](AsyncWebServerRequest * request) {
    request->send(SPIFFS, FILE_PHOTO, "image/jpg", false);
  });

  server.on("/flash", HTTP_GET, [](AsyncWebServerRequest * request) {
    toggleFlash = true;
    request->send(200, "text/plain", "Toggling Flash");
  });

  server.on("/videoON", HTTP_GET, [](AsyncWebServerRequest * request) {
    enableVideo = true;
    request->send(200, "text/plain", "Toggling Video");
  });

  server.on("/videoOFF", HTTP_GET, [](AsyncWebServerRequest * request) {
    enableVideo = false;
    request->send(200, "text/plain", "Toggling Video");
  });

  // Start server
  server.begin();
}


void loop() {
  while (enableVideo) {
    capturePhotoSaveSpiffs();
    delay(1);
  }
  
  if (takeNewPhoto) {
    capturePhotoSaveSpiffs();
    takeNewPhoto = false;
    attachInterrupt(digitalPinToInterrupt(BUTTON_PIN), onButtonPress, RISING); // re-enable interrupts
  }
  if (toggleFlash) {
    if (isFlashOn) {
      digitalWrite(FLASH_PIN, HIGH);
      Serial.println("Flash is ON"); 
    } else {
      digitalWrite(FLASH_PIN, LOW);
      Serial.println("Flash is OFF"); 
    }
    isFlashOn = 1 - isFlashOn;
    toggleFlash = false;
  }
  delay(1);
}


// Check if photo capture was successful
bool checkPhoto( fs::FS &fs ) {
  File f_pic = fs.open( FILE_PHOTO );
  unsigned int pic_sz = f_pic.size();
  return ( pic_sz > 100 );
}

// Capture Photo and Save it to SPIFFS
void capturePhotoSaveSpiffs( void ) {
  camera_fb_t * fb = NULL; // pointer
  bool ok = 0; // Boolean indicating if the picture has been taken correctly

  do {
    // Take a photo with the camera
    #ifdef SerialPrintEnable
    Serial.println("Taking a photo...");
    #endif

    fb = esp_camera_fb_get();
    if (!fb) {
      #ifdef SerialPrintEnable
      Serial.println("Camera capture failed");
      #endif
      return;
    }

    // Photo file name
    Serial.printf("Picture file name: %s\n", FILE_PHOTO);
    File file = SPIFFS.open(FILE_PHOTO, FILE_WRITE);

    // Insert the data in the photo file
    if (!file) {
      #ifdef SerialPrintEnable
      Serial.println("Failed to open file in writing mode");
      #endif
    }
    else {
      file.write(fb->buf, fb->len); // payload (image), payload length

      #ifdef SerialPrintEnable
      Serial.print("The picture has been saved in ");
      Serial.print(FILE_PHOTO);
      Serial.print(" - Size: ");
      Serial.print(file.size());
      Serial.println(" bytes");
      #endif
    }
    // Close the file
    file.close();
    esp_camera_fb_return(fb);

    // check if file has been correctly saved in SPIFFS
    ok = checkPhoto(SPIFFS);
  } while ( !ok );
}