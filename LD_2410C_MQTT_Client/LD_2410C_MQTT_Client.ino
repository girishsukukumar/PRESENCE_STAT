/*
  This program prints the current setup parameters
  of the HLK-LD2410 presence sensor.

  #define SERIAL_BAUD_RATE sets the serial monitor baud rate

  Communication with the sensor is handled by the 
  "MyLD2410" library Copyright (c) Iavor Veltchev 2024

  Use only hardware UART at the default baud rate 256000,
  or change the #define LD2410_BAUD_RATE to match your sensor.
  For ESP32 or other boards that allow dynamic UART pins,
  modify the RX_PIN and TX_PIN defines in "./board_select.h"

  Connection diagram:
  Arduino/ESP32 RX  -- TX LD2410 
  Arduino/ESP32 TX  -- RX LD2410
  Arduino/ESP32 GND -- GND LD2410
  Provide sufficient power to the sensor Vcc (200mA, 5-12V) 
*/
#include "Arduino.h"
//#include <WiFi.h>
#include <WiFiClient.h>
//#include <WebServer.h>
#include <ESPmDNS.h>
#include <Update.h>
#include <WiFiMulti.h>
//#include <ESP8266FtpServer.h>
#include <ArduinoJson.h>
#include "./board_select.h"
//Change the communication baud rate here, if necessary
//#define LD2410_BAUD_RATE 256000
#include "MyLD2410.h"
#include "ESP32MQTTClient.h"
#include <WebServer.h>  
#include <WebSerial.h>


#include <LittleFS.h>
//#include <FFat.h>
//#include <ESP8266FtpServer.h>
//#include <SimpleFTPServer.h>
#define JSON_CONFIG_FILE_NAME "/config.json" 
#define FTP_USER_NAME "apollo11"
#define FTP_PASSWORD  "eagle"
#define SSID_NAME_LEN 20 
#define SSID_PASSWD_LEN 20
#define NAME_LEN 40
#define MQTT_TOPIC_LENGTH 100
#define PRESENSE_DETECTED 1
#define NO_PRESENCE  0

// Uses : https://github.com/iavorvel/MyLD2410 
//Search for MyLD2410 library for installing this in arduino IDE
// MQTT Library : https://github.com/cyijun/ESP32MQTTClient

void TelnetPrintf(const char* fmt, ...) ;
// User defines
#define SERIAL_BAUD_RATE 115200
//#define NO_DEBUG 
//#define   DEBUG_TELNET_PORT  
#define   DEBUG_SERIAL_PORT 

// *******************Enable and disable serial print **************
#ifdef DEBUG_SERIAL_PORT
    #define DEBUG_PRINTLN(x)      Serial.println (x)
    #define DEBUG_PRINT(x)        Serial.print (x)
    #define DEBUG_PRINTF(f,...)   Serial.printf(f,##__VA_ARGS__)
#endif

#ifdef DEBUG_TELNET_PORT
    #define    DEBUG_PRINTF(f,...)  TelnetPrintf(f,##__VA_ARGS__)
    #define    DEBUG_PRINTLN(x) 
    #define    DEBUG_PRINT(x)
#endif

#ifdef NO_DEBUG
    #define DEBUG_PRINT(x)
    #define DEBUG_PRINTLN(x)
    #define DEBUG_PRINTF(f,...)
#endif


#ifdef NETWORK_DEBUG_OBSELATE 
    #define    DEBUG_PRINTF(f,...)  TelnetPrintf(f,##__VA_ARGS__)
    #define    DEBUG_PRINTLN(x) 
    #define    DEBUG_PRINT(x)
#endif



typedef struct configData 
{
  char     ssid1[SSID_NAME_LEN]   ;
  char     password1[SSID_PASSWD_LEN]  ;
  char     ssid2[SSID_NAME_LEN] ;
  char     password2[SSID_PASSWD_LEN] ;
  char     ssid3[SSID_NAME_LEN] ;
  char     password3[SSID_PASSWD_LEN] ;
  char     wifiDeviceName[NAME_LEN] ; 
  char     mqttBroker[MQTT_TOPIC_LENGTH] ; 
  char     PUBLISH_TOPIC[MQTT_TOPIC_LENGTH] ;
  char     SUBCRIBING_TOPIC[MQTT_TOPIC_LENGTH];

} configData_t;
configData_t  ConfigData;
MyLD2410 sensor(sensorSerial);

// Public MQTT test broker (see https://test.mosquitto.org), do not use in production
//const char *server = "mqtt://test.mosquitto.org:1883";

