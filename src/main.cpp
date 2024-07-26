#include <WiFi.h>
#include <SPIFFS.h>
#include <PubSubClient.h>
#include <WiFiClientSecure.h>
#include <ESPAsyncWebServer.h>
#include <AsyncTCP.h>
#include <AsyncElegantOTA.h>

// Upload SSID and Password that were saved.
String ssid, password;

// check status of ESP32 is whether an access point or not
bool result = 0; 

// define SSID and Password for Access Point
const char* apSSID = APSSID ;
const char* apPassword = APPASSWORD ;

// define iTSD port at 26, 22 as RX, TX respectively
// use for ESP32 communicates with iTSD
const int RXD1 = MYRXD1 ;
const int TXD1 = MYTXD1 ;

// Switch for deleting password of WiFi 
const int resetsw = MYSWITCH ;
// resetswitch for reseting wifi to be default value
const unsigned long holdtime = 3000 ;
bool buttonstate = LOW ;
bool lastbuttonstate = LOW ;
unsigned long buttonpresstime = 0 ;

// default state of iTSD
int state = 1;

// default time to send command to iTSD
long lastMsg = 0;
long lastMsg2 = 0;

// MQTT configuration *******************************************************
// HIVEMQ server >> Nay
const char *mqtt_server = MQTTSERVER ; // free broker test
const char *mqtt_username = MQTTUSERNAME ;
const char *mqtt_password = MQTTPASSWORD ;
const int mqtt_port = MQTTPORT ;

const char* Topic_sub = MYTOPICSUB ;
const char* Topic_pub = MYTOPICPUB ;
const char* clientid = MYCLIENTID ;

// create wifi client: Use WiFi to be the network for Client connect to Server
// WiFiClient espClient;
WiFiClientSecure espClient;

// create PubSubClient 
PubSubClient client(espClient);
// MQTT configuration ********************************************************

// build webserver
AsyncWebServer server(80);

// HTML Form for getting SSID and Password 
const char* htmlForm = R"rawliteral(
<!DOCTYPE HTML><html>
<head>
  <title>ESP32 WiFi Configuration</title>
  <meta name="viewport" content="width=device-width, initial-scale=1">
  <style>
    body {
      font-family: Arial, sans-serif;
      display: flex;
      justify-content: center;
      align-items: center;
      height: 100vh;
      margin: 0;
      background-color: #f0f0f0;
    }
    .container {
      background-color: white;
      padding: 20px;
      border-radius: 10px;
      box-shadow: 0 0 10px rgba(0, 0, 0, 0.1);
      max-width: 400px;
      width: 100%;
      text-align: center;
    }
    h2 {
      margin-bottom: 20px;
      color: #333;
    }
    input[type="text"], input[type="password"] {
      width: calc(100% - 22px);
      padding: 10px;
      margin: 10px 0;
      border: 1px solid #ccc;
      border-radius: 5px;
    }
    input[type="submit"] {
      background-color: #4CAF50;
      color: white;
      border: none;
      border-radius: 5px;
      padding: 10px 20px;
      cursor: pointer;
      margin-top: 10px;
    }
    input[type="submit"]:hover {
      background-color: #45a049;
    }
    .form-group {
      margin-bottom: 15px;
    }
  </style>
</head>
<body>
  <div class="container">
    <h2>WiFi Configuration</h2>
    <form action="/get" method="get">
      <div class="form-group">
        <label for="ssid">SSID:</label>
        <input type="text" id="ssid" name="ssid" required>
      </div>
      <div class="form-group">
        <label for="password">Password:</label>
        <input type="password" id="password" name="password" required>
      </div>
      <input type="submit" value="Save">
    </form>
    <form action="/reset" method="get">
      <input type="submit" value="Reset WiFi Configuration" style="background-color: #f44336;">
    </form>
  </div>
</body>
</html>
)rawliteral";


