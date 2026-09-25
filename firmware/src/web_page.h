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
</style></head><body><main>
<h1>Firewatch</h1>
<div class="row"><button class="primary" onclick="cmd('on')">Light the fire</button><button onclick="cmd('off')">Put it out</button></div>
<div class="row"><button onclick="cmd('relaunch')">Restart video</button></div>
<label for="b">Glow brightness</label>
<input id="b" type="range" min="0" max="100" onchange="fetch('/brightness?value='+this.value,{method:'POST'})">
<pre id="s">…</pre>
<script>
function cmd(c){fetch('/'+c,{method:'POST'}).then(poll)}
function poll(){fetch('/status').then(r=>r.json()).then(j=>{
 document.getElementById('s').textContent=JSON.stringify(j,null,1);
 const b=document.getElementById('b');if(document.activeElement!==b)b.value=j.brightness;})}
poll();setInterval(poll,2000);
</script></main></body></html>)HTML";
