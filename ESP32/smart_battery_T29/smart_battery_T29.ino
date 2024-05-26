#include <Arduino.h> // Thư viện chính cho lập trình Arduino
#include <HardwareSerial.h> // Thư viện để sử dụng các cổng UART
#include <freertos/FreeRTOS.h> // Thư viện FreeRTOS để sử dụng hệ điều hành thời gian thực
#include <freertos/task.h> // Thư viện FreeRTOS cho các tác vụ
#include <freertos/queue.h> // Thư viện FreeRTOS cho hàng đợi
#include <WiFi.h> // Thư viện để kết nối WiFi
#include <Firebase_ESP_Client.h> // Thư viện để kết nối với Firebase
#include "addons/TokenHelper.h" // Thư viện bổ sung cho Token của Firebase
#include "addons/RTDBHelper.h" // Thư viện bổ sung cho Real-time Database của Firebase
#include "time.h" // Thư viện để sử dụng các hàm liên quan đến thời gian

#define TIMEOUT_MS 5000 // Định nghĩa thời gian chờ (timeout) là 5000ms (5 giây)

#define pin_sw_hub1 21 // Định nghĩa chân điều khiển của Hub1 là chân 21
#define pin_sw_hub2 22 // Định nghĩa chân điều khiển của Hub2 là chân 22

#define WIFI_SSID "Hihi haha" // Định nghĩa SSID của mạng WiFi
#define WIFI_PASSWORD "6868686868" // Định nghĩa mật khẩu của mạng WiFi
#define API_KEY "AIzaSyBmFgHqu73w31OhvnGeXbYFLUcnUWFvICo" // Định nghĩa API Key của Firebase
#define DATABASE_URL "https://t29-smartbattery-3a88d-default-rtdb.asia-southeast1.firebasedatabase.app/" // Định nghĩa URL của Firebase Database

#define USER_EMAIL "admin@gmail.com" // Định nghĩa email người dùng cho Firebase Authentication
#define USER_PASSWORD "123456" // Định nghĩa mật khẩu người dùng cho Firebase Authentication

HardwareSerial UART_HUB1(1); // Định nghĩa UART1 cho Hub1
HardwareSerial UART_HUB2(2); // Định nghĩa UART2 cho Hub2

const int bufferSize = 1024; // Định nghĩa kích thước bộ đệm là 1024 byte

char Buffer1[bufferSize]; // Khai báo bộ đệm cho dữ liệu từ Hub1
char Buffer2[bufferSize]; // Khai báo bộ đệm cho dữ liệu từ Hub2

const int uart_baudrate = 115200; // Định nghĩa tốc độ baud rate của UART là 115200
const int Hub1_Rx_pin = 16; // Định nghĩa chân nhận dữ liệu (Rx) của Hub1 là chân 16
const int Hub1_Tx_pin = 17; // Định nghĩa chân truyền dữ liệu (Tx) của Hub1 là chân 17
const int Hub2_Rx_pin = 26; // Định nghĩa chân nhận dữ liệu (Rx) của Hub2 là chân 26
const int Hub2_Tx_pin = 27; // Định nghĩa chân truyền dữ liệu (Tx) của Hub2 là chân 27

struct databf { // Khai báo cấu trúc dữ liệu để lưu trữ thông tin của pin
  int have_Battery; // Biến để lưu trạng thái có pin hay không
  String ID_Battery; // Biến để lưu ID của pin
  int temp_Battery; // Biến để lưu nhiệt độ của pin
  int cap_Battery; // Biến để lưu dung lượng của pin
  int is_charging; // Biến để lưu trạng thái đang sạc của pin
  int is_full; // Biến để lưu trạng thái đầy pin
};

databf data_hub1, data_hub2; // Khai báo hai biến để lưu trữ dữ liệu của hai Hub

void task_hub1(void *pvParameters); // Khai báo hàm nhiệm vụ cho Hub1
void task_hub2(void *pvParameters); // Khai báo hàm nhiệm vụ cho Hub2

