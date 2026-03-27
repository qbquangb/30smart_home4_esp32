#include <Arduino.h>
#include "secrets.h"
#include <WiFi.h>
#include <BlynkSimpleEsp32.h>
#include "ESP32_SIM.h"

const int SENSOR_CUA = 3; // Sensor cua 1, phia duoi, co tac dong -> LOW, khong tac dong -> HIGH, dang su dung

const int LOA = 10; // Dieu khien loa
const int MAYTINH = 20; // Relay bat may tinh
const int BUZZER = 7; // Dieu khien BUZZER
const int DEN    = 4; // Dieu khien den

#define BIP_1      1
#define BIP_2      2
#define BIP_3      3
#define BIP_4      4

bool trangthaibaove = false;
bool isRunFirst = true;
const char targetNumber[] = NUMBER; // số gọi đến cần phát hiện
unsigned long start = 0;

bool trangthaiden = false; // trạng thái đèn, false=tắt, true=bật
bool trangthailoa = false; // trạng thái loa, false=tắt, true=bật

const String Lenhbatden = "batden";
const String Lenhtatden = "tatden";
const String Lenhbatloa = "batloa";
const String Lenhtatloa = "tatloa";
const String Lenhbatmaytinh = "batmaytinh";
const String Lenhkiemtra = "trangthai"; // kiểm tra trạng thái thiết bị và kiểm tra trạng thái bảo vệ
String simBuf = ""; // buffer để đọc dữ liệu từ module SIM

// Hàm xử lý tràn millis(), trả về thời gian đã trôi qua kể từ startTime
unsigned long timeoutDuration(unsigned long startTime) {
  unsigned long now = millis();
  if (now >= startTime) {
    return (now - startTime);
  } else {
    // Xử lý tràn millis()
    return (4294967295 - startTime + now + 1); // ULONG_MAX là giá trị lớn nhất của unsigned long, ULONG_MAX = 4294967295
  }
}

// Dọc một ký tự từ module SIM với timeout
char readCharFromSim(unsigned long timeout) {
  unsigned long start = millis();
  while (timeoutDuration(start) < timeout) {
    if (Serial1.available()) {
      return (char)Serial1.read();
    }
  }
  return '\0'; // Trả về ký tự null nếu hết thời gian chờ
}

String readLineFromSim(unsigned long timeout) {
  String line = "";
  unsigned long start = millis();
  while (timeoutDuration(start) < timeout) {
    while (Serial1.available()) {
      char c = (char)Serial1.read();
      if (c == '\r') continue;
      if (c == '\n') {
        if (line.length() > 0) return line;
      } else {
        line += c;
      }
    }
  }
  return line;
}

bool sendSMS(String message) {
  const unsigned long PROMPT_TIMEOUT   = 10000UL;  // thời gian chờ prompt '>' (ms)
  const unsigned long RESPONSE_TIMEOUT = 50000UL;  // thời gian chờ phản hồi sau khi gửi (50s như yêu cầu)

  String smsCmd = "AT+CMGS=\"" + String(targetNumber) + "\""; // gửi SMS đến số targetNumber
  Serial1.println(smsCmd);

  delay(10000);

  // 2) Gửi nội dung tin nhắn
  Serial1.print(message);
  Serial1.write(26); // Ctrl+Z to send
  Serial1.println();

  // 3) Chờ phản hồi của module (tối đa RESPONSE_TIMEOUT)
  String response = readLineFromSim(RESPONSE_TIMEOUT);
  if (response.indexOf("ERROR") != -1) {
    return false; // gửi thất bại
  } else {
    return true; // gửi thành công
  }

}

/* Hàm phụ trợ nhiệm vụ nếu nhận được phản hồi có điều kiện từ module SIM thì thoát, 
còn nếu hết thời gian chờ thì cũng thoát  response processing
*/
bool readUntilResponseOrTimeout(const String &expectedResponse, unsigned long timeout, bool responsesProcess) {
  start = millis();
  while (timeoutDuration(start) < timeout) {
    String line = readLineFromSim(3000);
    if ((line.indexOf(expectedResponse) != -1) && (responsesProcess == false)) {
      return false; // đã nhận được phản hồi mong đợi và không cần xử lý thêm
    }
    else if ((line.indexOf(expectedResponse) != -1) && (responsesProcess == true)) {
      return true; // đã nhận được phản hồi mong đợi và cần xử lý thêm
  }
}
  return false; // hết thời gian chờ mà không nhận được phản hồi mong đợi, không cần xử lý thêm
}

