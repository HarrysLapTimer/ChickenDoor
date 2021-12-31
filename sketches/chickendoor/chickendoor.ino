/**********************************************************************************
  chickendooor sketch
  (c) Harald Schlangmann

  compile for LOLIN (WEMOS) D1 R2 & mini
 **********************************************************************************/

#include <Bolbro.h>
#include <BolbroWebServer.h>

/**********************************************************************************
  common definitions
 **********************************************************************************/
 
#define DEBUG 1
#define USEOPENHAB 1 // customize

#define CHICKENDOOR "ChickenDoor1" // customize
#define CLOSE_RUNOVER_MILLIS 1000 // customize
#define OPEN_RUNOVER_MILLIS 500 // customize

/**********************************************************************************
  LED stuff 
 **********************************************************************************/

class Blinker {

#define ONOFF_MILLIS 200 // customize
  private:

    int mPin;

    bool mActive; // overall state, either off or active (blinking or permanently on)
    int mNumberOfBlinks; // substate while active, 0 = blinking permanently, >0 remaining blinks, -1 = permanently on
    long mLastChangeMillis; // substate while active, time of last switch
    
  public:

    Blinker(int pin) : mPin(pin), mActive(false) {
      pinMode(mPin, OUTPUT);
      digitalWrite(mPin, LOW);
    }

    void blink(int numberOfBlinks = 0) {
      if (mActive&&!mNumberOfBlinks&&!numberOfBlinks)
        //  already blinking, do not disturb
        return;
      mNumberOfBlinks = numberOfBlinks;
      mActive = true;
      mLastChangeMillis = millis();
      digitalWrite(mPin, HIGH);
    }

    void on() {
      blink(-1);
    }

    void stop() {
      mActive = false;
      digitalWrite(mPin, LOW);
    }

    void run() {
      if (mActive) {
        long currentMillis = millis();

        if (mNumberOfBlinks>=0) {
          if (currentMillis-mLastChangeMillis>ONOFF_MILLIS) {
            mLastChangeMillis = currentMillis;
            if (digitalRead(mPin)==HIGH)
              digitalWrite(mPin, LOW);
            else {
              if (mNumberOfBlinks!=1) { // continuous blink or not last blink 
                digitalWrite(mPin, HIGH);
                if (mNumberOfBlinks)
                  mNumberOfBlinks--;
              } else
                stop(); 
            }
          }
        }
      } else
          digitalWrite(mPin, LOW);
    }
};

#define REDLED_PIN 0 /*D3*/
#define GREENLED_PIN 4 /*D2*/

Blinker redBlinker(REDLED_PIN);
Blinker greenBlinker(GREENLED_PIN);

/**********************************************************************************
  motor stuff 
 **********************************************************************************/

#define CALIBRATION_CLOSED_PIN 5 /*D1*/
#define CALIBRATION_OPEN_PIN 13 /*D7*/

#define BUTTON_PIN 2 /*D4*/ 

#define IN1_PIN 14 /*D5*/ // direction
#define IN2_PIN 12 /*D6*/ // direction
#define IN3_PIN 15 /*D8*/ // lightning

class ChickenDoor {

  private:

    enum OpenerState {
      Inactive,
      OpeningDoor,
      OpeningRunover,
      ClosingDoor,
      ClosingRunover,
    } mState;

    long mRunoverStart;
    long mLastUpdateMillis;

    static const char *state2str(OpenerState state) {
      switch(state) {
        case Inactive:
          return "Inactive";
        case OpeningDoor:
          return "OpeningDoor";
        case OpeningRunover:
          return "OpeningRunover";
        case ClosingDoor:
          return "ClosingDoor";
        case ClosingRunover:
          return "ClosingRunover";
        default:
          return "Unknown";
      }
    }

