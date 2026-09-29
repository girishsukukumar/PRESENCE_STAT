


#include "Arduino.h"
#include <WiFiClient.h>

#include <ESPmDNS.h>
#include <Update.h>
#include <WiFiMulti.h>
#include <ArduinoJson.h>
#include "ESP32MQTTClient.h"
#include <WebSerial.h>
#include <LittleFS.h>
#include <M5Unified.h>
#include <SPI.h>
#include <SD.h>
#include <time.h>

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



/* ***************** Prototype Definitions************** */

void checkDailyRollover() ;
void checkOngoingToiletVisits() ;
void finalizeBedSession() ;
unsigned long getEpochTime() ;
void checkHourlyPatternImmediate(int hour)  ;
void maybeRecordWakeTime(time_t startEpoch, time_t endEpoch, int durationSec) ;

void IncrementBathRoomDayVisits() ;
void IncrementBathRoomNightVisits() ;
void UpdateBathRoomDayVisitDuration(int duration) ;
void UpdateBathRoomNightVisitDuration(int duration) ;
/* ***************** End Prototype Definitions ******************** */

const int NUM_ROOMS = 5;

const char* ROOMS[] = {"LivingRoom", "BedRoom", "Toilet-1", "Kitchen", "Hallway"};

typedef struct RoomState
 {
  bool     occupied = false;
  time_t   occupiedSinceEpoch = 0;
  time_t   lastMsgEpoch = 0;
  char     roomName[10];
  unsigned long timeSpend ;
} RoomState_t;


struct RoomState roomState[NUM_ROOMS];



void InitRoomDataStructure()
{
  int i ;
   for(i=0;i< NUM_ROOMS;i++)
  {
      roomState[i].occupied = false;
      roomState[i].occupiedSinceEpoch = 0;
      roomState[i].lastMsgEpoch = 0;
      roomState[i].timeSpend = 0 ;

  }

}





int findRoomIndex(const char* room) 
{
  for (int i = 0; i < NUM_ROOMS; i++) 
  {
    if (strcmp(ROOMS[i], room) == 0)
    { 
        return i;
    }
  }
  return -1;
}

bool GetOccupancyStateOfRoom(const char* roomName)
{
   int idx ;
   bool state ;
   idx = findRoomIndex(roomName) ;
   state = roomState[idx].occupied ;
   return state; 
}

time_t GetOccupiedEpocTimeOfRoom(const char* roomName)
{
   int idx ;
   time_t  OccupiedTime ;
   idx = findRoomIndex(roomName) ;
   OccupiedTime = roomState[idx].occupiedSinceEpoch ;
   return OccupiedTime; 
}

bool IsDayOrNight(unsigned long epochTime) 
{
	time_t rawTime = (time_t)epochTime;
  
  // CRITICAL CHANGE: Use localtime() instead of gmtime()
  struct tm *ti = localtime(&rawTime); 
  bool retVal = false ;
  
  int year   = ti->tm_year + 1900; 
  int month  = ti->tm_mon + 1;    
  int day    = ti->tm_mday;         
  int hour   = ti->tm_hour;        
  int minute = ti->tm_min;       
  int second = ti->tm_sec;

  if ((hour > 6 ) && (hour < 20))
     retVal = true ;
  else  
     retVal = false ;

  return retVal ;  

}
/* *****************************************************************************
Function Name : onDataReceived
INPUT PARAMETERS :
   1. MQTT topic of the received message
   2. Payload of the received message
RETURN
   None
FUNCTION
   MQTT ASYCN call back when a message is received 
******************************************************************************/

