#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <sstream>

#include <DNSServer.h>
#include <ESP8266WebServer.h>
#include <WiFiManager.h>
#include <ESP8266mDNS.h>
#include <BH1750.h>

ESP8266WebServer server(80);
BH1750 lightMeter;

void handleRoot()
{
	char temp[400];
	int sec = millis() / 1000;
	int min = sec / 60;
	int hr = min / 60;

	snprintf(temp, 400,

			 "<html>\
  <head>\
    <meta http-equiv='refresh' content='5'/>\
    <title>ESP8266 Demo</title>\
    <style>\
      body { background-color: #cccccc; font-family: Arial, Helvetica, Sans-Serif; Color: #000088; }\
    </style>\
  </head>\
  <body>\
    <h1>Hello from ESP8266!</h1>\
    <p>Uptime: %02d:%02d:%02d</p>\
    <img src=\"/test.svg\" />\
  </body>\
</html>",

			 hr, min % 60, sec % 60);
	server.send(200, "text/html", temp);
}

void handleNotFound()
{
	String message = "File Not Found\n\n";
	message += "URI: ";
	message += server.uri();
	message += "\nMethod: ";
	message += (server.method() == HTTP_GET) ? "GET" : "POST";
	message += "\nArguments: ";
	message += server.args();
	message += "\n";

	for (uint8_t i = 0; i < server.args(); i++)
	{
		message += " " + server.argName(i) + ": " + server.arg(i) + "\n";
	}

	server.send(404, "text/plain", message);
}

void handleReadLightMeter()
{
	float lux = lightMeter.readLightLevel();

	std::ostringstream ss;
	ss << lux;
	String message(ss.str().c_str());

	server.send(404, "text/plain", message);

	server.send(200, "text/plain", message);
}

void drawGraph()
{
	String out = "";
	char temp[100];
	out += "<svg xmlns=\"http://www.w3.org/2000/svg\" version=\"1.1\" width=\"400\" height=\"150\">\n";
	out += "<rect width=\"400\" height=\"150\" fill=\"rgb(250, 230, 210)\" stroke-width=\"1\" stroke=\"rgb(0, 0, 0)\" />\n";
	out += "<g stroke=\"black\">\n";
	int y = rand() % 130;
	for (int x = 10; x < 390; x += 10)
	{
		int y2 = rand() % 130;
		sprintf(temp, "<line x1=\"%d\" y1=\"%d\" x2=\"%d\" y2=\"%d\" stroke-width=\"1\" />\n", x, 140 - y, x + 10, 140 - y2);
		out += temp;
		y = y2;
	}
	out += "</g>\n</svg>\n";

	server.send(200, "image/svg+xml", out);
}

void turn_on()
{
	digitalWrite(D7, 1);
	drawGraph();
}

void turn_off()
{
	digitalWrite(D7, 0);
	drawGraph();
}

void handleBrightness()
{ //Handler for the body path

	if (server.hasArg("plain") == false)
	{ //Check if body received

		server.send(200, "text/plain", "Body not received");
		return;
	}

	String message = "Body received:\n";
	message += server.arg("plain");
	message += "\n";

	server.send(200, "text/plain", message);
	Serial.println(message);
	analogWrite(D7, server.arg("plain").toInt());
}

void setup()
{
	delay(5000);
	Serial.begin(9600);
	Wire.begin(D4, D3);
	lightMeter.begin();
	pinMode(D7, OUTPUT);
	WiFiManager wifiManager;
	wifiManager.autoConnect("NodeMCU-Arduino-PlatformIO");
	Serial.println("Connected!");

	if (MDNS.begin("esp8266"))
	{
		Serial.println("MDNS responder started");
	}

	server.on("/", handleRoot);
	server.on("/test.svg", drawGraph);
	server.on("/light-on", turn_on);
	server.on("/light-off", turn_off);
	server.on("/brightness", handleBrightness);
	server.on("/light-meter", handleReadLightMeter);
	server.on("/inline", []() {
		server.send(200, "text/plain", "this works as well");
	});
	server.onNotFound(handleNotFound);
	server.begin();
	Serial.println("HTTP server started");
	Serial.println("local ip");
	Serial.println(WiFi.localIP());
	Serial.println(WiFi.gatewayIP());
	Serial.println(WiFi.subnetMask());
	Serial.println("Idle...");
}

void loop()
{
	server.handleClient();
}