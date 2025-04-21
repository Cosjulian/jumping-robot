#include <Arduino.h>
#include <WiFi.h>

const char* ssid = "dingle_berry";

// Motor Pin Pins
const int output26 = 26;

String output26State = "off";

WiFiServer server(80);

// Distance from LIDAR
int last_distance = 0;

void setup() {
  Serial.begin(115200);


  pinMode(output26, OUTPUT);
  digitalWrite(output26, LOW);

  Serial.println("Connecting to WiFi...");
  WiFi.softAP(ssid);
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.print(".");
  }
  Serial.println("");
  Serial.println("WiFi connected.");
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());

  server.begin();
}

void loop() {
  // Read LIDAR distance if available
  if (Serial.available()) {
    String lidar_input = Serial.readStringUntil('\n');
    lidar_input.trim();
    if (lidar_input.length() > 0) {
      last_distance = lidar_input.toInt();
      Serial.print("Updated distance: ");
      Serial.println(last_distance);
    }
  }

  WiFiClient client = server.available(); // listen for incoming clients
  if (client) {
    Serial.println("New Client.");
    String currentLine = "";
    String header = "";

    while (client.connected()) {
      if (client.available()) {
        char c = client.read();
        header += c;
        if (c == '\n') {
          if (currentLine.length() == 0) {
            // Handle request
            if (header.indexOf("GET /26/on") >= 0) {
              Serial.println("GPIO 26 on");
              output26State = "on";
              digitalWrite(output26, HIGH);
            } else if (header.indexOf("GET /26/off") >= 0) {
              Serial.println("GPIO 26 off");
              output26State = "off";
              digitalWrite(output26, LOW);
            } else if (header.indexOf("GET /get_distance") >= 0) {
              Serial.println("Distance requested");
              client.println("HTTP/1.1 200 OK");
              client.println("Content-Type: application/json");
              client.println("Connection: close");
              client.println();
              client.print("{\"distance\": ");
              client.print(last_distance);
              client.println("}");
              break;
            }

            // HTML response for root or other endpoints
            client.println("HTTP/1.1 200 OK");
            client.println("Content-type:text/html");
            client.println("Connection: close");
            client.println();
            client.println("<!DOCTYPE html><html>");
            client.println("<head><meta name=\"viewport\" content=\"width=device-width, initial-scale=1\">");
            client.println("<style>body { font-family: Arial; } button { padding: 10px; margin: 5px; }</style></head>");
            client.println("<body><h1>ESP32 Web Server</h1>");
            client.println("<p>GPIO 26 - State: " + output26State + "</p>");
            client.println("<p><a href=\"/26/on\"><button>ON</button></a>");
            client.println("<a href=\"/26/off\"><button>OFF</button></a></p>");
            client.println("<p>Latest LIDAR Distance: " + String(last_distance) + " cm</p>");
            client.println("</body></html>");
            break;
          } else {
            currentLine = "";
          }
        } else if (c != '\r') {
          currentLine += c;
        }
      }
    }
    delay(1);
    client.stop();
    Serial.println("Client Disconnected.");
  }
}
