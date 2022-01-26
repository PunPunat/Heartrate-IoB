/** 
 *  TaskScheduler
 *
 *  Initially only tasks 1 and 2 are enabled
 *  Task1 runs every 2 seconds 1indefinitely
 *  Task2 runs every 15 seconds indefinitely
 */
 
 


#include <Arduino.h>  //Include Library from AIS 
#include <SIM76xx.h>
#include <GSMClient.h>
#include <GSMUdp.h>
#include <GsmNtpClient.h>
#include <Storage.h>  
#include <PubSubClient.h>  
//#include  <TimeSimple.h>

//Storage
GSMUdp ntpUDP;
GsmNtpClient timeClient(ntpUDP);
String file_name; 
String directory_name;
unsigned long  epoch;


//heartrate
#define heartratePin 33         
#include "DFRobot_Heartrate.h"
DFRobot_Heartrate heartrate(ANALOG_MODE);  // SETUP heart rate sensor : ANALOG_MODE or DIGTAL_MODE

//GPS
#include <SIM76xx.h>
#include <GPS.h>

const char *server = "broker.hivemq.com";   // SETUP mqtt broker server
char mqtt_payload_lati[50] = "";
char mqtt_payload_long[50] = "";
char rateh[50] = "";


// Callback function header
void callback(char* topic, byte* payload, unsigned int length);

GSMClient gsm_client;
PubSubClient client(server, 1883, callback, gsm_client);   //SETUP mqtt port 1883

// Callback function
void callback(char* topic, byte* payload, unsigned int length) {
  // In order to republish this payload, a copy must be made
  // as the orignal payload buffer will be overwritten whilst
  // constructing the PUBLISH packet.

  // Allocate the correct amount of memory for the payload copy
  byte* p = (byte*)malloc(length);
  // Copy the payload to the new buffer
  memcpy(p,payload,length);
  client.publish("outTopic", p, length);
  // Free the memory
  free(p);
}


// Callback methods prototypes
void t1Callback();
void t2Callback();
void save_data();



unsigned long i=0;
void t1Callback() {
    
    uint8_t rateValue;
    heartrate.getValue(heartratePin); ///< A33 foot sampled values
    rateValue = heartrate.getRate(); ///< Get heart rate value 
    Serial.println(rateValue);
    
    if(rateValue)  {  // est. 1 sec
      Serial.println(rateValue);
      snprintf (rateh, 50 , "%d", rateValue);
      client.publish("outTopic/heartpunpun",rateh);       //ตั้งค่า Topic : outTopic/heartpunpun ของ mqtt publish for sent heart rate value
    }

    delay(20);
   
}

//Function save data
void save_data(){
  bool write_ok;
  unsigned int Signal;
  
  Signal = analogRead(heartratePin);  // Read the PulseSensor's value.
  Serial.println(Signal);
  file_name = directory_name+ String("/") + String(millis()) + String(".txt");
  Serial.print("file_name= ");
  Serial.println(file_name);
  write_ok = Storage.fileWrite(file_name, String(Signal));
  delay(50);
  if (!write_ok) {
    Serial.println("Write file error !");
  }else{
    Serial.println("write");
  }
}

void t2Callback() {
    if (GPS.available()) {
      Serial.printf("Location: %.7f,%.7f\n", GPS.latitude(), GPS.longitude());
      snprintf (mqtt_payload_lati, 50, "%.7f", GPS.latitude());
      snprintf (mqtt_payload_long, 50, "%.7f", GPS.longitude());
      client.publish("outTopic/lati",mqtt_payload_lati);  //ตั้งค่า Topic : outTopic/lati ของ mqtt publish for sent latitude
      client.publish("outTopic/long",mqtt_payload_long);  //ตั้งค่า Topic : outTopic/long ของ mqtt publish for sent longitude
    } else {
      Serial.println("GPS not fixed");
      client.publish("outTopic/gpspun","GPS not fixed");    //ตั้งค่า Topic : outTopic/gpspun ของ mqtt publish for status GPS not fixed
  
    }
    delay(1000);  
}



unsigned long currentMillis ;
unsigned long prevMillis ;
void setup () {
    Serial.begin(115200);
  
    while(!GSM.begin()) {
    Serial.println("GSM setup fail");
    delay(2000);
    }

    if (client.connect("")) {
      ;
    }
  
    currentMillis = prevMillis = millis();
}

#define JOB1_MS (unsigned int) 20000 // 20 seconds
#define JOB2_MS (unsigned int) 60000 // 60 seconds
#define JOB3_MS (unsigned int) 10000 // 10 seconds
void loop () {
  bool write_ok;

    // job1 ====================================================
    while(millis()-prevMillis < JOB1_MS ){
         t1Callback();
         client.loop();
    }
    prevMillis= millis();
    
    // job2 ====================================================
    do {
        timeClient.update();  
        epoch = timeClient.getEpochTime();
        client.loop(); 
        delay(1000);
    }while(epoch<1642238436);
      
    Serial.println();
    Serial.print("epoch: ");
    Serial.println(epoch);
    
    directory_name = String("D:/")+ String(epoch);
    write_ok = Storage.mkdir(directory_name);
    if (!write_ok) {
       Serial.println("create dir error !");
     }
    Serial.print("dir name= ");
    Serial.println(directory_name);
    
    while(millis()-prevMillis < JOB2_MS ){
         save_data();
         client.loop();
    }
    prevMillis= millis();
    
    // job3   ====================================================
    while(millis()-prevMillis < JOB3_MS ){
         t2Callback();
    }
    prevMillis= millis();
    
    
}