// HiveMQ Cloud Let's Encrypt CA certificate
static const char *root_ca PROGMEM = R"EOF(
-----BEGIN CERTIFICATE-----
MIIFBTCCAu2gAwIBAgIQS6hSk/eaL6JzBkuoBI110DANBgkqhkiG9w0BAQsFADBP
MQswCQYDVQQGEwJVUzEpMCcGA1UEChMgSW50ZXJuZXQgU2VjdXJpdHkgUmVzZWFy
Y2ggR3JvdXAxFTATBgNVBAMTDElTUkcgUm9vdCBYMTAeFw0yNDAzMTMwMDAwMDBa
Fw0yNzAzMTIyMzU5NTlaMDMxCzAJBgNVBAYTAlVTMRYwFAYDVQQKEw1MZXQncyBF
bmNyeXB0MQwwCgYDVQQDEwNSMTAwggEiMA0GCSqGSIb3DQEBAQUAA4IBDwAwggEK
AoIBAQDPV+XmxFQS7bRH/sknWHZGUCiMHT6I3wWd1bUYKb3dtVq/+vbOo76vACFL
YlpaPAEvxVgD9on/jhFD68G14BQHlo9vH9fnuoE5CXVlt8KvGFs3Jijno/QHK20a
/6tYvJWuQP/py1fEtVt/eA0YYbwX51TGu0mRzW4Y0YCF7qZlNrx06rxQTOr8IfM4
FpOUurDTazgGzRYSespSdcitdrLCnF2YRVxvYXvGLe48E1KGAdlX5jgc3421H5KR
mudKHMxFqHJV8LDmowfs/acbZp4/SItxhHFYyTr6717yW0QrPHTnj7JHwQdqzZq3
DZb3EoEmUVQK7GH29/Xi8orIlQ2NAgMBAAGjgfgwgfUwDgYDVR0PAQH/BAQDAgGG
MB0GA1UdJQQWMBQGCCsGAQUFBwMCBggrBgEFBQcDATASBgNVHRMBAf8ECDAGAQH/
AgEAMB0GA1UdDgQWBBS7vMNHpeS8qcbDpHIMEI2iNeHI6DAfBgNVHSMEGDAWgBR5
tFnme7bl5AFzgAiIyBpY9umbbjAyBggrBgEFBQcBAQQmMCQwIgYIKwYBBQUHMAKG
Fmh0dHA6Ly94MS5pLmxlbmNyLm9yZy8wEwYDVR0gBAwwCjAIBgZngQwBAgEwJwYD
VR0fBCAwHjAcoBqgGIYWaHR0cDovL3gxLmMubGVuY3Iub3JnLzANBgkqhkiG9w0B
AQsFAAOCAgEAkrHnQTfreZ2B5s3iJeE6IOmQRJWjgVzPw139vaBw1bGWKCIL0vIo
zwzn1OZDjCQiHcFCktEJr59L9MhwTyAWsVrdAfYf+B9haxQnsHKNY67u4s5Lzzfd
u6PUzeetUK29v+PsPmI2cJkxp+iN3epi4hKu9ZzUPSwMqtCceb7qPVxEbpYxY1p9
1n5PJKBLBX9eb9LU6l8zSxPWV7bK3lG4XaMJgnT9x3ies7msFtpKK5bDtotij/l0
GaKeA97pb5uwD9KgWvaFXMIEt8jVTjLEvwRdvCn294GPDF08U8lAkIv7tghluaQh
1QnlE4SEN4LOECj8dsIGJXpGUk3aU3KkJz9icKy+aUgA+2cP21uh6NcDIS3XyfaZ
QjmDQ993ChII8SXWupQZVBiIpcWO4RqZk3lr7Bz5MUCwzDIA359e57SSq5CCkY0N
4B6Vulk7LktfwrdGNVI5BsC9qqxSwSKgRJeZ9wygIaehbHFHFhcBaMDKpiZlBHyz
rsnnlFXCb5s8HKn5LsUgGvB24L7sGNZP2CX7dhHov+YhD+jozLW2p9W4959Bz2Ei
RmqDtmiXLnzqTpXbI+suyCsohKRg6Un0RC47+cpiVwHiXZAW+cn8eiNIjqbVgXLx
KPpdzvvtTnOPlC7SQZSYmdunr3Bf9b77AiC/ZidstK36dRILKz7OA54=
-----END CERTIFICATE-----
)EOF";


// Try to conncect WiFi if There are SSID and Password.
 void setup_wifi(){
  if (ssid.length() > 0 && password.length() > 0) {
    WiFi.begin(ssid.c_str(), password.c_str());
    Serial.print("Connecting to WiFi");

    unsigned long startAttemptTime = millis();
    while (WiFi.status() != WL_CONNECTED && millis() - startAttemptTime < 20000) {
      delay(1000);
      Serial.print(".");
    }
    Serial.println();

    if (WiFi.status() == WL_CONNECTED) {
      Serial.println("Connected to WiFi");
      Serial.println(ssid);
      Serial.print("IP Address: ");
      Serial.println(WiFi.localIP());
      String ipAddress = "Connected to WiFi\nIP:" + WiFi.localIP().toString();
    } else if (!result && ssid.length() > 0 && password.length() > 0 ) {
        Serial.println("Cannot connect to WiFi, trying to connect");
    } else{
        Serial.println("Failed to connect to WiFi, starting AP mode...");
    }
  }
 }