QueueHandle_t queue; // Khai báo hàng đợi để giao tiếp giữa các nhiệm vụ

FirebaseData fbdo; // Khai báo biến để thao tác với Firebase
FirebaseAuth auth; // Khai báo biến để lưu thông tin xác thực của Firebase
FirebaseConfig config; // Khai báo biến để cấu hình Firebase

String Hub1_path = "chargers/charger1/batteries/hub1"; // Định nghĩa đường dẫn trong Firebase Database cho Hub1
String Hub2_path = "chargers/charger1/batteries/hub2"; // Định nghĩa đường dẫn trong Firebase Database cho Hub2

// Định nghĩa các đường dẫn con trong Firebase Database
String have_Battery_path = "/have_Battery";
String ID_Battery_path = "/ID_Battery";
String temp_Battery_path = "/temp_Battery";
String cap_Battery_path = "/cap_Battery";
String is_charging_path = "/is_charging";
String is_full_path = "/is_full";

String sw_path = "/sw"; // Định nghĩa đường dẫn cho công tắc (switch)


void setup() {
  // Khởi tạo Serial và UART
  Serial.begin(uart_baudrate);
  UART_HUB1.begin(uart_baudrate, SERIAL_8N1, Hub1_Rx_pin, Hub1_Tx_pin);
  UART_HUB2.begin(uart_baudrate, SERIAL_8N1, Hub2_Rx_pin, Hub2_Tx_pin);
  
  // Tạo hàng đợi với 100 phần tử kiểu int
  queue = xQueueCreate(100, sizeof(int));

  // Kết nối WiFi
  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  Serial.println("Connecting to Wi-Fi...");
  while (WiFi.status() != WL_CONNECTED) {
    Serial.print(".");
    delay(300);
  }
  Serial.println("Connected with IP: ");
  Serial.println(WiFi.localIP());

  // Cấu hình Firebase
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

  // Thiết lập các chân GPIO
  pinMode(pin_sw_hub1, OUTPUT);
  pinMode(pin_sw_hub2, OUTPUT);

  // Kích hoạt công tắc
  digitalWrite(pin_sw_hub1, HIGH);
  digitalWrite(pin_sw_hub2, HIGH);

  // Thiết lập giá trị công tắc trong Firebase
  Firebase.RTDB.setInt(&fbdo, (String)Hub1_path + sw_path, 1);
  Firebase.RTDB.setInt(&fbdo, (String)Hub2_path + sw_path, 1);

  // Tạo các task cho Hub1 và Hub2
  xTaskCreatePinnedToCore(task_hub1, "task_hub1", 4096, NULL, 1, NULL, 1);
  xTaskCreatePinnedToCore(task_hub2, "task_hub2", 4096, NULL, 1, NULL, 1);
}


void loop() {
  int receivedValue;
  // Nhận dữ liệu từ hàng đợi
  if (xQueueReceive(queue, (void *)&receivedValue, portMAX_DELAY) == pdTRUE) {
    switch (receivedValue) {
      case 1:
        // Cập nhật dữ liệu Hub1 lên Firebase
        Firebase.RTDB.setInt(&fbdo, (String)Hub1_path + have_Battery_path, data_hub1.have_Battery);
        Firebase.RTDB.setString(&fbdo, (String)Hub1_path + ID_Battery_path, data_hub1.ID_Battery);
        Firebase.RTDB.setInt(&fbdo, (String)Hub1_path + temp_Battery_path, data_hub1.temp_Battery);
        Firebase.RTDB.setInt(&fbdo, (String)Hub1_path + cap_Battery_path, data_hub1.cap_Battery);
        Firebase.RTDB.setInt(&fbdo, (String)Hub1_path + is_charging_path, data_hub1.is_charging);
        Firebase.RTDB.setInt(&fbdo, (String)Hub1_path + is_full_path, data_hub1.is_full);
        break;
      case 2:
        // Cập nhật dữ liệu Hub2 lên Firebase
        Firebase.RTDB.setInt(&fbdo, (String)Hub2_path + have_Battery_path, data_hub2.have_Battery);
        Firebase.RTDB.setString(&fbdo, (String)Hub2_path + ID_Battery_path, data_hub2.ID_Battery);
        Firebase.RTDB.setInt(&fbdo, (String)Hub2_path + temp_Battery_path, data_hub2.temp_Battery);
        Firebase.RTDB.setInt(&fbdo, (String)Hub2_path + cap_Battery_path, data_hub2.cap_Battery);
        Firebase.RTDB.setInt(&fbdo, (String)Hub2_path + is_charging_path, data_hub2.is_charging);
        Firebase.RTDB.setInt(&fbdo, (String)Hub2_path + is_full_path, data_hub2.is_full);
        break;
    }
  }
  // Điều khiển công tắc dựa trên giá trị từ Firebase
  digitalWrite(pin_sw_hub1, Firebase.RTDB.getInt(&fbdo, (String)Hub1_path + sw_path));
  digitalWrite(pin_sw_hub2, Firebase.RTDB.getInt(&fbdo, (String)Hub2_path + sw_path));
  vTaskDelay(50);
}


