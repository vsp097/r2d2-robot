#ifndef INDEX_HTML_H
#define INDEX_HTML_H

// Onboard control page, served directly from ESP32 flash memory (PROGMEM).
// Available at http://<robot-ip>/ once connected to WiFi.
const char PAGINA_HTML[] PROGMEM = R"=====(
<!DOCTYPE html>
<html>
<head>
<meta charset="utf-8">
<meta name="viewport" content="width=device-width, initial-scale=1">
<title>Arturito</title>
<style>
*{box-sizing:border-box}
body{font-family:Segoe UI,Arial,sans-serif;background:#1a1d21;color:#e8e8e8;margin:0;padding:24px;display:flex;flex-direction:column;align-items:center}
h1{font-size:20px;font-weight:600;margin:0 0 4px;color:#fff}
.sub{font-size:13px;color:#8a8f98;margin-bottom:20px}
.card{background:#22262b;border:1px solid #33383f;border-radius:12px;padding:18px;width:100%;max-width:380px;margin-bottom:16px}
.label{font-size:12px;color:#8a8f98;text-transform:uppercase;letter-spacing:.5px;margin-bottom:10px}
.cruceta{display:grid;grid-template-columns:64px 64px 64px;grid-template-rows:64px 64px 64px;gap:6px;justify-content:center;margin:4px 0}
.btn{background:#2d3238;border:1px solid #3d434b;border-radius:10px;color:#e8e8e8;font-size:20px;cursor:pointer;display:flex;align-items:center;justify-content:center;transition:background .15s;-webkit-user-select:none;user-select:none}
.btn:active{background:#3f6df5}
.btn-stop{background:#4a2020;border-color:#6b2b2b;color:#ff8080;grid-column:2;grid-row:2}
.up{grid-column:2;grid-row:1}
.down{grid-column:2;grid-row:3}
.left{grid-column:1;grid-row:2}
.right{grid-column:3;grid-row:2}
.fila{display:flex;gap:8px;flex-wrap:wrap;justify-content:center}
.btn-chico{flex:1;min-width:90px;padding:12px 8px;font-size:14px}
.dist{display:flex;align-items:baseline;justify-content:center;gap:6px;margin-top:6px}
.dist-num{font-size:32px;font-weight:600;color:#3f6df5}
.dist-unit{font-size:14px;color:#8a8f98}
#estado{font-size:12px;color:#5fb56a;margin-top:14px;height:16px}
</style>
</head>
<body>

<h1>Arturito</h1>
<div class="sub">Control panel</div>

<div class="card">
<div class="label">Movement</div>
<div class="cruceta">
  <button class="btn up"
          onmousedown="enviar('adelante')" onmouseup="enviar('parar')"
          ontouchstart="enviar('adelante'); event.preventDefault();" ontouchend="enviar('parar'); event.preventDefault();">&#9650;</button>

  <button class="btn left"
          onmousedown="enviar('izquierda')" onmouseup="enviar('parar')"
          ontouchstart="enviar('izquierda'); event.preventDefault();" ontouchend="enviar('parar'); event.preventDefault();">&#9664;</button>

  <button class="btn btn-stop" onclick="enviar('parar')">STOP</button>

  <button class="btn right"
          onmousedown="enviar('derecha')" onmouseup="enviar('parar')"
          ontouchstart="enviar('derecha'); event.preventDefault();" ontouchend="enviar('parar'); event.preventDefault();">&#9654;</button>

  <button class="btn down"
          onmousedown="enviar('atras')" onmouseup="enviar('parar')"
          ontouchstart="enviar('atras'); event.preventDefault();" ontouchend="enviar('parar'); event.preventDefault();">&#9660;</button>
</div>
</div>

<div class="card">
<div class="label">Distance to obstacle</div>
<div class="dist"><span class="dist-num" id="distNum">--</span><span class="dist-unit">cm</span></div>
</div>

<div class="card">
<div class="label">Emotions</div>
<div class="fila">
<button class="btn btn-chico" onclick="enviar('feliz')">Happy</button>
<button class="btn btn-chico" onclick="enviar('enojado')">Angry</button>
<button class="btn btn-chico" onclick="enviar('asustado')">Scared</button>
</div>
</div>

<div class="card">
<div class="label">Accessories</div>
<div class="fila">
<button class="btn btn-chico" onclick="enviar('laser_on')">Laser ON</button>
<button class="btn btn-chico" onclick="enviar('laser_off')">Laser OFF</button>
</div>
<div class="fila" style="margin-top:8px">
<button class="btn btn-chico" onclick="enviar('mira_izq')">Look left</button>
<button class="btn btn-chico" onclick="enviar('mira_der')">Look right</button>
</div>
</div>

<div id="estado">Ready</div>

<script>
function enviar(cmd){
  document.getElementById('estado').innerText = 'Sent: ' + cmd;
  fetch('/api/cmd?c=' + cmd).catch(function(){
    document.getElementById('estado').innerText = 'Connection lost';
  });
}

function actualizarDistancia(){
  fetch('/api/estado').then(function(r){ return r.json(); }).then(function(d){
    document.getElementById('distNum').innerText = (d.distancia >= 0) ? Math.round(d.distancia) : '--';
  }).catch(function(){});
}
setInterval(actualizarDistancia, 1000);
actualizarDistancia();
</script>

</body>
</html>
)=====";
#endif
