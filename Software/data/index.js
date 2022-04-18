
function onBtnStartClick() {
    _config.play = true;
}

function onBtnStopClick() {
    _config.play = false;
}

const _config = {
    play: true,
    nbMaxPoint: 50,
    lines: [
        {
            name: 'lineA'
        },
        {
            name: 'lineB'
        },
        {
            name: 'lineC'
        }
    ],
    lineFields: [
        {
            name: 'Voltage',
            fieldName: 'voltage',
            unit: 'V',
            color: '#00FF00',
        },
        {
            name: 'Current',
            fieldName: 'current',
            unit: 'A',
            color: '#FF0000',
        },
        {
            name: 'Power',
            fieldName: 'power',
            unit: 'W',
            color: '#0000FF',
        },
        {
            name: 'Cos phy',
            fieldName: 'cosPhy',
            unit: '',
            color: '#FF00FF',
        },
        {
            name: 'Conso',
            fieldName: 'conso',
            unit: 'kW',
            color: '#00FFFF',
        }
    ]
};

let _charts_data = _config.lines.map(line => ({
    labels: [],
    datasets: _config.lineFields.map(confField => ({
        label: confField.name,
        fieldName: confField.fieldName,
        borderColor: confField.color,
        backgroundColor: confField.color,
        yAxisID: confField.fieldName,
        fill: false,
        data: []
    }))
}));

let _charts_options = {
    responsive: true,
    stacked: true,
    tooltips: {
        mode: 'index',
        intersect: false
    },
    // hover: {
    //     mode: 'nearest',
    //     intersect: true
    // },
    scales: {
        conso: {
            type: "linear",
            position: "left",
            ticks: {
                stepSize: 0.5,
            },
            title: {
                display: true,
                text: 'Consumption (kWh)'
            }
        },
        cosPhy: {
            type: "linear",
            position: "left",
            suggestedMax: 1,
            suggestedMin: 0,
            ticks: {
                stepSize: 0.1,
            },
            title: {
                display: true,
                text: 'Cos Phy'
            }
        },
        power: {
            type: "linear",
            position: "left",
            suggestedMax: 1000,
            suggestedMin: 0,
            ticks: {
                stepSize: 100,
            },
            title: {
                display: true,
                text: 'Power (W)'
            }
        },
        current: {
            type: "linear",
            position: "left",
            suggestedMax: 10,
            suggestedMin: 0,
            ticks: {
                stepSize: 1,
            },
            title: {
                display: true,
                text: 'Current (A)'
            }
        },
        voltage: {
            type: "linear",
            position: "left",
            suggestedMax: 250,
            suggestedMin: 210,
            ticks: {
                stepSize: 5,
            },
            title: {
                display: true,
                text: 'Voltage (V)'
            },
        },
    }
};

// Create all Graph
let _charts = [];
_config.lines.forEach((currentLine, index) => {
    ctx = document.getElementById(currentLine.name + '-chart');
    _charts[index] = new Chart(ctx, {
        type: 'line',
        data: _charts_data[index],
        options: _charts_options
    });
});

const time_options = {
    timeZone:"Europe/Paris",
    hour12 : false,
    hour:  "2-digit",
    minute: "2-digit",
    second: "2-digit"
}

// Add data to chart
function addDataToChart(idx_chart, data) {
    time = new Date().toLocaleTimeString("fr-FR", time_options);

    // Iterate all datasets and search the right name
    _charts[idx_chart].data.datasets.forEach(dataset => {
        if (dataset.fieldName in data) {
            dataset.data.push(data[dataset.fieldName]);
            if (dataset.data.length > _config.nbMaxPoint) {
                dataset.data.shift();
            }
        }
    });

    let label = _charts[idx_chart].data.labels;
    label.push(time);
    if (label.length > _config.nbMaxPoint) {
        label.shift();
    }

    _charts[idx_chart].update();
}

let _ws = new WebSocket("ws://" + URI + ":8080");

_ws.onopen = function () {
    // Web Socket is connected, send data using send()
    // ws.send("Message to send");
    console.log("Web Socket connected !");
};

_ws.onmessage = function (evt) {
    let received_msg = JSON.parse(evt.data);
    console.log(received_msg);
    if (_config.play == false)
        return;

    // Add data to all charts
    received_msg.lines.forEach((data, idx) => {
        addDataToChart(idx, data);
    });
};

_ws.onclose = function () {
    // websocket is closed.
    console.log("Web Socket disconnected !");
};


function getConfig() {
    console.log("try to read config...");
    api_rest_get_configuration()
        .then((config) => {
            if (config != null) {

                // Get Line A informations
                if (config.lineA != null)
                {
                    lineA = config.lineA
                    // if (lineA.enable != null)
                    //     document.getElementById('lineA-enable').checked = lineA.enable;

                    if (lineA.name != null)
                        document.getElementById('lineA-name').innerText = lineA.name;
                }

                // Get Line B informations
                if (config.lineB != null)
                {
                    lineB = config.lineB
                    // if (lineB.enable != null)
                    //     document.getElementById('lineB-enable').checked = lineB.enable;

                    if (lineB.name != null)
                        document.getElementById('lineB-name').innerText = lineB.name;

                }

                // Get Line C informations
                if (config.lineC != null)
                {
                    lineC = config.lineC
                    // if (lineC.enable != null)
                    //     document.getElementById('lineC-enable').checked = lineC.enable;

                    if (lineC.name != null)
                        document.getElementById('lineC-name').innerText = lineC.name;
                }

            }
        }).catch((error) => {
            toastErrorShow("Unable to read the configuration !");
            console.error(error);
            // sleep(10000).then(() => getConfig());
        });
}

// Call it when document is ready
document.addEventListener('DOMContentLoaded', (function () {
    console.log("dashboard ready !");

    getConfig();
}));