/* ************************************************************************** */
/*    MQTT specific declarations 
****************************************************************************** */
//const char *subscribeTopic = "foo";
//const char *publishTopic = "bar/bar";
WiFiMulti      wifiMulti; 

ESP32MQTTClient mqttClient; // all params are set later

unsigned long  lastPrint = 0 ;
volatile bool wifiNeedsReconnect = false;
WiFiServer telnetServer(23);
WiFiClient telnetClient;

 /* ******************************************************************************/

int lastStateOfPresence = NO_PRESENCE ;
float lastAmbientLight = 0.0 ;
float smoothedLight = -1 ;
float alpha = 0.8 ;

WebServer server(80);

const char* loginIndex =
 "<form name='loginForm'>"
    "<table width='20%' bgcolor='A09F9F' align='center'>"
        "<tr>"
            "<td colspan=2>"
                "<center><font size=4><b>ESP32 Login Page</b></font></center>"
                "<center><font size=2>Hostname: %HOSTNAME%</font></center>"
                "<br>"
            "</td>"
        "</tr>"
        "<tr>"
            "<td>Username:</td>"
            "<td><input type='text' size=25 name='userid'><br></td>"
        "</tr>"
        "<tr>"
            "<td>Password:</td>"
            "<td><input type='Password' size=25 name='pwd'><br></td>"
        "</tr>"
        "<tr>"
            "<td><input type='submit' onclick='check(this.form)' value='Login'></td>"
        "</tr>"
    "</table>"
"</form>"
"<script>"
    "function check(form)"
    "{"
    "if(form.userid.value=='admin' && form.pwd.value=='admin')"
    "{"
    "window.open('/serverIndex')"
    "}"
    "else"
    "{"
    " alert('Error Password or Username')"
    "}"
    "}"
"</script>";
 
/*
 * Server Index Page
 */
 
const char* serverIndex = 
"<script src='https://ajax.googleapis.com/ajax/libs/jquery/3.2.1/jquery.min.js'></script>"
"<form method='POST' action='#' enctype='multipart/form-data' id='upload_form'>"
   "<input type='file' name='update'>"
        "<input type='submit' value='Update'>"
    "</form>"

  "<form action='/ViewLogFile' method='POST'>"
  "<button type='submit'>View Log File</button>"
"</form>"

  "<form action='/ViewStackLogFile' method='POST'>"
  "<button type='submit'>View Stack File</button>"
"</form>"

 "<div id='prg'>progress: 0%</div>"
 "<script>"
  "$('form').submit(function(e){"
  "e.preventDefault();"
  "var form = $('#upload_form')[0];"
  "var data = new FormData(form);"
  " $.ajax({"
  "url: '/update',"
  "type: 'POST',"
  "data: data,"
  "contentType: false,"
  "processData:false,"
  "xhr: function() {"
  "var xhr = new window.XMLHttpRequest();"
  "xhr.upload.addEventListener('progress', function(evt) {"
  "if (evt.lengthComputable) {"
  "var per = evt.loaded / evt.total;"
  "$('#prg').html('progress: ' + Math.round(per*100) + '%');"
  "}"
  "}, false);"
  "return xhr;"
  "},"
  "success:function(d, s) {"
  "console.log('success!')" 
 "},"
 "error: function (a, b, c) {"
 "}"
 "});"
 "});"
 "</script>";


void printValue(const byte &val) 
{
  DEBUG_PRINTF(" ");
  DEBUG_PRINTF("%d ", val);
}

