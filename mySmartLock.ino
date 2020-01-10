/*
This sketch demonstrates how to set up a simple HTTP-like server.
The server will set a GPIO pin depending on the request

http://server_ip/gpio/0 will set the GPIO2 low,
http://server_ip/gpio/1 will set the GPIO2 high
http://server_ip/mySmartLock/unlock will unlock the Smartlock

server_ip is the IP address of the ESP8266 module, will be
printed to Serial while the module is connected.
*/

#include <Servo.h>
#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include <ESP8266WebServer.h>
#include <ESP8266mDNS.h>
#include <ESP8266HTTPUpdateServer.h>

#define LED 2                    //led output pin
#define MYSERVOPIN 14            //servo output pin
#define FINGERPRINTSIGNAL 13     //fingerprint module Touch Signal input pin
#define UNLOCKPOSITION 0         //the position of unlock
#define RESETPOSITION 90         //the position of reset
#define TCPPORT 80
#define TRANSMISSIONPORT 81
#define DEBUGPIN 12

#define DEBUG true

WiFiClient client;
WiFiClient transClient;
Servo servo;
int currentServoPosition = RESETPOSITION;
bool modeSwitch;
bool transClientConnectingFlag;

const char* host = "";
const char* ssid = "";        //wifi name
const char* password = "";  //wifi password

const unsigned char CMD_GET_IMAGE[26] = { 0x55,0xAA,0x00,0x00,0x20,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x1f,0x01 };
const unsigned char CMD_GENERATE[26]  = { 0x55,0xaa,0x00,0x00,0x60,0x00,0x02,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x61,0x01 };
const unsigned char CMD_SEARCH[26]    = { 0x55,0xaa,0x00,0x00,0x63,0x00,0x06,0x00,0x00,0x00,0x01,0x00,0xf4,0x01,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x5e,0x02 };
const unsigned char CMD_SLED_CTRL[26] = { 0x55,0xaa,0x00,0x00,0x24,0x00,0x02,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x25,0x01 };

// Create an instance of the server
// specify the port to listen on as an argument
WiFiServer server(TCPPORT);
WiFiServer transServer(TRANSMISSIONPORT);

ESP8266WebServer httpServer(82);
ESP8266HTTPUpdateServer httpUpdater;

void setup() {
	WiFi.mode(WIFI_STA);
	Serial.begin(115200);
	delay(10);

	// prepare GPIO2 & GPIO13
	pinMode(LED, OUTPUT);
	pinMode(FINGERPRINTSIGNAL, INPUT);

	if(DEBUG)
		pinMode(DEBUGPIN, OUTPUT);

	//prepare GPIO14
	servo.attach(MYSERVOPIN);
	

	// Connect to WiFi network
	
	Serial.print("Connecting to ");
	Serial.println(ssid);

	WiFi.begin(ssid, password);

	while (WiFi.waitForConnectResult() != WL_CONNECTED) {
		WiFi.begin(ssid, password);
		Serial.println("WiFi failed, retrying.");
	}

	Serial.println();
	Serial.println("WiFi connected");

	// Start the server
	server.begin();
		Serial.println("TCP Server started at 80 port!");
	transServer.begin();
		Serial.println("Transmission Server started at 81 port!");

	if(DEBUG)
		digitalWrite(DEBUGPIN, HIGH); //initial DEBUGPIN 

	MoveServoToPosition(RESETPOSITION, 10);
	modeSwitch = false;

	MDNS.begin(host);

	httpUpdater.setup(&httpServer);
	httpServer.begin();

	MDNS.addService("http", "tcp", 82);
	Serial.printf("new HTTPUpdateServer ready! Open http://%s.local/update in your browser\n", host);
	
	Serial.println("Ready");
	Serial.print("IP address: ");
	Serial.println(WiFi.localIP());
}

void loop() {
	httpServer.handleClient();
	Serial.flush();
	//Check if a transmission client has connected
	transClient = transServer.available();
	if (transClient) {
		transClientConnectingFlag = true;
		while(transClient.connected()) {
			if (transClient.available()) {
				size_t counti = transClient.available();
				unsigned char* command = new unsigned char[counti];
				transClient.readBytes(command, counti);
				
				if (!strncmp((const char*)command, "debug()", counti)) {
					modeSwitch = true;
				}else if (!strncmp((const char*)command, "exit()", counti)) {
					modeSwitch = false;
				}else if(modeSwitch) {
					Serial.write(command, counti);
				}	
			}

			delay(1000);

			if (Serial.available())
			{
			size_t counti = Serial.available();
			uint8_t *sbuf=new uint8_t[counti];
			Serial.readBytes(sbuf, counti);
			transClient.write((const uint8_t*)sbuf, counti);
			delete(sbuf);
			}

			loopBody(modeSwitch);
		}
		transClient.stop();
	}else {
		transClientConnectingFlag = false;
		loopBody(modeSwitch);
	}
	
}

