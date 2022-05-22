/*
 * Import Modules
*/
#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <FS.h>
#include <ArduinoJson.h>

//Webserver Instantiated.
ESP8266WebServer server;

//Constants with data type.
uint8_t pin_led = 2;
char* ssid = "YOUR_SSID"; //not used
char* password = "YOUR_AP_PASSWORD";
String mySsid = "SandeTronics " + WiFi.macAddress();

/*
 * IP Address For default AP login.
*/
IPAddress local_ip(192,168,100,1);
IPAddress gateway(192,168,100,1);
IPAddress netmask(255,255,255,0);

/*
 * WebPage static code.
 */
char webpage[] PROGMEM = R"=====(
<html>
<head>
  <title>Wifi Setup</title>
  <meta name='viewport' content='width=device-width, initial-scale=1'>
  <link href='https://cdn.jsdelivr.net/npm/bootstrap@5.0.2/dist/css/bootstrap.min.css' rel='stylesheet' integrity='sha384-EVSTQN3/azprG1Anm3QDgpJLIm9Nao0Yz1ztcQTwFspd3yD65VohhpuuCOmLASjC' crossorigin='anonymous'>
</head>
<body class="container-fluid p-4">
  <div class="card">
    <form class="card-body">
      <fieldset>
        <div class="form-group my-2">
          <label class="form-label" for="ssid">SSID</label>      
          <input class="form-control" value="" id="ssid" placeholder="Enter your SSID name.">
        </div>
        <div class="form-group my-2">
          <label class="form-label" for="password">PASSWORD</label>
          <input class="form-control" type="password" value="" id="password" placeholder="Enter Your SSID password.">
        </div>
        <div>
          <button class="btn btn-primary mt-3 float-end" id="savebtn" type="button" onclick="myFunction()">Submit</button>
        </div>
      </fieldset>
    </form>
  </div>
</body>
<script>
function myFunction()
{
  console.log("button was clicked!");

  var ssid = document.getElementById("ssid").value;
  var password = document.getElementById("password").value;
  var data = {ssid:ssid, password:password};

  var xhr = new XMLHttpRequest();
  var url = "/settings";

  xhr.onreadystatechange = function() {
    if (this.readyState == 4 && this.status == 200) {
      // Typical action to be performed when the document is ready:
      if(xhr.responseText != null){
        console.log(xhr.responseText);
      }
    }
  };

  xhr.open("POST", url, true);
  xhr.send(JSON.stringify(data));
};
</script>
</html>
)=====";

void setup()
{
  pinMode(pin_led, OUTPUT);
  Serial.begin(115200);
  SPIFFS.begin();

  wifiConnect();

  server.on("/",[](){server.send_P(200,"text/html", webpage);});
  server.on("/toggle",toggleLED);
  server.on("/settings", HTTP_POST, handleSettingsUpdate);
  
  server.begin();
}

void loop()
{
  server.handleClient();
}

void handleSettingsUpdate()
{
  String data = server.arg("plain");
  DynamicJsonDocument jBuffer(1024);
  deserializeJson(jBuffer, data);
  JsonObject jObject = jBuffer.as<JsonObject>();

  File configFile = SPIFFS.open("/config.json", "w");
  serializeJson(jBuffer, configFile);  
  configFile.close();
  
  server.send(200, "application/json", "{\"status\" : \"ok\"}");
  delay(500);
  
  wifiConnect();
}

void wifiConnect()
{
  //reset networking
  WiFi.softAPdisconnect(true);
  WiFi.disconnect();          
  delay(1000);
  //check for stored credentials
  if(SPIFFS.exists("/config.json")){
    const char * _ssid = "", *_pass = "";
    File configFile = SPIFFS.open("/config.json", "r");
    if(configFile){
      size_t size = configFile.size();
      std::unique_ptr<char[]> buf(new char[size]);
      configFile.readBytes(buf.get(), size);
      configFile.close();

      DynamicJsonDocument jsonBuffer(1024);
      deserializeJson(jsonBuffer, buf.get());
      JsonObject jObject = jsonBuffer.as<JsonObject>();
      if(!jObject.isNull())
      {
        _ssid = jObject["ssid"];
        _pass = jObject["password"];
        WiFi.mode(WIFI_STA);
        WiFi.begin(_ssid, _pass);
        unsigned long startTime = millis();
        while (WiFi.status() != WL_CONNECTED) 
        {
          delay(500);
          Serial.println("Connecting...");
          digitalWrite(pin_led,!digitalRead(pin_led));
          if ((unsigned long)(millis() - startTime) >= 5000) break;
        }
      }
    }
  }

  if (WiFi.status() == WL_CONNECTED)
  {
    digitalWrite(pin_led,HIGH);
  } else 
  {
    WiFi.mode(WIFI_AP);
    WiFi.softAPConfig(local_ip, gateway, netmask);
    WiFi.softAP(mySsid); 
    digitalWrite(pin_led,LOW);      
  }
  Serial.println("");
  WiFi.printDiag(Serial);
  Serial.println(WiFi.localIP());
}

void toggleLED()
{
  digitalWrite(pin_led,!digitalRead(pin_led));
  server.send(204,"");
}