void printParameters() 
{
  sensor.configMode();
   if (sensor.setNoOneWindow(1)) 
  {
    DEBUG_PRINTF("No-one window updated to 1s\n");
  } 
  else 
  {
    DEBUG_PRINTF("Failed to update no-one window\n");
  }
  sensor.requestParameters();
  DEBUG_PRINTF("Firmware: ");
  String fw(sensor.getFirmware());
  DEBUG_PRINT(fw);
  if (!fw.startsWith(LD2410_LATEST_FIRMWARE)) 
  {
    DEBUG_PRINT("To get the lastest features, upgrade your firmware to ");
    DEBUG_PRINTLN(LD2410_LATEST_FIRMWARE);
  }
  DEBUG_PRINTF("Protocol version: ");
  DEBUG_PRINTLN(sensor.getVersion());
  DEBUG_PRINT("Bluetooth MAC address: ");
  DEBUG_PRINTLN(sensor.getMACstr());

  const MyLD2410::ValuesArray &mThr = sensor.getMovingThresholds();
  const MyLD2410::ValuesArray &sThr = sensor.getStationaryThresholds();

  DEBUG_PRINT("Resolution (gate-width): ");
  DEBUG_PRINT(sensor.getResolution());
  DEBUG_PRINT("cm\nMax range: ");
  DEBUG_PRINT(sensor.getRange_cm());
  DEBUG_PRINT("cm\nMoving thresholds    [0,");
  DEBUG_PRINT(mThr.N);
  DEBUG_PRINT("]:");
  //Print using global function
  mThr.forEach(printValue);
  DEBUG_PRINT("\nStationary thresholds[0,");
  DEBUG_PRINT(sThr.N);
  DEBUG_PRINT("]:");
  //Print using lambda
  sThr.forEach([](const byte &val) {
    DEBUG_PRINT(' ');
    DEBUG_PRINT(val);
  });
  DEBUG_PRINT("\nNo-one window: ");
  DEBUG_PRINT(sensor.getNoOneWindow());
  DEBUG_PRINTLN('s');

  
  if (sensor.requestAuxConfig()) 
  {
    DEBUG_PRINT("Auxiliary Configuration: ");
    switch (sensor.getLightControl()) 
    {
      case LightControl::NO_LIGHT_CONTROL:
        DEBUG_PRINTLN("no light control");
        break;
      case LightControl::LIGHT_BELOW_THRESHOLD:
        DEBUG_PRINTLN("active when light is below the threshold of ");
        DEBUG_PRINTLN(sensor.getLightThreshold());
        break;
      case LightControl::LIGHT_ABOVE_THRESHOLD:
        DEBUG_PRINTLN("active when light is above the threshold of ");
        DEBUG_PRINTLN(sensor.getLightThreshold());
        break;
      default:
        break;
    }
    switch (sensor.getOutputControl()) 
    {
      case OutputControl::DEFAULT_LOW:
        DEBUG_PRINTLN("Default output level: LOW");
        break;
      case OutputControl::DEFAULT_HIGH:
        DEBUG_PRINTLN("Default output level: HIGH");
        break;
      default:
        break;
    }
  }
  sensor.configMode(false);
}


void DisplayConfigValues()
{
   DEBUG_PRINTF("ssid1 %s \n",ConfigData.ssid1);
   DEBUG_PRINTF("Password %s \n", ConfigData.password1);

   DEBUG_PRINTF("ssid2 %s \n",ConfigData.ssid2);
   DEBUG_PRINTF("Password2 %s \n", ConfigData.password2);

   DEBUG_PRINTF("ssid3 %s \n",ConfigData.ssid3);
   DEBUG_PRINTF("Password3 %s \n", ConfigData.password3);

   DEBUG_PRINTF("Device name = %s ", ConfigData.wifiDeviceName);
}
void ReadConfigValuesFromSPIFF()
{
  File jsonFile ;
  //const size_t capacity = JSON_OBJECT_SIZE(8) + 240;
  JsonDocument  doc;

  //const char* json = "{\"ssid1\":\"xxxxxxxxxxxxxxxxxxxx\",\"password1\":\"xxxxxxxxxxxxxxxxxxxx\",\"ssid2\":\"xxxxxxxxxxxxxxxxxxxx\",\"password2\":\"xxxxxxxxxxxxxxxxxxxx\",\"ssid3\":\"xxxxxxxxxxxxxxxxxxxx\",\"password3\":\"xxxxxxxxxxxxxxx\",\"wheelDiameter\":85.99,\"devicename\":\"xxxxxxxxxxxxxx\"}";
  
  jsonFile = LittleFS.open(JSON_CONFIG_FILE_NAME, FILE_READ);
  
  if (!jsonFile)
  {
     DEBUG_PRINTF("Unable to open %s",JSON_CONFIG_FILE_NAME);
     return ;
  }
  
  deserializeJson(doc, jsonFile);
  
  const char* ssid1               = doc["ssid1"] ; // "xxxxxxxxxxxxxxxxxxxx"
  const char* password1           = doc["password1"] ; // "xxxxxxxxxxxxxxxxxxxx"
  const char* ssid2               = doc["ssid2"] ; // "xxxxxxxxxxxxxxxxxxxx"
  const char* password2           = doc["password2"] ; // "xxxxxxxxxxxxxxxxxxxx"
  const char* ssid3               = doc["ssid3"] ; // "xxxxxxxxxxxxxxxxxxxx"
  const char* password3           = doc["password3"] ; // "xxxxxxxxxxxxxxx"  
  const char* devicename          = doc["devicename"] ; // "xxxxxxxxxxxxxx"
  const char* mqttBroker          = doc["mqttbroker"] ; // "xxxxxxxxxxxxxx"
  const char* mqttPublishingtopic = doc["mqttPublishingtopic"] ;
  const char* mqttSubcribingTopic  = doc["mqttSubcribingTopic"] ;
  jsonFile.close();

  DEBUG_PRINTF("-*--Reading for SPIFF starts \n");
  
  strcpy(ConfigData.ssid1,ssid1);
  strcpy(ConfigData.password1,password1);
  DEBUG_PRINTF("%s \n", ssid1);
  DEBUG_PRINTF("%s \n", password1);
  
  strcpy(ConfigData.ssid2,ssid2);
  strcpy(ConfigData.password2,password2);
  DEBUG_PRINTF("%s \n", ssid2);
  DEBUG_PRINTF("%s \n", password2);
  
  strcpy(ConfigData.ssid3,ssid3);
  strcpy(ConfigData.password3,password3);
  DEBUG_PRINTF("%s \n", ssid3);
  DEBUG_PRINTF("%s \n", password3);

  if (strlen(devicename) == 0 )
  {
    DEBUG_PRINTF("devicename field was not read for config.json\n") ;
    return ;
  }
  
  DEBUG_PRINTF(" device name %s \n", devicename);
  strcpy(ConfigData.wifiDeviceName,devicename);
  DEBUG_PRINTF("ReadConfigValues :1 \n") ;
  if (strlen(mqttBroker) == 0 )
  {
    DEBUG_PRINTF("mqttBroked field was not read for config.json\n") ;
    return ;
  }
  DEBUG_PRINTF("%s \n", mqttBroker);
  strcpy(ConfigData.mqttBroker,mqttBroker);

  strcpy(ConfigData.SUBCRIBING_TOPIC, mqttSubcribingTopic);
  strcpy(ConfigData.PUBLISH_TOPIC, mqttPublishingtopic);
}