void loopBody(bool modeSwitch) {
	//Check if a user has touched the fingerprint module
	int status = digitalRead(FINGERPRINTSIGNAL);
	//Serial.println(status);
	if (status&&!modeSwitch) {
		unsigned char* g_packet = new unsigned char[26];
		size_t size=26;
		Serial.write(CMD_GET_IMAGE, 26);
		get_gPacket(g_packet, 26);
		broadcast((const char*)g_packet, size);

			if (g_packet[0] = 0xAA && g_packet[1] == 0x55 && g_packet[4] == 0x20 && g_packet[8] == 0x00 && g_packet[9] == 0x00) {
				memset(g_packet, 0, 26);
				Serial.write(CMD_GENERATE, 26);
				get_gPacket(g_packet, 26);
				broadcast((const char*)g_packet, size);
					
					if (g_packet[0] = 0xAA && g_packet[1] == 0x55 && g_packet[4] == 0x60 && g_packet[8] == 0x00 && g_packet[9] == 0x00) {
						memset(g_packet, 0, 26);
						Serial.write(CMD_SEARCH, 26);
						get_gPacket(g_packet, 26);
						broadcast((const char*)g_packet, size);

							if (g_packet[0] = 0xAA && g_packet[1] == 0x55 && g_packet[4] == 0x63 && g_packet[6] == 0x05 && g_packet[8] == 0x00 && g_packet[9] == 0x00) {
								unlock();
							}
						
					}
				
			}
			
			Serial.write(CMD_SLED_CTRL, 26);
			delay(500);
			get_gPacket(g_packet, 26);
			broadcast((const char*)g_packet, size);

		}
	else {
		delay(1000);
	}


	

	// Check if a Network client has connected
	client = server.available();
	if (client) {
		// Read the first line of the request
		String req = client.readStringUntil('\r');

		client.flush();
		delay(10);

		// Match the request
		int val = 1;
		if (req.indexOf("/gpio/0") != -1)
			ledControl(LED, 1);
		else if (req.indexOf("/gpio/1") != -1)
			ledControl(LED, 0);
		else if (req.indexOf("/mySmartLock/unlock") != -1) {
			unlock();
		}
		else {
			//Serial.println("invalid request");
			client.stop();
			return;
		}

		//Serial.println("Client disonnected");
		// The client will actually be disconnected
		// when the function returns and 'client' object is detroyed
	}
}

bool get_gPacket(unsigned char* p,size_t size) {
	size_t recLen = 0;
	size_t totalLen = 0;
	while (totalLen < size) {
		recLen = Serial.available();
		Serial.readBytes(p+totalLen,recLen);
		totalLen = totalLen + recLen;
	}
	return true;
}

void broadcast(const char* info, size_t size) {
	if (transClientConnectingFlag) {
		transClient.write(info, size);
	}
}

void ledControl(int ledPin,int val) {
	// Set GPIO2 according to the request
	digitalWrite(LED, val);

	// Prepare the response
	String s = "HTTP/1.1 200 OK\r\nContent-Type: text/html\r\n\r\n<!DOCTYPE HTML>\r\n<html>\r\nGPIO is now ";
	s += (val) ? "low" : "high";
	s += "</html>\n";
	// Send the response to the client
	client.print(s);
	delay(1);
}

void unlock() {
	MoveServoToPosition(UNLOCKPOSITION, 1);
	delay(3000);
	MoveServoToPosition(RESETPOSITION, 1);
	if (DEBUG) {
		digitalWrite(DEBUGPIN, LOW);
		delay(2000);
		digitalWrite(DEBUGPIN, HIGH);
	}
	// Prepare the response
	String s = "HTTP/1.1 200 OK\r\nContent-Type: text/html\r\n\r\n<!DOCTYPE HTML>\r\n<html>\r\n";
	s += "Unlock succeeded!";
	s += "</html>\n";

	// Send the response to the client
	client.print(s);
	delay(1);
}

void MoveServoToPosition(int position, int speed) {
	if (position < currentServoPosition)
	{
		for (int i = currentServoPosition; i > position; i--)
		{
			servo.write(i);
			delay(speed);
		}
	}
	else if (position > currentServoPosition)
	{
		for (int i = currentServoPosition; i < position; i++)
		{
			servo.write(i);
			delay(speed);
		}
	}
	currentServoPosition = position;
}
