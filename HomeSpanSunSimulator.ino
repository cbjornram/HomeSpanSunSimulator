
#define NEOPIXEL_RGB_PIN  27                    // Set pin for LED strip.
#define NEOPIXEL_COUNT    300                   // Set number of LEDs in the LED strip.
#define TIME_ZONE         "UTC-1:00"            // Set time zone according to the Proleptic Format for TZ. See https://sourceware.org/glibc/manual/latest/html_node/Proleptic-TZ.html for examples.
#define NTP_SERVER        "pool.ntp.org"        // Optional: Set desired NTP server.
 
#include "HomeSpan.h"
#include <WebServer.h>

///////////////////////////////

struct NeoPixel_RGB : Service::LightBulb {      // Addressable single-wire RGB LED Strand (e.g. NeoPixel)
 
  Characteristic::On power{0,true};
  Characteristic::Hue H{0,true};
  Characteristic::Saturation S{0,true};
  Characteristic::Brightness V{100,true};
  Pixel *pixel;
  int nPixels;
  
  NeoPixel_RGB(uint8_t pin, int nPixels) : Service::LightBulb(){
    V.setRange(5,100,1);                      // sets the range of the Brightness to be from a min of 5%, to a max of 100%, in steps of 1%
    pixel=new Pixel(pin);                     // creates Pixel RGB LED on specified pin
    this->nPixels=nPixels;                    // save number of Pixels in this LED Strand
    update();                                 // manually call update() to set pixel with restored initial values
  }

  boolean update() override {

    int p=power.getNewVal();
    
    float h=H.getNewVal<float>();       // range = [0,360]
    float s=S.getNewVal<float>();       // range = [0,100]
    float v=V.getNewVal<float>();       // range = [0,100]

    Pixel::Color color;

    pixel->set(color.HSV(h*p, s*p, v*p),nPixels);       // sets all nPixels to the same HSV color
          
    return(true);
  }
};

struct DEV_LED : Service::LightBulb {

  SpanCharacteristic *power;
  int alarmHour;
  int alarmMinute;
  int lastUpdatedSimulationTime;
  
  DEV_LED(int alarmHour, int alarmMinute) : Service::LightBulb(){
    power=new Characteristic::On();
    this->alarmHour=alarmHour;
    this->alarmMinute=alarmMinute;
    this->lastUpdatedSimulationTime=0;
  }

  boolean update(){
    if (this->power->getVal() == false && this->lastUpdatedSimulationTime != 0){
      this->lastUpdatedSimulationTime = 0;
    }            
    return(true);  
  }
};

///////////////////////////////

NeoPixel_RGB *leds;
DEV_LED *sunSimulation;
WebServer webServer(80);

void setup() {
  
  Serial.begin(115200);

  homeSpan.setHostNameSuffix("");
  homeSpan.setPortNum(1201);
  homeSpan.setConnectionCallback(setupWeb);
  homeSpan.enableWebLog(0,NTP_SERVER,TIME_ZONE,NULL);  
  homeSpan.begin(Category::Lighting,"Pixel Sun");

  new SpanAccessory();
    new Service::AccessoryInformation();
      new Characteristic::Name("Pixel LEDS");
      new Characteristic::Identify();
    leds=new NeoPixel_RGB(NEOPIXEL_RGB_PIN,NEOPIXEL_COUNT);

  new SpanAccessory();
    new Service::AccessoryInformation();
      new Characteristic::Name("Sun Simulation");
      new Characteristic::Identify();
    sunSimulation=new DEV_LED(7, 0);

  new SpanUserCommand('T',"- show current time",printTime);
}

///////////////////////////////

void loop() {
  homeSpan.poll();
  webServer.handleClient();
  runSunSimulation();
}

void printTime(const char *buf) {
  struct tm myTime;        // create a tm structure
  getLocalTime(&myTime);   // populate the tm structure with current date and time
  Serial.printf("Current Time = %02d:%02d:%02d\n", myTime.tm_hour, myTime.tm_min, myTime.tm_sec);
}

///////////////////////////////

void setupWeb(int count){

  if(count>1)
    return;
  
  Serial.printf("Starting Web Server");
  webServer.begin();

  webServer.on("/", []() {
    
    String response = "<html><head><title>Sun Simulation</title>";
    response += "<body></body></html>";
    webServer.send(200, "text/html", response);

  });

  // Set alarm time. Expected format is "[IP Address]/set?time=07:30"
  webServer.on("/set", []() {

    String time = webServer.arg(0).c_str();
    String hour, minute;
    hour = time.substring(0, 2);
    minute = time.substring(3, 5);
    sunSimulation->alarmHour = hour.toInt();
    sunSimulation->alarmMinute = minute.toInt();

    String response = "<html>";
    response += "Sun simulation is set to ";

    sunSimulation->power->setVal(true);
    
    response += String(sunSimulation->alarmHour);
    response += ":";
    response += String(sunSimulation->alarmMinute);
    response += ".</html>";

    webServer.send(200, "text/html", response);

  });

  // Request current simulation state
  webServer.on("/state", []() {

    int index=atoi(webServer.arg(0).c_str());

    String response = "<html><head><title>Sun Simulation</title><meta http-equiv='refresh' content = '3; url=/'/></head>";
    response += "<body>Sun simulation is: ";
    if (sunSimulation->power->getVal()) {
      response += "enabled";
    } else {
      response += "disabled";
    }
    response += ".<br>Alarm set to: ";
    response += String(sunSimulation->alarmHour);
    response += ":";
    response += String(sunSimulation->alarmMinute);
    response += ".<br>Time is: ";
    struct tm currentTime;
    getLocalTime(&currentTime);
    response += String(currentTime.tm_hour);
    response += ":";
    response += String(currentTime.tm_min);
    response += ".</body></html>";

    webServer.send(200, "text/html", response);

  });
}

void runSunSimulation() {

  // If sun simulation is enabled
  if(sunSimulation->power->getVal()){

    // Save current time and alarm time as minute ints
    struct tm currentTime;
    getLocalTime(&currentTime);

    int alarmInMinutes = sunSimulation->alarmHour*60 + sunSimulation->alarmMinute;
    int currentTimeInMinutes = currentTime.tm_hour*60 + currentTime.tm_min;

    // If within simulation window, run simulation
    if(alarmInMinutes - currentTimeInMinutes <= 30 && currentTimeInMinutes > sunSimulation->lastUpdatedSimulationTime && alarmInMinutes >= currentTimeInMinutes){
      if(leds->power.getVal() == false){
        leds->H.setVal(30);
        leds->power.setVal(true);
      }

      // Set brightness. 30 (+5) = full sun, 5 = minimum allowed brightness.
      leds->V.setVal(30-(alarmInMinutes-currentTimeInMinutes)+5);
      // Set saturation. 80 = goal saturation, 90 = start saturation.
      leds->S.setVal(80+(alarmInMinutes-currentTimeInMinutes)/3);

      leds->update();
      sunSimulation->lastUpdatedSimulationTime = currentTimeInMinutes;
    }

    if(alarmInMinutes == currentTimeInMinutes){
      sunSimulation->power->setVal(false);
      sunSimulation->update();
      sunSimulation->lastUpdatedSimulationTime = 0;
    }
  }
}