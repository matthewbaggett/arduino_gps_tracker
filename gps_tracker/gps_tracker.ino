#include <SD.h>
#include <MicroNMEA.h>
#include <SoftwareSerial.h>
HardwareSerial& console = Serial;
SoftwareSerial gps(2, 3); // configure software serial port -

char nmeaBuffer[100];
MicroNMEA nmea(nmeaBuffer, sizeof(nmeaBuffer));
bool ledState = LOW;
volatile bool ppsTriggered = false;

void ppsHandler(void);


void ppsHandler(void)
{
  ppsTriggered = true;
}

void printUnknownSentence(MicroNMEA& nmea)
{
  console.println();
  console.print("Unknown sentence: ");
  console.println(nmea.getSentence());
}

void gpsHardwareReset()
{
  // Empty input buffer
  while (gps.available()) {
    gps.read();
  }

  digitalWrite(A0, LOW);
  delay(50);
  digitalWrite(A0, HIGH);

  // Reset is complete when the first valid message is received
  while (1) {
    while (gps.available()) {
      char c = gps.read();
      if (nmea.process(c)) {
        return;
      }
    }
  }
}

void setup()
{
  gps.begin(9600);
  console.begin(115200);

  if (!SD.begin(10)) {
    Serial.println("Card failed, or not present");
    // stop the sketch
    return;
  }
  Serial.println("SD card is ready");

  nmea.setUnknownSentenceHandler(printUnknownSentence);

  pinMode(A0, OUTPUT);
  digitalWrite(A0, HIGH);
  console.println(F("Resetting GPS module ..."));
  gpsHardwareReset();
  console.println(F("... done"));

  // Clear the list of messages which are sent.
  MicroNMEA::sendSentence(gps, "$PORZB");

  // Send only RMC and GGA messages.
  MicroNMEA::sendSentence(gps, "$PORZB,RMC,1,GGA,1");

  // Disable compatability mode (NV08C-CSM proprietary message) and
  // adjust precision of time and position fields
  MicroNMEA::sendSentence(gps, "$PNVGNME,2,9,1");
  // MicroNMEA::sendSentence(gps, "$PONME,2,4,1,0");

  pinMode(6, INPUT);
  attachInterrupt(digitalPinToInterrupt(2), ppsHandler, RISING);
}

void loop(void)
{
  if (ppsTriggered) {
    ppsTriggered = false;
    ledState = !ledState;
    digitalWrite(LED_BUILTIN, ledState);

    console.println(F("\n\n\n\n\n"));

    long latitude_mdeg = nmea.getLatitude();
    long longitude_mdeg = nmea.getLongitude();
    long alt;

    File locationLog = SD.open("LOCATION.XML", FILE_WRITE);

    if(locationLog){
      locationLog.print(F("<location "));
      locationLog.print(F("latitude=\"")); locationLog.print(latitude_mdeg / 10000000., 6); locationLog.print(F("\" "));
      locationLog.print(F("longitude=\"")); locationLog.print(longitude_mdeg / 10000000., 6); locationLog.print(F("\" "));
      if (nmea.getAltitude(alt)){
        locationLog.print(F("altitude=\"")); locationLog.println(alt / 1000., 3); locationLog.print(F("\" "));
      }
      locationLog.print(F("speed=\"")); locationLog.println(nmea.getSpeed() / 1000., 3); locationLog.print(F("\" "));
      locationLog.print(F("course=\"")); locationLog.println(nmea.getCourse() / 1000., 3); locationLog.print(F("\" "));
      locationLog.println(F("/>"));
      locationLog.close(); // this is mandatory   

      console.println(F("Written Location to SD card."));
    }else{
      console.println(F("SD Card not available, or invalid format."));
    }

    // Output GPS information from previous second
    console.print(F("Valid fix: "));
    console.println(nmea.isValid() ? F("yes") : F("no"));

    console.print(F("Nav. system: "));
    if (nmea.getNavSystem()) {
      console.println(nmea.getNavSystem());
    } else {
      console.println(F("none"));
    }

    console.print(F("Num. satellites: "));
    console.println(nmea.getNumSatellites());

    console.print(F("HDOP: "));
    console.println(nmea.getHDOP() / 10., 1);

    console.print(F("Date/time: "));
    console.print(nmea.getYear());
    console.print('-');
    console.print(int(nmea.getMonth()));
    console.print('-');
    console.print(int(nmea.getDay()));
    console.print('T');
    console.print(int(nmea.getHour()));
    console.print(':');
    console.print(int(nmea.getMinute()));
    console.print(':');
    console.println(int(nmea.getSecond()));

    console.print(F("Latitude (deg): "));
    console.println(latitude_mdeg / 1000000., 6);

    console.print(F("Longitude (deg): "));
    console.println(longitude_mdeg / 1000000., 6);

    console.print(F("Altitude (m): "));
    if (nmea.getAltitude(alt)){
      console.println(alt / 1000., 3);
    }else{
      console.println(F("not available"));
    }

    console.print(F("Speed: "));
    console.println(nmea.getSpeed() / 1000., 3);
    console.print(F("Course: "));
    console.println(nmea.getCourse() / 1000., 3);


    nmea.clear();
  }

  while (!ppsTriggered && gps.available()) {
    char c = gps.read();
    //console.print(c);
    nmea.process(c);
  }

}