bool offBuzzerAfterTimeout(unsigned long startBuzzerOn, unsigned long duration) { // duration mặc định là 60 giây
  if (timeoutDuration(startBuzzerOn) >= duration) {
    digitalWrite(BUZZER, LOW);
    return true; // Buzzer đã tắt
  }
  return false; // Buzzer vẫn đang bật
}

void process(String &s) { // &s: biến s truyền dưới dạng tham chiếu để tiết kiệm bộ nhớ
  s.trim(); // Loại bỏ khoảng trắng đầu và cuối chuỗi bao gồm ' ', '\r', '\n', '\t'
  if (s.length() == 0) return;

  // 1) Xử lý +CLIP (incoming caller id)
  if (s.startsWith("+CLIP:")) {
    delay(3000);
    // Kết thúc cuộc gọi
    // sendAT("AT+CHUP");
    //sendAT("ATA", 100);
    //delay(3000);
    Serial1.println("ATH");
    readUntilResponseOrTimeout("OK", 15000);

    int firstQuote = s.indexOf('"');
    int secondQuote = s.indexOf('"', firstQuote + 1);
    if (firstQuote != -1 && secondQuote != -1) {
      String number = s.substring(firstQuote + 1, secondQuote);

      // Gọi điện đến số targetNumber để thay đổi biến trangthaibaove
      if (number == String(targetNumber)) {
        trangthaibaove = !trangthaibaove;
        if (trangthaibaove == true) {
        Blynk.virtualWrite(V7,HIGH);
        control_BUZZER(BIP_2);
        delay(20000);
        // Goi dien den so targetNumber de thong bao trang thai bao ve da duoc bat
        String callCmd = "ATD" + String(targetNumber) + ";";

        Serial1.println(callCmd);
        if (readUntilResponseOrTimeout("OK", 10000, true)) {
          delay(8000);
        }
        
        Serial1.println("ATH"); // ngắt cuộc gọi
        readLineFromSim(10000);
        }
        else {
          Blynk.virtualWrite(V7,LOW);
          control_BUZZER(BIP_4);
        }
      }
    }
    return;
  }

  // 2) Xử lý +CMT (SMS delivered directly with text mode)
  // Format example:
  // +CMT: "+84123456789","","22/12/31,12:34:56+08"
  // On relay2
  if (s.startsWith("+CMT:")) {
    // Kiểm tra số gửi SMS có trùng với targetNumber không
    int firstQuote = s.indexOf('"');
    int secondQuote = s.indexOf('"', firstQuote + 1);
    if (firstQuote == -1 || secondQuote == -1) return; // lỗi định dạng
    String number = s.substring(firstQuote + 1, secondQuote);
    // Chỉ xử lý với chuổi number có chiều dài lớn hơn 6
    if (number.length() <= 6) return;
    // Hiệu chỉnh cho phù hợp với định dạng số điện thoại từ +84123456789 sang 0123456789
    if (number.startsWith("+84")) {
      number = "0" + number.substring(3);
    }
    // Nếu số gửi SMS không phải là targetNumber thì bỏ qua
    if (number != String(targetNumber)) {
      return;
    }
    // next line is the SMS body - read it
    String body = readLineFromSim();
    body.trim();
    body.toLowerCase();

    // Nhắn tin bật đèn
    if (body.indexOf(Lenhbatden) != -1) {
      String msg = "Den dang bat.";
      digitalWrite(DEN, HIGH);
      Blynk.virtualWrite(V0, HIGH);
      trangthaiden = true;
      if (!sendSMS(msg)) {
        delay(20000);
        sendSMS(msg);
      }
    // Nhắn tin tắt đèn
    } else if (body.indexOf(Lenhtatden) != -1) {
      String msg = "Den da tat.";
      digitalWrite(DEN, LOW);
      Blynk.virtualWrite(V0, LOW);
      trangthaiden = false;
      if (!sendSMS(msg)) {
        delay(20000);
        sendSMS(msg);
      }
    // Nhắn tin bật máy tính
    } else if (body.indexOf(Lenhbatmaytinh) != -1) {
      digitalWrite(MAYTINH, HIGH);
      delay(800);
      digitalWrite(MAYTINH, LOW);
      String msg = "May tinh da bat.";
      if (!sendSMS(msg)) {
        delay(20000);
        sendSMS(msg);
      }
    // Nhắn tin bật loa
    } else if (body.indexOf(Lenhbatloa) != -1) {
      String msg = "Loa dang bat.";
      digitalWrite(LOA, HIGH);
      Blynk.virtualWrite(V4, HIGH);
      trangthailoa = true;
      if (!sendSMS(msg)) {
        delay(20000);
        sendSMS(msg);
      }
    // Nhắn tin tắt loa
    } else if (body.indexOf(Lenhtatloa) != -1) {
      String msg = "Loa da tat.";
      digitalWrite(LOA, LOW);
      Blynk.virtualWrite(V4, LOW);
      trangthailoa = false;
      if (!sendSMS(msg)) {
        delay(20000);
        sendSMS(msg);
      }
    // Nhắn tin kiểm tra trạng thái thiết bị và kiểm tra trạng thái biến trangthaibaove
    } else if (body.indexOf(Lenhkiemtra) != -1) {
      String trangthaidenstr = trangthaiden ? "BAT" : "TAT";
      String trangthailoastr = trangthailoa ? "BAT" : "TAT";
      String trangthaibaovestr = trangthaibaove ? "BAT" : "TAT";
      String msg = "Trang thai hien tai:\r\nDen: " + trangthaidenstr + "\r\nLoa: " + 
                    trangthailoastr + "\r\nBao ve: " + trangthaibaovestr + ".";
      if (!sendSMS(msg)) {
        delay(20000);
        sendSMS(msg);
      }
    } else {
      String msg = "Tin nhan khong hop le.";
      if (!sendSMS(msg)) {
        delay(20000);
        sendSMS(msg);
      }
    }
    return;
  }
}