void WiFiStationConnected(WiFiEvent_t event, WiFiEventInfo_t info) 
{
  DEBUG_PRINTF("Connected to AP successfully!\n");
}

void WiFiGotIP(WiFiEvent_t event, WiFiEventInfo_t info)
{
  DEBUG_PRINTF("WiFi connected\n");
  DEBUG_PRINTF("IP address: ");
  DEBUG_PRINTLN(WiFi.localIP());
}

void WiFiStationDisconnected(WiFiEvent_t event, WiFiEventInfo_t info)
{
  DEBUG_PRINTF("Disconnected from WiFi access point\n");
  DEBUG_PRINTF("WiFi lost connection. Reason: ");
  DEBUG_PRINT(info.wifi_sta_disconnected.reason);
  DEBUG_PRINTF("Attempting to reconnect...");
  //WiFi.reconnect();  // Tries to reconnect using stored credentials[web:1]
  // Commented the above code as per recommendataion from Claude and added the
  // following code

   wifiNeedsReconnect = true;  
}

bool ConnectToWifi()
{
  int count ;
  char ssid[40];
  char ipAddr[25];
  String IPaddress;
  DEBUG_PRINTF("ConnectToWifi: begin\n");

  DEBUG_PRINTF("ConnectToWifi: Hostname: %s\n",ConfigData.wifiDeviceName);
  WiFi.onEvent(WiFiStationConnected, ARDUINO_EVENT_WIFI_STA_CONNECTED);
  WiFi.onEvent(WiFiGotIP, ARDUINO_EVENT_WIFI_STA_GOT_IP);
  WiFi.onEvent(WiFiStationDisconnected, ARDUINO_EVENT_WIFI_STA_DISCONNECTED);

  WiFi.setHostname(ConfigData.wifiDeviceName);
  WiFi.setSleep(false);         // <-- disable power save, add this

  wifiMulti.addAP(ConfigData.ssid1, ConfigData.password1);   
  wifiMulti.addAP(ConfigData.ssid2, ConfigData.password2);    
  wifiMulti.addAP(ConfigData.ssid3, ConfigData.password3);    
  count  = 0 ;


  while  (wifiMulti.run()  !=  WL_CONNECTED) 
  { 
    //  Wait  for the Wi-Fi to  connect:  scan  for Wi-Fi networks, and connect to  the strongest of  the networks  above       
    delay(1000);        
    DEBUG_PRINTF("*");    
    count++ ;
    if (count > 40)
    {
       return false ;  
    }
  }   
  delay(5000);
  WiFi.setTxPower(WIFI_POWER_19_5dBm);



  DEBUG_PRINTF("\n");   
  strcpy(ssid,WiFi.SSID().c_str() );
  IPaddress = WiFi.localIP().toString();
  strcpy(ipAddr,IPaddress.c_str());
  DEBUG_PRINTF("Connected to  ");   
  DEBUG_PRINTF("%s\n",ssid);         
  DEBUG_PRINTF("IP  address: %s ",ipAddr);   
  DEBUG_PRINTLN(); 

   // Set mDNS
  if (!MDNS.begin(ConfigData.wifiDeviceName)) 
  {
     DEBUG_PRINTF("Error setting up MDNS responder! \n");
     delay(10000);
  }

  // Add service to MDNS-SD
  MDNS.addService("_http", "_tcp", 80);
  DEBUG_PRINTLN("mDNS responder started. Access your ESP32 at http://" + String(ConfigData.wifiDeviceName) + ".local");

  WiFi.softAPdisconnect (true);   //Disable the Access point mode.
  DEBUG_PRINTF("ConnectToWifi: End\n");

 
  return true ;
}


