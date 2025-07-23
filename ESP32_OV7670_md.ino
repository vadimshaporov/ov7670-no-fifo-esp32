#include "OV7670.h"

#include <WiFi.h>
#include <WiFiClient.h>
#include "BMP.h"
#include <PubSubClient.h>

const int SIOD = 21; //SDA
const int SIOC = 22; //SCL

const int VSYNC = 32;
const int HREF = 33;

const int XCLK = 26;
const int PCLK = 25;

const int D0 = 27;
const int D1 = 23;
const int D2 = 19;
const int D3 = 18;
const int D4 = 15;
const int D5 = 12;
const int D6 = 14;
const int D7 = 13;
#define MD_PIN 4
#define ssid       "wifi-id"
#define password   "wifi-pwd"
const char* mqtt_server = "matt server";
const char* mqtt_topic = "mqtt topic";
const char* mqtt_user = "mqtt user";
const char* mqtt_pass = "mqtt pwd"; 

WiFiClient espClient;
PubSubClient mqtt_client(espClient);

OV7670 *camera;

WiFiServer server(80);
unsigned char bmpHeader[BMP::headerSize];

void serve() {
  WiFiClient client = server.available();
  if (client) 
  {
    String currentLine = "";
    while (client.connected()) 
    {
      if (client.available()) 
      {
        char c = client.read();

        if (c == '\n') 
        {
          if (currentLine.length() == 0) 
          {
            client.println("HTTP/1.1 200 OK");
            client.println("Content-type:text/html");
            client.println();
            client.print(
              "<style>body{margin: 0}\nimg{height: 100%; width: auto}</style>"
              "<img id='a' src='/camera' onload='this.style.display=\"initial\"; var b = document.getElementById(\"b\"); b.style.display=\"none\"; b.src=\"camera?\"+Date.now(); '>"
              "<img id='b' style='display: none' src='/camera' onload='this.style.display=\"initial\"; var a = document.getElementById(\"a\"); a.style.display=\"none\"; a.src=\"camera?\"+Date.now(); '>");
            client.println();
            break;
          } 
          else 
          {
            currentLine = "";
          }
        } 
        else if (c != '\r') 
        {
          currentLine += c;
        }
        
        if(currentLine.endsWith("GET /camera"))
        {
            client.println("HTTP/1.1 200 OK");
            client.println("Content-type:image/bmp");
            client.println();
            Serial.println("started writing bmp");
            client.write(bmpHeader, BMP::headerSize);
            
            Serial.print("Free heap memory: ");
            Serial.println(ESP.getFreeHeap());

            int blk_count = 0;
            blk_count = camera->yres/I2SCamera::blockSlice;//30, 60, 120
            for (int i=0; i<blk_count; i++) {

                if (i == 0) {
                    camera->startBlock = 1;
                    camera->endBlock = I2SCamera::blockSlice;
                }

                camera->oneFrame();
                
                size_t blockSize = camera->xres * I2SCamera::blockSlice * 2;

                client.write(camera->frame, blockSize);

                camera->startBlock += I2SCamera::blockSlice;
                camera->endBlock   += I2SCamera::blockSlice;
            }
        }
      }
    }
    client.stop();
  }
}

void setup_wifi() {
  delay(10);
  Serial.println("Connecting to Wi-Fi...");
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("WiFi connected!");
}

void reconnect_mqtt() {
  while (!mqtt_client.connected()) {
    Serial.print("Reconnecting...");
    if (!mqtt_client.connect("ESP32Client", mqtt_user, mqtt_pass)) {
      Serial.print("failed, rc=");
      Serial.print(mqtt_client.state());
      delay(500);
    }
  }
}

void setup() {
  delay(500);

  Serial.begin(115200);
  setup_wifi();
  mqtt_client.setServer(mqtt_server, 1883);
  pinMode(MD_PIN, INPUT); 

  String message = "esp32 client initialized";
  if (mqtt_client.publish(mqtt_topic, message.c_str())) {
    Serial.println("Message published successfully!");
  } else {
    Serial.println("Message publish failed!");
  }

  camera = new OV7670(OV7670::Mode::QVGA_RGB565, SIOD, SIOC, VSYNC, HREF, XCLK, PCLK, D0, D1, D2, D3, D4, D5, D6, D7);
  BMP::construct16BitHeader(bmpHeader, camera->xres, camera->yres);
  server.begin();
  Serial.println("Http web server started.");
}

void loop()
{
  int motionDetected = digitalRead(MD_PIN); // Read the sensor output
  if (motionDetected == HIGH) {
    Serial.println("Motion detected!");
    String message = "MOTION";
    reconnect_mqtt();
    if (mqtt_client.publish(mqtt_topic, message.c_str())) {
      Serial.println("Message published successfully!");
    } else {
      Serial.println("Message publish failed!");
    }
  } else {
    Serial.println("No motion.");
  }
  delay(1000);

  serve();
}

