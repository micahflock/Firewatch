#pragma once

static const char kIndexHtml[] PROGMEM = R"HTML(<!doctype html>
<html lang="en"><head><meta charset="utf-8">
<meta name="viewport" content="width=device-width,initial-scale=1">
<title>Firewatch</title>
<style>
:root{color-scheme:dark;--bg:#140d0a;--card:#23160f;--text:#f3e6dc;--muted:#b09a8c;--accent:#ff7a2f}
body{margin:0;background:var(--bg);color:var(--text);font:16px/1.4 system-ui,sans-serif;display:flex;justify-content:center}
main{width:100%;max-width:420px;padding:24px 16px}
h1{font-size:22px;margin:0 0 16px}
.row{display:flex;gap:8px;margin-bottom:12px}
button{flex:1;padding:16px 0;font-size:17px;border:0;border-radius:12px;background:var(--card);color:var(--text)}
button.primary{background:var(--accent);color:#1a0d05;font-weight:600}
label{display:block;color:var(--muted);margin:16px 0 6px}
input[type=range]{width:100%;accent-color:var(--accent)}
pre{background:var(--card);border-radius:12px;padding:12px;color:var(--muted);font-size:13px;white-space:pre-wrap}
details{background:var(--card);border-radius:12px;padding:4px 14px;margin:20px 0}
summary{padding:12px 0;font-weight:600;cursor:pointer}
h2{font-size:15px;margin:18px 0 0;color:var(--accent)}
.ctl label{display:flex;justify-content:space-between;color:var(--text);margin:14px 0 4px}
.ctl label span{color:var(--muted);font-variant-numeric:tabular-nums}
.ctl small{display:block;color:var(--muted);font-size:13px}
details button,details pre{background:var(--bg)}
.note{color:var(--muted);font-size:13px}
</style></head><body><main>
<h1>Firewatch</h1>
<div class="row"><button class="primary" onclick="cmd('on')">Light the fire</button><button onclick="cmd('off')">Put it out</button></div>
<div class="row"><button onclick="cmd('relaunch')">Restart video</button></div>
<label for="b">Glow brightness</label>
<input id="b" type="range" min="0" max="100" onchange="fetch('/brightness?value='+this.value,{method:'POST'})">
<details id="tune"><summary>Tune the glow</summary>
<p class="note">Changes show on the strip right away and are saved after a couple of seconds. Light the fire to see them.</p>
<div id="ctls"></div>
<div class="row" style="margin-top:18px"><button onclick="resetTune()">Reset to defaults</button><button onclick="showTune()">Show settings</button></div>
<pre id="tv" hidden></pre>
</details>
<pre id="s">…</pre>
<script>
function cmd(c){fetch('/'+c,{method:'POST'}).then(poll)}
function poll(){fetch('/status').then(r=>r.json()).then(j=>{
 document.getElementById('s').textContent=JSON.stringify(j,null,1);
 const b=document.getElementById('b');if(document.activeElement!==b)b.value=j.brightness;})}
const fmt=(v,s)=>(+v).toFixed(s<0.1?2:1),pend={};let tmr,ctl=[];
function send(k,v){pend[k]=v;clearTimeout(tmr);tmr=setTimeout(()=>{
 fetch('/tune?'+new URLSearchParams(pend),{method:'POST'});for(const x in pend)delete pend[x]},120)}
function draw(j){ctl=j.controls;const box=document.getElementById('ctls');box.innerHTML='';let g;
 for(const c of ctl){if(c.group!==g){g=c.group;const h=document.createElement('h2');h.textContent=g;box.append(h)}
  const d=document.createElement('div');d.className='ctl';
  d.innerHTML='<label><b></b><span></span></label><input type="range"><small></small>';
  const [b,sp]=d.querySelectorAll('b,span'),r=d.querySelector('input');
  b.textContent=c.label;d.querySelector('small').textContent=c.hint;
  Object.assign(r,{min:c.min,max:c.max,step:c.step,value:c.value});
  const show=()=>{sp.textContent=fmt(r.value,c.step)+(Math.abs(r.value-c.default)>c.step/2?' (default '+fmt(c.default,c.step)+')':'')};
  r.oninput=()=>{c.value=+r.value;show();send(c.key,r.value)};show();box.append(d)}}
function loadTune(){fetch('/tune').then(r=>r.json()).then(draw)}
function resetTune(){if(confirm('Put every glow setting back to its default?'))fetch('/tune/reset',{method:'POST'}).then(r=>r.json()).then(draw)}
function showTune(){const p=document.getElementById('tv');
 p.textContent=ctl.map(c=>c.key+'='+fmt(c.value,c.step)).join('\n');p.hidden=false}
document.getElementById('tune').ontoggle=e=>{if(e.target.open)loadTune()};
poll();setInterval(poll,2000);
</script></main></body></html>)HTML";