void control_BUZZER(uint8_t para) {
    if (para == BIP_2) {
        digitalWrite(BUZZER, HIGH);
        delay(500);
        digitalWrite(BUZZER, LOW);
        delay(2000);
        digitalWrite(BUZZER, HIGH);
        delay(500);
        digitalWrite(BUZZER, LOW);
        delay(2000);
}
    if (para == BIP_1) {
        digitalWrite(BUZZER, HIGH);
        delay(500);
        digitalWrite(BUZZER, LOW);
        delay(2000);
    }
    if (para == BIP_3)
    {
        digitalWrite(BUZZER, HIGH);
        delay(500);
        digitalWrite(BUZZER, LOW);
        delay(2000);
        digitalWrite(BUZZER, HIGH);
        delay(500);
        digitalWrite(BUZZER, LOW);
        delay(2000);
        digitalWrite(BUZZER, HIGH);
        delay(500);
        digitalWrite(BUZZER, LOW);
        delay(2000);
    }
    if (para == BIP_4)
    {
        digitalWrite(BUZZER, HIGH);
        delay(500);
        digitalWrite(BUZZER, LOW);
        delay(2000);
        digitalWrite(BUZZER, HIGH);
        delay(500);
        digitalWrite(BUZZER, LOW);
        delay(2000);
        digitalWrite(BUZZER, HIGH);
        delay(500);
        digitalWrite(BUZZER, LOW);
        delay(2000);
        digitalWrite(BUZZER, HIGH);
        delay(500);
        digitalWrite(BUZZER, LOW);
        delay(2000);
    }
}

BlynkTimer timer;

void turnOffAction()
{
  digitalWrite(MAYTINH, LOW);
  Blynk.virtualWrite(V2, 0);   // cập nhật trạng thái MAYTINH trên app
  Blynk.virtualWrite(V3, 0);   // đưa nút về OFF
}

BLYNK_WRITE(V1)
{
  int pinValue = param.asInt();
  if (pinValue == 1)
  {
    digitalWrite(DEN, HIGH);
    Blynk.virtualWrite(V0, HIGH);
  }
  else
  {
    digitalWrite(DEN, LOW);
    Blynk.virtualWrite(V0,LOW);
  }
}

BLYNK_WRITE(V3)
{
  int pinValue = param.asInt();

  if (pinValue == 1)
  {
    digitalWrite(MAYTINH, HIGH);
    Blynk.virtualWrite(V2, 1);

    timer.setTimeout(500, turnOffAction);  // sau 500 ms tự tắt và đưa button về OFF
  }
}

