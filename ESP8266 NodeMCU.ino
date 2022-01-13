#include <Arduino.h>
#include <espnow.h>
#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <ESP8266HTTPClient.h>
#include <WiFiClient.h>
#include <NTPClient.h>
#include <WiFiUdp.h>
#include <Wire.h>
#include <Adafruit_GFX.h>       // Include core graphics library for the display
#include <Adafruit_SSD1306.h>   // Include Adafruit_SSD1306 library to drive the display
#include <Fonts/FreeMonoBold9pt7b.h>  // Add a custom font
#include <Adafruit_MLX90614.h>  //for infrared thermometer

#define SCREEN_WIDTH 128 // OLED display width, in pixels
#define SCREEN_HEIGHT 32 // OLED display height, in pixels
#define OLED_RESET -1

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

Adafruit_MLX90614 mlx = Adafruit_MLX90614();  //for infrared thermometer
int temp;  // Create a variable to have something dynamic to show on the display

int day = 0;
int FACE_ID = -1;
int absent[4] = {0};
int NoStudent = 4;
String student, temperature, remark, NoPhone, notify;
String weekDays[7]={"Sunday", "Monday", "Tuesday", "Wednesday", "Thursday", "Friday", "Saturday"};
String months[12]={"January", "February", "March", "April", "May", "June", "July", "August", "September", "October", "November", "December"};

const char* ssid = "4G-WIFI-7269";
const char* password = "1234567890";

const char *serverA = "maker.ifttt.com";
const char* resourceA = "/trigger/Kelas A/with/key/j_K4PvfcImzr1ygbaKF4oHT8XNQnHDOk5URFx9EUDE3";

const char *serverB = "maker.ifttt.com";
const char* resourceB = "/trigger/Kelas B/with/key/j_K4PvfcImzr1ygbaKF4oHT8XNQnHDOk5URFx9EUDE3";

const char *serverC = "maker.ifttt.com";
const char* resourceC = "/trigger/Absent & Fever/with/key/j_K4PvfcImzr1ygbaKF4oHT8XNQnHDOk5URFx9EUDE3";

const char *host = "http://192.168.0.100/";

typedef struct message {
   int faceid;
} message;
message myMessage;

void onDataReceiver(uint8_t * mac, uint8_t *incomingData, uint8_t len) {
   Serial.println("Message received.");
   memcpy(&myMessage, incomingData, sizeof(myMessage));Serial.print("Face ID Detected:");
   Serial.println(myMessage.faceid);
   FACE_ID = myMessage.faceid;
}

WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, "pool.ntp.org");

void setup() {
  Serial.begin(115200);
  Serial.println();
  pinMode(D0,INPUT);
  
  WiFi.begin(ssid, password);
  
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("");
  Serial.println("WiFi connected");
  
  Serial.print("Mac Address: ");
  Serial.print(WiFi.macAddress());
  Serial.println("\nESP-Now Receiver");
  
  if (esp_now_init() != 0) {
    Serial.println("Problem during ESP-NOW init");
    return;
  }
  esp_now_register_recv_cb(onDataReceiver);

  timeClient.begin();
  timeClient.setTimeOffset(28800);

  Serial.print("Current Face Id:");
  Serial.println(FACE_ID);

  Serial.println("Adafruit MLX90614 test");               
  delay(1000);  // This delay is needed to let the display to initialize
  display.begin(SSD1306_SWITCHCAPVCC, 0x3C);  // Initialize display with the I2C address of 0x3C
  display.clearDisplay();  // Clear the buffer
  display.setTextColor(WHITE);  // Set color of the text
  if (!mlx.begin()) {
    Serial.println("Error connecting to MLX sensor. Check wiring.");
    while (1);                                                                                                               
  };
  Serial.print("Emissivity = "); Serial.println(mlx.readEmissivity());
  Serial.println("================================================");
}

