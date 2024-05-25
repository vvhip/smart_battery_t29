#include <Arduino.h>
#include <HardwareSerial.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <freertos/queue.h>
#include <WiFi.h>
#include <Firebase_ESP_Client.h>
#include "addons/TokenHelper.h"
#include "addons/RTDBHelper.h"
#include "time.h"

#define TIMEOUT_MS 5000

#define pin_sw_hub1 21
#define pin_sw_hub2 22

#define WIFI_SSID "wifi namne"
#define WIFI_PASSWORD "Password of wifi"
#define API_KEY "API key of firebase web"
#define DATABASE_URL "url of database"

#define USER_EMAIL "email login firebase"
#define USER_PASSWORD "password of email"

HardwareSerial UART_HUB1(1);
HardwareSerial UART_HUB2(2);

const int bufferSize = 1024;

char Buffer1[bufferSize];
char Buffer2[bufferSize];

const int uart_baudrate = 115200;
const int Hub1_Rx_pin = 16;
const int Hub1_Tx_pin = 17;
const int Hub2_Rx_pin = 26;
const int Hub2_Tx_pin = 27;

struct databf {
  int have_Battery;
  String ID_Battery;
  int temp_Battery;
  int cap_Battery;
  int is_charging;
  int is_full;
};

databf data_hub1, data_hub2;

void task_hub1(void *pvParameters);
void task_hub2(void *pvParameters);

QueueHandle_t queue;

FirebaseData fbdo;
FirebaseAuth auth;
FirebaseConfig config;

String Hub1_path = "/hub1";
String Hub2_path = "/hub2";

String have_Battery_path = "/have_Battery";
String ID_Battery_path = "/ID_Battery";
String temp_Battery_path = "/temp_Battery";
String cap_Battery_path = "/cap_Battery";
String is_charging_path = "/is_charging";
String is_full_path = "/is_full";

String sw_path = "/sw";

void setup() {
  Serial.begin(uart_baudrate);
  UART_HUB1.begin(uart_baudrate, SERIAL_8N1, Hub1_Rx_pin, Hub1_Tx_pin);
  UART_HUB2.begin(uart_baudrate, SERIAL_8N1, Hub2_Rx_pin, Hub2_Tx_pin);
  queue = xQueueCreate(100, sizeof(int));

  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  Serial.println("Connecting to Wi-Fi...");
  while (WiFi.status() != WL_CONNECTED) {
    Serial.print(".");
    delay(300);
  }
  Serial.println("Connected with IP: ");
  Serial.println(WiFi.localIP());

  config.api_key = API_KEY;
  auth.user.email = USER_EMAIL;
  auth.user.password = USER_PASSWORD;
  config.database_url = DATABASE_URL;

  fbdo.setResponseSize(2048);
  config.token_status_callback = tokenStatusCallback;
  Firebase.signUp(&config, &auth, USER_EMAIL, USER_PASSWORD);
  config.max_token_generation_retry = 5;
  fbdo.setBSSLBufferSize(2048, 1024);

  Firebase.begin(&config, &auth);
  Firebase.reconnectWiFi(true);

  pinMode(pin_sw_hub1, OUTPUT);
  pinMode(pin_sw_hub2, OUTPUT);

  digitalWrite(pin_sw_hub1, HIGH);
  digitalWrite(pin_sw_hub2, HIGH);

  Firebase.RTDB.setInt(&fbdo, (String)Hub1_path + sw_path, 1);
  Firebase.RTDB.setInt(&fbdo, (String)Hub2_path + sw_path, 1);

  xTaskCreatePinnedToCore(task_hub1, "task_hub1", 4096, NULL, 1, NULL, 1);
  xTaskCreatePinnedToCore(task_hub2, "task_hub2", 4096, NULL, 1, NULL, 1);
}

