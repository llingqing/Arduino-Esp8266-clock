#include <TimeLib.h>
#include <ESP8266HTTPClient.h>
#include <ESP8266WebServer.h>
#include <ArduinoJson.h>
#include <ESP8266WiFi.h>
#include <WiFiUdp.h>
#include <SPI.h>
#include <U8g2lib.h>
#include <Wire.h>

U8G2_SSD1306_128X64_NONAME_F_SW_I2C u8g2(U8G2_R0,SCL,SDA,U8X8_PIN_NONE);

//---------------修改此处""内的信息---------------------------------------------
const char ssid[] = "Lingluo";                       //WiFi名
const char pass[] = "Chen6666";                   //WiFi密码
const int ledPin = D7; // LED 连接到 GPIO 13

bool ledState=LOW;
ESP8266WebServer server(80);
//-----------------------------------------------------------------------------

static const char ntpServerName[] = "ntp.aliyun.com"; //NTP服务器，阿里云
const int timeZone = 8;                                //时区，北京时间为+8

const unsigned long HTTP_TIMEOUT = 5000;
WiFiClient client;
HTTPClient http;
WiFiUDP Udp;
unsigned int localPort = 8888; // 用于侦听UDP数据包的本地端口

time_t getNtpTime();
void sendNTPpacket(IPAddress& address);
void oledClockDisplay();
void initdisplay();
void handleRoot();

boolean isNTPConnected = false;



String response;
String responseview;
int follower = 0;
int view = 0;
const int slaveSelect = 5;
const int scanLimit = 7;

int times = 0;


void setup()
{

  Serial.begin(9600);
  u8g2.begin();
  Serial.print("Connecting WiFi...");
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, pass);
  while (WiFi.status() != WL_CONNECTED){
      delay(500);
      Serial.print(".");
  }
  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
  Serial.println("Starting UDP");
  Udp.begin(localPort);
  Serial.print("Local port: ");
  Serial.println(Udp.localPort());
  Serial.println("waiting for sync");
  setSyncProvider(getNtpTime);
  setSyncInterval(300); //每300秒同步一次时间
  isNTPConnected = true;
  //灯控
  pinMode(ledPin, OUTPUT);
  digitalWrite(ledPin, ledState);
  server.on("/", handleRoot);
  server.begin();
  Serial.println("Server started");
}
void handleRoot() {
  if (server.hasArg("state")) {
    String state = server.arg("state");
    if (state == "on") {
      ledState = HIGH;      
    } else if (state == "off") {
      ledState = LOW;
    }
    digitalWrite(ledPin, ledState);
  }

  String html = "<html><body>";
  



  html += "<h1>LED Control " + String(ledState) +"</h1>";
  html += "<p>LED is currently " + String(ledState == HIGH ? "on" : "off") + "</p>";
  html += "<p><a href=\"/?state=on\">Turn On</a> | <a href=\"/?state=off\">Turn Off</a></p>";
  html += "<form action='/' method='get'>";
  html += "<label for='time'>Enter time to turn on LED (hh:mm:ss):</label><br>";
  html += "<input type='text' id='time' name='time'><br>";
  html += "<input type='submit' value='Set Time'>";
  html += "</form>";
  
  html += "</body></html>";

  server.send(200, "text/html", html);

}

void loop()
{
  if (times == 0) {
    if (timeStatus() != timeNotSet)
    {
        oledClockDisplay();
    }
  }
  times += 1;
  if (times >= 1) {
    times = 0;
  }
  delay(1000);
  server.handleClient(); 
  //整点报时
  if (times == 0)
    {
        if (timeStatus() != timeNotSet)
        {
            oledClockDisplay();
            int currentMinute = minute();
            int currentsecond = second();
            // 每小时的整点时刻
            if (currentsecond == 0||currentsecond == 30)
            {
                digitalWrite(ledPin, HIGH); // 点亮灯
                delay(3000); // 保持1秒
                digitalWrite(ledPin, LOW); // 熄灭灯
            }
        }
    }
    // 在loop函数中检查用户输入的时间，若匹配则在该时间亮起LED
  if (server.hasArg("time") && timeStatus() != timeNotSet)
  {
    String setTime = server.arg("time");
    int setHour = setTime.substring(0, 2).toInt();
    int setMinute = setTime.substring(3, 5).toInt();
    int setSecond = setTime.substring(6, 8).toInt();
    int currentHour = hour();
    int currentMinute = minute();
    int currentSecond = second();
    if (currentHour == setHour && currentMinute == setMinute && currentSecond == setSecond)
    {
        digitalWrite(ledPin, HIGH); // 亮起LED
        delay(3000); // 保持3秒
        digitalWrite(ledPin, LOW); // 熄灭LED
    }
  }
    times += 1;
    if (times >= 1)
    {
        times = 0;
    }
    delay(1000);
    server.handleClient();
}





void initdisplay()
{
  u8g2.begin();
  u8g2.enableUTF8Print();
}