void onDataReceived(const std::string &topic, const std::string &payload)
{

  char message[200];
  JsonDocument doc;
  bool detected ;


  
  DeserializationError error = deserializeJson(doc, payload);
  if (error) 
  {
    Serial.print("JSON parse failed: ");
    Serial.println(error.c_str());
    return;
  }
  noOfMQTTMessages++ ;

  const char* room   = doc["sensor_room"];
  const char* status = doc["status"];
  bool movingTarget     = doc["Moving Target"];
  bool stationaryTarget = doc["Stationary Target"];
  char event[10] ; // To be incorportated later


  int movingDist     = doc["Moving Target Dist"];
  int stationaryDist = doc["Stationary Target Dist"];
  
  int idx = findRoomIndex(room);

    if (idx < 0) 
    {
      return;
    }

    time_t now = time(nullptr);
    roomState[idx].lastMsgEpoch = now;
    if ((strcmp(status, "DETECTED") == 0) && (roomState[idx].occupied == false))
    {
        Serial.printf("%s :status == DETECTED and occupied == False\n",room) ;
        roomState[idx].occupied = true;
        roomState[idx].occupiedSinceEpoch = now;

	     if (strcmp(room, "Toilet-1")==0)
	     {

		      DEBUG_PRINTF("Presence detected in Toilet-1\n");
		
		      if (IsDayOrNight(getEpochTime()) == true)
		      {
             DEBUG_PRINTF("IncrementBathRoomDayVisits\n") ;
		
			       IncrementBathRoomDayVisits() ;
		      }
		      else
		      {
              DEBUG_PRINTF("IncrementBathRoomNightVisits\n") ;
			        IncrementBathRoomNightVisits() ;
		      }
       }
    
	
	     if (strcmp(room, "BedRoom")==0)
	     {
		          DEBUG_PRINTF("Presence detected in BedRoom\n");

		         //if (IsDayOrNight(getEpochTime()) == false)
 		         //{
                 DEBUG_PRINTF("Presence detected in BedRoom at night\n");
                 /* Track movements only during night */
			           if (movingTarget == true)
                 {
                    incrementBedRoomMovementCounter() ;
                 }
		         //}		
        }

    } 
    else if ((strcmp(status, "NOT_DETECTED") == 0) && 
             (roomState[idx].occupied == true)) 
    {
        Serial.printf("%s: status == NOT_DETECTED and occupied == true\n",room) ;
        roomState[idx].occupied = false;
        WhenRoomIsEmpty(room, roomState[idx].occupiedSinceEpoch, now);
    }

    else if ((strcmp(status, "KEEP_ALIVE") == 0)) 
    {
        struct tm *currentTimeDate ;
        int hour ;
        int minutes ;
        roomState[idx].lastMsgEpoch = time(nullptr);
        //Get current time,
        currentTimeDate = convertEpochToHumanReadableFormat(getEpochTime()) ;
        hour =  currentTimeDate->tm_hour ;
        minutes = currentTimeDate->tm_min ;
         
        if (hour == 11)
        {
          if (minutes > 55)
          {
              char message[100];
              /* Generate room wise break up report */

              int TimeSpendinLivingRoom = roomState[0].timeSpend;
              int TimeSpendinBedRoom    = roomState[1].timeSpend ;
              int TimeSpendinToilet     = roomState[2].timeSpend ;
              int TimeSpendinKitchen    = roomState[3].timeSpend ;
              int totalTime = TimeSpendinLivingRoom + TimeSpendinBedRoom + TimeSpendinToilet + TimeSpendinKitchen ;
              
              float percentageSpendLivingRoom  = (TimeSpendinLivingRoom/totalTime)/100 ;
              float percentageSpendBedRoom     = (TimeSpendinBedRoom/totalTime)/100 ;
              float percentageSpendToilet      = (TimeSpendinToilet/totalTime)/100 ;
              float percentageSpendKitchen     = (TimeSpendinKitchen/totalTime)/100 ;

              snprintf(message,sizeof(message), 
                       "%d-%d-%d , %f , %f , %f , %f", currentTimeDate->tm_mday,
                                                       currentTimeDate->tm_mon,
                                                       currentTimeDate->tm_year,
                                                       percentageSpendLivingRoom,
                                                       percentageSpendBedRoom,
                                                       percentageSpendToilet,
                                                       percentageSpendKitchen) ;

              mqttClient.publish(ConfigData.REPORT_TOPIC, message, 0, false);
          }
        }
    }


}


// ---------------- SD audit log ----------------
void logIntervalToSD(const char* room, time_t startEpoch, time_t endEpoch, int durationSec) 
{
  struct tm *tmNow = localtime(&endEpoch);
  char filename[100];
  snprintf(filename, sizeof(filename), "/intervals/%04d-%02d-%02d.csv",
           tmNow->tm_year + 1900, tmNow->tm_mon + 1, tmNow->tm_mday);
  
  Serial.printf("logIntervalToSD: %s \n",filename);

  File f = SD.open(filename, FILE_APPEND);
  if (f) 
  {
    f.printf("%s,%lu,%lu,%d\n", room, (unsigned long)startEpoch, (unsigned long)endEpoch, durationSec);
    Serial.printf("%s,%lu,%lu,%d\n", room, (unsigned long)startEpoch, (unsigned long)endEpoch, durationSec) ;
    f.close();
  }
}


void WhenRoomIsEmpty(const char* room, time_t startEpoch, time_t endEpoch) 
{
  
  int timeSpendinRoom = (int)(endEpoch - startEpoch);

  logIntervalToSD(room, startEpoch, endEpoch, timeSpendinRoom);
  
  DEBUG_PRINTF("WhenRoomIsEmpty %s\n",room);

  int idx = findRoomIndex(room);
  
  roomState[idx].timeSpend =  roomState[idx].timeSpend + timeSpendinRoom ;

  if (strcmp(room, "Toilet-1") == 0)
  {
    // Real-time per-visit duration check.
    // NOTE: the bathroom sensor on the leaves is named "Toilet-1" (there is
    // no "BathRoom" room); this branch was previously dead because the
    // string never matched.
  
    if (IsDayOrNight(getEpochTime()) == true)
	  {
		   UpdateBathRoomDayVisitDuration(timeSpendinRoom);  
	  }
	  else
	  {
		   UpdateBathRoomNightVisitDuration(timeSpendinRoom); 
	  }
    
    PrintBathRoomUsageData();
  }


  if (strcmp(room, "BedRoom") == 0)
  {
      
    
    //maybeRecordWakeTime(startEpoch, endEpoch, timeSpendinRoom);
    
  }

  if (strcmp(room, "LivingRoom") == 0) 
  {

  } 

  if (strcmp(room, "Kitchen") == 0) 
  {
  
  }

}






