#include <Arduino.h>
#include <Wire.h>
#include <WiFi.h>
#include <WebServer.h>
#include <time.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SH110X.h>
Adafruit_SH1106G screen(128,64,&Wire,-1,100000,100000);
WebServer server(80);
bool ready=false, dirty=true;
int previous=-1, mode=0, expression=0;
String message="Hello!", lastClock="";
const char page[] PROGMEM=R"HTML(
<!doctype html><html><head><meta name="viewport" content="width=device-width,initial-scale=1"><title>Desk Buddy</title><style>body{font:18px system-ui;background:#152030;color:white;max-width:480px;margin:30px auto;padding:20px}button,input,select{font:inherit;padding:12px;margin:6px 0;border-radius:10px;max-width:100%;box-sizing:border-box}button{background:#a3e7d0;border:0}label{display:block;margin-top:20px}#status{min-height:24px}</style></head><body><h1>Desk Buddy</h1><p>Choose what your buddy shows.</p><button onclick="send('mode=0')">Face</button> <button onclick="clock()">Clock</button> <button onclick="send('mode=2')">Message</button><label>Expression <select onchange="send('expression='+this.value)"><option value="0">Animated</option><option value="1">Eyes closed</option><option value="2">Look left</option><option value="3">Look right</option></select></label><label>Message (up to 80 characters)</label><input id="msg" maxlength="80" placeholder="Hello buddy!"><button onclick="send('mode=2&text='+encodeURIComponent(document.getElementById('msg').value))">Show message</button><label>Brightness <input type="range" min="0" max="255" value="127" onchange="send('brightness='+this.value)"></label><p id="status"></p><p>This is a local control panel, not AI chat. Clock time comes from your phone or computer.</p><script>async function send(q){try{let r=await fetch('/control',{method:'POST',headers:{'Content-Type':'application/x-www-form-urlencoded'},body:q});if(!r.ok)throw Error();document.getElementById('status').textContent='Updated!'}catch(e){document.getElementById('status').textContent='Connection lost. Reconnect to DeskBuddy Wi-Fi.'}}function clock(){send('mode=1&epoch='+Math.floor(Date.now()/1000)+'&offset='+(-new Date().getTimezoneOffset()*60))}</script></body></html>
)HTML";
void drawFace(int pose) {
  screen.clearDisplay();
  screen.setTextSize(1);
  screen.setTextColor(SH110X_WHITE);
  screen.setCursor(34,2);
  screen.print("DESK BUDDY");
  int shift=pose==2 ? -5 : pose==3 ? 5 : 0;
  if(pose==1) {
    screen.fillRoundRect(25,29,25,4,2,SH110X_WHITE);
    screen.fillRoundRect(78,29,25,4,2,SH110X_WHITE);
  } else {
    screen.fillRoundRect(25+shift,20,25,23,7,SH110X_WHITE);
    screen.fillRoundRect(78+shift,20,25,23,7,SH110X_WHITE);
  }
  screen.drawLine(55,49,60,53,SH110X_WHITE);
  screen.drawLine(60,53,68,53,SH110X_WHITE);
  screen.drawLine(68,53,73,49,SH110X_WHITE);
  screen.display();
}

void drawOther() {
  screen.clearDisplay(); screen.setTextColor(SH110X_WHITE); screen.setTextWrap(true);
  if(mode==2){screen.setTextSize(1);screen.setCursor(0,0);screen.println("YOUR MESSAGE");screen.setCursor(0,16);screen.print(message);}
  else {
    time_t now=time(nullptr); struct tm t; gmtime_r(&now,&t);
    screen.setTextSize(1);screen.setCursor(34,0);screen.print("DESK CLOCK");
    if(now<1700000000){screen.setCursor(0,24);screen.print("Open panel and tap");screen.setCursor(0,36);screen.print("Clock to set time.");}
    else {char b[24];strftime(b,sizeof(b),"%H:%M:%S",&t);screen.setTextSize(2);screen.setCursor(16,22);screen.print(b);strftime(b,sizeof(b),"%Y-%m-%d",&t);screen.setTextSize(1);screen.setCursor(34,50);screen.print(b);}
  }
  screen.display();
}
void setup(){
 Serial.begin(115200); Wire.begin(21,22); Wire.setClock(100000);
 uint8_t address=0; for(uint8_t a : {0x3C,0x3D}){Wire.beginTransmission(a);if(Wire.endTransmission()==0){address=a;break;}}
 if(address) ready=screen.begin(address,true);
 Serial.println(ready?"Desk buddy display ready.":"OLED not found or initialization failed.");
 WiFi.mode(WIFI_AP); bool ap=WiFi.softAP("DeskBuddy","buddy1234");
 Serial.println(ap?"Connect to Wi-Fi DeskBuddy (password buddy1234)":"Wi-Fi access point failed");
 Serial.print("Open http://");Serial.println(WiFi.softAPIP());
 server.on("/",HTTP_GET,[]{server.send_P(200,"text/html",page);});
 server.on("/control",HTTP_POST,[]{
   if(server.hasArg("mode"))mode=constrain(server.arg("mode").toInt(),0,2);
   if(server.hasArg("expression"))expression=constrain(server.arg("expression").toInt(),0,3);
   if(server.hasArg("text")){message=server.arg("text").substring(0,80);for(unsigned int i=0;i<message.length();i++)if((uint8_t)message[i]<32||(uint8_t)message[i]>126)message.setCharAt(i,'?');}
   if(server.hasArg("epoch")) {long long epoch=atoll(server.arg("epoch").c_str());long offset=constrain(server.arg("offset").toInt(),-50400,50400);if(epoch>=1700000000LL&&epoch<4102444800LL){struct timeval tv;tv.tv_sec=epoch+offset;tv.tv_usec=0;settimeofday(&tv,nullptr);}}
   if(server.hasArg("brightness")&&ready){screen.oled_command(0x81);screen.oled_command(constrain(server.arg("brightness").toInt(),0,255));}
   dirty=true;server.send(200,"text/plain","OK");
 });
 server.onNotFound([]{server.send(404,"text/plain","Not found");});server.begin();
}
void loop(){
 server.handleClient();
 if(ready){
 if(mode==0){uint32_t phase=millis()%6000;int pose=expression?expression:phase>=5500&&phase<5650?1:phase>=3500&&phase<4500?2:phase>=4500&&phase<5500?3:0;if(dirty||pose!=previous){drawFace(pose);previous=pose;dirty=false;}}
 else if(mode==1){String stamp=String((long long)time(nullptr));if(dirty||stamp!=lastClock){drawOther();lastClock=stamp;dirty=false;}}
 else if(dirty){drawOther();dirty=false;}
 }delay(2);
}