void task_hub1(void *pvParameters) {
  TickType_t lastReceiveTime = xTaskGetTickCount(); // Lấy thời gian tick hiện tại
  int message = 1; // Định nghĩa thông điệp cho hàng đợi
  while (1) {
    if (UART_HUB1.available()) { // Kiểm tra nếu có dữ liệu từ UART Hub1
      lastReceiveTime = xTaskGetTickCount(); // Cập nhật thời gian nhận dữ liệu cuối cùng
      String sID_battery = "", stemp_battery = "", scap_battery = "", sis_charging = "", sis_full = "";
      int len = UART_HUB1.readBytesUntil('\n', Buffer1, bufferSize - 1); // Đọc dữ liệu từ UART cho đến khi gặp ký tự xuống dòng
      Buffer1[len] = '\0'; // Thêm ký tự null vào cuối chuỗi để kết thúc chuỗi
      int dem = 0;
      int space[10];
      for (int i = 0; i < len; i++) {
        if ((char)Buffer1[i] == ' ') { // Tìm các khoảng trắng trong chuỗi để xác định vị trí của các trường dữ liệu
          space[dem] = i;
          dem++;
        }
      }
      for (int i = 0; i < space[0]; i++) {
        sID_battery += Buffer1[i]; // Lấy ID pin
      }
      for (int i = space[0] + 1; i < space[1]; i++) {
        stemp_battery += Buffer1[i]; // Lấy nhiệt độ pin
      }
      stemp_battery += '\0';
      for (int i = space[1] + 1; i < space[2]; i++) {
        scap_battery += Buffer1[i]; // Lấy dung lượng pin
      }
      scap_battery += '\0';
      for (int i = space[2] + 1; i < space[3]; i++) {
        sis_charging += Buffer1[i]; // Lấy trạng thái đang sạc
      }
      sis_charging += '\0';
      for (int i = space[3] + 1; i < len; i++) {
        sis_full += Buffer1[i]; // Lấy trạng thái đầy pin
      }
      sis_full += '\0';

      // Cập nhật dữ liệu pin cho Hub1
      data_hub1.have_Battery = 1;
      data_hub1.ID_Battery = sID_battery;
      data_hub1.temp_Battery = stemp_battery.toInt();
      data_hub1.cap_Battery = scap_battery.toInt();
      data_hub1.is_charging = sis_charging.toInt();
      data_hub1.is_full = sis_full.toInt();
      xQueueSend(queue, (void *)&message, portMAX_DELAY); // Gửi thông điệp vào hàng đợi
    } else {
      // Kiểm tra nếu đã vượt quá thời gian timeout mà không nhận được dữ liệu
      if (xTaskGetTickCount() - lastReceiveTime >= pdMS_TO_TICKS(TIMEOUT_MS)) {
        // Cập nhật dữ liệu khi không có pin
        data_hub1.have_Battery = 0;
        data_hub1.ID_Battery = "NULL";
        data_hub1.temp_Battery = -1;
        data_hub1.cap_Battery = -1;
        data_hub1.is_charging = -1;
        data_hub1.is_full = -1;
        lastReceiveTime = xTaskGetTickCount(); // Cập nhật thời gian nhận dữ liệu cuối cùng
        xQueueSend(queue, (void *)&message, portMAX_DELAY); // Gửi thông điệp vào hàng đợi
      }
    }

    vTaskDelay(pdMS_TO_TICKS(100)); // Trì hoãn task trong 100ms
  }
}

