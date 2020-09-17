#include <Arduino.h>
#include <WiFi.h>          //builtin library
#include "config.h"


WiFiClient client;
int wifi_reconnect_tries = 0;
long wifi_reconnect_time = 0L;
long wifi_check_time = 15000L;
long tv_reconnect_time = 0L;
long tv_check_time = 15000L;
int tv_status = WL_IDLE_STATUS;
uint64_t update_timer_time = 0;

void connectTV();
void checkTvPower();

void connectWiFi()
{

    // WiFi.hostname(HOSTNAME);
    ++wifi_reconnect_tries;
    boolean networkScan = false;
    int n = WiFi.scanNetworks();
    delay(300);
    for (int i = 0; i < n; ++i) {

        if (WiFi.SSID(i) == WIFI_SSID) {
            String this_print = String(WIFI_SSID) + " is available";
            Serial.println(this_print);
            networkScan = true;
            break;
        }
    }
    if(networkScan) {
        String this_print = "Connecting to " + String(WIFI_SSID);
        Serial.println(this_print);
        long wifi_initiate = millis();
        WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
        while (WiFi.status() != WL_CONNECTED) {
            /*Serial.print(".");*/
            if (WiFi.status() == WL_CONNECTED) {
                break;
            }
            if ((millis() - wifi_initiate) > 15000L) {
                break;
            }
            delay(500);
        }
        if (WiFi.status() == WL_CONNECTED) {
            Serial.println("Connected!!");
            Serial.println(WiFi.localIP());
            wifi_reconnect_tries = 0;
        } else if ((WiFi.status() != WL_CONNECTED) && (wifi_reconnect_tries > 3)) {
            String this_print = " Failed to connect to " + String(WIFI_SSID) + " Rebooting...";
            Serial.println(this_print);
            ESP.restart();
        }
    } else {
        if (wifi_reconnect_tries > 3) {
            wifi_check_time = 300000L;
            wifi_reconnect_tries = 0;
            Serial.println("System will try again after 5 minutes");
        }
    }
}



void keeplive()
{
    //client.loop();
    if((WiFi.status() != WL_CONNECTED) && ((millis() - wifi_reconnect_time) > wifi_check_time)) {
        wifi_check_time = 15000L;
        wifi_reconnect_time = millis();
        connectWiFi();
    }
    if((!tv_status) && WiFi.status() == WL_CONNECTED && ((millis() - tv_reconnect_time) > tv_check_time)) {
        tv_check_time = 15000L;
        tv_reconnect_time = millis();
        connectTV();
    }
}


void connectTV(){
  // Use WiFiClient class to create TCP connections

     tv_status = client.connect(TV_IP, TV_TCP_PORT);
     if (!tv_status) {
         Serial.println("TV connection failed");
         return;
     }
     else{
         Serial.println("Connected to TV");
     }
}


void checkTvPower(){

  Serial.println("Check tv status");
  if(!tv_status) return;

     // This will send the request to the server
     client.print("*SEPOWR################\n");
     unsigned long timeout = millis();
     while (client.available() == 0) {
         if (millis() - timeout > 5000) {
             Serial.println(">>> Client Timeout !");
             tv_status = 0;
             client.stop();

             // digitalWrite(LED_PIN, 0);
             // digitalWrite(OUTPUT_PIN, 1);
             return;
           }
     }

     // Read all the lines of the reply from server and print them to Serial
     while(client.available()) {
         String line = client.readStringUntil('\n');
         char powerStatus = line.charAt(line.length()-1) - 48;
         Serial.println(line);

         if(line.substring(0,7) != "*SAPOWR") return;
         if(powerStatus != 1 && powerStatus != 0)
           return;



         digitalWrite(LED_PIN, powerStatus);
         digitalWrite(OUTPUT_PIN, !powerStatus);
     }



}




void setup() {

    Serial.begin(115200);
    Serial.println("Boot OK!!");
    WiFi.mode(WIFI_STA);
    connectWiFi();


    pinMode(LED_PIN, OUTPUT);
    pinMode(OUTPUT_PIN, OUTPUT);
    digitalWrite(OUTPUT_PIN, HIGH);
    update_timer_time = millis();
    connectTV();
}

void loop() {

    keeplive();   //necessary to call keep alive for proper functioning

    //update info every UPDATE_INTERVAL
    if(millis()-update_timer_time >= UPDATE_INTERVAL){
        // init_io();
        checkTvPower();
        update_timer_time = millis();
    }
}
