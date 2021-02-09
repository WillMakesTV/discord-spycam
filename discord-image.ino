#include "arduino_secrets.h"
#include <WiFi.h>
#include <WiFiClientSecure.h>
#include "soc/soc.h"
#include "soc/rtc_cntl_reg.h"
#include "esp_camera.h"
#include <HTTPClient.h>

// CAMERA_MODEL_AI_THINKER
#define PWDN_GPIO_NUM 32
#define RESET_GPIO_NUM -1
#define XCLK_GPIO_NUM 0
#define SIOD_GPIO_NUM 26
#define SIOC_GPIO_NUM 27
#define Y9_GPIO_NUM 35
#define Y8_GPIO_NUM 34
#define Y7_GPIO_NUM 39
#define Y6_GPIO_NUM 36
#define Y5_GPIO_NUM 21
#define Y4_GPIO_NUM 19
#define Y3_GPIO_NUM 18
#define Y2_GPIO_NUM 5
#define VSYNC_GPIO_NUM 25
#define HREF_GPIO_NUM 23
#define PCLK_GPIO_NUM 22

// Define GPIO for PIR Motion Sensor
int gpioPIR = 15;

void setup()
{
  WRITE_PERI_REG(RTC_CNTL_BROWN_OUT_REG, 0);

  Serial.begin(115200);
  delay(10);

  // Connect to wifi
  WiFi.mode(WIFI_STA);
  Serial.println("");
  Serial.print("Connecting to ");
  Serial.println(SECRET_NETWORK_SSID);
  WiFi.begin(SECRET_NETWORK_SSID, SECRET_NETWORK_PASS);
  long int StartTime = millis();
  while (WiFi.status() != WL_CONNECTED)
  {
    delay(500);
    if ((StartTime + 10000) < millis())
      break;
  }

  Serial.println("");
  Serial.println("STAIP address: ");
  Serial.println(WiFi.localIP());
  Serial.println("");

  if (WiFi.status() != WL_CONNECTED)
  {
    Serial.println("Reset");

    ledcAttachPin(4, 3);
    ledcSetup(3, 5000, 8);
    ledcWrite(3, 10);
    delay(200);
    ledcWrite(3, 0);
    delay(200);
    ledcDetachPin(3);
    delay(1000);
    ESP.restart();
  }
  else
  {
    ledcAttachPin(4, 3);
    ledcSetup(3, 5000, 8);
    for (int i = 0; i < 5; i++)
    {
      ledcWrite(3, 10);
      delay(200);
      ledcWrite(3, 0);
      delay(200);
    }
    ledcDetachPin(3);
  }

  // Camera Config
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
  config.pin_sscb_sda = SIOD_GPIO_NUM;
  config.pin_sscb_scl = SIOC_GPIO_NUM;
  config.pin_pwdn = PWDN_GPIO_NUM;
  config.pin_reset = RESET_GPIO_NUM;
  config.xclk_freq_hz = 20000000;
  config.pixel_format = PIXFORMAT_JPEG;

  if (psramFound())
  {
    config.frame_size = FRAMESIZE_SXGA;
    config.jpeg_quality = 10; //0-63 lower number means higher quality
    config.fb_count = 2;
  }
  else
  {
    config.frame_size = FRAMESIZE_QVGA;
    config.jpeg_quality = 12; //0-63 lower number means higher quality
    config.fb_count = 1;
  }

  // camera init
  esp_err_t err = esp_camera_init(&config);
  if (err != ESP_OK)
  {
    Serial.printf("Camera init failed with error 0x%x", err);
    delay(1000);
    ESP.restart();
  }

  sensor_t *s = esp_camera_sensor_get();
  s->set_framesize(s, FRAMESIZE_XGA);
}

void loop()
{
  // Get PIR sensor value
  pinMode(gpioPIR, INPUT_PULLUP);
  int v = digitalRead(gpioPIR);
  Serial.println(v);

  // Detect motion
  if (v == 1)
  {
    // Send alert to Discord
    alerts2Discord();
    delay(10000);
  }
  delay(1000);
}
/**
 * Send photo to Discord via webhook
 */
String alerts2Discord()
{

  String getAll = "", getBody = "";

  // Initialize camera
  camera_fb_t *fb = NULL;
  fb = esp_camera_fb_get();
  if (!fb)
  {
    Serial.println("Camera capture failed");
    delay(1000);
    ESP.restart();
    return "Camera capture failed";
  }

  // Initalize connection to Discord
  WiFiClientSecure client_tcp;

  if (client_tcp.connect(SECRET_DISCORD_DOMAIN, 443))
  {
    // Set Discord CA Certificate
    client_tcp.setCACert(SECRET_DISCORD_CERT);

    Serial.println("Connected to " + String(SECRET_DISCORD_DOMAIN));

    // Build data for webhook
    String head = "--Cam\r\nContent-Disposition: form-data; name=\"Discord Spycam\"; filename=\"Discord-Spycam.jpg\"\r\nContent-Type: image/jpeg\r\n\r\n";
    String tail = "\r\n--Cam--\r\n";

    // Capture frame data
    uint16_t imageLen = fb->len;
    uint16_t extraLen = head.length() + tail.length();
    uint16_t totalLen = imageLen + extraLen;

    // Define content for POST to discord webhook
    client_tcp.println("POST " + String(SECRET_DISCORD_WEBHOOK) + " HTTP/1.1");
    client_tcp.println("Host: " + String(SECRET_DISCORD_DOMAIN));
    client_tcp.println("Content-Length: " + String(totalLen));
    client_tcp.println("Content-Type: multipart/form-data; boundary=Cam");
    client_tcp.println();
    client_tcp.print(head);

    // Capture frame buffer (photo)
    uint8_t *fbBuf = fb->buf;
    size_t fbLen = fb->len;

    for (size_t n = 0; n < fbLen; n = n + 1024)
    {

      if (n + 1024 < fbLen)
      {
        client_tcp.write(fbBuf, 1024);
        fbBuf += 1024;
      }
      else if (fbLen % 1024 > 0)
      {
        size_t remainder = fbLen % 1024;
        client_tcp.write(fbBuf, remainder);
      }
    }

    // Complete webhook data
    client_tcp.print(tail);

    // Close frame buffer and wait 10 seconds
    esp_camera_fb_return(fb);
    int waitTime = 10000;
    long startTime = millis();
    boolean state = false;

    while ((startTime + waitTime) > millis())
    {
      Serial.print(".");
      delay(100);
      while (client_tcp.available())
      {
        char c = client_tcp.read();
        if (c == '\n')
        {
          if (getAll.length() == 0)
            state = true;
          getAll = "";
        }
        else if (c != '\r')
          getAll += String(c);
        if (state == true)
          getBody += String(c);
        startTime = millis();
      }
      if (getBody.length() > 0)
        break;
    }
    client_tcp.stop();
  }
  else
  {
    getBody = "Connection to Discord failed.";
    Serial.println("Connection to Discord failed.");
  }

  return getBody;
}