// -----------------------------------------------------------------
// Named callback function for incoming subscribed messages.
// Using a plain function instead of a lambda is handy when the
// handling logic is long, or you want to reuse it / unit test it,
// or you want the same handler for multiple topics.
//
// Signature must match: void(const String &topic, const String &payload)
// (there's also a single-arg form: void(const String &payload) if you
// don't need to know which topic it came from)
// -----------------------------------------------------------------
void onDataReceived(const std::string &topic, const std::string &payload)

{
  DEBUG_PRINTF("[MQTT] Data received | topic: %s | payload: %s\n",
                topic.c_str(), payload.c_str());
 
  // Route/parse the payload here
  /*
  if (topic == SUB_TOPIC) {
    if (payload == "LED_ON") {
      // digitalWrite(LED_BUILTIN, HIGH);
    } else if (payload == "LED_OFF") {
      // digitalWrite(LED_BUILTIN, LOW);
    } else {
      // e.g. parse JSON, numeric sensor setpoints, etc.
    }
  }
  */
}
 
// -----------------------------------------------------------------
// This callback is REQUIRED by the library. It is invoked once the
// underlying esp-mqtt client successfully connects to the broker.
// This is where you should subscribe to topics and/or publish.
// -----------------------------------------------------------------
void onMqttConnect(esp_mqtt_client_handle_t client)
{
 char message[100];
 long rssi = WiFi.RSSI();
  DEBUG_PRINTF("[MQTT] Connected to broker\n");
 
  // Subscribe and point it at the named function above.
  // (You can pass a function pointer directly — no lambda needed.)
  mqttClient.subscribe(ConfigData.SUBCRIBING_TOPIC, onDataReceived);
 
  // You can subscribe to more topics, each with its own handler:
  // mqttClient.subscribe("esp32/other/topic", onDataReceived);
 
  // Publish a "hello" message right after connecting
  snprintf(message, sizeof(message),
               "{\"sensor_room\":\"%s\", \"status\" : \"ON_LINE\", \"Wifi\" : \"%ld\" }", ConfigData.wifiDeviceName,rssi);
  //sprintf(message,"{  \"Message\" : \"%s connected\"  }",ConfigData.wifiDeviceName) ;
  mqttClient.publish(ConfigData.PUBLISH_TOPIC , message, 0, false);

}

void handleMQTT(void *handler_args, esp_event_base_t base,
                int32_t event_id, void *event_data)
{
  (void)base;
  (void)event_id;
  auto *client = static_cast<ESP32MQTTClient *>(handler_args);
  auto *event  = static_cast<esp_mqtt_event_handle_t>(event_data);
  client->onEventCallback(event);
}