    void changeState(OpenerState newState) {
      if (DEBUG) {
        Serial.print("changing from ");
        Serial.print(state2str(mState));
        Serial.print(" to ");
        Serial.println(state2str(newState));
      }
      mState = newState;
      switch(mState) {
        default:
          greenBlinker.stop();
          redBlinker.stop();
          break;
        case OpeningDoor:
          greenBlinker.blink();
          redBlinker.blink();
          break;
        case OpeningRunover:
          //  no change
          break;
        case Inactive:
          if (isOpened()) {
            greenBlinker.stop();
            redBlinker.on();
          } else if (isClosed()) {
            greenBlinker.on();
            redBlinker.stop();
          } else {
            redBlinker.blink();
            greenBlinker.stop();
          }
          break;
        case ClosingDoor:
          greenBlinker.blink();
          redBlinker.blink();
          break;
        case ClosingRunover:
          //  no change
          break;
      }
  
#if USEOPENHAB
      //  update openhabian
      Bolbro.updateItem("ESP32_" CHICKENDOOR, state());
#endif
}

#define OPEN true
#define CLOSE false

    void moveMotor(bool direction) {
      if (direction==OPEN) {
        if (DEBUG)
          Serial.println("opening door...");
        digitalWrite(IN1_PIN, HIGH);
        digitalWrite(IN2_PIN, LOW);
      } else {
        if (DEBUG)
          Serial.println("closing door...");
        digitalWrite(IN1_PIN, LOW);
        digitalWrite(IN2_PIN, HIGH);
      }
    }

    void stopMotor() {
      if (DEBUG)
          Serial.println("stopped motor.");
      digitalWrite(IN1_PIN, LOW);
      digitalWrite(IN2_PIN, LOW);
    }

  public:

    ChickenDoor() {
      changeState(Inactive);

      pinMode(CALIBRATION_OPEN_PIN, INPUT_PULLUP);
      pinMode(CALIBRATION_CLOSED_PIN, INPUT_PULLUP);

      pinMode(IN1_PIN, OUTPUT);
      pinMode(IN2_PIN, OUTPUT);
      pinMode(IN3_PIN, OUTPUT);

      mLastUpdateMillis = millis();

      //illuminate(false);
      stopMotor();
    }

    const char *state() {
      switch(mState) {
        case Inactive:
          if (isOpened())
            return "OPEN";
          else if (isClosed())
            return "CLOSED";
          else
            return "ERROR";
        case OpeningDoor:
        case OpeningRunover:
          return "OPENING";
        case ClosingDoor:
        case ClosingRunover:
          return "CLOSING";
        default:
          return "ERROR";
      }    
    }

    //  run an explicit calibration - usually when the system is booted
    void calibrate() {
      if (mState==Inactive) {
        if (!isClosed()&&!isOpened()) // keep existing state if defined
          openDoor();
      }
    }

    //  open the door
    void openDoor() {
      if (!isOpened()) {
        moveMotor(OPEN);
        changeState(OpeningDoor);
      }
    }

    //  open the door
    void closeDoor() {
      if (!isClosed()) {
        moveMotor(CLOSE);
        changeState(ClosingDoor);
      }
    }

    void toggle() {
      switch(mState) {
        case Inactive:
          if (isOpened())
            closeDoor();
          else if (isClosed())
            openDoor();
          break;       
        case OpeningDoor:
        case OpeningRunover:
          closeDoor();
          break;
        case ClosingDoor:
        case ClosingRunover:
          openDoor();
          break;
      }
    }

    bool isClosed() {
      return !digitalRead(CALIBRATION_CLOSED_PIN);
    }

    bool isOpened() {
      return !digitalRead(CALIBRATION_OPEN_PIN);
    }

    void illuminate(bool newIllumination) {
      digitalWrite(IN3_PIN, newIllumination?HIGH:LOW);

#if USEOPENHAB
      Bolbro.updateItem("ESP32_" CHICKENDOOR "_Illumination", newIllumination?"ON":"OFF");
#endif
    }

    bool isIlluminated() {
      return digitalRead(IN3_PIN)!=LOW;  
    }

