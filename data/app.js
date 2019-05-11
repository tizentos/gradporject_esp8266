var distance;
var led_29 = false;
var led_31 = false;
var reloadPeriod = 1000;
var running = false;
var ControlGpioData;

function loadDistanceValues(){
  if(!running) return;
  var xh = new XMLHttpRequest();
  xh.onreadystatechange = function(){
    if (xh.readyState == 4){
      if(xh.status == 200) {
        var res = JSON.parse(xh.responseText);
        distance.add(res.distance);
        setTimeout(loadDistanceValues, reloadPeriod);
      }
    }
  };
  xh.open("GET", "/distance", true);
  xh.send(null);
};

//Function to handle button color
function setButtonAppearance(id,value){
  var button =  document.getElementById(id);
  if (id == "led-29"){
    (Boolean(value))? button.style.backgroundImage = "red":button.style.backgroundImage = "gray";
  } else if (id == "led-31"){
    (Boolean(value))? button.style.backgroundImage = "blue":button.style.backgroundImage = "gray"; 
  } else {
    console.log("Can't find LED button");
  }
};

//XHR Function for sending GET request to ESP8266
function loadGpioStatus(){
  if(!running) return;
  var xh = new XMLHttpRequest();
  xh.onreadystatechange = function(){
    if (xh.readyState == 4){
      if(xh.status == 200) {
        var res = JSON.parse(xh.responseText);
        led_29 = res["29"];
        led_31= res["31"];
        setButtonAppearance("led-29",led_29);
        setButtonAppearance("led-31",led_31);
        setTimeout(loadGpioStatus, reloadPeriod);
      } 
    }
  };
  xh.open("GET", "/gpio", true);
  xh.send(null);
};


//XHR Function for sending POST request ESP8266
function ControlGpio(){
  var xh = new XMLHttpRequest();
  xh.onreadystatechange = function(){
    if (xh.readyState == 4){
      if(xh.status == 200) {
        loadGpioStatus();
      } 
    }
  };
  xh.open("POST", "/gpio", true);
  xh.send(ControlGpioData);
};

function run(){
  if(!running){
    running = true;
    loadValues();
  }
}

function onBodyLoad(){
  var refreshInput = document.getElementById("refresh-rate");
  refreshInput.value = reloadPeriod;
  refreshInput.onchange = function(e){
    var value = parseInt(e.target.value);
    reloadPeriod = (value > 0)?value:0;
    e.target.value = reloadPeriod;
  }
  var stopButton = document.getElementById("stop-button");
  stopButton.onclick = function(e){
    running = false;
  }
  var startButton = document.getElementById("start-button");
  startButton.onclick = function(e){
    run();
  }

  //DOM for LED 29
  var led29Button = document.getElementById("led-29");
  led29Button.onclick = function(e){
     (led_29)? ControlGpioData="29=0": ControlGpioData = "29=1";
     ControlGpio();
  }

  //DOM for LED 31
  var led31Button = document.getElementById("led-31");
  led31Button.onclick = function(e){
    (led_31)? ControlGpioData="31=0": ControlGpioData = "31=1";
    ControlGpio();
  }

  distance = createGraph(document.getElementById("distance"), "Distance", 100, 128, 0, 1023, false, "cyan");
  run();
}