BLYNK_WRITE(V5)
{
  int pinValue = param.asInt();
  if (pinValue == 1)
  {
    digitalWrite(LOA, HIGH);
    Blynk.virtualWrite(V4, HIGH);
  }
  else
  {
    digitalWrite(LOA, LOW);
    Blynk.virtualWrite(V4,LOW);
  }
}

BLYNK_WRITE(V7)
{
  int pinValue = param.asInt();
  if (pinValue == 1)
  {
    trangthaibaove = true;
    control_BUZZER(BIP_2);
  }
  else
  {
    trangthaibaove = false;
    control_BUZZER(BIP_4);
  }
}

void setup()
{
  Serial1.begin(2400, SERIAL_8N1, 5, 19); // RX=5, TX=19
  delay(200);
  pinMode(SENSOR_CUA, INPUT);
  pinMode(LOA, OUTPUT);
  pinMode(MAYTINH, OUTPUT);
  pinMode(BUZZER, OUTPUT);
  pinMode(DEN, OUTPUT);
  digitalWrite(LOA, LOW);
  digitalWrite(MAYTINH, LOW);
  digitalWrite(BUZZER, LOW);
  digitalWrite(DEN, LOW);

  trangthaibaove = false;
  isRunFirst = true;
  trangthaiden = false;
  trangthailoa = false;

  Blynk.begin(BLYNK_AUTH_TOKEN, WIFI_SSID, WIFI_PASSWORD);
  control_BUZZER(BIP_2);
  Blynk.virtualWrite(V0,LOW);
  Blynk.virtualWrite(V1,LOW);
  Blynk.virtualWrite(V2,LOW);
  Blynk.virtualWrite(V3,LOW);
  Blynk.virtualWrite(V4,LOW);
  Blynk.virtualWrite(V5,LOW);
  Blynk.virtualWrite(V6,LOW);
  Blynk.virtualWrite(V7,LOW);

  // Chờ cho đến khi module sim phản hồi "READY" hoặc hết thời gian chờ 40 giây
  readUntilResponseOrTimeout("READY", 40000);

  // Một vài lệnh khởi tạo
  Serial1.println("AT"); // Kiểm tra kết nối, thời gian chờ phản hồi tối đa 15s
  readUntilResponseOrTimeout("OK", 15000);

  Serial1.println("ATE0"); // tắt echo
  readUntilResponseOrTimeout("OK", 15000);

  Serial1.println("AT+CLIP=1");   // bật caller ID
  readUntilResponseOrTimeout("OK", 15000);

  Serial1.println("AT+CVHU=0");
  readUntilResponseOrTimeout("OK", 15000);

  Serial1.println("AT+CMGF=1");   // SMS text mode
  readUntilResponseOrTimeout("OK", 15000);

  Serial1.println("AT+CNMI=2,2,0,0,0"); // yêu cầu module gửi +CMT khi có SMS mới đến
  readUntilResponseOrTimeout("OK", 15000);

  // Nhắn tin thông báo khởi động thành công
  if (!sendSMS("He thong khoi dong thanh cong.")) {
    delay(20000);
    sendSMS("He thong khoi dong thanh cong.");
  }
  delay(3000);

}

void loop()
{
  Blynk.run();
  timer.run();

  if (digitalRead(SENSOR_CUA) == HIGH && trangthaibaove == true && isRunFirst == true) {
    delay(2000);
    if (digitalRead(SENSOR_CUA) == HIGH) {

    digitalWrite(BUZZER, HIGH);
    delay(20000);
    digitalWrite(BUZZER, LOW);

    // Cập nhật lên blynk
    Blynk.virtualWrite(V6, 1);

    // Thay đổi biến isRunFirst
    isRunFirst = false;

  }
}

  // đọc dữ liệu từ Serial1 (non-blocking)
  while (Serial1.available()) {
    char c = (char)Serial1.read();
    // append để parse từng dòng
    if (c == '\n') {
      // process accumulated line
      String line = simBuf;
      simBuf = "";
      line.trim();
      if (line.length() > 0) {
        process(line);
      }
    } else if (c != '\r') {
      simBuf += c;
      // keep buffer limited
      if (simBuf.length() > 300) simBuf = simBuf.substring(simBuf.length() - 300);
    }
  }

}
