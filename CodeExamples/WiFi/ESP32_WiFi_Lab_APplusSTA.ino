/*
 Name:		ESP32_WiFi_Lab.ino
 Created:	14-Jul-21 9:38:27 PM
 Author:	giltal
*/

#include "PCF8574.h"
#include <Wire.h>
#include "graphics.h"
#include "SPIFFS.h" 

#define DHT_PRESENT

#ifdef DHT_PRESENT
#include "dht.h"
#define DHTPIN 12       // modify to the pin we connected
#define DHTTYPE AM2301   // AM2301 
DHT dht(DHTPIN, DHTTYPE);
#endif

bool spiffsOK = false;

// WiFi part
#include <WiFi.h>
#include <ESPmDNS.h>
#include <WebServer.h>

WebServer server(80);
void handle_OnConnect();
void wifiSetup_page();
void toggleIO_page();
void handle_NotFound();

bool wifiSetupPageVisited = false;
bool toggleIOpageVisited = false;

PCF8574 pcf8574(0x21);
ST77XX_FB lcd;

// the setup function runs once when you press reset or power the board
void setup()
{
	// Setup the IOs
	Serial.begin(115200);
	pcf8574.begin();
	pcf8574.pinMode(P6, OUTPUT); // Red LED
	pcf8574.pinMode(P7, OUTPUT); // Yellow LED
	pcf8574.digitalWrite(P7, HIGH);
	pcf8574.digitalWrite(P6, HIGH);

	if (!lcd.init())
	{
		printf("LCD init error\n");
		while (1);
	}

#define WIFI_TIMEOUT	20 // 10 seconds
	// WiFi Part
	WiFi.mode(WIFI_AP_STA);
	if (!WiFi.softAP("ESP32AP", "12345678"))
	{
		Serial.println("Failed to init WiFi AP");
	}
	else
	{
		Serial.println("IP address of AP is:");
		Serial.println((WiFi.softAPIP()));
	}
	if (!MDNS.begin("esp32"))
	{
		printf("Error setting up MDNS responder!\n");
	}
	else
		printf("mDNS responder started\n");
	MDNS.addService("http", "tcp", 80);
	// Web pages setup
	server.on("/", handle_OnConnect);
	server.on("/wifiSetupSelected", wifiSetup_page);
	server.on("/ioPageSelected", toggleIO_page); //toggleIO_page
	server.on("/handleTempHumidityPage", handleTempHumidity);//To get update of ADC Value only
	server.on("/readADC", handleADC);//To get update of ADC Value only
	server.onNotFound(handle_NotFound);
	server.begin();

	spiffsOK = false;
	if (SPIFFS.begin())
	{
		Serial.println("SPIFFS mounted");
		spiffsOK = true;
	}
	else
	{
		if (SPIFFS.format())
		{
			printf("SPIFFS formated\n");
			if (SPIFFS.begin())
			{
				printf("SPIFFS mounted\n");
				spiffsOK = true;
			}
		}
	}
	if (spiffsOK == false)
	{
		printf("SPIFFS error!\n");
	}
#ifdef DHT_PRESENT
	dht.begin();
#endif
}

// the loop function runs over and over again until power down or reset

String wifiName = "", wifiPass = "";

