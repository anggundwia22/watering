#ifndef WEBPAGE_H
#define WEBPAGE_H

const char index_html[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html lang="id">
<head>
<meta charset="UTF-8">
<meta name="viewport" content="width=device-width, initial-scale=1.0, maximum-scale=1.0, user-scalable=no">
<meta name="theme-color" content="#eef1f6">
<title>Kontrol Penyiraman</title>
<style>
  :root{
    --bg:#eef1f6; --card:#ffffff; --line:#e4e8ee;
    --ink:#1f2937; --muted:#6b7280;
    --leaf:#16a34a; --water:#0284c7; --amber:#d97706; --rose:#e11d48;
    --blue:#2563eb; --blue-d:#1d4ed8;
    --shadow:0 1px 2px rgba(16,24,40,.04), 0 6px 20px -12px rgba(16,24,40,.16);
  }
  *{box-sizing:border-box;-webkit-tap-highlight-color:transparent}
  html,body{margin:0}
  body{font-family:-apple-system,BlinkMacSystemFont,"Segoe UI",Roboto,sans-serif;
    color:var(--ink);background:var(--bg);min-height:100vh;padding:12px 14px 16px}
  .wrap{max-width:440px;margin:0 auto;display:flex;flex-direction:column;gap:10px}

  header{display:flex;align-items:center;justify-content:space-between;padding:0 2px}
  .title{font-size:16px;font-weight:700;letter-spacing:.2px}
  .conn{display:flex;align-items:center;gap:6px;font-size:11px;color:var(--muted)}
  .dot{width:8px;height:8px;border-radius:50%;background:var(--rose);display:inline-block;transition:.3s}
  .dot.live{background:var(--leaf)}

  .card{background:var(--card);border:1px solid var(--line);border-radius:16px;
    padding:14px;box-shadow:var(--shadow)}

  /* Status */
  .status{display:flex;flex-direction:column;align-items:center;gap:5px;padding-top:12px;padding-bottom:14px}
  .clk{font-size:30px;font-weight:700;letter-spacing:1px;font-variant-numeric:tabular-nums;line-height:1.1}
  .dte{font-size:11.5px;color:var(--muted)}
  .orb{position:relative;width:92px;height:92px;display:grid;place-items:center;margin:3px 0 1px}
  .orb .ring{position:absolute;inset:0;border-radius:50%;border:2px solid rgba(148,163,184,.3)}
  .orb .disc{width:64px;height:64px;border-radius:50%;display:grid;place-items:center;
    background:#f1f5f9;border:1px solid var(--line);z-index:2;transition:.3s}
  .orb .ic{font-size:28px;line-height:1}
  .orb.spray .ring{border-color:rgba(2,132,199,.5);animation:rip 1.9s ease-out infinite}
  .orb.spray .ring:nth-child(2){animation-delay:.95s}
  .orb.spray .disc{background:#e0f2fe}
  @keyframes rip{0%{transform:scale(.7);opacity:.85}100%{transform:scale(1.16);opacity:0}}
  .orb.rest .ring{border-color:rgba(22,163,74,.4)} .orb.rest .disc{background:#dcfce7}
  .orb.amber .ring{border-color:rgba(217,119,6,.5)} .orb.amber .disc{background:#fef3c7}
  .orb.off .ring{border-color:rgba(148,163,184,.3)} .orb.off .disc{background:#f1f5f9}
  .sname{font-size:18px;font-weight:700;margin-top:1px}
  .count{font-size:12.5px;color:var(--muted)}
  .count b{font-size:14px;color:var(--ink);font-variant-numeric:tabular-nums;margin-left:4px}

  /* Mode */
  .seg{display:grid;grid-template-columns:repeat(3,1fr);gap:8px}
  .seg button{appearance:none;border:1px solid var(--line);background:#f8fafc;color:var(--ink);
    padding:11px 4px;border-radius:11px;font-size:13.5px;font-weight:600;cursor:pointer;transition:.15s}
  .seg button:active{transform:scale(.97)}
  .seg button.on[data-m="auto"]{background:#dcfce7;border-color:var(--leaf);color:#14532d}
  .seg button.on[data-m="on"]{background:#e0f2fe;border-color:var(--water);color:#075985}
  .seg button.on[data-m="off"]{background:#ffe4e6;border-color:var(--rose);color:#9f1239}

  /* Terakhir siram */
  .lastline{display:flex;align-items:center;justify-content:space-between}
  .ll-label{font-size:14px;color:var(--ink)}
  .ll-time{font-size:22px;font-weight:700;color:var(--water);font-variant-numeric:tabular-nums}

  /* Setting */
  .sect-h{display:flex;align-items:center;justify-content:space-between;cursor:pointer;user-select:none}
  .sect-h h3{margin:0;font-size:14px;font-weight:700}
  .chev{color:var(--muted);transition:.25s;font-size:13px}
  .sect-h.open .chev{transform:rotate(180deg)}
  .sect-body{max-height:0;overflow:hidden;transition:max-height .3s ease}
  .sect-body.open{max-height:600px}
  .sect-inner{padding-top:14px;display:flex;flex-direction:column;gap:12px}
  .row{display:flex;align-items:center;justify-content:space-between;gap:12px}
  .row label{font-size:13.5px}
  .hint{font-size:11px;color:var(--muted)}
  .field{display:flex;align-items:center;gap:6px}
  input[type=number],input[type=time]{background:#fff;border:1px solid var(--line);color:var(--ink);
    border-radius:9px;padding:8px 10px;font-size:15px;width:78px;text-align:center;font-variant-numeric:tabular-nums}
  input[type=time]{width:118px}
  input:focus{outline:none;border-color:var(--blue)}
  .unit{font-size:12px;color:var(--muted)}
  .toggle{position:relative;width:48px;height:27px;flex:0 0 auto}
  .toggle input{opacity:0;width:0;height:0}
  .track{position:absolute;inset:0;background:#cbd5e1;border-radius:999px;transition:.2s}
  .track:before{content:"";position:absolute;width:21px;height:21px;left:3px;top:3px;border-radius:50%;
    background:#fff;transition:.2s;box-shadow:0 1px 2px rgba(0,0,0,.2)}
  .toggle input:checked+.track{background:var(--leaf)}
  .toggle input:checked+.track:before{transform:translateX(21px)}

  /* Tombol */
  .btn{appearance:none;border:none;border-radius:11px;padding:12px;font-size:14px;font-weight:700;
    cursor:pointer;width:100%;transition:.15s}
  .btn:active{transform:scale(.98)}
  .btn-blue{background:var(--blue);color:#fff}
  .btn-blue:active{background:var(--blue-d)}
  .btn.sm{width:120px;padding:10px 0px}
  .btn-restart{background:#fff;color:var(--rose);border:1px solid var(--rose)}
  .btn-restart:active{background:#fff1f2}

  .sysbox{display:flex;flex-direction:column;gap:12px}
  .syspre{margin:0;padding:0;background:transparent;border:none;
    font-family:ui-monospace,SFMono-Regular,Menlo,Consolas,monospace;font-size:12px;line-height:1.55;
    white-space:pre;color:var(--ink)}

  .rtcline{display:flex;align-items:center;justify-content:space-between;gap:12px}
  .rtcline .now{font-size:12.5px;color:var(--muted)}
  .rtcline .now b{color:var(--ink);font-variant-numeric:tabular-nums}

  footer{text-align:center;font-size:10.5px;color:var(--muted);margin-top:2px}

  .toast{position:fixed;left:50%;bottom:20px;transform:translateX(-50%) translateY(16px);
    background:#1f2937;color:#fff;padding:10px 16px;border-radius:11px;font-size:13px;opacity:0;
    pointer-events:none;transition:.25s;z-index:50;box-shadow:var(--shadow);max-width:90vw}
  .toast.show{opacity:1;transform:translateX(-50%) translateY(0)}
  .toast.err{background:#b91c1c}
</style>
</head>
<body>
<div class="wrap">
  <header>
    <span class="title">Penyiraman Otomatis</span>
    <span class="conn"><i class="dot" id="dot"></i><span id="connlbl">Menghubungkan</span></span>
  </header>

  <!-- STATUS -->
  <div class="card status">
    <div class="clk" id="clk">--:--:--</div>
    <div class="dte" id="dte">memuat waktu…</div>
    <div class="orb off" id="orb">
      <span class="ring"></span><span class="ring"></span>
      <span class="disc"><span class="ic" id="orbIcon">🌙</span></span>
    </div>
    <div class="sname" id="stName">—</div>
    <div class="count" id="countBox" style="display:none"><span id="cdLbl"></span><b id="cdVal">--:--</b></div>
  </div>

  <!-- MODE -->
  <div class="card">
    <div class="seg">
      <button data-m="auto" onclick="setMode('auto')">Otomatis</button>
      <button data-m="on" onclick="setMode('on')">Nyala</button>
      <button data-m="off" onclick="setMode('off')">Mati</button>
    </div>
  </div>

  <!-- TERAKHIR SIRAM -->
  <div class="card lastline">
    <span class="ll-label">Terakhir siram</span>
    <span class="ll-time" id="lastWater">—</span>
  </div>

  <!-- JADWAL & SETTING (default tertutup) -->
  <div class="card">
    <div class="sect-h" id="setH" onclick="toggleSect('set')">
      <h3>Jadwal &amp; Pengaturan</h3><span class="chev">▾</span>
    </div>
    <div class="sect-body" id="setBody">
      <div class="sect-inner">
        <div class="row">
          <div><label>Jam mulai</label><div class="hint">alat mulai siklus</div></div>
          <input type="time" id="startT" value="10:00">
        </div>
        <div class="row">
          <div><label>Jam selesai</label><div class="hint">alat berhenti</div></div>
          <input type="time" id="endT" value="14:00">
        </div>
        <div class="row">
          <div><label>Durasi siram</label><div class="hint">pompa menyala</div></div>
          <div class="field"><input type="number" id="sprayM" min="0" max="240" step="1" value="15"><span class="unit">menit</span></div>
        </div>
        <div class="row">
          <div><label>Durasi istirahat</label><div class="hint">pompa mati</div></div>
          <div class="field"><input type="number" id="restM" min="0" max="240" step="1" value="15"><span class="unit">menit</span></div>
        </div>
        <div style="text-align:center"> 
        <button class="btn btn-blue sm" onclick="saveSettings()">Simpan</button>
        </div>
      </div>
    </div>
  </div>

  <!-- WAKTU RTC -->
  <div class="card rtcline">
    <div class="now">Waktu modul (RTC)<br><b id="rtcStat">—</b></div>
    <button class="btn btn-blue sm" onclick="syncTime()">Sinkron HP</button>
  </div>

  <!-- STATUS SISTEM -->
  <div class="card">
    <div class="sect-h open" id="sysH" onclick="toggleSect('sys')">
      <h3>Status Sistem</h3><span class="chev">▾</span>
    </div>
    <div class="sect-body open" id="sysBody">
      <div class="sect-inner sysbox">
        <pre class="syspre" id="sysPre">memuat…</pre>
        <button class="btn btn-restart" onclick="restartDev()">Restart Perangkat</button>
      </div>
    </div>
  </div>

  <footer>Sistem Penyiraman Otomatis • ESP32 • v1.1</footer>
</div>

<div class="toast" id="toast"></div>

<script>
const $=id=>document.getElementById(id);
let dirty=false, failCount=0;

function toast(msg,err){const t=$('toast');t.textContent=msg;t.className='toast show'+(err?' err':'');
  clearTimeout(t._h);t._h=setTimeout(()=>t.className='toast',2200);}
function pad(n){return String(n).padStart(2,'0');}
function fmtCd(s){if(s<0)s=0;const h=Math.floor(s/3600),m=Math.floor(s%3600/60),x=s%60;
  return h>0?h+':'+pad(m)+':'+pad(x):pad(m)+':'+pad(x);}

['startT','endT','sprayM','restM'].forEach(id=>{
  const el=$(id);el.addEventListener('input',()=>dirty=true);el.addEventListener('focus',()=>dirty=true);
});

function toggleSect(k){const h=$(k+'H'),b=$(k+'Body');h.classList.toggle('open');b.classList.toggle('open');}

async function poll(){
  try{
    const r=await fetch('/api/status',{cache:'no-store'});
    const d=await r.json();
    failCount=0;$('dot').classList.add('live');$('connlbl').textContent='Terhubung';
    render(d);
  }catch(e){
    failCount++;
    if(failCount>2){$('dot').classList.remove('live');$('connlbl').textContent='Terputus';}
  }
}

function render(d){
  $('clk').textContent=d.time||'--:--:--';
  $('dte').textContent=d.rtcOk?(d.dateText||d.date):'⚠️ RTC tidak terbaca';
  $('rtcStat').textContent=d.rtcOk?(d.date+' '+d.time):'tidak terdeteksi';

  const orb=$('orb');orb.className='orb';let icon='🌙';
  if(d.state==='MENYIRAM'){orb.classList.add('spray');icon='💧';}
  else if(d.state==='ISTIRAHAT'){orb.classList.add('rest');icon='🌿';}
  else if(d.state==='MANUAL ON'){orb.classList.add('amber');icon='💦';}
  else if(d.state==='MANUAL OFF'){orb.classList.add('off');icon='✋';}
  else if(d.state==='AUTO NONAKTIF'){orb.classList.add('off');icon='🚫';}
  else{orb.classList.add('off');icon='🌙';}
  $('orbIcon').textContent=icon;
  $('stName').textContent=d.state||'—';

  const cb=$('countBox');
  if(d.countdownLabel){cb.style.display='';$('cdLbl').textContent=d.countdownLabel;$('cdVal').textContent=fmtCd(d.countdownSec);}
  else{cb.style.display='none';}

  $('lastWater').textContent=d.lastWater?d.lastWater:'—';

  document.querySelectorAll('.seg button').forEach(b=>{
    b.classList.toggle('on', b.dataset.m===(d.mode||'').toLowerCase());
  });

  if(!dirty&&d.settings){
    const s=d.settings;
    $('startT').value=pad(s.startHour)+':'+pad(s.startMinute);
    $('endT').value=pad(s.endHour)+':'+pad(s.endMinute);
    $('sprayM').value=Math.round(s.sprayDurationSec/60);
    $('restM').value=Math.round(s.restDurationSec/60);
  }

  if(d.system){
    const s=d.system;
    $('sysPre').textContent=
      'STATUS SISTEM\n'+
      'Reset Terakhir : '+s.resetReason+'\n'+
      'Boot Count     : '+s.bootCount+'\n'+
      'Uptime         : '+s.uptime+'\n'+
      'Free Heap      : '+s.freeHeapKB+' KB';
  }
}

async function setMode(m){
  try{await fetch('/api/control',{method:'POST',headers:{'Content-Type':'application/x-www-form-urlencoded'},
    body:'mode='+m});
    toast(m==='auto'?'Mode otomatis aktif':m==='on'?'Pompa dinyalakan':'Pompa dimatikan');poll();
  }catch(e){toast('Gagal kirim perintah',1);}
}

async function saveSettings(){
  const [sh,sm]=$('startT').value.split(':').map(Number);
  const [eh,em]=$('endT').value.split(':').map(Number);
  const body=new URLSearchParams({
    startHour:sh,startMinute:sm,endHour:eh,endMinute:em,
    sprayDurationSec:Math.max(0,Math.round($('sprayM').value*60)),
    restDurationSec:Math.max(0,Math.round($('restM').value*60))
  });
  try{await fetch('/api/settings',{method:'POST',headers:{'Content-Type':'application/x-www-form-urlencoded'},body});
    dirty=false;toast('Pengaturan tersimpan');poll();
  }catch(e){toast('Gagal menyimpan',1);}
}

async function syncTime(){
  const n=new Date();
  const body=new URLSearchParams({Y:n.getFullYear(),M:n.getMonth()+1,D:n.getDate(),
    h:n.getHours(),m:n.getMinutes(),s:n.getSeconds()});
  try{await fetch('/api/time',{method:'POST',headers:{'Content-Type':'application/x-www-form-urlencoded'},body});
    toast('Waktu RTC disinkronkan');poll();
  }catch(e){toast('Gagal sinkron waktu',1);}
}

async function restartDev(){
  if(!confirm('Restart perangkat sekarang?'))return;
  try{
    await fetch('/api/restart',{method:'POST'});
    toast('Perangkat dimulai ulang…');
    $('dot').classList.remove('live');
    $('connlbl').textContent='Memulai ulang';
  }catch(e){toast('Gagal restart',1);}
}

poll();setInterval(poll,1000);
</script>
</body>
</html>
)rawliteral";

#endif