// If disconnected WiFi
 void accesspoint(){
    if (WiFi.status() != WL_CONNECTED) {
    // Turn ESP32 to be Access Point
      result = WiFi.softAP(apSSID, apPassword);
    }
    if(result) {
      Serial.println("Access Point Started");
      //Check IP Address of Access Point
      IPAddress IP = WiFi.softAPIP();
      Serial.print("AP IP address: ");
      Serial.println(IP);
    } else{
      Serial.println("Access Point Failed");
    }
  }

// Function for save SSID and Password of WiFi to store them in SOIFFS
void saveWiFiConfig(String ssid, String password) {
  File file = SPIFFS.open("/wifi_config.txt", FILE_WRITE);
  if (file) {
    file.println(ssid);
    file.println(password);
    file.close();
    Serial.println("WiFi configuration saved.");
  } else {
    Serial.println("Failed to save WiFi configuration.");
  }
}

// Function for upload WiFi SSID and Password from SPIFFS
void loadWiFiConfig(String &ssid, String &password) {
  File file = SPIFFS.open("/wifi_config.txt", FILE_READ);
  if (file) {
    ssid = file.readStringUntil('\n');
    password = file.readStringUntil('\n');
    ssid.trim();
    password.trim();
    file.close();
    Serial.println("WiFi configuration loaded.");
  } else {
    Serial.println("Failed to load WiFi configuration.");
  }
}

// Function for deleting SSID and Password of from SPIFFS
void resetWiFiConfig() {
  SPIFFS.remove("/wifi_config.txt");
  File file = SPIFFS.open("/wifi_config.txt", FILE_WRITE);
  if (file) {
    file.println("4G Home WiFi_217962");
    file.println("50217962");
    file.close();
    Serial.println("WiFi configuration back to default");
  } else {
    Serial.println("Failed to be back to default.");
  }
  Serial.println("WiFi configuration reset.");
  ESP.restart(); 
}


// Function to reconnect if disconnected from the server
void reconnect() {
  while (!client.connected()) {
    Serial.print("Attempting MQTT connection...");
    //random client ID, you can put whatever you want.
    if (client.connect(clientid, mqtt_username, mqtt_password))
    {
      Serial.println("connected");
      client.subscribe(Topic_sub);
      if (client.subscribe(Topic_sub)) {
       Serial.println("Subscription successful");
      } else {
         Serial.println("Subscription failed");
      }
    } 
    else
     {
      Serial.print("failed, rc=");
      //if not connected, print the error code
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      delay(5000);
      if (client.state() ==  -2){
        Serial.print("network failed -> try to connect wifi");
        setup_wifi();
      }
    }
  }
}

// callback function for topic subscription
void callback(char *topic, byte *payload, unsigned int length) {
    String dataIn;  

    Serial.println("trigger !!!");
    Serial.print("Topic: ");
    Serial.println(topic);
    for (int i = 0; i < length; i++) 
    {
        // Serial.print((char) payload[i]);
        dataIn += (char)payload[i];
    }
    
    Serial.println("dataIn: " + dataIn);

    if (dataIn == "auto")
    {
      state = 1;
      Serial.print("state change to => ");
      Serial.println(state);
        
    } 
    else if (dataIn == "ch1")
    {
      state = 3;
      Serial.print("state change to => ");
      Serial.println(state);
        
    } 
    else 
    {
        state = 0;
        Serial.print("state change to => ");
        Serial.println(state);
    }

    if (state == 0)
      {
       Serial1.print(dataIn + "\r");
      }
    
}

void blinky() {
  digitalWrite(2 , HIGH);
  delay(500);
  digitalWrite(2 , LOW);
  delay(500);
  digitalWrite(2 , HIGH);
  delay(500);
}


