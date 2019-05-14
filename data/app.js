var distance;
var led_29 = false;
var led_31 = false;
var reloadPeriod = 1000;
var running = false;
var ControlGpioData;
var chart;

var dps = []; // dataPoints
var xVal = 0;
var yVal = 100; 
var updateInterval = 1000;
var dataLength = 20; // number of dataPoints visible at any point

var updateChart = function (count) {

	count = count || 1;

	// for (var j = 0; j < count; j++) {
	// 	// yVal = yVal +  Math.round(5 + Math.random() *(-5-5));
	// 	dps.push({
	// 		x: xVal,
	// 		y: yVal
	// 	});
	// 	xVal++;
	// }

	if (dps.length > dataLength) {
		dps.shift();
	}

	chart.render();
};

//Function to handle button color
function setButtonAppearance(id,value){
  var button =  document.getElementById(id);
  if (id == "led-29"){
    (Boolean(value))? button.style.backgroundColor = "red":button.style.backgroundColor = "gray";
  } else if (id == "led-31"){
    (Boolean(value))? button.style.backgroundColor = "blue":button.style.backgroundColor = "gray"; 
  } else {
    console.log("Can't find LED button");
  }
};

function getTelemetry(){
  if(!running) return;
  var xh = new XMLHttpRequest();
  xh.onreadystatechange = function(){
    if (xh.readyState == 4){
      if(xh.status == 200) {
        var res = JSON.parse(xh.responseText);
        if (res.hasOwnProperty("distance")){
          // distance.add(res.distance);
          dps.push({
            y:res.distance,
            x: xVal
          });
          xVal++;
          updateChart(dps.length);
        }
        if (res.hasOwnProperty("29")){
          led_29 = res["29"];
          led_31= res["31"];
          setButtonAppearance("led-29",led_29);
          setButtonAppearance("led-31",led_31);
        }
        setTimeout(getTelemetry, reloadPeriod);
      }
    }
  };
  xh.open("GET", "/telemetry", true);
  xh.send(null);
};

//XHR Function for sending POST request ESP8266
function ControlGpio(){
  var xh = new XMLHttpRequest();
  xh.onreadystatechange = function(){
    if (xh.readyState == 4){
      if(xh.status == 200) {
        getTelemetry();
      } 
    }
  };
  xh.open("POST", "/gpio", true);
  xh.setRequestHeader("Content-type", "application/x-www-form-urlencoded");
  xh.send(ControlGpioData);
};

function run(){
  if(!running){
    running = true;
    getTelemetry();
  }
}

function onBodyLoad(){
  //DOM for LED 29
  var led29Button = document.getElementById("led-29");
  led29Button.onclick = function(e){
     if (led_29){
       ControlGpioData ="29=0";
       led_29 = false;
     }
     else{
         ControlGpioData = "29=1";
         led_29 = true;
      }
     ControlGpio();
  }

  //DOM for LED 31
  var led31Button = document.getElementById("led-31");
  led31Button.onclick = function(e){
    if (led_31){
      ControlGpioData ="31=0";
      led_31 = false;
    }
    else{
        ControlGpioData = "31=1";
        led_31 = true;
     }
    ControlGpio();
  }

   chart = new CanvasJS.Chart("chartContainer", {
    title :{
      text: "Proximity Sensor"
    },
    axisY: {
      includeZero: false
    },      
    data: [{
      type: "line",
      dataPoints: dps
    }]
  });

  // distance = createGraph(document.getElementById("distance"), "Distance", 500, 200, 0, 500, false, "cyan");
  run();
}