void SendStackLogFileToBrowser()
{
  File    filePtr ;
  String  selectedRecordFileNameStr, recordFile ;
  char    SelectedFileName[50], header[100];
  int     len,sent ;

     strcpy(SelectedFileName,"/stack.txt");
     if (LittleFS.exists("/stack.txt"))
     { 
      
        filePtr = LittleFS.open(SelectedFileName,  "r");
        recordFile = selectedRecordFileNameStr.substring(1); // Removing the / from the beginning of file name
        len = recordFile.length();
        recordFile.toCharArray(SelectedFileName,len+1); 
        sprintf(header,"filename=\"%s\"",SelectedFileName);
        server.sendHeader("Content-Disposition",header);
        sent = server.streamFile(filePtr, "application/text");  
        filePtr.close();
        server.send(200, "text/html", "<HTML> File sent </HTML>");   
     }
     else
     {
        char error[100];
        sprintf(error,"<HTML> <H1> File %s not found </H1> </HTML>",SelectedFileName);
        server.sendHeader("Connection", "close");
        server.send(200, "text/html", error);
     }
 
}
void SendLogFileToBrowser()
{
  File    filePtr ;
  String  selectedRecordFileNameStr, recordFile ;
  char    SelectedFileName[50], header[100];
  int     len , sent ;
 
     DEBUG_PRINTF("SendLogFileToBrowser Entry\n");
     strcpy(SelectedFileName,"/logging.txt");

     if (LittleFS.exists(SelectedFileName))
     { 
      
        filePtr = LittleFS.open(SelectedFileName,  "r");
        recordFile = selectedRecordFileNameStr.substring(1); // Removing the / from the beginning of file name
        len = recordFile.length();
        recordFile.toCharArray(SelectedFileName,len+1); 
        sprintf(header,"filename=\"%s\"",SelectedFileName);
        server.sendHeader("Content-Disposition",header);
        sent = server.streamFile(filePtr, "application/text");  
        filePtr.close();
        server.send(200, "text/html", "<HTML> File sent </HTML>");   
        DEBUG_PRINTF("SendLogFileToBrowser File Sent\n");
     }
     else
     {
        char error[100];
        sprintf(error,"<HTML> <H1> File %s not found </H1> </HTML>",SelectedFileName);
        server.sendHeader("Connection", "close");
        server.send(200, "text/html", error);
        DEBUG_PRINTF("SendLogFileToBrowser File not found\n");
     }
     DEBUG_PRINTF("SendLogFileToBrowser Exit\n");
}
void StartWebServer()
{
  
    server.on("/ViewLogFile",      HTTP_POST,  SendLogFileToBrowser);
    server.on("/ViewStackLogFile",      HTTP_POST,  SendStackLogFileToBrowser);
  
    server.on("/", HTTP_GET, []() 
    {
    String html = String(loginIndex);
    html.replace("%HOSTNAME%", ConfigData.wifiDeviceName);  // insert this device's hostname
    server.sendHeader("Connection", "close");
    server.send(200, "text/html", html);
    });
  server.on("/serverIndex", HTTP_GET, []() 
  {
    server.sendHeader("Connection", "close");
    server.send(200, "text/html", serverIndex);
  });
  /*handling uploading firmware file */
  server.on("/update", HTTP_POST, []() 
  {
  
    server.sendHeader("Connection", "close");
    server.send(200, "text/plain", (Update.hasError()) ? "FAIL" : "OK");
    Serial.printf("Error: %s\n", Update.errorString());
    ESP.restart();
 
  }, []() {
    HTTPUpload& upload = server.upload();
    if (upload.status == UPLOAD_FILE_START) {
      DEBUG_PRINTF("Update: %s\n", upload.filename.c_str());
      if (!Update.begin(UPDATE_SIZE_UNKNOWN)) { //start with max available size
        Update.printError(Serial);
      }
    } else if (upload.status == UPLOAD_FILE_WRITE) {
      /* flashing firmware to ESP*/
      if (Update.write(upload.buf, upload.currentSize) != upload.currentSize) {
        Update.printError(Serial);
      }
    } else if (upload.status == UPLOAD_FILE_END) {
      if (Update.end(true)) { //true to set the size to the current progress
        DEBUG_PRINTF("Free SketchSpace : %u",ESP.getFreeSketchSpace());
        DEBUG_PRINTF("Update Success: %u\nRebooting...\n", upload.totalSize);
      } else {
        Update.printError(Serial);
      }
    }
  });
  server.begin();
}
void setup() 
{
  char message[100];
  File fp;
  Serial.begin(SERIAL_BAUD_RATE);
  delay(2000);
  DEBUG_PRINTF("Serial port started\n") ;

  if(LittleFS.begin(true) ==  false)
  {
    DEBUG_PRINTF("LittleFS mount failed - formating\n");
  }
  DEBUG_PRINTF("Littel FS intialized\n") ;
  ReadConfigValuesFromSPIFF();
  DisplayConfigValues();
  ConnectToWifi(); 
  DEBUG_PRINTF("rx_pin= %d, tx_pn= %d", RX_PIN,TX_PIN);
  sensorSerial.begin(LD2410_BAUD_RATE, SERIAL_8N1, RX_PIN, TX_PIN);
  delay(2000);
  DEBUG_PRINT(__FILE__);
  if (!sensor.begin()) 
  {
    DEBUG_PRINTF("Failed to communicate with the sensor.\n");
    //while (true) {}
  }
  if (sensor.check() == MyLD2410::Response::DATA)
  {
    DEBUG_PRINTF("Sensor Check successfull\n");
  }
  else
  {
    DEBUG_PRINTLN(sensor.check()) ;
    DEBUG_PRINTF("Sensor Check not successfull\n");
  }

  if(sensor.presenceDetected() )
  {
    DEBUG_PRINTF("Setup: Presence detected\n");
  }
  else
  {
    DEBUG_PRINTF("Setup: Presence not detected\n");
  }

  if (LittleFS.exists("/logging.txt") == false)
  {
    fp = LittleFS.open("/logging.txt", FILE_WRITE);   
    fp.print("Logging\n") ;
    fp.close();
  }

  if (LittleFS.exists("/stack.txt") == false )
  {
    fp = LittleFS.open("/stack.txt", FILE_WRITE);
    fp.print("Stack logging\n");
    fp.close();
  }

  printParameters();
  //ftpSrv.begin(FTP_USER_NAME,FTP_PASSWORD);
  delay(2000);
  DEBUG_PRINTF("Done!\n");
  // Reduce it, e.g. to 2 seconds
  sensor.enhancedMode(true) ;
  DEBUG_PRINTF("1\n");
  mqttClient.enableDebuggingMessages();
  DEBUG_PRINTF("2\n");
  mqttClient.setURI(ConfigData.mqttBroker);
  DEBUG_PRINTF("3\n");
  mqttClient.setMqttClientName(ConfigData.wifiDeviceName);
  DEBUG_PRINTF("4\n");
   snprintf(message, sizeof(message),
               "{ \"sensor_room\":\"%s\", \"status\" : \"OFF_LINE\" }" ,ConfigData.wifiDeviceName);

  //sprintf(message,"{  \"Message\" : \"%s connected\"  }",ConfigData.wifiDeviceName) ;
  //sprintf(message, "{\"mqttStatus\" :\"Going OffLine\",\"sensor_room\" : \"%s\"   }",ConfigData.wifiDeviceName);
  mqttClient.enableLastWillMessage(ConfigData.PUBLISH_TOPIC , message, true);
  DEBUG_PRINTF("5\n");
  mqttClient.setKeepAlive(30);
 
  mqttClient.loopStart(); // returns immediately; runs in background task
  DEBUG_PRINTF("Web Server Above start\n");
  StartWebServer();
  DEBUG_PRINTF("Web Server Setup Done\n");
  StartTelnet() ;

  xTaskCreatePinnedToCore(PROCESS_LD2410c,
                          "Process LD2410c",
                          4096,
                          NULL,
                          1,
                          NULL,
                          1);
  DEBUG_PRINTF("Setup Completed \n");
  }