    void run() {

      switch (mState) {
        case OpeningDoor:
          if (isOpened()) {
            //  open position reached, append run over
            mRunoverStart = millis();
            changeState(OpeningRunover);
          }
          break;
        case OpeningRunover:
          if (millis()-mRunoverStart>OPEN_RUNOVER_MILLIS) {
            stopMotor();
            changeState(Inactive);
          }
          break;
        case ClosingDoor:
          if (isClosed()) {
            //  close position reached, append run over
            mRunoverStart = millis();
            changeState(ClosingRunover);
          }
          break;
        case ClosingRunover:
          if (millis()-mRunoverStart>CLOSE_RUNOVER_MILLIS) {
            stopMotor();
            changeState(Inactive);
          }
          break;
      }

#if USEOPENHAB
      //  regular update for openhabian
      long currentMillis = millis();

      if (currentMillis-mLastUpdateMillis>5*60*1000ul) {
        Bolbro.updateItem("ESP32_" CHICKENDOOR, state());
        mLastUpdateMillis = currentMillis;
      }
#endif
    }
};

ChickenDoor chickenDoor;

/**********************************************************************************
  webserver stuff 
 **********************************************************************************/

class ChickenDoorWebServer:public BolbroWebServer
{
  public:

    ChickenDoorWebServer() : BolbroWebServer () {
    }

    void begin() {

      //  dynamic stuff
      on("/status", [this]() { handleStatus(); });
      on("/opendoor", [this]() { handleOpenDoor(); });
      on("/closedoor", [this]() { handleCloseDoor(); });
      on("/statusillumination", [this]() { handleStatusIllumination(); });
      on("/illuminate", [this]() { handleIlluminate(true); });
      on("/illuminationoff", [this]() { handleIlluminate(false); });
    
      BolbroWebServer::begin();    
    }
    
  private:
  
    void handleStatus() {    
      send(200, "text/plain", chickenDoor.state());
      LOG->println("/status generated and sent");
    }

    void handleOpenDoor() {
      chickenDoor.openDoor();
      send(200, "text/plain", "OK");
      LOG->println("/opendoor handled");
    }

    void handleCloseDoor() {
      chickenDoor.closeDoor();
      send(200, "text/plain", "OK");
      LOG->println("/closedoor handled");
    }

    void handleStatusIllumination() {    
      send(200, "text/plain", chickenDoor.isIlluminated()?"ON":"OFF");
      LOG->println("/statusillumination generated and sent");
    }

    void handleIlluminate(bool illumination) {
      chickenDoor.illuminate(illumination);
      send(200, "text/plain", "OK");
      LOG->println(illumination?"/illuminate handled":"/illuminationoff handled");
    }
};

ChickenDoorWebServer server;

void setup()
{
  Serial.begin(115200);
  delay(10);
  Serial.printf("Started...\n");

  //  configure Bolbro
  Bolbro.setSerialBaud(115200l);
  Bolbro.addWiFi("HarrysHomeNet", "m8921um8921u"); // customize
  Bolbro.addWiFi("HarrysGardenNet", "m8921um8921u"); // customize

#if USEOPENHAB
  Bolbro.setOpenHABHost("openhabian"); // customize or remove
  Bolbro.setSignalStrengthItem("ESP32_" CHICKENDOOR "_SignalStrength"); // customize or remove
#endif

  Bolbro.setup(CHICKENDOOR, DEBUG);
  
  Bolbro.connectToWiFi();

  Bolbro.configureTime();

  Serial.printf("ChickenDoor setup...\n");

  //  configure manual action
  pinMode(BUTTON_PIN, INPUT_PULLUP);

  //  run an initial calibration
  chickenDoor.calibrate();

  //  setup web server
  server.begin();
  Serial.println("HTTP server started");
}

void loop() {

  //  Handle requests to server
  server.handleClient();
  delay(10); // work around for slow web server response?
  
  //  allow door opening by pressing a local button
  static int lastButtonState = -1;

  int currentButtonState = digitalRead(BUTTON_PIN);

  if (currentButtonState!=lastButtonState) {
    //  change
    lastButtonState = currentButtonState;

    if (currentButtonState==LOW) {
      if (DEBUG) {
        Serial.println("button pressed, trying open...");
      }
      chickenDoor.toggle();
    }
  }

  chickenDoor.run();
  redBlinker.run();
  greenBlinker.run();
  
  Bolbro.loop();
}
