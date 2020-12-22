var pressuresPlot = document.getElementById('pressures');
var XLsDiv = document.getElementById('XLs');
var XLdDiv = document.getElementById('XLd');
var LsDiv = document.getElementById('Ls');
var LdDiv = document.getElementById('Ld');
var MsDiv = document.getElementById('Ms');
var MdDiv = document.getElementById('Md');
var SsDiv = document.getElementById('Ss');
var SdDiv = document.getElementById('Sd');

var speedsPlot = document.getElementById('speeds');
var XLspeedDiv = document.getElementById('XLspeed');
var LspeedDiv = document.getElementById('Lspeed');
var MspeedDiv = document.getElementById('Mspeed');
var SspeedDiv = document.getElementById('Sspeed');

var venturiDiv = document.getElementById("venturi");
var magnusDiv = document.getElementById("magnus");

var bar = document.getElementById("bar");

var logDiv = document.getElementById("logInfo");
// Plotly.newPlot( TESTER, [{
// x: [1, 2, 3, 4, 5],
// y: [1, 2, 4, 8, 16] }], {
// margin: { t: 0 } } );

var globalX = 0;
var parsePressure = [];

var backupAllPressures = [{ x: [], y: [] }, { x: [], y: [] }, { x: [], y: [] }, { x: [], y: [] }, { x: [], y: [] }, { x: [], y: [] }, { x: [], y: [] }, { x: [], y: [] }];

var backupSpeeds = [{ x: [], y: [] }, { x: [], y: [] }, { x: [], y: [] }, { x: [], y: [] }];

var trace1 = { x: [], y: [], mode: "lines+markers", name: "XL pipe static pressure" };
var trace2 = { x: [], y: [], mode: "lines+markers", name: "XL pipe dynamic pressure" };
var trace3 = { x: [], y: [], mode: "lines+markers", name: "L pipe static pressure" };
var trace4 = { x: [], y: [], mode: "lines+markers", name: "L pipe dynamic pressure" };
var trace5 = { x: [], y: [], mode: "lines+markers", name: "M pipe static pressure" };
var trace6 = { x: [], y: [], mode: "lines+markers", name: "M pipe dynamic pressure" };
var trace7 = { x: [], y: [], mode: "lines+markers", name: "S pipe static pressure" };
var trace8 = { x: [], y: [], mode: "lines+markers", name: "S pipe dynamic pressure" };

var trace9 = { x: [], y: [], mode: "lines+markers", name: "Flow speed in XL-pipe" };
var trace10 = { x: [], y: [], mode: "lines+markers", name: "Flow speed in L-pipe" };
var trace11 = { x: [], y: [], mode: "lines+markers", name: "Flow speed in M-pipe" };
var trace12 = { x: [], y: [], mode: "lines+markers", name: "Flow speed in S-pipe" };

var speedsBar = [{
    x: ["XL", "L", "M", "S"],
    y: [0, 0, 0, 0],
    type: 'bar'
}];

var pressureTraces = [trace1, trace2, trace3, trace4, trace5, trace6, trace7, trace8];
var speedTraces = [trace9, trace10, trace11, trace12];
var speedsZeroFactor = [0, 0, 0, 0];

var speedAverageBuf = [[], [], [], []];
const speedAverageSamplesCount = 20;
var speedAverageSum = [0, 0, 0, 0];

function calibrationWEB(){
    for(let i = 0; i < 4; i++){
        speedsZeroFactor[i] = (speedAverageSum[i] / speedAverageSamplesCount);
    }
}