#if 0
// *****************************************************************************
//                          Beginning of BLOCKED CODE c
// ******************************************************************************


// *********************************************************************************************
//                                    END OF BLOCKED CODE
// ********************************************************************************************* 
#endif

void PROCESS_LD2410c(void *parameters)
{
  int  currentPresenceState   = NO_PRESENCE;
  std::string presence        = "X";
  bool statusChanged          = false;
  bool StationaryTargetDetected = false;
  bool MovingTargetDetected     = false;
  int  StationaryTargetDistance = -1;
  int  MovingTargetDistance     = -1;
  unsigned long idlingTime      = 0;
  unsigned long lastIdleMessage = 0;

  const unsigned long idleInterval = 30000;  // ms - keep alive while no presence

  char mqttMessage[200];   // fixed buffer, no heap allocation, sized for this payload
  

  DEBUG_PRINTF("PROCESS_LD2410c Started\n");

  while (true)
  {
    if (sensor.check() == MyLD2410::Response::DATA)
    {
      if (sensor.presenceDetected())
      {
        currentPresenceState = PRESENSE_DETECTED;
        presence = "DETECTED";
       
      }
      else
      {
        currentPresenceState = NO_PRESENCE;
        presence = "NOT_DETECTED" ;
      }

      if (lastStateOfPresence != currentPresenceState)
      {
        statusChanged = true;
        lastStateOfPresence = currentPresenceState;
        
      }

      // Reflect current reading each pass instead of latching forever
      StationaryTargetDetected = sensor.stationaryTargetDetected();
      StationaryTargetDistance = StationaryTargetDetected ? sensor.stationaryTargetDistance() : -1;

      MovingTargetDetected = sensor.movingTargetDetected();
      MovingTargetDistance = MovingTargetDetected ? sensor.movingTargetDistance() : -1;

      // Publish ONLY on a genuine state transition (NO_PRESENCE <-> PRESENCE_DETECTED)
      bool shouldPublish = mqttClient.isConnected() && statusChanged;

      if (shouldPublish)
      {
        long rssi = WiFi.RSSI();
        lastIdleMessage = millis();  // presence activity also resets idle timer

        // Hand-built JSON: no ArduinoJson, no heap allocation, no fragmentation risk.
        // Safe because every field here is either a known-fixed string or a number
        // we control — none of it is arbitrary/user-supplied text needing escaping.
        int written = snprintf(mqttMessage, sizeof(mqttMessage),
          "{\"sensor_room\":\"%s\",\"status\":\"%s\","
          "\"Moving Target\":%s,\"Moving Target Dist\":%d,"
          "\"Stationary Target\":%s,\"Stationary Target Dist\":%d, \"Wifi\":%ld}",
          ConfigData.wifiDeviceName,
          presence.c_str(),
          MovingTargetDetected     ? "true" : "false",
          MovingTargetDistance,
          StationaryTargetDetected ? "true" : "false",
          StationaryTargetDistance,
          rssi
        );
        

        if (written < 0 || written >= (int)sizeof(mqttMessage))
        {
           DEBUG_PRINTF("[MQTT] WARNING: message truncated (needed %d bytes, buffer is %d)\n",
                        written, (int)sizeof(mqttMessage));
        }

        bool ok = mqttClient.publish(ConfigData.PUBLISH_TOPIC, mqttMessage, 0, false);
        DEBUG_PRINTF("[MQTT] Publish %s -> %s\n", ok ? "OK" : "FAILED", mqttMessage);

        statusChanged = false;
      }
    } /* End of sensor.check() == DATA */

    // Keep-alive: independent of whether a fresh sensor frame arrived this pass,
    // and only while there is genuinely no presence.
    idlingTime = millis() - lastIdleMessage;
    if (idlingTime > idleInterval)
    {
      long rssi = WiFi.RSSI();
      snprintf(mqttMessage, sizeof(mqttMessage),
               "{\"sensor_room\":\"%s\", \"status\" : \"KEEP_ALIVE\", \"Wifi\" : \"%ld\" }", ConfigData.wifiDeviceName,rssi);

      bool ok = mqttClient.publish(ConfigData.PUBLISH_TOPIC, mqttMessage, 0, false);
      DEBUG_PRINTF("[MQTT] KeepAlive %s -> %s\n", ok ? "OK" : "FAILED", mqttMessage);
      

      lastIdleMessage = millis();
    }

    vTaskDelay(pdMS_TO_TICKS(20));  // yield to RTOS scheduler / avoid watchdog trips
  } /* while loop */
} /* end */

