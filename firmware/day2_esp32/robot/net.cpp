#include "net.h"
#include "config.h"

#if SIM
void netBegin() {}
void netLoop() {}
#else
#include <WiFi.h>
#include <WebServer.h>

static WebServer web(80);

static const char PAGE[] PROGMEM = R"html(<!doctype html><html lang="uk"><head><meta charset="utf-8">
<meta name="viewport" content="width=device-width,initial-scale=1"><title>sosny · робот</title>
<style>
body{font-family:system-ui,sans-serif;margin:0;padding:16px;max-width:460px;background:#f4f1e8;color:#12261e}
h2{margin:0 0 8px}.st{font-size:30px;font-weight:800}.bar{height:22px;background:#ddd;border-radius:6px;overflow:hidden;margin:8px 0}
.bar div{height:100%;background:#2e7d32;transition:width .3s}.g{display:grid;grid-template-columns:1fr 1fr;gap:6px;margin:10px 0}
.c{background:#fff;border-radius:8px;padding:8px}.c b{display:block;font-size:22px}.c span{font-size:12px;color:#555}
button{font-size:20px;padding:14px;border:0;border-radius:8px;width:48%;margin:2px 0}
.s{display:flex;gap:4px;margin:6px 0}.s div{flex:1;background:#ddd;height:36px;border-radius:4px;position:relative;font-size:11px;text-align:center}
.s i{position:absolute;bottom:0;left:0;right:0;background:#12261e}.s em{position:relative;color:#c00;font-style:normal}
ol{padding-left:18px;font-size:13px;margin:4px 0}.ESTOP,.LOST{color:#c62828}.BYPASS{color:#ef6c00}.DONE,.ARRIVED{color:#2e7d32}
</style></head><body>
<h2>sosny · робот-транспортер</h2>
<div class="st" id="st">—</div><div class="bar"><div id="pb" style="width:0"></div></div>
<div class="g">
<div class="c"><b id="pc">0 %</b><span>маршрут</span></div><div class="c"><b id="pa">0 см</b><span>пройдено</span></div>
<div class="c"><b id="cr">0/0</b><span>поперечні лінії</span></div><div class="c"><b id="ds">—</b><span>до перешкоди</span></div>
<div class="c"><b id="ob">0 / 0</b><span>перешкод / об'їздів</span></div><div class="c"><b id="bt">—</b><span>АКБ · час</span></div>
</div>
<button onclick="go('/start')" style="background:#c8e6c9">▶ Старт</button>
<button onclick="go('/stop')" style="background:#ffcdd2">■ СТОП</button>
<p>Зупинитись на лінії №
<input id="n" type="number" min="1" max="50" style="width:60px;font-size:18px">
<button onclick="go('/set?n='+n.value)" style="width:auto;padding:8px 14px;font-size:16px">OK</button></p>
<div class="s"><div><i id="s0"></i><em>L</em></div><div><i id="s1"></i><em>C</em></div><div><i id="s2"></i><em>R</em></div><div><i id="s3"></i><em>X</em></div></div>
<b>Події</b><ol id="lg"></ol>
<script>
function go(u){fetch(u)}
async function t(){try{
 let d=await(await fetch('/status')).json();
 st.textContent=d.state;st.className='st '+d.state;pb.style.width=d.progress+'%';pc.textContent=d.progress+' %';
 pa.textContent=d.path_cm+' см';cr.textContent=d.cross+'/'+d.target;ds.textContent=d.dist>=999?'вільно':d.dist+' см';
 ob.textContent=d.obstacles+' / '+d.bypasses;bt.textContent=d.batt+' В · '+d.time_s+' с';
 d.sens.forEach((v,i)=>document.getElementById('s'+i).style.height=(v/10)+'%');
 lg.innerHTML=d.log.map(e=>'<li>'+e+'</li>').join('');
 if(document.activeElement!==n)n.value=d.target;
}catch(e){st.textContent='немає зв\'язку';st.className='st LOST'}}
setInterval(t,500);t()
</script></body></html>)html";

void netBegin() {
  WiFi.mode(WIFI_AP);
  WiFi.softAP(WIFI_SSID, WIFI_PASS, WIFI_CH, 0, 4);   // канал, не ховати, до 4 клієнтів
  WiFi.setSleep(false);                              // без енергозбереження — стабільніше
  web.on("/", [] { web.send_P(200, "text/html", PAGE); });
  web.on("/status", [] { web.send(200, "application/json", telemetryJson()); });
  web.on("/start", [] { cmdStart(); web.send(200, "text/plain", "ok"); });
  web.on("/stop", [] { cmdStop("веб"); web.send(200, "text/plain", "ok"); });
  web.on("/set", [] { cmdTarget(web.arg("n").toInt()); web.send(200, "text/plain", "ok"); });
  web.begin();
  Serial.printf("Wi-Fi %s / %s, канал %d → http://%s\n", WIFI_SSID, WIFI_PASS, WIFI_CH, WiFi.softAPIP().toString().c_str());
}
void netLoop() { web.handleClient(); }
#endif