void loop() {
  timeClient.update(); 

  unsigned long epochTime = timeClient.getEpochTime();

  int data;
  
  int currentHour = timeClient.getHours();
  Serial.print("Current Hour:");
  Serial.println(currentHour);  

  int currentMinute = timeClient.getMinutes();
  Serial.print("Current minute:");
  Serial.println(currentMinute);   
   
  int currentSecond = timeClient.getSeconds(); 

  String weekDay = weekDays[timeClient.getDay()];

  struct tm *ptm = gmtime ((time_t *)&epochTime);

  int monthDay = ptm->tm_mday;

  int currentMonth = ptm->tm_mon+1;

  String currentMonthName = months[currentMonth-1];

  int currentYear = ptm->tm_year+1900;

  String currentDate = String(currentYear) + "." + String(currentMonth) + "." + String(monthDay);

  Serial.print("Current Face Id:");
  Serial.println(FACE_ID);

  if((currentHour == 7 && currentMinute <= 15) || (currentHour == 22 && currentMinute <= 45)){ //normal
    data = 1;

    int h = digitalRead(D0);
    if (h==1){
     readtemp();
     student = getdata(data);
     Serial.print("Student name is:");
     Serial.println(student);
     
     if(temp > 37){
      remark = "Temperature high";
      data = 2;
      notify = String(weekDay + ", " + currentDate + ". Your child with name " + student + " maybe not feeling well because his/her body temperature exceed the normal limit.");
      NoPhone = getdata(data);
      makeIFTTTRequestC();
     }
     absent[FACE_ID] = 1;
    if (FACE_ID >= 0 && FACE_ID <=1){
      makeIFTTTRequestA();
      delay(1000);
    }  
    if (FACE_ID >=2 && FACE_ID<=3){ 
      makeIFTTTRequestB();
      delay(1000);
    }
  }
  remark = " ";
 }
  
  if((currentHour == 7 && currentMinute > 15) || currentHour == 22 && currentMinute > 45){
    for(FACE_ID = 0; FACE_ID < NoStudent; FACE_ID++) {
      Serial.print("Face ID:");
      Serial.println(FACE_ID);
      Serial.print("Absent Contain:");
      Serial.println(absent[FACE_ID]);
      if(absent[FACE_ID] == 0){
        data = 1;
        student = getdata(data);
        Serial.print("Student Name:");
        Serial.println(student);
        data = 2;
        NoPhone = getdata(data);
        Serial.print("Phone Number:");
        Serial.println(NoPhone);
        notify = String("Your child with name " + student + " is not attending school on " + weekDay + ", " + currentDate);
        makeIFTTTRequestC();
        delay(5000);
      }
      absent[FACE_ID] = 1;
    }
      if (timeClient.getDay() != day){
      absent[4] = {0};
      day = timeClient.getDay();
    }
  }
}

String getdata(int data){
  HTTPClient http;

  String GetAddress, LinkGet, getData;
  if (data == 1){
    GetAddress = "students_info/name.php";
  }
  else{
    GetAddress = "students_info/phone.php";
  }
  
  LinkGet = host + GetAddress;
  getData = "face_id=" + String(FACE_ID);
  Serial.println("----------------Connect to Server-----------------");
  Serial.println("Get data from Server or Database");
  Serial.print("Request Link : ");
  Serial.println(LinkGet);
  http.begin(LinkGet);
  http.addHeader("Content-Type", "application/x-www-form-urlencoded");
  int httpCodeGet = http.POST(getData);
  String payloadGet = http.getString();
  Serial.print("Response Code : ");
  Serial.println(httpCodeGet);
  Serial.print("Returned data from Server : ");
  Serial.println(payloadGet);
  Serial.println("----------------Closing Connection----------------");
  http.end();
  Serial.println();

  return payloadGet;
}

void makeIFTTTRequestA() {
  Serial.print("Connecting to "); 
  Serial.print(serverA);
    
  WiFiClient client;
  int retries = 5;
  while(!!!client.connect(serverA, 80) && (retries-- > 0)) {
    Serial.print(".");
  }
  
  Serial.println();
  if(!!!client.connected()) {
    Serial.println("Failed to connect...");
  } 
   
  Serial.print("Request resource: "); 
  Serial.println(resourceA);
  temperature = String(temp);
  String jsonObject = "{\"value1\":\"" + student + "\",\"value2\":\"" + temperature + "\",\"value3\":\"" + remark + "\"}";  
                     
  client.println(String("POST ") + resourceA + " HTTP/1.1");
  client.println(String("Host: ") + serverA); 
  client.println("Connection: close\r\nContent-Type: application/json");
  client.print("Content-Length: ");
  client.println(jsonObject.length());
  client.println();
  client.println(jsonObject);
          
  int timeout = 5 * 10; // 5 seconds             
  while(!!!client.available() && (timeout-- > 0)){
    delay(100);
  }
  if(!!!client.available()) {
    Serial.println("No response...");
  }
  while(client.available()){
    Serial.write(client.read());
  }  
  Serial.println("\nclosing connection");
  client.stop(); 
}