void oledClockDisplay()
{
  int years, months, days, hours, minutes, seconds, weekdays;
  years = year();
  months = month();
  days = day();
  hours = hour();
  minutes = minute();
  seconds = second();
  weekdays = weekday();
  Serial.printf("%d/%d/%d %d:%d:%d Weekday:%d\n", years, months, days, hours, minutes, seconds, weekdays);
  u8g2.clearBuffer();
  u8g2.setFont(u8g2_font_unifont_t_chinese2);
  u8g2.setCursor(0, 14);
  if (isNTPConnected)
    u8g2.print("当前时间 (UTC+8)");
  else
    u8g2.print("无网络!"); //如果上次对时失败，则会显示无网络
  String currentTime = "";
  if (hours < 10)
    currentTime += 0;
  currentTime += hours;
  currentTime += ":";
  if (minutes < 10)
    currentTime += 0;
  currentTime += minutes;
  currentTime += ":";
  if (seconds < 10)
    currentTime += 0;
  currentTime += seconds;
  String currentDay = "";
  currentDay += years;
  currentDay += "/";
  if (months < 10)
    currentDay += 0;
  currentDay += months;
  currentDay += "/";
  if (days < 10)
    currentDay += 0;
  currentDay += days;

  u8g2.setFont(u8g2_font_logisoso24_tr);
  u8g2.setCursor(0, 44);
  u8g2.print(currentTime);
  u8g2.setCursor(0, 61);
  u8g2.setFont(u8g2_font_unifont_t_chinese2);
  u8g2.print(currentDay);
  // u8g2.drawXBM(80, 48, 16, 16, xing);
  u8g2.setCursor(95, 62);

  // u8g2.print("星期");
  if (weekdays == 1)
    u8g2.print("Sun");
  else if (weekdays == 2)
    u8g2.print("Mon"); 
  else if (weekdays == 3)
    u8g2.print("Tue");
  else if (weekdays == 4)
    u8g2.print("Wed");
  else if (weekdays == 5)
    u8g2.print("Thr");
  else if (weekdays == 6)
    u8g2.print("Fri");
  else if (weekdays == 7)
    // u8g2.drawXBM(111, 49, 16, 16, liu);
    u8g2.print("Sat");
  u8g2.sendBuffer();
}

/*-------- NTP 代码 ----------*/

const int NTP_PACKET_SIZE = 48;     // NTP时间在消息的前48个字节里
byte packetBuffer[NTP_PACKET_SIZE]; // 输入输出包的缓冲区


time_t getNtpTime()
{
  IPAddress ntpServerIP;          // NTP服务器的地址

  while (Udp.parsePacket() > 0);  // 丢弃以前接收的任何数据包
  Serial.println("Transmit NTP Request");
  // 从池中获取随机服务器
  WiFi.hostByName(ntpServerName, ntpServerIP);
  Serial.print(ntpServerName);
  Serial.print(": ");
  Serial.println(ntpServerIP);
  sendNTPpacket(ntpServerIP);
  uint32_t beginWait = millis();
  while (millis() - beginWait < 1500)
  {
    int size = Udp.parsePacket();
    if (size >= NTP_PACKET_SIZE)
    {
      Serial.println("Receive NTP Response");
      isNTPConnected = true;
      Udp.read(packetBuffer, NTP_PACKET_SIZE); // 将数据包读取到缓冲区
      unsigned long secsSince1900;
      // 将从位置40开始的四个字节转换为长整型，只取前32位整数部分
      secsSince1900 = (unsigned long)packetBuffer[40] << 24;
      secsSince1900 |= (unsigned long)packetBuffer[41] << 16;
      secsSince1900 |= (unsigned long)packetBuffer[42] << 8;
      secsSince1900 |= (unsigned long)packetBuffer[43];
      Serial.println(secsSince1900);
      Serial.println(secsSince1900 - 2208988800UL + timeZone * SECS_PER_HOUR);
      return secsSince1900 - 2208988800UL + timeZone * SECS_PER_HOUR;
    }
  }
  Serial.println("No NTP Response :-("); //无NTP响应
  isNTPConnected = false;
  return 0; //如果未得到时间则返回0
}


// 向给定地址的时间服务器发送NTP请求
void sendNTPpacket(IPAddress& address)
{
  memset(packetBuffer, 0, NTP_PACKET_SIZE);
  packetBuffer[0] = 0b11100011; // LI, Version, Mode
  packetBuffer[1] = 0;          // Stratum, or type of clock
  packetBuffer[2] = 6;          // Polling Interval
  packetBuffer[3] = 0xEC;       // Peer Clock Precision
  // 8 bytes of zero for Root Delay & Root Dispersion
  packetBuffer[12] = 49;
  packetBuffer[13] = 0x4E;
  packetBuffer[14] = 49;
  packetBuffer[15] = 52;
  Udp.beginPacket(address, 123); //NTP需要使用的UDP端口号为123
  Udp.write(packetBuffer, NTP_PACKET_SIZE);
  Udp.endPacket();
}