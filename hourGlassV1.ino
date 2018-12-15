Servo myservo;
bool moving=false;
// bool newState = true;

const int magPin = A0;
const int motorPin = D3;

// int pos = 0;    // variable to store the servo position
int noMagVal = 2898;

int waitMS= 1;
int stateBuffNum = 500;

int state = 0;
int globalState=3;
bool matchState=false;
int stateCount=0;
int lastState=0;

unsigned long lastTime = 0;

const int numReadings = 2;

int readings[numReadings];      // the readings from the analog input
int readIndex = 0;              // the index of the current reading
int total = 0;                  // the running total
int average = 0;  // the average

unsigned long startTime=0;

void stateHandler(const char *event, const char *data)
{
  if (data){
    Serial.println(atoi(data));
    globalState=atoi(data);
    matchState=true;
    }
  else {Serial.println("NULL");}
}

void setup()
{
    myservo.attach(motorPin);
    pinMode(D7, OUTPUT);  // set D7 as an output so we can flash the onboard LED
    delay(1000);
    pinMode(magPin, INPUT);

    for (int thisReading = 0; thisReading < numReadings; thisReading++) {readings[thisReading] = 0;}
    Particle.subscribe("globalState", stateHandler, MY_DEVICES);
    Serial.begin(9600);
}

void loop()
{
    total = total - readings[readIndex];
    readings[readIndex] = analogRead(magPin)-noMagVal;
    total = total + readings[readIndex];
    readIndex = readIndex + 1;
    if (readIndex >= numReadings) {readIndex = 0;}

    average = total / numReadings;

    int state = getState(average, lastState); //get state from value

    unsigned long now = millis(); //print things
  if ((now - lastTime) >= 1000) {
    lastTime = now;
        // Serial.println(String(state) + ", "+ String(average) + ", " + String(stateCount)+", "+String(moving));
        Serial.println(String(globalState)+", "+ String(state)+", "+ String(matchState));
  }
  if (matchState){
    if (state == globalState){
      matchState=false;
      myservo.write(90);
      myservo.detach();
    }
    else{
      if(!myservo.attached()){myservo.attach(motorPin);}
      myservo.write(95);
    }
  }

  else{
    if (state==lastState){      if(stateCount<5000){stateCount+=1;} } //increment stateCount
    else {stateCount=0;}



    if (moving){
        if (state ==2 || state == -2){
            delay(100);
            myservo.write(90);
            myservo.detach();
            moving = false;

            globalState=state;
            Particle.publish("globalState", String(state), 100, PRIVATE);
        }
        else if (state == 0){
            if (stateCount > stateBuffNum){
                myservo.write(90);
                myservo.detach();
                moving=false;
            }
        }
        if (state==1 || state == -1){
            if ( getSpeed(state) != myservo.read() && stateCount > 300){
                myservo.write(getSpeed(state));
                delay(10);
            }

        }
    }
    else{//not moving
        if (state == 0){
            // moving=true;
            int pass =0;
            // newState=true;
        }
        else if (state==-1 || state == 1){//South or north
            if(true){
              if (stateCount > 500){
                  myservo.attach(motorPin);
                  myservo.write(getSpeed(state));
                  moving=true;
                  startTime=millis();
                  }
                }
          }
          else if (state == -1*globalState){
            globalState=state;
            Particle.publish("globalState", String(state), 100, PRIVATE);
          }
    }
    lastState=state;
  }
    delay(waitMS);

}

int getState(int value, int pState){
    int onBaseTH = 100;
    int negPoleTH = -380;
    int posPoleTH = 330;

    float thresh = 0.9;

    int sign=0;
    if (value > 0){sign=1;}else if(value<0){sign=-1;}

    if (abs(value) > onBaseTH) {
      if (value > posPoleTH || value <  negPoleTH){return sign*2;}
      else if( value > posPoleTH*thresh || value < negPoleTH*thresh){ return pState;}
      else{return sign*1;}
    }
    else{ return 0;}
}

int getSpeed(int state){
    if (state==1){return 84;}
    else if (state == -1){return 95;}
    else{ return 90;}
}