function redrawSpeeds(){
    var speeds = [];
    for(let i = 0; i < 4; i++){
        speeds[i] = (Math.sqrt(2 * Math.abs(Math.abs(parsePressure[2*i+1]) - Math.abs(parsePressure[2*i])) / 1000));
        if(isNaN(speeds[i])) speeds[i] = speedAverageBuf[i][speedAverageBuf.length - 1];
    }
    //console.log(parsePressure);
    //console.log(speeds);

    if(speedAverageBuf[0].length < speedAverageSamplesCount){
        for(let i = 0; i < 4; i++){
            speedAverageBuf[i].push(speeds[i]);
            speedAverageSum[i] += speeds[i];
        }
    }else{
        for(let i = 0; i < 4; i++){
            speedAverageSum[i] -= speedAverageBuf[i][0];
            speedAverageBuf[i].shift();
            speedAverageBuf[i].push(speeds[i]);
            speedAverageSum[i] += speeds[i];
        }
    }

    XLspeedDiv.innerHTML = "Flow speed in XL-pipe <br />" + (speedAverageSum[0] / speedAverageSamplesCount - speedsZeroFactor[0]).toFixed(4) + "<br /> m/s";
    LspeedDiv.innerHTML = "Flow speed in L-pipe <br />" + (speedAverageSum[1] / speedAverageSamplesCount - speedsZeroFactor[1]).toFixed(4) + "<br /> m/s";
    MspeedDiv.innerHTML = "Flow speed in M-pipe <br />" + (speedAverageSum[2] / speedAverageSamplesCount - speedsZeroFactor[2]).toFixed(4) + "<br /> m/s";
    SspeedDiv.innerHTML = "Flow speed in S-pipe <br />" + (speedAverageSum[3] / speedAverageSamplesCount - speedsZeroFactor[3]).toFixed(4) + "<br /> m/s";

    for(let i = 0; i < 4; i++){
        backupSpeeds[i].x.push(globalX);
        backupSpeeds[i].y.push((speedAverageSum[i] / speedAverageSamplesCount - speedsZeroFactor[0]));
        speedTraces[i].x.push(globalX);
        speedTraces[i].y.push((speedAverageSum[i] / speedAverageSamplesCount - speedsZeroFactor[0]));
        if(globalX > 60){
            speedTraces[i].x.shift();
            speedTraces[i].y.shift();
        }
        speedsBar[0].y[i] = speedAverageSum[i] / speedAverageSamplesCount - speedsZeroFactor[i];
    }
    Plotly.newPlot(speedsPlot, speedTraces, {title: "Speed of water in the pipes"});
    Plotly.newPlot(bar, speedsBar, {title: "Speed of water in the pipes"});
}

function redrawPressures(){
    //parseData = parseData.pop();
    XLsDiv.innerHTML = "XL static pressure:<br />" + parsePressure[0].toFixed(2) + "<br /> Pa";
    XLdDiv.innerHTML = "XL dynamic pressure:<br />" + parsePressure[1].toFixed(2) + "<br /> Pa";
    LsDiv.innerHTML = "L static pressure:<br />" + parsePressure[2].toFixed(2) + "<br /> Pa";
    LdDiv.innerHTML = "L dynamic pressure:<br />" + parsePressure[3].toFixed(2) + "<br /> Pa";
    MsDiv.innerHTML = "M static pressure:<br />" + parsePressure[4].toFixed(2) + "<br /> Pa";
    MdDiv.innerHTML = "M dynamic pressure:<br />" + parsePressure[5].toFixed(2) + "<br /> Pa";
    SsDiv.innerHTML = "S static pressure:<br />" + parsePressure[6].toFixed(2) + "<br /> Pa";
    SdDiv.innerHTML = "S dynamic pressure:<br />" + parsePressure[7].toFixed(2) + "<br /> Pa";
    for(let i = 0; i < 8; i++){
        pressureTraces[i].x.push(globalX);
        pressureTraces[i].y.push(parsePressure[i]);
        backupAllPressures[i].x.push(globalX);
        backupAllPressures[i].y.push(parsePressure[i]);
        if(globalX > 60){
            pressureTraces[i].x.shift();
            pressureTraces[i].y.shift();
        }
    }
    Plotly.newPlot(pressuresPlot, pressureTraces, {title: "All pressures"});
}

function parseAnswer(thisAnswer){
    //console.log("parseAnswer() is running");
    console.log(thisAnswer);
    var parseData = thisAnswer.toString().split("@");
    if(parseData.length > 8) logDiv.innerText = parseData[8];
    //console.log(parseData);
    // for(let i = 0; i < 8; i++){
    //     parsePressure.push(parseFloat(parseData[i]));
    // }
    for(let i = 0; i < 8; i++){
        parsePressure[i] = parseFloat(parseData[i]) * 4.0089907229549276389201478804426;
    }
    //console.log(parsePressure);
    globalX++;
    redrawPressures();
    redrawSpeeds();
}

