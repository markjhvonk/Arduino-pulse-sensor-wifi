#include <SoftwareSerial.h>

/*  Pulse Sensor Amped 1.5    by Joel Murphy and Yury Gitman   http://www.pulsesensor.com

----------------------  Notes ----------------------  ----------------------
This code:
1) Blinks an LED to User's Live Heartbeat   PIN 13
2) Fades an LED to User's Live HeartBeat    PIN 5
3) Determines BPM
4) Prints All of the Above to Serial

Read Me:
https://github.com/WorldFamousElectronics/PulseSensor_Amped_Arduino/blob/master/README.md
 ----------------------       ----------------------  ----------------------
*/

#define PROCESSING_VISUALIZER 1
#define SERIAL_PLOTTER  2

//  Variables
int pulsePin = 0;                 // Pulse Sensor purple wire connected to analog pin 0
int blinkPin = 13;                // pin to blink led at each beat
int fadePin = 5;                  // pin to do fancy classy fading blink at each beat
int fadeRate = 0;                 // used to fade LED on with PWM on fadePin


//Wifi module variables
String ssid ="WLAN_Vonk";           // Network name
String password="PASSWORD";       // Network password
SoftwareSerial esp(6, 7);// RX, TX  // Serial pins
String data;                        // The post request
String server = "www.markvonk.com"; // Web server domain name
String uri = "/sleep/receiver.php"; // File to link to (uri)
String sensorData;                  // Fetch BPM later
String deviceUser = "1";                  // User linked to device
String sessionId = "1";
boolean connectionState = false;    // State of connection
volatile int dbCounter = 0;   // timer that counts
volatile int dbTime = 60000;  // time before data is uploaded to db
boolean dataSent = true;
// 30000 = 1 minute if rate is 2ms
//int postTime = 10;                  // After how many readings to post data
//int postTimer = 0;


// Volatile Variables, used in the interrupt service routine!
volatile int BPM;                   // int that holds raw Analog in 0. updated every 2mS
volatile int Signal;                // holds the incoming raw data
volatile int IBI = 600;             // int that holds the time interval between beats! Must be seeded!
volatile boolean Pulse = false;     // "True" when User's live heartbeat is detected. "False" when not a "live beat".
volatile boolean QS = false;        // becomes true when Arduoino finds a beat.

// SET THE SERIAL OUTPUT TYPE TO YOUR NEEDS
// PROCESSING_VISUALIZER works with Pulse Sensor Processing Visualizer
//      https://github.com/WorldFamousElectronics/PulseSensor_Amped_Processing_Visualizer
// SERIAL_PLOTTER outputs sensor data for viewing with the Arduino Serial Plotter
//      run the Serial Plotter at 115200 baud: Tools/Serial Plotter or Command+L
static int outputType = SERIAL_PLOTTER;


void setup(){
  pinMode(blinkPin,OUTPUT);         // pin that will blink to your heartbeat!
  pinMode(fadePin,OUTPUT);          // pin that will fade to your heartbeat!
  Serial.begin(9600);               // we agree to talk fast!
  esp.begin(9600);                  // Start esp serial
  interruptSetup();                 // sets up to read Pulse Sensor signal every 2mS
   // IF YOU ARE POWERING The Pulse Sensor AT VOLTAGE LESS THAN THE BOARD VOLTAGE,
   // UN-COMMENT THE NEXT LINE AND APPLY THAT VOLTAGE TO THE A-REF PIN
//   analogReference(EXTERNAL);

  reset();        // Reset esp module
  connectWifi();  // Attempt connect to internet
}


//reset the esp8266 module
void reset() {

  esp.println("AT+RST");
  delay(1000);
  if(esp.find("OK") ) Serial.println("Module Reset");

}
//connect to your wifi network
void connectWifi() {

  String cmd = "AT+CWJAP=\"WLAN_Vonk\",\"PASSWORD\"";
//  String cmd = "AT+CWJAP=\"arduino_test\",\"PASSWORD\"";
//  String cmd = "AT+CWJAP=\"" +ssid+"\",\"" + password + "\"";
  

  Serial.println("Trying to connect...");

  esp.println(cmd);
  delay(4000);

  if(esp.find("OK")) {
    Serial.println("Connected!");
    connectionState = true;
  }
  else {
    Serial.println("Failed to connect to wifi"); 
    connectWifi();
  }

}

//  Where the Magic Happens
void loop(){
  if(connectionState){
    serialOutput() ;
    if (QS == true){  // A Heartbeat Was Found, BPM and IBI have been Determined, Quantified Self "QS" true when arduino finds a heartbeat
        fadeRate = 255; // Makes the LED Fade Effect Happen, Set 'fadeRate' Variable to 255 to fade LED with pulse
        serialOutputWhenBeatHappens();   // A Beat Happened, Output that to serial.
        QS = false;                      // reset the Quantified Self flag for next time
    }
    ledFadeToBeat();                      // Makes the LED Fade Effect Happen
    delay(20);                             //  take a break
  }
  
  // wifi testing code!
  if(dbCounter >= 30000){
//    cli();
      Serial.println("Sending data to database...");
      // create data post
      sensorData = BPM;
      data = "data=" + sensorData + "&user=" + deviceUser + "&sessionId=" + sessionId;// data sent must be under this form //name1=value1&name2=value2.
      httppost();
      dbCounter = 0;
//    sei();   
  }
}

void ledFadeToBeat(){
    fadeRate -= 15;                         //  set LED fade value
    fadeRate = constrain(fadeRate,0,255);   //  keep LED fade value from going into negative numbers!
    analogWrite(fadePin,fadeRate);          //  fade LED
  }


// Post to database function
void httppost () {
  dataSent = false;
  esp.println("AT+CIPSTART=\"TCP\",\"" + server + "\",80");//start a TCP connection.

  if( esp.find("OK")) {
    Serial.println("TCP connection ready");
  } 
  delay(1000);
  String postRequest =
  "POST " + uri + " HTTP/1.0\r\n" +
  "Host: " + server + "\r\n" +
  "Accept: *" + "/" + "*\r\n" +
  "Content-Length: " + data.length() + "\r\n" +
  "Content-Type: application/x-www-form-urlencoded\r\n" +
  "\r\n" + data;
  String sendCmd = "AT+CIPSEND=";//determine the number of caracters to be sent.
  esp.print(sendCmd);
  esp.println(postRequest.length() );
  delay(500);
  if(esp.find(">")) { 
    Serial.println("Sending.."); esp.print(postRequest);
    if( esp.find("SEND OK")) { 
      delay(100);
      Serial.println("Packet sent");
  
      while (esp.available()) {
        delay(100);
        String tmpResp = esp.readString();
        Serial.println(tmpResp);
      }
      delay(100);  
      // close the connection
      esp.println("AT+CIPCLOSE");
      Serial.println("Data sent, closing connection...");
      dataSent = true;
    }
  }
}