void loop()
{
	printf("Temp %f\n",dht.readHumidity()*1.57);
	printf("Hume %f\n", dht.readTemperature());

	
	int numOfClientsConnected;
	lcd.loadFonts(ORBITRON_LIGHT24);
	File file;
	char tempStr[32];
	bool wifiBeginCalled = false, IO3set = true;

	if (spiffsOK)
	{
		file = SPIFFS.open("/wifiSetup.txt", "r");
		if (file)
		{
			wifiName = file.readStringUntil('\n');
			printf("wifi name: %s\n", wifiName.c_str());
			wifiPass = file.readStringUntil('\n');
			printf("wifi pass: %s\n", wifiPass.c_str());
			file.close();
		}
	}

	/*if (wifiName != "")
	{
		connectToWiFiSTA(wifiName.c_str(), wifiPass.c_str());
	}*/

	while (1)
	{
		numOfClientsConnected = WiFi.softAPgetStationNum();
		// If WiFi is not connected and we have a user connected to our AP, trying to connect keeps changing the WiFi channel and will cause disconnectation of the user from our AP
		if ((numOfClientsConnected > 0) && (WiFi.status() != WL_CONNECTED))
		{
			WiFi.disconnect();
			delay(1000);
			wifiBeginCalled = false;
		}
		if ((numOfClientsConnected == 0) && (WiFi.status() != WL_CONNECTED))
		{
			// only once!!
			if (!wifiBeginCalled)
			{
				WiFi.begin(wifiName.c_str(), wifiPass.c_str());
				wifiBeginCalled = true;
			}
		}
		if ((numOfClientsConnected > 0) || (WiFi.status() == WL_CONNECTED))
		{
			server.handleClient();
			if (wifiSetupPageVisited)
			{
				if (server.args() >= 2)
				{ // Arguments were received
					String ssidName = server.arg(0);
					String ssidPassword = server.arg(1);

					Serial.println(server.args());
					Serial.println((const char*)ssidName.c_str());
					Serial.println((const char*)ssidPassword.c_str());
					wifiSetupPageVisited = false;
					if (spiffsOK)
					{
						if (ssidName != wifiName || ssidPassword != wifiPass)
						{
							file = SPIFFS.open("/wifiSetup.txt", "w");
							if (file)
							{
								sprintf(tempStr, "%s\n", ssidName.c_str());
								file.write((const unsigned char*)tempStr, strlen(tempStr));
								sprintf(tempStr, "%s\n", ssidPassword.c_str());
								file.write((const unsigned char*)tempStr, strlen(tempStr));

								file.flush();
								file.close();
								wifiName = ssidName;
								wifiPass = ssidPassword;
								WiFi.disconnect();
								delay(1000);
								WiFi.begin(wifiName.c_str(), wifiPass.c_str());
							}
						}
					}
				}
			}
			if (toggleIOpageVisited)
			{
				toggleIOpageVisited = false;
				if (server.args() > 0)
				{ // Arguments were received
					String ioState = server.arg(0);

					Serial.println(server.args());
					Serial.println((const char*)ioState.c_str());
					if (!strcmp(ioState.c_str(), "IO 1 ON"))
					{
						pcf8574.digitalWrite(P7, HIGH);
					}
					if (!strcmp(ioState.c_str(), "IO 1 OFF"))
					{
						pcf8574.digitalWrite(P7, LOW);
					}
					if (!strcmp(ioState.c_str(), "IO 2 ON"))
					{
						pcf8574.digitalWrite(P6, HIGH);
					}
					if (!strcmp(ioState.c_str(), "IO 2 OFF"))
					{
						pcf8574.digitalWrite(P6, LOW);
					}
					if (!strcmp(ioState.c_str(), "IO 3 ON"))
					{
						IO3set = true;
					}
					if (!strcmp(ioState.c_str(), "IO 3 OFF"))
					{
						IO3set = false;
					}
				}
			}
		}
		lcd.fillScr(0, 0, 0);
		lcd.setColor(255, 255, 255);
		if (IO3set)
		{
			lcd.print("IO3 ON", 0, 20, true);
		}
		else
		{
			lcd.print("IO3 OFF", 0, 20, true);
		}
		if (WiFi.status() == WL_CONNECTED)
		{
			lcd.print("WiFi Connected", 0, 60, true);
			lcd.print((char *)WiFi.localIP().toString().c_str(), 0, 100, true);
		}
		lcd.flushFB();
	}
}

#define WIFI_TIMEOUT	20 // 10 seconds
bool connectToWiFiSTA(const char * name, const char * pass)
{
	unsigned int timeOutCounter = 0;
	WiFi.begin(name, pass);
	while ((WiFi.status() != WL_CONNECTED) && (timeOutCounter < WIFI_TIMEOUT))
	{
		delay(500);
		Serial.print(".");
		timeOutCounter++;
	}
	if (timeOutCounter != WIFI_TIMEOUT)
	{
		printf("\nWiFi connected.\n");
		Serial.println(WiFi.localIP().toString().c_str());
		Serial.println(WiFi.macAddress());
		return true;
	}
	else
	{
		printf("WiFi: cannot connect to: %s\n", name);
		return false;
	}
}

