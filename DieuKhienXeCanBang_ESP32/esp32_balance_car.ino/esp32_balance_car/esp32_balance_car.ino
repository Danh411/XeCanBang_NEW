// ESP32 DevKit v1: cau noi wifi <-> STM32 cho xe tu can bang
//
// - ESP32 phat wifi rieng (AP): ten "BalanceCar", mat khau "12345678"
// - Dien thoai mo trinh duyet vao http://192.168.4.1 (giao dien dieu khien)
// - Trang web <-> ESP32 qua WebSocket (cong 81), ESP32 <-> STM32 qua UART2
//
// Day noi:  ESP32 GPIO17 (TX2) -> STM32 PA3 (USART2_RX)
//           ESP32 GPIO16 (RX2) <- STM32 PA2 (USART2_TX)
//           GND chung. ESP32 cap 5V rieng vao chan VIN.
//
// Thu vien can cai (Library Manager): "WebSockets" cua Markus Sattler (arduinoWebSockets)
// Board: ESP32 Dev Module

#include <WiFi.h>
#include <WebServer.h>
#include <WebSocketsServer.h>
#include <Preferences.h>

#define STM_RX_PIN 16
#define STM_TX_PIN 17

uint32_t rxBytes = 0, lastPrint = 0;

WebServer server(80);
WebSocketsServer ws(81);
Preferences prefs;

String cfg[3];                              // dong lenh G, V, Y gan nhat
const char *cfgKey[3] = {"G", "V", "Y"};
bool bootSent = false;
String rxLine;

