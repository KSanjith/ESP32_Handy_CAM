
const char index_html[] PROGMEM = R"rawliteral(
<!DOCTYPE HTML><html>
<head>
  <meta name="viewport" content="width=device-width, initial-scale=1">
  <style>
    body { text-align:center; }
    .vert { margin-bottom: 10%; }
    .hori { margin-bottom: 0%; }
  </style>
</head>
<body>
  <div id="container">
    <h2>ESP32-CAM Page</h2>
    <p>Wait a few seconds between photo capture and page refresh.</p>
    <p>
      <button onclick="rotatePhoto();">ROTATE</button>
      <button onclick="capturePhoto()">CAPTURE PHOTO</button>
      <button onclick="location.reload();">REFRESH PAGE</button>
      <button onclick="toggleFlash();">TOGGLE FLASH</button>
      <button onclick="videoON();">ENABLE VIDEO</button>
      <button onclick="videoOFF();">DISABLE VIDEO</button>
    </p>
  </div>
  <div>
    <a href="saved-photo" download="ESP32_CAM_Pic.jpg">
      <img src="saved-photo" id="photo" width="70%"></div>
    </a>
</body>
<script>
  var deg = 0;
  function capturePhoto() {
    var xhr = new XMLHttpRequest();
    xhr.open('GET', "/capture", true);
    xhr.send();
  }
  function toggleFlash() {
    var xhr = new XMLHttpRequest();
    xhr.open('GET', "/flash", true);
    xhr.send();
  }
  function videoON() {
    var xhr = new XMLHttpRequest();
    xhr.open('GET', "/videoON", true);
    xhr.send();
  }
  function videoOFF() {
    var xhr = new XMLHttpRequest();
    xhr.open('GET', "/videoOFF", true);
    xhr.send();
  }
  function rotatePhoto() {
    var img = document.getElementById("photo");
    deg += 90;
    if(isOdd(deg/90)){ document.getElementById("container").className = "vert"; }
    else{ document.getElementById("container").className = "hori"; }
    img.style.transform = "rotate(" + deg + "deg)";
  }
  function isOdd(n) { return Math.abs(n % 2) == 1; }
</script>
</html>)rawliteral";