const char mainMenuPage[]PROGMEM = R"=====(
<!DOCTYPE html> 
<html>
<head><meta name="viewport" content="width=device-width, initial-scale=1.0, user-scalable=no">
<title>ESP32 WebPage</title>
<style>
html { font-family: Helvetica; display: inline-block; margin: 0px auto; text-align: center;}
body{margin-top: 50px;} h1 {color: #444444;margin: 50px auto 30px;} h3 {color: #444444;margin-bottom: 50px;}
.button {display: block;width: 80px;background-color: #1abc9c;border: none;color: white;padding: 13px 30px;text-decoration: none;font-size: 25px;margin: 0px auto 35px;cursor: pointer;border-radius: 10px;}
.button-on {background-color: #1abc9c;}
.button-on:active {background-color: #16a085;}
p {font-size: 14px;color: #888;margin-bottom: 10px;}
</style>
</head>
<body>
<meta charset="utf-8">
<html lang="he">
<h1>Intel's Makers Demo Web Page</h1>
<a class="button button-on" href="/wifiSetupSelected">WiFi Setup</a>
<a class="button button-on" href="/ioPageSelected">IO Control</a>
<a class="button button-on" href="/handleTempHumidityPage">Temp</a>
</body>
<href="/">
</html>)=====";

const char setupWiFiHTMLpart1[]PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
  <meta name="viewport" content="width=device-width, initial-scale=1">
<style>
html {font-family: Arial; display: inline-block; text-align: center;}
.button {display: block;width: 80px;background-color: #1abc9c;border: none;color: white;padding: 13px 30px;text-decoration: none;font-size: 25px;margin: 0px auto 35px;cursor: pointer;border-radius: 10px;}
.boxStyle {  padding: 12px 20px;  margin: 8px 0;  box-sizing: border-box;  border: 2px solid red;  border-radius: 10px; font-size: 20px;text-align: center;}
</style>
</head>
<body>
<form action="/" method="POST">
Access Point Name:<br>
<input type="text" class="boxStyle" name="AccessPoint" value=
)rawliteral";
//""><br>
const char setupWiFiHTMLpart2[]PROGMEM = R"rawliteral(
Password:<br>
<input type="text" class="boxStyle" name="Password" value=
)rawliteral";
//""><br>
const char setupWiFiHTMLpart3[]PROGMEM = R"rawliteral(
<input type="submit" class="button" value="OK">
</form>
</body>
<href="/">
</html>
)rawliteral";

const char index_html[] PROGMEM = R"rawliteral(
<!DOCTYPE HTML>
<html>
<head>
  <meta name="viewport" content="width=device-width, initial-scale=1">
  <style>
    html {font-family: Arial; display: inline-block; text-align: center;}
    h2 {font-size: 3.0rem;}
    p {font-size: 3.0rem;}
    body {max-width: 600px; margin:0px auto; padding-bottom: 25px;}
    .switch {position: relative; display: inline-block; width: 120px; height: 68px} 
    .switch input {display: none}
    .slider {position: absolute; top: 0; left: 0; right: 0; bottom: 0; background-color: #ccc; border-radius: 34px}
    .slider:before {position: absolute; content: ""; height: 52px; width: 52px; left: 8px; bottom: 8px; background-color: #fff; -webkit-transition: .4s; transition: .4s; border-radius: 68px}
    input:checked+.slider {background-color: #2196F3}
    input:checked+.slider:before {-webkit-transform: translateX(52px); -ms-transform: translateX(52px); transform: translateX(52px)}
    .button {display: block;width: 80px;background-color: #1abc9c;border: none;color: white;padding: 13px 30px;text-decoration: none;font-size: 25px;margin: 0px auto 35px;cursor: pointer;border-radius: 10px;}
  </style>
</head>
<body>
<h2>ESP Web Server</h2>
<h4>IO #1</h4>
<label class="switch"><input type="checkbox" onchange="toggleCheckbox(this)" id="1" checked><span class="slider"></span></label>
<h4>IO #2</h4>
<label class="switch"><input type="checkbox" onchange="toggleCheckbox(this)" id="2" checked><span class="slider"></span></label>
<h4>IO #3</h4>
<label class="switch"><input type="checkbox" onchange="toggleCheckbox(this)" id="3" checked><span class="slider"></span></label>
<h4><a class="button button-on" href="/">Home</a></h4>
<script>function toggleCheckbox(element) {
	var request = new XMLHttpRequest();
	var strOn	= "IO " + element.id + " ON";
	var strOff	= "IO " + element.id + " OFF";
	if(element.checked){ request.open("POST", "", true); request.setRequestHeader("Content-type", " ");request.send(strOn);}
	else { request.open("POST", "", true); request.setRequestHeader("Content-type", " ");request.send(strOff);}
	die();
}</script>
</body>
<href="/">
</html>
)rawliteral";

void handle_OnConnect()
{
	toggleIOpageVisited = false;
	server.send(200, "text/html", mainMenuPage);
}

void wifiSetup_page()
{
	wifiSetupPageVisited = true;
	toggleIOpageVisited = false;
	String setupWiFiHTML = setupWiFiHTMLpart1;
	//""><br>
	setupWiFiHTML += "\"";
	setupWiFiHTML += wifiName;
	setupWiFiHTML += "\"><br>";
	setupWiFiHTML += setupWiFiHTMLpart2;
	setupWiFiHTML += "\"";
	setupWiFiHTML += wifiPass;
	setupWiFiHTML += "\"><br>";
	setupWiFiHTML += setupWiFiHTMLpart3;

	server.send(200, "text/html", setupWiFiHTML);
	printf("wifiSetupPageVisited\n");
}

void toggleIO_page()
{
	toggleIOpageVisited = true;
	wifiSetupPageVisited = false;
	server.send(200, "text/html", index_html);
	printf("toggleIOpageVisited\n");
}

void handle_NotFound()
{
	server.send(404, "text/plain", "Not found");
}

const char tempHumePage[] PROGMEM = R"=====(
<!DOCTYPE html>
<html>
    <head>
        <title>ESP32 Weather Station</title>
        <script>
        var data_val = 0;
        var data_val2 = 0;
<!-- Gauge Code Starts -->
var Gauge=function(b){function l(a,b){for(var c in b)"object"==typeof b[c]&&"[object Array]"!==Object.prototype.toString.call(b[c])&&"renderTo"!=c?("object"!=typeof a[c]&&(a[c]={}),l(a[c],b[c])):a[c]=b[c]}function q(){z.width=b.width;z.height=b.height;A=z.cloneNode(!0);B=A.getContext("2d");C=z.width;D=z.height;t=C/2;u=D/2;f=t<u?t:u;A.i8d=!1;B.translate(t,u);B.save();a.translate(t,u);a.save()}function v(a){var b=new Date;G=setInterval(function(){var c=(new Date-b)/a.duration;1<c&&(c=1);var f=("function"==
typeof a.delta?a.delta:M[a.delta])(c);a.step(f);1==c&&clearInterval(G)},a.delay||10)}function k(){G&&clearInterval(G);var a=I-n,h=n,c=b.animation;v({delay:c.delay,duration:c.duration,delta:c.fn,step:function(b){n=parseFloat(h)+a*b;E.draw()}})}function e(a){return a*Math.PI/180}function g(b,h,c){c=a.createLinearGradient(0,0,0,c);c.addColorStop(0,b);c.addColorStop(1,h);return c}function p(){var m=93*(f/100),h=f-m,c=91*(f/100),e=88*(f/100),d=85*(f/100);a.save();b.glow&&(a.shadowBlur=h,a.shadowColor=
"rgba(0, 0, 0, 0.5)");a.beginPath();a.arc(0,0,m,0,2*Math.PI,!0);a.fillStyle=g("#ddd","#aaa",m);a.fill();a.restore();a.beginPath();a.arc(0,0,c,0,2*Math.PI,!0);a.fillStyle=g("#fafafa","#ccc",c);a.fill();a.beginPath();a.arc(0,0,e,0,2*Math.PI,!0);a.fillStyle=g("#eee","#f0f0f0",e);a.fill();a.beginPath();a.arc(0,0,d,0,2*Math.PI,!0);a.fillStyle=b.colors.plate;a.fill();a.save()}function w(a){var h=!1;a=0===b.majorTicksFormat.dec?Math.round(a).toString():a.toFixed(b.majorTicksFormat.dec);return 1<b.majorTicksFormat["int"]?
(h=-1<a.indexOf("."),-1<a.indexOf("-")?"-"+(b.majorTicksFormat["int"]+b.majorTicksFormat.dec+2+(h?1:0)-a.length)+a.replace("-",""):""+(b.majorTicksFormat["int"]+b.majorTicksFormat.dec+1+(h?1:0)-a.length)+a):a}function d(){var m=81*(f/100);a.lineWidth=2;a.strokeStyle=b.colors.majorTicks;a.save();if(0===b.majorTicks.length){for(var h=(b.maxValue-b.minValue)/5,c=0;5>c;c++)b.majorTicks.push(w(b.minValue+h*c));b.majorTicks.push(w(b.maxValue))}for(c=0;c<b.majorTicks.length;++c)a.rotate(e(45+c*(270/(b.majorTicks.length-
1)))),a.beginPath(),a.moveTo(0,m),a.lineTo(0,m-15*(f/100)),a.stroke(),a.restore(),a.save();b.strokeTicks&&(a.rotate(e(90)),a.beginPath(),a.arc(0,0,m,e(45),e(315),!1),a.stroke(),a.restore(),a.save())}function J(){var m=81*(f/100);a.lineWidth=1;a.strokeStyle=b.colors.minorTicks;a.save();for(var h=b.minorTicks*(b.majorTicks.length-1),c=0;c<h;++c)a.rotate(e(45+c*(270/h))),a.beginPath(),a.moveTo(0,m),a.lineTo(0,m-7.5*(f/100)),a.stroke(),a.restore(),a.save()}function s(){for(var m=55*(f/100),h=0;h<b.majorTicks.length;++h){var c=
F(m,e(45+h*(270/(b.majorTicks.length-1))));a.font=20*(f/200)+"px Arial";a.fillStyle=b.colors.numbers;a.lineWidth=0;a.textAlign="center";a.fillText(b.majorTicks[h],c.x,c.y+3)}}function x(a){var h=b.valueFormat.dec,c=b.valueFormat["int"];a=parseFloat(a);var f=0>a;a=Math.abs(a);if(0<h){a=a.toFixed(h).toString().split(".");h=0;for(c-=a[0].length;h<c;++h)a[0]="0"+a[0];a=(f?"-":"")+a[0]+"."+a[1]}else{a=Math.round(a).toString();h=0;for(c-=a.length;h<c;++h)a="0"+a;a=(f?"-":"")+a}return a}function F(a,b){var c=
Math.sin(b),f=Math.cos(b);return{x:0*f-a*c,y:0*c+a*f}}function N(){a.save();for(var m=81*(f/100),h=m-15*(f/100),c=0,g=b.highlights.length;c<g;c++){var d=b.highlights[c],r=(b.maxValue-b.minValue)/270,k=e(45+(d.from-b.minValue)/r),r=e(45+(d.to-b.minValue)/r);a.beginPath();a.rotate(e(90));a.arc(0,0,m,k,r,!1);a.restore();a.save();var l=F(h,k),p=F(m,k);a.moveTo(l.x,l.y);a.lineTo(p.x,p.y);var p=F(m,r),n=F(h,r);a.lineTo(p.x,p.y);a.lineTo(n.x,n.y);a.lineTo(l.x,l.y);a.closePath();a.fillStyle=d.color;a.fill();
a.beginPath();a.rotate(e(90));a.arc(0,0,h,k-0.2,r+0.2,!1);a.restore();a.closePath();a.fillStyle=b.colors.plate;a.fill();a.save()}}function K(){var m=12*(f/100),h=8*(f/100),c=77*(f/100),d=20*(f/100),k=4*(f/100),r=2*(f/100),l=function(){a.shadowOffsetX=2;a.shadowOffsetY=2;a.shadowBlur=10;a.shadowColor="rgba(188, 143, 143, 0.45)"};l();a.save();n=0>n?Math.abs(b.minValue-n):0<b.minValue?n-b.minValue:Math.abs(b.minValue)+n;a.rotate(e(45+n/((b.maxValue-b.minValue)/270)));a.beginPath();a.moveTo(-r,-d);a.lineTo(-k,
0);a.lineTo(-1,c);a.lineTo(1,c);a.lineTo(k,0);a.lineTo(r,-d);a.closePath();a.fillStyle=g(b.colors.needle.start,b.colors.needle.end,c-d);a.fill();a.beginPath();a.lineTo(-0.5,c);a.lineTo(-1,c);a.lineTo(-k,0);a.lineTo(-r,-d);a.lineTo(r/2-2,-d);a.closePath();a.fillStyle="rgba(255, 255, 255, 0.2)";a.fill();a.restore();l();a.beginPath();a.arc(0,0,m,0,2*Math.PI,!0);a.fillStyle=g("#f0f0f0","#ccc",m);a.fill();a.restore();a.beginPath();a.arc(0,0,h,0,2*Math.PI,!0);a.fillStyle=g("#e8e8e8","#f5f5f5",h);a.fill()}
function L(){a.save();a.font=40*(f/200)+"px Led";var b=x(y),h=a.measureText("-"+x(0)).width,c=f-33*(f/100),g=0.12*f;a.save();var d=-h/2-0.025*f,e=c-g-0.04*f,h=h+0.05*f,g=g+0.07*f,k=0.025*f;a.beginPath();a.moveTo(d+k,e);a.lineTo(d+h-k,e);a.quadraticCurveTo(d+h,e,d+h,e+k);a.lineTo(d+h,e+g-k);a.quadraticCurveTo(d+h,e+g,d+h-k,e+g);a.lineTo(d+k,e+g);a.quadraticCurveTo(d,e+g,d,e+g-k);a.lineTo(d,e+k);a.quadraticCurveTo(d,e,d+k,e);a.closePath();d=a.createRadialGradient(0,c-0.12*f-0.025*f+(0.12*f+0.045*f)/
2,f/10,0,c-0.12*f-0.025*f+(0.12*f+0.045*f)/2,f/5);d.addColorStop(0,"#888");d.addColorStop(1,"#666");a.strokeStyle=d;a.lineWidth=0.05*f;a.stroke();a.shadowBlur=0.012*f;a.shadowColor="rgba(0, 0, 0, 1)";a.fillStyle="#babab2";a.fill();a.restore();a.shadowOffsetX=0.004*f;a.shadowOffsetY=0.004*f;a.shadowBlur=0.012*f;a.shadowColor="rgba(0, 0, 0, 0.3)";a.fillStyle="#444";a.textAlign="center";a.fillText(b,-0,c);a.restore()}Gauge.Collection.push(this);this.config={renderTo:null,width:200,height:200,title:!1,
maxValue:100,minValue:0,majorTicks:[],minorTicks:10,strokeTicks:!0,units:!1,valueFormat:{"int":2,dec:2},majorTicksFormat:{"int":1,dec:0},glow:!0,animation:{delay:10,duration:250,fn:"cycle"},colors:{plate:"#fff",majorTicks:"#444",minorTicks:"#666",title:"#888",units:"#888",numbers:"#444",needle:{start:"rgba(240, 128, 128, 1)",end:"rgba(255, 160, 122, .9)"}},highlights:[{from:20,to:60,color:"#eee"},{from:60,to:80,color:"#ccc"},{from:80,to:100,color:"#999"}]};var y=0,E=this,n=0,I=0,H=!1;this.setValue=
function(a){n=b.animation?y:a;var d=(b.maxValue-b.minValue)/100;I=a>b.maxValue?b.maxValue+d:a<b.minValue?b.minValue-d:a;y=a;b.animation?k():this.draw();return this};this.setRawValue=function(a){n=y=a;this.draw();return this};this.clear=function(){y=n=I=this.config.minValue;this.draw();return this};this.getValue=function(){return y};this.onready=function(){};l(this.config,b);this.config.minValue=parseFloat(this.config.minValue);this.config.maxValue=parseFloat(this.config.maxValue);b=this.config;n=
y=b.minValue;if(!b.renderTo)throw Error("Canvas element was not specified when creating the Gauge object!");var z=b.renderTo.tagName?b.renderTo:document.getElementById(b.renderTo),a=z.getContext("2d"),A,C,D,t,u,f,B;q();this.updateConfig=function(a){l(this.config,a);q();this.draw();return this};var M={linear:function(a){return a},quad:function(a){return Math.pow(a,2)},quint:function(a){return Math.pow(a,5)},cycle:function(a){return 1-Math.sin(Math.acos(a))},bounce:function(a){a:{a=1-a;for(var b=0,
c=1;;b+=c,c/=2)if(a>=(7-4*b)/11){a=-Math.pow((11-6*b-11*a)/4,2)+Math.pow(c,2);break a}a=void 0}return 1-a},elastic:function(a){a=1-a;return 1-Math.pow(2,10*(a-1))*Math.cos(30*Math.PI/3*a)}},G=null;a.lineCap="round";this.draw=function(){if(!A.i8d){B.clearRect(-t,-u,C,D);B.save();var g={ctx:a};a=B;p();N();J();d();s();b.title&&(a.save(),a.font=24*(f/200)+"px Arial",a.fillStyle=b.colors.title,a.textAlign="center",a.fillText(b.title,0,-f/4.25),a.restore());b.units&&(a.save(),a.font=22*(f/200)+"px Arial",
a.fillStyle=b.colors.units,a.textAlign="center",a.fillText(b.units,0,f/3.25),a.restore());A.i8d=!0;a=g.ctx;delete g.ctx}a.clearRect(-t,-u,C,D);a.save();a.drawImage(A,-t,-u,C,D);if(Gauge.initialized)L(),K(),H||(E.onready&&E.onready(),H=!0);else var e=setInterval(function(){Gauge.initialized&&(clearInterval(e),L(),K(),H||(E.onready&&E.onready(),H=!0))},10);return this}};Gauge.initialized=!1;
(function(){var b=document,l=b.getElementsByTagName("head")[0],q=-1!=navigator.userAgent.toLocaleLowerCase().indexOf("msie"),v="@font-face {font-family: 'Led';src: url('fonts/digital-7-mono."+(q?"eot":"ttf")+"');}",k=b.createElement("style");k.type="text/css";if(q)l.appendChild(k),l=k.styleSheet,l.cssText=v;else{try{k.appendChild(b.createTextNode(v))}catch(e){k.cssText=v}l.appendChild(k);l=k.styleSheet?k.styleSheet:k.sheet||b.styleSheets[b.styleSheets.length-1]}var g=setInterval(function(){if(b.body){clearInterval(g);
var e=b.createElement("div");e.style.fontFamily="Led";e.style.position="absolute";e.style.height=e.style.width=0;e.style.overflow="hidden";e.innerHTML=".";b.body.appendChild(e);setTimeout(function(){Gauge.initialized=!0;e.parentNode.removeChild(e)},250)}},1)})();Gauge.Collection=[];
Gauge.Collection.get=function(b){if("string"==typeof b)for(var l=0,q=this.length;l<q;l++){if((this[l].config.renderTo.tagName?this[l].config.renderTo:document.getElementById(this[l].config.renderTo)).getAttribute("id")==b)return this[l]}else return"number"==typeof b?this[b]:null};function domReady(b){window.addEventListener?window.addEventListener("DOMContentLoaded",b,!1):window.attachEvent("onload",b)}
domReady(function(){function b(b){for(var e=b[0],d=1,g=b.length;d<g;d++)e+=b[d].substr(0,1).toUpperCase()+b[d].substr(1,b[d].length-1);return e}for(var l=document.getElementsByTagName("canvas"),q=0,v=l.length;q<v;q++)if("canv-gauge"==l[q].getAttribute("data-type")){var k=l[q],e={},g,p=parseInt(k.getAttribute("width"),10),w=parseInt(k.getAttribute("height"),10);e.renderTo=k;p&&(e.width=p);w&&(e.height=w);p=0;for(w=k.attributes.length;p<w;p++)if(g=k.attributes.item(p).nodeName,"data-type"!=g&&"data-"==
g.substr(0,5)){var d=g.substr(5,g.length-5).toLowerCase().split("-");if(g=k.getAttribute(g))switch(d[0]){case "colors":d[1]&&(e.colors||(e.colors={}),"needle"==d[1]?(d=g.split(/\s+/),e.colors.needle=d[0]&&d[1]?{start:d[0],end:d[1]}:g):(d.shift(),e.colors[b(d)]=g));break;case "highlights":e.highlights||(e.highlights=[]);g=g.match(/(?:(?:-?\d*\.)?(-?\d+){1,2} ){2}(?:(?:#|0x)?(?:[0-9A-F|a-f]){3,8}|rgba?\(.*?\))/g);for(var d=0,J=g.length;d<J;d++){var s=g[d].replace(/^\s+|\s+$/g,"").split(/\s+/),x={};
s[0]&&""!=s[0]&&(x.from=s[0]);s[1]&&""!=s[1]&&(x.to=s[1]);s[2]&&""!=s[2]&&(x.color=s[2]);e.highlights.push(x)}break;case "animation":d[1]&&(e.animation||(e.animation={}),"fn"==d[1]&&/^\s*function\s*\(/.test(g)&&(g=eval("("+g+")")),e.animation[d[1]]=g);break;default:d=b(d);if("onready"==d)continue;if("majorTicks"==d)g=g.split(/\s+/);else if("strokeTicks"==d||"glow"==d)g="true"==g?!0:!1;else if("valueFormat"==d)if(g=g.split("."),2==g.length)g={"int":parseInt(g[0],10),dec:parseInt(g[1],10)};else continue;
e[d]=g}}e=new Gauge(e);k.getAttribute("data-value")&&e.setRawValue(parseFloat(k.getAttribute("data-value")));k.getAttribute("data-onready")&&(e.onready=function(){eval(this.config.renderTo.getAttribute("data-onready"))});e.draw()}});window.Gauge=Gauge;
<!-- Gauge Code Ends -->
        
      function DisplayCurrentTime() {
          var dt = new Date();
  var weekday = new Array(7);
  weekday[0] = "Sunday";
  weekday[1] = "Monday";
  weekday[2] = "Tuesday";
  weekday[3] = "Wednesday";
  weekday[4] = "Thursday";
  weekday[5] = "Friday";
  weekday[6] = "Saturday";
  var dow = weekday[dt.getDay()];
document.getElementById("datetime").innerHTML = (dow) +" "+ (dt.toLocaleString());
        setTimeout('DisplayCurrentTime()', 1000);
      }
        function GetArduinoInputs()
        {
          var xhttp = new XMLHttpRequest();
  xhttp.onreadystatechange = function() {
    if (this.readyState == 4 && this.status == 200) {
      
                            data_val  = this.responseXML.getElementsByTagName('analog')[0].childNodes[0].nodeValue;
                            data_val2 = this.responseXML.getElementsByTagName('analog')[1].childNodes[0].nodeValue;
    }
  };
  xhttp.open("GET", "readADC", true);
  xhttp.send();
            setTimeout('GetArduinoInputs()', 3000);
            
        }
      document.addEventListener('DOMContentLoaded', function() {
        DisplayCurrentTime(),GetArduinoInputs();
      }, false);
    </script>
      <style>
         body {
        text-align: center;
             margin: 5;
             padding: 5;
             background-color: rgba(72,72,72,0.4);
         }
         .text {
         background-color: #ff0000;
         font-size:76px;
         color: #ffff99;
         }
      table {
        border: 2px solid #ff00ff;
        background-color: #ffffff;
        width:100%;
         color: #0000ff;
         -moz-border-radius: 7px;
         -webkit-border-radius: 7px;
      }
      td {
        border: 2px solid #ff0000;
        padding: 16px;
         -moz-border-radius: 7px;
         -webkit-border-radius: 7px;
      }
   </style>
    </head>
    <body>
   <b style= "color: #ff0000;font-size:35px">ESP32 Weather Station</b>
    <table>
   <tr>
        <td style="width:48%"><canvas id="an_gauge_1" data-title="Temperature" data-units="&deg;C" width="400" height="500" data-major-ticks="-20 -10 0 10 20 30 40 50 60 70" data-type="canv-gauge" data-min-value="-20" data-max-value="70" data-highlights="-20 0 #4D89F2, 0 10 #25B8D9, 10 30 #0BB950, 30 40 #cc5, 40 70 #f33" data-onready="setInterval( function() { Gauge.Collection.get('an_gauge_1').setValue(data_val);}, 200);"></canvas></td>
        <td style="width:48%"><canvas id="an_gauge_2" data-title="Humidity" data-units="%RH" width="400" height="500" data-major-ticks="0 10 20 30 40 50 60 70 80 90 100" data-type="canv-gauge" data-min-value="0" data-max-value="100" data-highlights="0 10 #f33, 10 30 #cc5, 30 80 #0BB950, 80 100 #4D89F2" data-onready="setInterval( function() { Gauge.Collection.get('an_gauge_2').setValue(data_val2);}, 200);"></canvas></td>
  </tr>
    </table>
   <b style= "color: #ff0000;font-size:35px"><span id="datetime"></span></b>
    </body>
</html>
)=====";

void handleTempHumidity() 
{
	server.send(200, "text/html", tempHumePage); //Send web page
}

void handleADC() 
{
	// send XML file containing input states
#ifdef DHT_PRESENT
	float h = dht.readHumidity();
	float t = dht.readTemperature();
#else
	float h = 0;
	float t = 0;
#endif
	String content = "<?xml version = \"1.0\" ?>";
	content += "<inputs>";
	content += "<analog>";
	content += t;
	content += "</analog>";
	content += "<analog>";
	content += h;
	content += "</analog>";
	content += "</inputs>";
	server.sendHeader("Cache-Control", "no-cache");
	server.send(200, "text/xml", content); //Send web page
}
