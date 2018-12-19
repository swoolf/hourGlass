Servo myservo;

const int magPin = A0;
const int motorPin = D3;

int noMagVal=0; //zero Magnetic Field Value
int negPoleTH = 0;
int posPoleTH = 0;

int stateBuffNum = 500; //wait for this state count before accepting state
int stateCount=0;//how many times has a state been seen

bool moving=false; //is hourGlass moving

int state = 0;
int globalState=3;
bool matchState=false;
int lastState=0;

unsigned long lastTime = 0;
unsigned long lastGlobalState = millis();
const int numReadings = 2;

int readings[numReadings];      // the readings from the analog input
int readIndex = 0;              // the index of the current reading
int total = 0;                  // the running total
int average = 0;  // the average
int states2global = 0;

int maxV=0;
int minV=0;

int unitNo=1;

void setup()
{
    myservo.attach(motorPin);
    pinMode(magPin, INPUT);
    for (int thisReading = 0; thisReading < numReadings; thisReading++) {readings[thisReading] = 0;}

    myservo.write(100);
    unsigned long start =millis();
    int maxV=0;
    int minV =10000;
    int val;

    while(millis()-start<20000){
      int val = analogRead(magPin);
      if (val<minV){minV=val;}
      if(val>maxV){maxV=val;}
      delay(1);
    }
    myservo.detach();

    noMagVal = (maxV+minV)*0.5;
    negPoleTH = 0.95*(minV - noMagVal);
    posPoleTH = 0.95*(maxV - noMagVal);
    Particle.publish("maxmin",String(posPoleTH)+", "+String(negPoleTH)+", "+String(noMagVal), 100, PRIVATE);
    Serial.println(String(maxV)+", "+String(minV));
    Serial.println(String(posPoleTH)+", "+String(negPoleTH));


    Particle.subscribe("globalState", stateHandler, MY_DEVICES);
    Serial.begin(9600);

    String myID = System.deviceID();
    if (myID == "270039000c47363339343638"){
      unitNo=2;
      // noMagVal = 2028;
      // negPoleTH = -250;
      // posPoleTH = 250;
    }
    else{
      unitNo=1;
      // noMagVal = 2028;
      // negPoleTH = -265;
      // posPoleTH = 265;
    }

    delay(1000);
    Serial.println(unitNo);
    Serial.println("begin");
}


void loop()
{
    average = getAverage();
    state = getState(average, lastState); //get state from value

///////////////////////////////////////Print Things
    if (state != lastState && abs(state)==2 && state !=globalState && (millis()-lastGlobalState>1000 && !matchState)){
      lastGlobalState=millis();
      Serial.println("New "+String(state));
      Particle.publish("globalState"+String(unitNo), String(state), 100, PRIVATE);
    }

  unsigned long now = millis(); //print things
  if ((now - lastTime) >= 1000) {
    lastTime = now;
        // Serial.println(String(state) + ", "+ String(average) + ", " + String(stateCount)+", "+String(moving));
        Serial.println(String(globalState)+", "+ String(state)+", "+ String(average));
  }
  //////////////////////////////////////////

  if (matchState){gotoGlobalState();}

  else{//notMatchState
    if (moving){
        if (abs(state) ==2){
            delay(100);
            stopServo();
            }
        else if (state == 0 && stateCount>stateBuffNum){
                stopServo();
                }
        else if (abs(state)==1){
            if ( getSpeed(state) != myservo.read() && stateCount > 300){
                myservo.write(getSpeed(state));
                delay(10);
                }
            }
        }
    else{//not moving
        if (abs(state)==1 && stateCount >500){
                startServo();
            }
        }
    }
    lastState=state;
    delay(1);

}//end of loop

int getAverage(){
  total = total - readings[readIndex];
  readings[readIndex] = analogRead(magPin)-noMagVal;
  total = total + readings[readIndex];
  readIndex = readIndex + 1;
  if (readIndex >= numReadings) {readIndex = 0;}
  return total / numReadings;

}

int gotoGlobalState(){
  int states2see[] = {1,-1,1};

  if (state==states2see[states2global]){
    states2global +=1;
    if (states2global==3){
      states2global=0;
      matchState=false;
    }
  }
  if (state == globalState){
    matchState=false;
    myservo.write(90);
    myservo.detach();
  }else if (state==0 && stateCount>1000){
    myservo.write(90);
    myservo.detach();
  }
  else{
    if(!myservo.attached()){myservo.attach(motorPin);}
    myservo.write(95);
  }
}

int getState(int value, int pState){
  // int negPoleTH = -380;
  // int posPoleTH = 330;
  //
  // if (unitNo==2){
  //   negPoleTH = -500;
  //   posPoleTH = 380;
  // }

    int onBaseTH = 60;
    float thresh = 0.8;

    int sign=0;
    int newState;

    if (value > 0){sign=1;}else if(value<0){sign=-1;}

    if (abs(value) > onBaseTH) {
      if (value > posPoleTH || value <  negPoleTH){newState= sign*2;}
      else if( value > posPoleTH*thresh || value < negPoleTH*thresh){ newState= pState;}
      else{newState=sign*1;}
    }
    else{ newState= 0;}
    if (newState==pState){if(stateCount<5000){stateCount+=1;} } //increment stateCount
    else {stateCount=0;}

    return newState;
}

int getSpeed(int state){
    if (state==1){return 83;}
    else if (state == -1){return 96;}
    else{ return 90;}
}

void startServo(){
  myservo.attach(motorPin);
  myservo.write(getSpeed(state));
  moving=true;
}

void stopServo(){
  myservo.write(90);
  myservo.detach();
  moving=false;
}

void stateHandler(const char *event, const char *data)
{
  if (data){
    if (String(event)!="globalState"+String(unitNo) ){
      Serial.println("here");
      Serial.println(String(globalState)+", "+String(data)+", "+String(state));
      globalState=atoi(data);
      if (globalState!=state){ matchState=true;}
      }
    }
  else {Serial.println("NULL");}
}