void StartTelnet()
{
  telnetServer.begin();
  telnetServer.setNoDelay(true);
}

// Call this in loop(), and replace Serial.printf with TelnetPrintf
void HandleTelnet()
{
  if (telnetServer.hasClient())
  {
    if (!telnetClient || !telnetClient.connected())
    {
      if (telnetClient) telnetClient.stop();
      telnetClient = telnetServer.available();
    }
    else
    {
      telnetServer.available().stop(); // reject extra connections
    }
  }
}

void TelnetPrintf(const char* fmt, ...)
{
  char buf[200];
  va_list args;
  va_start(args, fmt);
  vsnprintf(buf, sizeof(buf), fmt, args);
  va_end(args);

  //DEBUG_PRINTF("%s\n",buf);              // keep normal serial too
  if (telnetClient && telnetClient.connected())
  {
    telnetClient.print(buf);      // mirror to telnet if connected
  }
}

void loop() 
{
    //int WifiStatus = 0;
    //bool mqttConnected = false ;
    //int FreeHeap, MinFreeHeap ;
    //File fp ;
    char msg[150] ;
    unsigned long currentTime ;
    
    server.handleClient();
    HandleTelnet();
    // following code is added for reconnection as per the 
    // recommendataion made by claude

    if (wifiNeedsReconnect) 
    {
       wifiNeedsReconnect = false;
       TelnetPrintf("Reconnecting via wifiMulti...\n");
       wifiMulti.run();   // <-- see point 3, use this instead of WiFi.reconnect()
    }
    currentTime = millis();
    if ((currentTime - lastPrint) >= 10000)
    {
       snprintf(msg, sizeof(msg), "[%lu] SSID:%s BSSID:%s RSSI:%d Status:%d\n",
       millis(), WiFi.SSID().c_str(), WiFi.BSSIDstr().c_str(), WiFi.RSSI(), WiFi.status());
       TelnetPrintf("%s",msg) ;

       lastPrint = currentTime;
     }
}