// setup()
void setup() {

  // set LED blink
  pinMode(2 , OUTPUT);

  // set pin resetsw to be pin for input
  pinMode(resetsw , INPUT);

  // initialize serial communication at 115200 kB/s
  Serial.begin(115200);

  // initialize iTSD communication at 9600 kB/s
  Serial1.begin(9600, SERIAL_8N1, RXD1, TXD1);

  // Set the ESP32 to be both an access point and a station
  WiFi.mode(WIFI_AP_STA);

  // Initialize SPIFFS
  if (!SPIFFS.begin(true)) {
    Serial.println("An error occurred while mounting SPIFFS");
    return;
  }
  
  // get value of ssid and password from SPIFFS 
  loadWiFiConfig(ssid, password);
 
  setup_wifi();

  accesspoint();

  // connecting to a mqtt broker
  espClient.setCACert(root_ca); // sent CER to server // 
  client.setServer(mqtt_server, mqtt_port); // set server
  client.setCallback(callback); // set trigger function when topic is published
  client.subscribe(Topic_sub);
  
  // Received SSID and Password via filling the form
  server.on("/get", HTTP_GET, [](AsyncWebServerRequest *request){ // A server listens to http request and then respond with the http response.
    String ssid = request->getParam("ssid")->value();
    String password = request->getParam("password")->value();
    saveWiFiConfig(ssid, password);
    request->send(200, "text/html", "WiFi configuration saved. Please restart the ESP32.");
    ESP.restart();
  });

  // Reset SSID and Password of WiFi (Reset in the webpage)
  server.on("/reset", HTTP_GET, [](AsyncWebServerRequest *request){
    resetWiFiConfig();
    request->send(200, "text/html", "WiFi configuration reset. ESP32 is restarting...");
  });

  // Define route: URL and show form HTML
  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send(200, "text/html", htmlForm);
  });


  AsyncElegantOTA.begin(&server);    // Start ElegantOTA
  // Start Server
  server.begin();
  
  Serial.print("Current state: ");
  Serial.println(state);
  blinky();
  blinky();
  blinky();

}


// loop()
void loop() {

  // Press resetsw to reset wifi network to be default value
  buttonstate = digitalRead(resetsw);

  if(buttonstate == HIGH && lastbuttonstate == LOW){
    buttonpresstime = millis();
  }

  if(buttonstate == HIGH && (millis() - buttonpresstime >= holdtime)){
     Serial.println("Reset switch pressed, resetting WiFi configuration...");
     resetWiFiConfig();
  }

  lastbuttonstate = buttonstate;
  

  if (WiFi.status() != WL_CONNECTED) {
    setup_wifi();
  }

  if (!client.connected()) 
  {
    // reconnect to MQTT broker if not connected
    reconnect();
  }
  client.loop();
  
  
  
  long now = millis();
  long now2 = millis();

  if (state == 1)
  {
    if (now2 - lastMsg2 > 5000)
     {
      lastMsg2 = now2;
      Serial1.println(":i");
      Serial.println("state: "+ String(state));
    }
  } 
  else if (state == 3) 
  {
    if (now2 - lastMsg2 > 5000) 
    {
      lastMsg2 = now2;
      Serial1.println(":dA");
      Serial.println("state: "+ String(state));
    }
  }
  
  if (now - lastMsg > 100) 
  {
    lastMsg = now;
    
    if(Serial1.available() > 0) 
    {
        String msg_device = Serial1.readStringUntil('\r');

        if (msg_device == NULL || msg_device == "" || msg_device == " " || msg_device == "\r" ||  msg_device == "\n")
        {
          Serial.println("empty str or cr or lf");
        } 
        else if (msg_device.length() != 0) 
        {
          msg_device.trim();
          Serial.println(msg_device);
          client.publish(MYTOPICPUB, msg_device.c_str()); // c_str = C-Style String
          blinky();
          msg_device = ""; // have = not have
        }
    }

  }

  if(Serial.available() > 0)
  { 
  
            String db = Serial.readStringUntil('\r');
            if (db.substring(0,3) == "pub") // Checks if the first three characters of the string db are "pub". If they are, the following code block is executed.
            {
            db.remove(0,3); // Removes the first three characters from the string db.
              Serial.print("trigger test public cmd => ");
              Serial.println(db);
              Serial1.print(db + "\r");
              blinky();
              Serial.flush(); // Waits for the transmission of outgoing serial data to complete. (Prior to Arduino 1.0, this instead removed any buffered incoming serial data.)
              db = "";
            }
            delay(100); 
  }


}