void loop() {
  int receivedValue;
  if (xQueueReceive(queue, (void *)&receivedValue, portMAX_DELAY) == pdTRUE) {
    switch (receivedValue) {
      case 1:
        Firebase.RTDB.setInt(&fbdo, (String)Hub1_path + have_Battery_path, data_hub1.have_Battery);
        Firebase.RTDB.setString(&fbdo, (String)Hub1_path + ID_Battery_path, data_hub1.ID_Battery);
        Firebase.RTDB.setInt(&fbdo, (String)Hub1_path + temp_Battery_path, data_hub1.temp_Battery);
        Firebase.RTDB.setInt(&fbdo, (String)Hub1_path + cap_Battery_path, data_hub1.cap_Battery);
        Firebase.RTDB.setInt(&fbdo, (String)Hub1_path + is_charging_path, data_hub1.is_charging);
        Firebase.RTDB.setInt(&fbdo, (String)Hub1_path + is_full_path, data_hub1.is_full);
        break;
      case 2:
        Firebase.RTDB.setInt(&fbdo, (String)Hub2_path + have_Battery_path, data_hub2.have_Battery);
        Firebase.RTDB.setString(&fbdo, (String)Hub2_path + ID_Battery_path, data_hub2.ID_Battery);
        Firebase.RTDB.setInt(&fbdo, (String)Hub2_path + temp_Battery_path, data_hub2.temp_Battery);
        Firebase.RTDB.setInt(&fbdo, (String)Hub2_path + cap_Battery_path, data_hub2.cap_Battery);
        Firebase.RTDB.setInt(&fbdo, (String)Hub2_path + is_charging_path, data_hub2.is_charging);
        Firebase.RTDB.setInt(&fbdo, (String)Hub2_path + is_full_path, data_hub2.is_full);
        break;
    }
  }
  digitalWrite(pin_sw_hub1, Firebase.RTDB.getInt(&fbdo, (String)Hub1_path + sw_path));
  digitalWrite(pin_sw_hub2, Firebase.RTDB.getInt(&fbdo, (String)Hub2_path + sw_path));
  vTaskDelay(50);
}
void task_hub1(void *pvParameters) {
  TickType_t lastReceiveTime = xTaskGetTickCount();
  int message = 1;
  while (1) {
    if (UART_HUB1.available()) {
      lastReceiveTime = xTaskGetTickCount();
      String sID_battery = "", stemp_battery = "", scap_battery = "", sis_charging = "", sis_full = "";
      int len = UART_HUB1.readBytesUntil('\n', Buffer1, bufferSize - 1);
      Buffer1[len] = '\0';
      int dem = 0;
      int space[10];
      for (int i = 0; i < len; i++) {
        if ((char)Buffer1[i] == ' ') {
          space[dem] = i;
          dem++;
        }
      }
      for (int i = 0; i < space[0]; i++) {
        sID_battery += Buffer1[i];
      }
      for (int i = space[0] + 1; i < space[1]; i++) {
        stemp_battery += Buffer1[i];
      }
      stemp_battery += '\0';
      for (int i = space[1] + 1; i < space[2]; i++) {
        scap_battery += Buffer1[i];
      }
      scap_battery += '\0';
      for (int i = space[2] + 1; i < space[3]; i++) {
        sis_charging += Buffer1[i];
      }
      sis_charging += '\0';
      for (int i = space[3] + 1; i < len; i++) {
        sis_full += Buffer1[i];
      }
      sis_full += '\0';

      data_hub1.have_Battery = 1;
      data_hub1.ID_Battery = sID_battery;
      data_hub1.temp_Battery = stemp_battery.toInt();
      data_hub1.cap_Battery = scap_battery.toInt();
      data_hub1.is_charging = sis_charging.toInt();
      data_hub1.is_full = sis_full.toInt();
      xQueueSend(queue, (void *)&message, portMAX_DELAY);
    } else {
      if (xTaskGetTickCount() - lastReceiveTime >= pdMS_TO_TICKS(TIMEOUT_MS)) {
        data_hub1.have_Battery = 0;
        data_hub1.ID_Battery = "NULL";
        data_hub1.temp_Battery = -1;
        data_hub1.cap_Battery = -1;
        data_hub1.is_charging = -1;
        data_hub1.is_full = -1;
        lastReceiveTime = xTaskGetTickCount();
        xQueueSend(queue, (void *)&message, portMAX_DELAY);
      }
    }

    vTaskDelay(pdMS_TO_TICKS(100));
  }
}

void task_hub2(void *pvParameters) {
  TickType_t lastReceiveTime = xTaskGetTickCount();
  int message = 2;
  while (1) {
    if (UART_HUB2.available()) {
      lastReceiveTime = xTaskGetTickCount();
      String sID_battery = "", stemp_battery = "", scap_battery = "", sis_charging = "", sis_full = "";
      int len = UART_HUB2.readBytesUntil('\n', Buffer2, bufferSize - 1);
      Buffer2[len] = '\0';
      int dem = 0;
      int space[10];
      for (int i = 0; i < len; i++) {
        if ((char)Buffer2[i] == ' ') {
          space[dem] = i;
          dem++;
        }
      }
      for (int i = 0; i < space[0]; i++) {
        sID_battery += Buffer2[i];
      }
      for (int i = space[0] + 1; i < space[1]; i++) {
        stemp_battery += Buffer2[i];
      }
      stemp_battery += '\0';
      for (int i = space[1] + 1; i < space[2]; i++) {
        scap_battery += Buffer2[i];
      }
      scap_battery += '\0';
      for (int i = space[2] + 1; i < space[3]; i++) {
        sis_charging += Buffer2[i];
      }
      sis_charging += '\0';
      for (int i = space[3] + 1; i < len; i++) {
        sis_full += Buffer2[i];
      }
      sis_full += '\0';

      data_hub2.have_Battery = 1;
      data_hub2.ID_Battery = sID_battery;
      data_hub2.temp_Battery = stemp_battery.toInt();
      data_hub2.cap_Battery = scap_battery.toInt();
      data_hub2.is_charging = sis_charging.toInt();
      data_hub2.is_full = sis_full.toInt();
      xQueueSend(queue, (void *)&message, portMAX_DELAY);
    } else {
      if (xTaskGetTickCount() - lastReceiveTime >= pdMS_TO_TICKS(TIMEOUT_MS)) {
        data_hub2.have_Battery = 0;
        data_hub2.ID_Battery = "NULL";
        data_hub2.temp_Battery = -1;
        data_hub2.cap_Battery = -1;
        data_hub2.is_charging = -1;
        data_hub2.is_full = -1;
        lastReceiveTime = xTaskGetTickCount();
        xQueueSend(queue, (void *)&message, portMAX_DELAY);
      }
    }

    vTaskDelay(pdMS_TO_TICKS(100));
  }
}