function requestPressure(){
    //console.log("requestPressure() is running");
    let xhr = new XMLHttpRequest();
    //let body = "getPressure=1";
    xhr.open("POST", '/post', true);
    xhr.setRequestHeader('Content-Type', 'application/x-www-form-urlencoded');
    xhr.onreadystatechange = function() {
        if (this.readyState != 4){
            console.log("fail.")
            return;
        }
        //console.log(this.responseText);
        parseAnswer(this.responseText);
    };
    xhr.send(setParameters(1, 0, 0, 0, 0));
}

function showVenturi(){
    magnusDiv.style.display = "none";
    venturiDiv.style.display = "block";
}

function showMagnus() {
    venturiDiv.style.display = "none";
    magnusDiv.style.display = "block";
}

function downloadLog(){
    clearInterval(refreshInterval);
    let myString = "pressure 1 X\tpressure 1 Y\tpressure 2 X\tpressure 2 Y\tpressure 3 X\tpressure 3 Y\tpressure 4 X\tpressure 4 Y\tpressure 5 X\tpressure 5 Y\tpressure 6 X\tpressure 6 Y\tpressure 7 X\tpressure 7 Y\tpressure 8 X\tpressure 8 Y\tspeed XL abscissa\tspeed XL ordinate\tspeed L abscissa\tspeed L ordinate\tspeed M abscissa\tspeed M ordinate\tspeed S abscissa\tspeed S ordinate\n"
    for(let i = 0; i < globalX; i++){
        for(let q = 0; q < 8; q++){
            myString += (backupAllPressures[q].x[i].toString() + "\t" + backupAllPressures[q].y[i].toString() + "\t");
        }
        for(let q = 0; q < 4; q++){
            myString += (backupSpeeds[q].x[i].toString() + "\t" + backupSpeeds[q].y[i].toString() + "\t");
        }
        myString += "\n";
    }
    let a = document.createElement("a");
    let file = new Blob([myString], {type: 'application/json'});
    a.href = URL.createObjectURL(file);
    a.download = "example.txt";
    a.click();
    refreshInterval = setInterval(requestPressure, 50);
}

function calibration(){
    clearInterval(refreshInterval);
    let xhr = new XMLHttpRequest();
    //let body = "calibration=1";
    xhr.open("POST", '/post', true);
    xhr.setRequestHeader('Content-Type', 'application/x-www-form-urlencoded');
    xhr.onreadystatechange = function() {
        if (this.readyState != 4){
            console.log("fail.")
            return;
        }else{
            //console.log(this.responseText);
            document.getElementsByClassName("buttonCalibration").innerText = "OK";
            setTimeout(function(){
                document.getElementsByClassName("buttonCalibration").innerText = "Calibration";
            }, 2000);
        }
    };
    xhr.send(setParameters(0, 1, 0, 0, 0));
    refreshInterval = setInterval(requestPressure, 50);
}

function setParameters(getPressure, calibration, getMagnusParams, _motorStart, _motorStop){
    let req = "getPressure=" + getPressure.toString() + "&calibration=" + calibration.toString() + "&getMagnusParams=" + getMagnusParams.toString() + "&motorStart=" + _motorStart.toString() + "&motorStop=" + _motorStop.toString();
    console.log(req);
    return req;
}

function motorStart() {
    clearInterval(refreshInterval);
    let xhr = new XMLHttpRequest();
    //let body = "calibration=1";
    xhr.open("POST", '/post', true);
    xhr.setRequestHeader('Content-Type', 'application/x-www-form-urlencoded');
    xhr.onreadystatechange = function() {
        if (this.readyState != 4){
            console.log("fail.")
            return;
        }
    };
    xhr.send(setParameters(0, 0, 0, 1, 0));
    refreshInterval = setInterval(requestPressure, 50);
}

function motorStop() {
    clearInterval(refreshInterval);
    let xhr = new XMLHttpRequest();
    //let body = "calibration=1";
    xhr.open("POST", '/post', true);
    xhr.setRequestHeader('Content-Type', 'application/x-www-form-urlencoded');
    xhr.onreadystatechange = function() {
        if (this.readyState != 4){
            console.log("fail.")
            return;
        }
    };
    xhr.send(setParameters(0, 0, 0, 0, 1));
    refreshInterval = setInterval(requestPressure, 50);
}

var refreshInterval = setInterval(requestPressure, 50);