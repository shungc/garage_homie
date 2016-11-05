#include <Homie.h>

/*
 ESP8266 MQTT control of Garage Door Opener
 Shung Chieh 6/9/2016
 
 This attempts to connect to the mqtt broker without blocking the main loop.

*/

#include <Homie.h>

// Update these with values suitable for your controller

#define DOORG_DIS 13
#define DOORG_TRIG 12
#define DOORF_DIS 5
#define DOORF_TRIG 4
#define DOORG_SENS 2
#define DOORF_SENS 0
#define DOOR_LIMIT 8E6
#define TIME_LIMIT 1E7

boolean doorf;
boolean doorg;
boolean doorf_dis;
boolean doorg_dis;
boolean prev_doorf;
boolean prev_doorg;

long  watchdog_timer;

long lastMsg = 0;
char msg[50];
int value = 0;

bool globalInputHandler(String node, String property, String value);

HomieNode garageFNode("doorf","switch");
HomieNode garageGNode("doorg","switch");

void setupHandler() {
  Homie.setNodeProperty(garageFNode,"trigger",false);
  Homie.setNodeProperty(garageFNode,"disable",false);
  doorf_dis = 0;
  Homie.setNodeProperty(garageGNode,"trigger",false);
  Homie.setNodeProperty(garageGNode,"disable",false);
  doorg_dis = 0; 

  doorf = digitalRead(DOORF_SENS);
  doorg = digitalRead(DOORG_SENS);

  if (doorf) {
    Homie.setNodeProperty(garageFNode,"state","OPEN");
    Serial.println("doorf OPEN");
    } else {
    Homie.setNodeProperty(garageFNode,"state","CLOSED");
    Serial.println("doorf CLOSED");
  }
  if (doorg) {
    Homie.setNodeProperty(garageGNode,"state","OPEN");
    Serial.println("doorg OPEN");
    } else {
    Homie.setNodeProperty(garageGNode,"state","CLOSED");
    Serial.println("doorg CLOSED");
  }
}

void setup() {
  Serial.begin(115200);
  delay(1000);
  Serial.println("Setting up...");
    
  pinMode(BUILTIN_LED, OUTPUT);     // Initialize the BUILTIN_LED pin as an output

// set up door switch inputs
  pinMode(DOORG_SENS, INPUT_PULLUP); // Initialize door switches (originally with INPUT_PULLUP, but not any more) 1=OPEN
  pinMode(DOORF_SENS, INPUT_PULLUP);

// set up DOOR DIS AND TRIG
  pinMode(DOORG_DIS, OUTPUT);
  pinMode(DOORG_TRIG, OUTPUT);
  pinMode(DOORF_DIS, OUTPUT);
  pinMode(DOORF_TRIG, OUTPUT);

// initialize with all relays off
  digitalWrite(DOORG_DIS, LOW);
  digitalWrite(DOORG_TRIG, LOW);
  digitalWrite(DOORF_DIS, LOW);
  digitalWrite(DOORF_TRIG, LOW);

  prev_doorf = digitalRead(DOORF_SENS);
  prev_doorg = digitalRead(DOORG_SENS);
  doorf = prev_doorf;
  doorg = prev_doorg;
  
  watchdog_timer = millis();
  Homie.setGlobalInputHandler(globalInputHandler);
  Homie.setFirmware("ESP8266_GARAGE","1.0.0");
  Homie.registerNode(garageFNode);
  Homie.registerNode(garageGNode);
//  Homie.setSetupFunction(setupHandler);
  Homie.setLoopFunction(loopHandler);

// Disable reset trigger
  Homie.disableResetTrigger();

  garageFNode.subscribe("trigger");
  garageGNode.subscribe("trigger");
  garageFNode.subscribe("disable");
  garageGNode.subscribe("disable");
  
  Homie.setup();

}

