#include <WiFi.h>
#include <HardwareSerial.h> 
HardwareSerial lidarSerial(2);
#define RXD2 27 // Lidar ports
#define TXD2 26 // Lidar ports

uint16_t last_distance = 0;

const char* ssid = "Jumping-Robot";

WiFiServer server(80); // 80 is the port

String header; // HTTP Header
String output26State = "off"; // Maintains state of DC motors (i.e. on or off)

const int output26 = 13;  // GPIO pin for motors

// For website
unsigned long currentTime = millis();
unsigned long previousTime = 0; 
const long timeoutTime = 2000;

void setup() {
  Serial.begin(115200);
  pinMode(output26, OUTPUT);
  digitalWrite(output26, LOW);

  lidarSerial.begin(115200, SERIAL_8N1, RXD2, TXD2); // Starts the lidar ports 

  Serial.print("Connecting to ");
  Serial.println(ssid);
  WiFi.softAP(ssid);

  Serial.println("");
  Serial.println("WiFi connected.");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
  server.begin();
}

uint16_t distance = 0;

void loop(){
  WiFiClient client = server.available();   // Listen for incoming clients
  uint8_t buf[9] = {0};

  lidarSerial.readBytes(buf, 9);
    if( buf[0] == 0x59 && buf[1] == 0x59)
    {
      distance = buf[2] + buf[3] * 256;
    }

  if (client) {                           
    currentTime = millis();
    previousTime = currentTime;
    Serial.println("New Client.");         
    String currentLine = "";               
    while (client.connected()) {  
      currentTime = millis();
      if (client.available()) {             
        char c = client.read();             
        header += c;
        if (c == '\n') {                   
          if (currentLine.length() == 0) {
            client.println("HTTP/1.1 200 OK");
            client.println("Content-type:text/html");
            client.println("Connection: close");
            client.println();
            
            if (header.indexOf("GET /26/on") >= 0) {
              Serial.println("GPIO 13 ON");
              digitalWrite(output26, HIGH);
              output26State = "on";
              delay(pow(distance, 1.2) * 4.98 * 1000);
              digitalWrite(output26, LOW);
              Serial.println("GPIO 13 OFF");

            }  else if (header.indexOf("GET /get_distance") >= 0) {
              Serial.println("Distance requested");
              client.print("{\"distance\": ");
              client.print(distance);
              client.println("}");
              break;
            }

            client.print("<div class=\"container\" style=\"max-width: 500px; margin-top: 50px; background: white; padding: 20px; border-radius: 10px; box-shadow: 0 0 10px rgba(0, 0, 0, 0.1);\">");
            client.print("<h1 style=\"color: #007bff;\">Jumping Robot Interface</h1>");

            client.print("<p>Lidar Detected Distance: <span id=\"distanceValue\">--</span> cm <button onclick=\"updateDistance()\" class=\"btn btn-sm btn-outline-primary\">Get Distance</button></p>");

            client.print("<a href=\"/26/on\"><button type=\"button\" class=\"btn btn-primary\" style=\"width: 100%; font-size: 1.2rem;\">Compress</button></a>");

            client.print("<script>");
            client.print("function updateDistance() {");
            client.print("fetch('/get_distance').then(res => res.json()).then(data => {");
            client.print("document.getElementById('distanceValue').textContent = data.distance;");
            client.print("}).catch(err => {");
            client.print("console.log(err); ");
            client.print("document.getElementById('distanceValue').textContent = 'Error';");
            client.print("});");
            client.print("}");
            client.print("</script>");
            client.println("</div></body></html>");

            client.println();
            break;
          } else {
            currentLine = "";
          }
        } else if (c != '\r') {
          currentLine += c;
        }
      }
      last_distance = distance;
    }
    header = "";
    client.stop();
    Serial.println("Client disconnected.");
    Serial.println("");
  }
}