const char PAGE[] PROGMEM = R"rawliteral(
<!doctype html><html><head><meta charset="utf-8">
<meta name="viewport" content="width=device-width,initial-scale=1,user-scalable=no">
<title>Balance Car</title>
<style>
body{font-family:sans-serif;margin:0;padding:12px;background:#111;color:#eee}
h3{margin:0 0 8px}
.row{display:flex;gap:8px;align-items:center;margin:4px 0}
.row label{width:90px;font-size:13px}
.row input[type=range]{flex:1}
.row span{width:56px;text-align:right;font-size:13px}
button{padding:12px 18px;font-size:16px;border:0;border-radius:8px;background:#2a6;color:#fff;margin-right:8px}
button.off{background:#c33}
#pad{position:relative;width:220px;height:220px;border-radius:50%;background:#222;border:2px solid #555;touch-action:none;margin:12px auto}
#knob{position:absolute;left:80px;top:80px;width:60px;height:60px;border-radius:50%;background:#2a6;opacity:.85;pointer-events:none}
canvas{width:100%;background:#000;border-radius:6px}
fieldset{border:1px solid #444;border-radius:8px;margin:10px 0}
#tel{font-family:monospace;margin:6px 0}
</style></head><body>
<h3>Balance Car <small id="conn">...</small></h3>
<div id="tel">-</div>
<canvas id="cv" width="320" height="100"></canvas>
<div style="margin:10px 0"><button id="arm">BAT CAN BANG</button><button id="save">LUU PID</button></div>
<div id="pad"><div id="knob"></div></div>
<div id="params"></div>
<script>
var ws, armState=0, VMAX=0.3, TMAX=80, vx=0, vy=0, touching=false, hist=[];
var G={
 G:{name:'Vong goc',p:[['Kp',0,0.2,0.001,0.076],['Ki',0,1,0.01,0],['Kd',0,0.01,0.0001,0.0017],['AngleSet',-5,5,0.05,0.22]]},
 V:{name:'Vong toc do',p:[['Kp',0,20,0.25,4],['Ki',0,5,0.05,2],['Limit',0,10,0.5,4]]},
 Y:{name:'Vong re',p:[['Kp',0,0.01,0.0001,0.002],['Ki',0,0.1,0.001,0.01]]}
};
function send(s){ if(ws && ws.readyState==1) ws.send(s); }
function groupLine(k){
  return k+' '+G[k].p.map(function(q,i){return document.getElementById(k+i).value;}).join(' ');
}
var host=document.getElementById('params');
Object.keys(G).forEach(function(k){
  var fs=document.createElement('fieldset'), lg=document.createElement('legend');
  lg.textContent=G[k].name; fs.appendChild(lg);
  G[k].p.forEach(function(q,i){
    var r=document.createElement('div'); r.className='row';
    r.innerHTML='<label>'+q[0]+'</label><input type="range" id="'+k+i+'" min="'+q[1]+'" max="'+q[2]+'" step="'+q[3]+'" value="'+q[4]+'"><span id="'+k+i+'v">'+q[4]+'</span>';
    fs.appendChild(r);
  });
  host.appendChild(fs);
});
Object.keys(G).forEach(function(k){
  G[k].p.forEach(function(q,i){
    var el=document.getElementById(k+i);
    el.addEventListener('input',function(){
      document.getElementById(k+i+'v').textContent=el.value;
      send(groupLine(k));
    });
  });
});
var cv=document.getElementById('cv'), cx=cv.getContext('2d');
function draw(){
  cx.clearRect(0,0,320,100);
  cx.strokeStyle='#444'; cx.beginPath(); cx.moveTo(0,50); cx.lineTo(320,50); cx.stroke();
  cx.strokeStyle='#0f0'; cx.beginPath();
  hist.forEach(function(y,i){ var py=50-y*50/30; if(i==0) cx.moveTo(i*2,py); else cx.lineTo(i*2,py); });
  cx.stroke();
}
function connect(){
  ws=new WebSocket('ws://'+location.hostname+':81/');
  ws.onopen=function(){document.getElementById('conn').textContent='(da ket noi)';};
  ws.onclose=function(){document.getElementById('conn').textContent='(mat ket noi)'; setTimeout(connect,1000);};
  ws.onmessage=function(ev){
    var a=ev.data.trim().split(' ');
    if(a[0]=='T'){
      var ang=a[1]/100, v=a[2]/1000, st=+a[3], u=a[4]/1000; armState=+a[5];
      document.getElementById('tel').textContent='goc '+ang.toFixed(2)+'  v '+v.toFixed(2)+' m/s  u '+u.toFixed(2)+'  '+['CHO','CAN BANG','NGA'][st];
      var b=document.getElementById('arm');
      b.textContent=armState?'TAT':'BAT CAN BANG'; b.className=armState?'off':'';
      hist.push(ang); if(hist.length>160) hist.shift(); draw();
    } else if(a[0]=='P' && G[a[1]]){
      var k=a[1];
      G[k].p.forEach(function(q,i){
        document.getElementById(k+i).value=a[2+i];
        document.getElementById(k+i+'v').textContent=a[2+i];
      });
    }
  };
}
connect();
document.getElementById('arm').onclick=function(){ send('A '+(armState?0:1)); };
document.getElementById('save').onclick=function(){ send('SAVE'); };
var pad=document.getElementById('pad');
function setFrom(e){
  var r=pad.getBoundingClientRect(), t=e.touches?e.touches[0]:e;
  var x=(t.clientX-r.left)/r.width*2-1, y=(t.clientY-r.top)/r.height*2-1;
  vx=Math.max(-1,Math.min(1,x)); vy=-Math.max(-1,Math.min(1,y));
  moveKnob();
}
var knob=document.getElementById('knob');
function moveKnob(){ knob.style.left=(80+vx*80)+'px'; knob.style.top=(80-vy*80)+'px'; }
function release(){ touching=false; vx=0; vy=0; moveKnob(); }
pad.addEventListener('touchstart',function(e){touching=true;setFrom(e);e.preventDefault();},{passive:false});
pad.addEventListener('touchmove',function(e){setFrom(e);e.preventDefault();},{passive:false});
pad.addEventListener('touchend',release);
pad.addEventListener('touchcancel',release);
pad.addEventListener('mousedown',function(e){touching=true;setFrom(e);});
window.addEventListener('mousemove',function(e){if(touching)setFrom(e);});
window.addEventListener('mouseup',release);
setInterval(function(){
  send('C '+(vy*VMAX).toFixed(2)+' '+(vx*TMAX).toFixed(0));
},50);
</script></body></html>
)rawliteral";

void toStm(const String &s) {
  Serial2.print(s);
  Serial2.print('\n');
}

int cfgIndex(char c) {
  return c == 'G' ? 0 : c == 'V' ? 1 : c == 'Y' ? 2 : -1;
}

void onWs(uint8_t num, WStype_t type, uint8_t *payload, size_t len) {
  if (type == WStype_CONNECTED) {
    for (int i = 0; i < 3; i++) {
      if (cfg[i].length()) {
        String m = "P " + cfg[i];
        ws.sendTXT(num, m);
      }
    }
  } else if (type == WStype_DISCONNECTED) {
    toStm("C 0 0");                       // mat dieu khien -> dung
  } else if (type == WStype_TEXT) {
    String s((const char *)payload, len);
    s.trim();
    if (s.length() == 0) return;
    if (s == "SAVE") {
      for (int i = 0; i < 3; i++)
        if (cfg[i].length()) prefs.putString(cfgKey[i], cfg[i]);
      return;
    }
    int i = cfgIndex(s[0]);
    if (i >= 0) cfg[i] = s;
    if (s[0] == 'C' || s[0] == 'A' || i >= 0) toStm(s);
  }
}

void setup() {
  Serial.begin(115200);
  Serial2.begin(115200, SERIAL_8N1, STM_RX_PIN, STM_TX_PIN);

  prefs.begin("car", false);
  for (int i = 0; i < 3; i++) cfg[i] = prefs.getString(cfgKey[i], "");

  WiFi.mode(WIFI_AP);
  WiFi.softAP("BalanceCar", "12345678");

  server.on("/", []() { server.send_P(200, "text/html", PAGE); });
  server.begin();

  ws.begin();
  ws.onEvent(onWs);
}

void loop() {
  server.handleClient();
  ws.loop();

  // Telemetry tu STM32 -> tat ca trinh duyet dang ket noi
  while (Serial2.available()) {
    char c = Serial2.read();
    rxBytes++;
    if (c == '\n') {
      if (rxLine.length()) ws.broadcastTXT(rxLine.c_str());
      rxLine = "";
    } else if (c != '\r' && rxLine.length() < 80) {
      rxLine += c;
    }
  }

  // Sau khi khoi dong 5 giay, nap lai PID da luu xuong STM32
  if (!bootSent && millis() > 5000) {
    for (int i = 0; i < 3; i++)
      if (cfg[i].length()) toStm(cfg[i]);
    bootSent = true;
  }

  if (millis() - lastPrint > 1000) {
    lastPrint = millis();
    Serial.printf("rxBytes=%u\n", rxBytes);
  }
}