bool globalInputHandler(String node, String property, String value) {
  int door;
  Serial.print("Message arrived.  Node:");
  Serial.println(node);
  Serial.print("Property: ");
  Serial.println(property);
  Serial.print("Value: ");
  Serial.println(value);

// doorf or doorg
  switch (node[4]) {
    case 'f':
      door=DOORF_TRIG;
      Serial.print("Door F");
      break;
    case 'g':
      door=DOORG_TRIG;
      Serial.print("Door G");
      break;
    default:
      door=0;
      break;
  }

  doorf = digitalRead(DOORF_SENS);
  doorg = digitalRead(DOORG_SENS);

  switch (property[0]) {
     case 't':
        if (value == "true") {
          Serial.println("toggle door");
          toggleDoor(door);
          Homie.setNodeProperty(garageFNode,"trigger","false");
          Homie.setNodeProperty(garageGNode,"trigger","false");
        }
        break;
     case 'd':
        if (value ==  "true") {
          switch (door) {
            case DOORF_TRIG:
              disableDoor(DOORF_DIS, &(doorf_dis));
              break;
            case DOORG_TRIG:
              disableDoor(DOORG_DIS, &(doorg_dis));
              break;
          }
        } else {
          switch (door) {
            case DOORF_TRIG:
              enableDoor(DOORF_DIS, &(doorf_dis));
              break;
            case DOORG_TRIG:
              enableDoor(DOORG_DIS, &(doorg_dis));
              break;
          }
        }
        break;
      default:
         doorf = digitalRead(DOORF_SENS);
         doorg = digitalRead(DOORG_SENS);
      break;
  }
  return true;
}


void disableotherDoor() {
  delay(100);
//  doorf = digitalRead(DOORF_SENS);
//  doorg = digitalRead(DOORG_SENS);
  if (!doorf_dis) {
  digitalWrite(DOORF_DIS, doorg);
  }
  if (!doorg_dis) {
  digitalWrite(DOORG_DIS, doorf);
  }
}

void disableDoor(int doorid, boolean *disable_id) {
      digitalWrite(doorid, HIGH);
      delay(500);
      *disable_id = 1;
}

void enableDoor(int doorid, boolean *disable_id) {
      digitalWrite(doorid, LOW);
      delay(500);
      *disable_id = 0;
}

void toggleDoor(int doorid)
{                     
    switch (doorid)
    {
        case DOORG_TRIG:
           digitalWrite(DOORF_DIS, HIGH);
           delay(500);
           digitalWrite(DOORG_TRIG, HIGH);
           delay(500);
           digitalWrite(DOORG_TRIG, LOW);
           Serial.println("Toggle G");
           delay(500);
        break;
        case DOORF_TRIG:
           digitalWrite(DOORG_DIS, HIGH);
           delay(500);
           digitalWrite(DOORF_TRIG, HIGH);
           delay(500);
           digitalWrite(DOORF_TRIG, LOW);
           Serial.println("Toggle F");
           delay(500);
        break;
        default:
        break;
    }
}

void rstCtrllr() {
  if (doorf) { toggleDoor(DOORF_TRIG); }
  delay(500);
  if (doorg) { toggleDoor(DOORG_TRIG); }
  delay(500);
  doorf_dis = 0;
  doorg_dis = 0;
  enableDoor(DOORF_DIS, &(doorf_dis));
  enableDoor(DOORG_DIS, &(doorg_dis));
}

void loopHandler() {
  doorf = digitalRead(DOORF_SENS);
  doorg = digitalRead(DOORG_SENS);
  
  if (prev_doorf != doorf) {
     Serial.print("door F changed\n");
     if (doorf) {
      Homie.setNodeProperty(garageFNode,"state","OPEN");
      Serial.println("doorf OPEN");
     } else {
      Homie.setNodeProperty(garageFNode,"state","CLOSED");
      Serial.println("doorf CLOSED");
     }
  }
  if (prev_doorg != doorg) {
     Serial.print("door G changed\n");
     if (doorg) {
      Homie.setNodeProperty(garageGNode,"state","OPEN");
       Serial.println("doorG OPEN");
     } else {
      Homie.setNodeProperty(garageGNode,"state","CLOSED");
      Serial.println("doorG CLOSED");
     }
  }
  disableotherDoor();
  
  long now = millis();
  if (now - lastMsg > 30000) {
    lastMsg = millis();
    if (doorf) {
      Homie.setNodeProperty(garageFNode,"state","OPEN");
    } else {
      Homie.setNodeProperty(garageFNode,"state","CLOSED");
    }
    if (doorg) {
      Homie.setNodeProperty(garageGNode,"state","OPEN");
    } else {
      Homie.setNodeProperty(garageGNode,"state","CLOSED");
    }
  }
  prev_doorf=doorf;
  prev_doorg=doorg;
  now = millis();
  if ( now - watchdog_timer > TIME_LIMIT || now < watchdog_timer) {
       rstCtrllr();
       watchdog_timer= millis();
  }
} 


void loop() {
  Homie.loop();
}

