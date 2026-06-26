#ifndef WEBPAGE_H
#define WEBPAGE_H

const char index_html[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html lang="id">
<head>
<meta charset="UTF-8">
<meta name="viewport" content="width=device-width, initial-scale=1.0, maximum-scale=1.0, user-scalable=no">
<meta name="theme-color" content="#0a1f1a">
<title>Kontrol Penyiraman</title>
<style>
  :root{
    --bg-0:#08191400; --leaf:#4ade80; --leaf-dim:#22c55e;
    --water:#38bdf8; --water-dim:#0ea5e9; --amber:#fbbf24;
    --rose:#fb7185; --ink:#eafff4; --muted:#8fb6a6;
    --glass:rgba(255,255,255,.055); --glass-line:rgba(255,255,255,.10);
    --shadow:0 18px 50px -18px rgba(0,0,0,.7);
  }
  *{box-sizing:border-box;-webkit-tap-highlight-color:transparent}
  html,body{margin:0;padding:0}
  body{
    font-family:-apple-system,BlinkMacSystemFont,"Segoe UI",Roboto,sans-serif;
    color:var(--ink); min-height:100vh; padding:18px 16px 40px;
    background:
      radial-gradient(900px 480px at 80% -8%, rgba(56,189,248,.16), transparent 60%),
      radial-gradient(800px 520px at 0% 100%, rgba(74,222,128,.14), transparent 55%),
      linear-gradient(170deg,#0a1f1a 0%, #0c2a21 48%, #08191b 100%);
    background-attachment:fixed;
  }
  .wrap{max-width:460px;margin:0 auto;display:flex;flex-direction:column;gap:14px}

  /* Header */
  header{display:flex;align-items:center;justify-content:space-between;gap:10px;padding:2px 4px}
  .brand{display:flex;flex-direction:column;line-height:1.1}
  .brand b{font-size:18px;font-weight:700;letter-spacing:.2px}
  .brand span{font-size:11px;color:var(--muted);letter-spacing:1.4px;text-transform:uppercase}
  .conn{display:flex;align-items:center;gap:7px;font-size:11px;color:var(--muted);
    background:var(--glass);border:1px solid var(--glass-line);padding:7px 11px;border-radius:999px}
  .dot{width:8px;height:8px;border-radius:50%;background:var(--rose);transition:.3s}
  .dot.live{background:var(--leaf);box-shadow:0 0 0 0 rgba(74,222,128,.6);animation:beat 2s infinite}
  @keyframes beat{0%{box-shadow:0 0 0 0 rgba(74,222,128,.5)}70%{box-shadow:0 0 0 7px rgba(74,222,128,0)}100%{box-shadow:0 0 0 0 rgba(74,222,128,0)}}

  /* Clock */
  .clock{text-align:center;padding:4px 0 2px}
  .clock .t{font-size:46px;font-weight:700;letter-spacing:2px;font-variant-numeric:tabular-nums;
    text-shadow:0 0 26px rgba(56,189,248,.25)}
  .clock .d{font-size:12.5px;color:var(--muted);margin-top:2px}

  .card{background:var(--glass);border:1px solid var(--glass-line);border-radius:22px;
    padding:18px;box-shadow:var(--shadow);backdrop-filter:blur(14px);-webkit-backdrop-filter:blur(14px)}

  /* Core / status utama */
  .core{display:flex;flex-direction:column;align-items:center;gap:14px;padding:24px 18px 22px}
  .orb{position:relative;width:158px;height:158px;display:grid;place-items:center}
  .orb .ring{position:absolute;inset:0;border-radius:50%;border:2px solid var(--ring,rgba(143,182,166,.25));transition:.4s}
  .orb .core-dot{width:104px;height:104px;border-radius:50%;display:grid;place-items:center;
    background:radial-gradient(circle at 50% 36%, var(--c-hi,#1d3b33), var(--c-lo,#0c211c));
    border:1px solid var(--glass-line);transition:.4s;position:relative;z-index:2}
  .orb .icon{font-size:42px;line-height:1}
  .orb.spray .ring{border-color:rgba(56,189,248,.55);animation:ripple 1.9s ease-out infinite}
  .orb.spray .ring:nth-child(2){animation-delay:.95s}
  .orb.spray{--c-hi:#0c4a6e;--c-lo:#082f49}
  @keyframes ripple{0%{transform:scale(.62);opacity:.9}100%{transform:scale(1.18);opacity:0}}
  .orb.rest{--c-hi:#14532d;--c-lo:#0a2e18}
  .orb.rest .ring{border-color:rgba(74,222,128,.35)}
  .orb.amber{--c-hi:#854d0e;--c-lo:#422006}
  .orb.amber .ring{border-color:rgba(251,191,36,.5)}
  .orb.off .ring{border-color:rgba(143,182,166,.18)}

  .state-name{font-size:23px;font-weight:700;letter-spacing:.3px}
  .state-sub{font-size:12.5px;color:var(--muted);margin-top:-6px}
  .count{display:flex;align-items:baseline;gap:8px;margin-top:2px}
  .count .lbl{font-size:11.5px;color:var(--muted);text-transform:uppercase;letter-spacing:1px}
  .count .v{font-size:28px;font-weight:700;font-variant-numeric:tabular-nums;color:var(--water)}
  .orb.rest~* .count .v,.statewrap.rest .count .v{color:var(--leaf)}

  /* Mode buttons */
  .seg{display:grid;grid-template-columns:repeat(3,1fr);gap:8px;margin-top:4px}
  .seg button{appearance:none;border:1px solid var(--glass-line);background:rgba(255,255,255,.03);
    color:var(--ink);padding:13px 6px;border-radius:14px;font-size:13px;font-weight:600;cursor:pointer;
    display:flex;flex-direction:column;align-items:center;gap:4px;transition:.18s}
  .seg button .em{font-size:18px}
  .seg button:active{transform:scale(.96)}
  .seg button.on[data-m="auto"]{background:rgba(74,222,128,.16);border-color:var(--leaf);color:#dcfce7}
  .seg button.on[data-m="on"]{background:rgba(56,189,248,.16);border-color:var(--water);color:#e0f2fe}
  .seg button.on[data-m="off"]{background:rgba(251,113,133,.15);border-color:var(--rose);color:#ffe4e6}

  /* Section title */
  .sect-h{display:flex;align-items:center;justify-content:space-between;cursor:pointer;user-select:none}
  .sect-h h3{margin:0;font-size:14px;font-weight:700;letter-spacing:.3px;display:flex;align-items:center;gap:8px}
  .sect-h h3 .em{font-size:16px}
  .chev{color:var(--muted);transition:.25s;font-size:13px}
  .sect-h.open .chev{transform:rotate(180deg)}
  .sect-body{max-height:0;overflow:hidden;transition:max-height .32s ease}
  .sect-body.open{max-height:1500px}
  .sect-inner{padding-top:16px;display:flex;flex-direction:column;gap:14px}

  .row{display:flex;align-items:center;justify-content:space-between;gap:12px}
  .row label{font-size:13.5px;color:#cfe9dd}
  .row .hint{font-size:11px;color:var(--muted)}
  .field{display:flex;align-items:center;gap:6px}
  input[type=number],input[type=time]{
    background:rgba(0,0,0,.28);border:1px solid var(--glass-line);color:var(--ink);
    border-radius:11px;padding:10px 11px;font-size:15px;width:80px;text-align:center;font-variant-numeric:tabular-nums}
  input[type=time]{width:118px}
  input:focus{outline:none;border-color:var(--water)}
  .unit{font-size:12px;color:var(--muted)}

  .toggle{position:relative;width:50px;height:28px;flex:0 0 auto}
  .toggle input{opacity:0;width:0;height:0}
  .track{position:absolute;inset:0;background:rgba(255,255,255,.12);border-radius:999px;transition:.25s}
  .track:before{content:"";position:absolute;width:22px;height:22px;left:3px;top:3px;border-radius:50%;
    background:#fff;transition:.25s}
  .toggle input:checked+.track{background:var(--leaf-dim)}
  .toggle input:checked+.track:before{transform:translateX(22px)}

  .btn{appearance:none;border:none;border-radius:14px;padding:13px;font-size:14px;font-weight:700;
    cursor:pointer;width:100%;transition:.18s;display:flex;align-items:center;justify-content:center;gap:8px}
  .btn:active{transform:scale(.98)}
  .btn-primary{background:linear-gradient(120deg,var(--leaf-dim),var(--water-dim));color:#04130d}
  .btn-ghost{background:rgba(255,255,255,.06);border:1px solid var(--glass-line);color:var(--ink)}

  .rtcline{display:flex;align-items:center;justify-content:space-between}
  .rtcline .now{font-size:13px;color:var(--muted)}
  .rtcline .now b{color:var(--ink);font-variant-numeric:tabular-nums}

  /* Terakhir nyiram (satu baris) */
  .muted{color:var(--muted)}
  .lastline{display:flex;align-items:center;justify-content:space-between;gap:12px}
  .ll-label{font-size:14.5px;color:#cfe9dd}
  .ll-time{font-size:26px;font-weight:700;letter-spacing:1px;font-variant-numeric:tabular-nums;
    color:var(--water);text-shadow:0 0 18px rgba(56,189,248,.25)}
  footer{text-align:center;font-size:11px;color:var(--muted);margin-top:6px;letter-spacing:.4px}
  .toast{position:fixed;left:50%;bottom:24px;transform:translateX(-50%) translateY(20px);
    background:rgba(8,30,24,.96);border:1px solid var(--glass-line);color:var(--ink);
    padding:11px 18px;border-radius:13px;font-size:13px;opacity:0;pointer-events:none;
    transition:.3s;box-shadow:var(--shadow);z-index:50;max-width:90vw}
  .toast.show{opacity:1;transform:translateX(-50%) translateY(0)}
  .toast.err{border-color:var(--rose);color:#ffe4e6}
</style>
</head>
<body>
<div class="wrap">
  <header>
    <div class="brand"><b>Penyiraman Otomatis</b></div>
    <div class="conn"><span class="dot" id="dot"></span><span id="connlbl">Menghubungkan</span></div>
  </header>

  <div class="clock">
    <div class="t" id="clk">--:--:--</div>
    <div class="d" id="dte">memuat waktu RTC…</div>
  </div>

  <!-- STATUS UTAMA -->
  <div class="card core statewrap" id="statewrap">
    <div class="orb off" id="orb">
      <div class="ring"></div><div class="ring"></div>
      <div class="core-dot"><div class="icon" id="orbIcon">💤</div></div>
    </div>
    <div style="text-align:center">
      <div class="state-name" id="stName">—</div>
      <div class="state-sub" id="stSub">menunggu data</div>
    </div>
    <div class="count"><span class="lbl" id="cdLbl">—</span><span class="v" id="cdVal">--:--</span></div>
  </div>

  <!-- MODE -->
  <div class="card">
    <div class="seg">
      <button data-m="auto" onclick="setMode('auto')"><span class="em">⏱️</span>Otomatis</button>
      <button data-m="on" onclick="setMode('on')"><span class="em">💧</span>Nyala</button>
      <button data-m="off" onclick="setMode('off')"><span class="em">⏹️</span>Mati</button>
    </div>
  </div>

  <!-- JADWAL & SETTING -->
  <div class="card">
    <div class="sect-h open" id="setH" onclick="toggleSect('set')">
      <h3>Jadwal &amp; Pengaturan</h3><span class="chev">▾</span>
    </div>
    <div class="sect-body open" id="setBody">
      <div class="sect-inner">
        <div class="row">
          <label>Mode otomatis aktif</label>
          <span class="toggle"><input type="checkbox" id="autoEn"><span class="track"></span></span>
        </div>
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
        <button class="btn btn-primary" onclick="saveSettings()">Simpan Pengaturan</button>
      </div>
    </div>
  </div>

  <!-- TERAKHIR SIRAM -->
  <div class="card">
    <div class="lastline">
      <span class="ll-label">Terakhir siram</span>
      <span class="ll-time" id="lastWater">—</span>
    </div>
  </div>

  <!-- WAKTU RTC -->
  <div class="card">
    <div class="rtcline">
      <div class="now">Waktu modul (RTC)<br><b id="rtcStat">—</b></div>
      <button class="btn btn-ghost" style="width:auto;padding:11px 16px" onclick="syncTime()">Sinkron HP</button>
    </div>
  </div>
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

// tandai input "dirty" supaya polling tdk menimpa ketikan user
['autoEn','startT','endT','sprayM','restM'].forEach(id=>{
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

  // orb + label per state
  const orb=$('orb'),sw=$('statewrap');
  orb.className='orb';sw.className='card core statewrap';
  let icon='💤',sub='';
  if(d.state==='MENYIRAM'){orb.classList.add('spray');sw.classList.add('spray');icon='💧';sub='pompa mengalir';}
  else if(d.state==='ISTIRAHAT'){orb.classList.add('rest');sw.classList.add('rest');icon='🌿';sub='jeda siklus';}
  else if(d.state==='MANUAL ON'){orb.classList.add('amber');icon='💦';sub='dikendalikan manual';}
  else if(d.state==='MANUAL OFF'){orb.classList.add('off');icon='✋';sub='dimatikan manual';}
  else if(d.state==='AUTO NONAKTIF'){orb.classList.add('off');icon='🚫';sub='mode otomatis mati';}
  else{orb.classList.add('off');icon='🌙';sub='di luar jam jadwal';}
  $('orbIcon').textContent=icon;
  $('stName').textContent=d.state||'—';
  $('stSub').textContent=sub;

  $('cdLbl').textContent=d.countdownLabel||'';
  $('cdVal').textContent=d.countdownLabel?fmtCd(d.countdownSec):'—';

  $('lastWater').textContent=d.lastWater?d.lastWater:'—';

  // mode buttons
  document.querySelectorAll('.seg button').forEach(b=>{
    b.classList.toggle('on', b.dataset.m===(d.mode||'').toLowerCase());
  });

  // setting (jangan timpa kalau user sedang mengetik)
  if(!dirty&&d.settings){
    const s=d.settings;
    $('autoEn').checked=s.autoEnabled;
    $('startT').value=pad(s.startHour)+':'+pad(s.startMinute);
    $('endT').value=pad(s.endHour)+':'+pad(s.endMinute);
    $('sprayM').value=Math.round(s.sprayDurationSec/60);
    $('restM').value=Math.round(s.restDurationSec/60);
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
    autoEnabled:$('autoEn').checked?1:0,
    startHour:sh,startMinute:sm,endHour:eh,endMinute:em,
    sprayDurationSec:Math.max(0,Math.round($('sprayM').value*60)),
    restDurationSec:Math.max(0,Math.round($('restM').value*60))
  });
  try{await fetch('/api/settings',{method:'POST',headers:{'Content-Type':'application/x-www-form-urlencoded'},body});
    dirty=false;toast('Pengaturan tersimpan ✓');poll();
  }catch(e){toast('Gagal menyimpan',1);}
}

async function syncTime(){
  const n=new Date();
  const body=new URLSearchParams({Y:n.getFullYear(),M:n.getMonth()+1,D:n.getDate(),
    h:n.getHours(),m:n.getMinutes(),s:n.getSeconds()});
  try{await fetch('/api/time',{method:'POST',headers:{'Content-Type':'application/x-www-form-urlencoded'},body});
    toast('Waktu RTC disinkronkan ✓');poll();
  }catch(e){toast('Gagal sinkron waktu',1);}
}

poll();setInterval(poll,1000);
</script>
</body>
</html>
)rawliteral";

#endif