void makeIFTTTRequestB() {
  Serial.print("Connecting to "); 
  Serial.print(serverB);
    
  WiFiClient client;
  int retries = 5;
  while(!!!client.connect(serverB, 80) && (retries-- > 0)) {
    Serial.print(".");
  }
  
  Serial.println();
  if(!!!client.connected()) {
    Serial.println("Failed to connect...");
  } 
   
  Serial.print("Request resource: "); 
  Serial.println(resourceB);

  temperature = String(temp);
  String jsonObject = String("{\"value1\":\"" + student + "\",\"value2\":\"" + temp + "\",\"value3\":\"" + remark + "\"}");  
                     
  client.println(String("POST ") + resourceB + " HTTP/1.1");
  client.println(String("Host: ") + serverB); 
  client.println("Connection: close\r\nContent-Type: application/json");
  client.print("Content-Length: ");
  client.println(jsonObject.length());
  client.println();
  client.println(jsonObject);
          
  int timeout = 5 * 10; // 5 seconds             
  while(!!!client.available() && (timeout-- > 0)){
    delay(100);
  }
  if(!!!client.available()) {
    Serial.println("No response...");
  }
  while(client.available()){
    Serial.write(client.read());
  }  
  Serial.println("\nclosing connection");
  client.stop(); 
}

void makeIFTTTRequestC() {
  Serial.print("Connecting to "); 
  Serial.print(serverC);
    
  WiFiClient client;
  int retries = 5;
  while(!!!client.connect(serverC, 80) && (retries-- > 0)) {
    Serial.print(".");
  }
  
  Serial.println();
  if(!!!client.connected()) {
    Serial.println("Failed to connect...");
  } 
   
  Serial.print("Request resource: "); 
  Serial.println(resourceC);

  String jsonObject = String("{\"value1\":\"") + student + "\",\"value2\":\"" + NoPhone + "\",\"value3\":\"" + notify + "\"}";  
                     
  client.println(String("POST ") + resourceC + " HTTP/1.1");
  client.println(String("Host: ") + serverC); 
  client.println("Connection: close\r\nContent-Type: application/json");
  client.print("Content-Length: ");
  client.println(jsonObject.length());
  client.println();
  client.println(jsonObject);
          
  int timeout = 5 * 10; // 5 seconds             
  while(!!!client.available() && (timeout-- > 0)){
    delay(100);
  }
  if(!!!client.available()) {
    Serial.println("No response...");
  }
  while(client.available()){
    Serial.write(client.read());
  }  
  Serial.println("\nclosing connection");
  client.stop(); 
}


void readtemp(){
  Serial.print("Ambient = "); Serial.print(mlx.readAmbientTempC());
  Serial.print("*C\tObject = "); Serial.print(mlx.readObjectTempC()); Serial.println("*C");
  Serial.println();
  //delay(1000);

  temp++;  // Increase value for testing
  if(temp > 43)  // If temp is greater than 150
  {
    temp = 0;  // Set temp to 0
  }
  
  temp = mlx.readObjectTempC(); //comment this line if you want to test
  
  display.clearDisplay();  // Clear the display so we can refresh

  // Print text:
  display.setFont();
  display.setCursor(45,5);  // (x,y)
  display.println("TEMPERATURE");  // Text or value to print

  // Print temperature
  char string[10];  // Create a character array of 10 characters
  // Convert float to a string:
  dtostrf(temp, 4, 0, string);  // (variable, no. of digits we are going to use, no. of decimal digits, string name)
  
  display.setFont(&FreeMonoBold9pt7b);  // Set a custom font
  display.setCursor(22,25);  // (x,y)
  display.println(string);  // Text or value to print
  display.setCursor(90,25);  // (x,y)
  display.println("C");  // Text or value to print
  display.setFont(); 
  display.setCursor(78,15);  // (x,y)
  display.cp437(true);
  display.write(167);
  
  // Draw a filled circle:
    display.fillCircle(18, 27, 5, WHITE); // Draw filled circle (x,y,radius,color). X and Y are the coordinates for the center point
    
  // Draw rounded rectangle:
   display.drawRoundRect(16, 3, 5, 24, 2, WHITE); // Draw rounded rectangle (x,y,width,height,radius,color)
   
   // It draws from the location to down-right
    // Draw ruler step

   for (int i = 3; i<=18; i=i+2){
    display.drawLine(21, i, 22, i, WHITE);  // Draw line (x0,y0,x1,y1,color)
  }
  
//  //Draw temperature
//  temp = temp*0.43; //ratio for show
  display.drawLine(18, 23, 18, 23-temp, WHITE);  // Draw line (x0,y0,x1,y1,color)
  
  display.display(); 
  delay(500);
}