void task_hub2(void *pvParameters) {
  TickType_t lastReceiveTime = xTaskGetTickCount(); // Lấy thời gian tick hiện tại
  int message = 2; // Định nghĩa thông điệp cho hàng đợi
  while (1) {
    if (UART_HUB2.available()) { // Kiểm tra nếu có dữ liệu từ UART Hub2
      lastReceiveTime = xTaskGetTickCount(); // Cập nhật thời gian nhận dữ liệu cuối cùng
      String sID_battery = "", stemp_battery = "", scap_battery = "", sis_charging = "", sis_full = "";
      int len = UART_HUB2.readBytesUntil('\n', Buffer2, bufferSize - 1); // Đọc dữ liệu từ UART cho đến khi gặp ký tự xuống dòng
      Buffer2[len] = '\0'; // Thêm ký tự null vào cuối chuỗi để kết thúc chuỗi
      int dem = 0;
      int space[10];
      for (int i = 0; i < len; i++) {
        if ((char)Buffer2[i] == ' ') { // Tìm các khoảng trắng trong chuỗi để xác định vị trí của các trường dữ liệu
          space[dem] = i;
          dem++;
        }
      }
      for (int i = 0; i < space[0]; i++) {
        sID_battery += Buffer2[i]; // Lấy ID pin
      }
      for (int i = space[0] + 1; i < space[1]; i++) {
        stemp_battery += Buffer2[i]; // Lấy nhiệt độ pin
      }
      stemp_battery += '\0';
      for (int i = space[1] + 1; i < space[2]; i++) {
        scap_battery += Buffer2[i]; // Lấy dung lượng pin
      }
      scap_battery += '\0';
      for (int i = space[2] + 1; i < space[3]; i++) {
        sis_charging += Buffer2[i]; // Lấy trạng thái đang sạc
      }
      sis_charging += '\0';
      for (int i = space[3] + 1; i < len; i++) {
        sis_full += Buffer2[i]; // Lấy trạng thái đầy pin
      }
      sis_full += '\0';

      // Cập nhật dữ liệu pin cho Hub2
      data_hub2.have_Battery = 1;
      data_hub2.ID_Battery = sID_battery;
      data_hub2.temp_Battery = stemp_battery.toInt();
      data_hub2.cap_Battery = scap_battery.toInt();
      data_hub2.is_charging = sis_charging.toInt();
      data_hub2.is_full = sis_full.toInt();
      xQueueSend(queue, (void *)&message, portMAX_DELAY); // Gửi thông điệp vào hàng đợi
    } else {
      // Kiểm tra nếu đã vượt quá thời gian timeout mà không nhận được dữ liệu
      if (xTaskGetTickCount() - lastReceiveTime >= pdMS_TO_TICKS(TIMEOUT_MS)) {
        // Cập nhật dữ liệu khi không có pin
        data_hub2.have_Battery = 0;
        data_hub2.ID_Battery = "NULL";
        data_hub2.temp_Battery = -1;
        data_hub2.cap_Battery = -1;
        data_hub2.is_charging = -1;
        data_hub2.is_full = -1;
        lastReceiveTime = xTaskGetTickCount(); // Cập nhật thời gian nhận dữ liệu cuối cùng
        xQueueSend(queue, (void *)&message, portMAX_DELAY); // Gửi thông điệp vào hàng đợi
      }
    }

    vTaskDelay(pdMS_TO_TICKS(100)); // Trì hoãn task trong 100ms
  }
}

