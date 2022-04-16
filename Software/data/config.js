let config = {};


function onBtnSaveClick() {
    setConfig();
}

function onBtnUpdateClick() {
    const files = document.getElementById('url-file').files[0];
    const formData = new FormData();
    console.log();

    if (files) {
        formData.append('image', files);

        toastInfoShow("Send file in progress...")

        api_rest_upload_firmware(formData)
            .then((response) => {
                toastInfoShow(response);
            })
            .catch((error) => {
                toastErrorShow(error);
            });
    }
}

function onBtnRebootClick() {
    api_rest_restart()
        .then((response) => {
            toastInfoShow(response);
        })
        .catch((error) => {
            toastErrorShow(error);
        });
}

function onBtnResetClick() {
    api_rest_reset_configuration()
        .then((response) => {
            toastInfoShow(response);
        })
        .catch((error) => {
            toastErrorShow(error);
        });
}

function setConfig() {
    // Get all information
    let config = {
        'hostname': document.getElementById('hostname-text').value,
        'timeSaveData': document.getElementById('period-data-save').value,
        'timeSendData': document.getElementById('period-data-send').value,
        'mode': document.getElementById('mode-select').value,
        'timeoutRelay': document.getElementById('timeout-relay').value,
        'mqtt': {
            'enable': document.getElementById('mqtt-enable').checked,
            'hostname': document.getElementById('mqtt-hostname').value,
            'port': document.getElementById('mqtt-port').value,
            'username': document.getElementById('mqtt-username').value,
            'password': document.getElementById('mqtt-password').value,
            'topic': document.getElementById('mqtt-topic').value
        },
        'lineA': {
            'enable': document.getElementById('lineA-enable').checked,
            'name': document.getElementById('lineA-name').value,
            'currentClamp': document.getElementById('lineA-clamp').value,
            'conso': document.getElementById('lineA-consumption').value
        },
        'lineB': {
            'enable': document.getElementById('lineB-enable').checked,
            'name': document.getElementById('lineB-name').value,
            'currentClamp': document.getElementById('lineB-clamp').value,
            'conso': document.getElementById('lineB-consumption').value
        },
        'lineC': {
            'enable': document.getElementById('lineC-enable').checked,
            'name': document.getElementById('lineC-name').value,
            'currentClamp': document.getElementById('lineC-clamp').value,
            'conso': document.getElementById('lineC-consumption').value
        }
    }

    // Send config
    api_rest_set_configuration(config)
        .then((response) => {
            toastInfoShow("Save configuration successfully");
        })
        .catch((error) => {
            toastErrorShow(error);
        });
}

function getConfig() {
    console.log("try to read informations...");
    api_rest_get_informations()
        .then((info) => {
            if (info != null) {
                // Get Version
                if (info.version != null)
                    document.getElementById('version-text').value = info.version;
                // Get Build Date
                if (info.buildDate != null)
                    document.getElementById('buildDate-text').value = info.buildDate;
            }
        }).catch((error) => {
            toastErrorShow("Unable to read the configuration !");
            console.error(error);
            sleep(10000).then(() => getConfig());
        });

    console.log("try to read config...");
    api_rest_get_configuration()
        .then((config) => {
            if (config != null) {
                console.log(config);
                // Get Hostname
                if (config.hostname != null)
                    document.getElementById('hostname-text').value = config.hostname;

                // Get period data save
                if (config.timeSaveData != null)
                    document.getElementById('period-data-save').value = config.timeSaveData;

                // Get period data send
                if (config.timeSendData != null)
                    document.getElementById('period-data-send').value = config.timeSendData;

                // Get Mode
                if (config.mode != null)
                    document.getElementById('mode-select').value = config.mode;

                // Get timeout relay
                if (config.timeoutRelay != null)
                    document.getElementById('timeout-relay').value = config.timeoutRelay;

                // Get MQTT informations
                if (config.mqtt != null)
                {
                    mqtt = config.mqtt
                    if (mqtt.enable != null)
                        document.getElementById('mqtt-enable').checked = mqtt.enable;
                    if (mqtt.enable == true)
                        document.getElementById('card-mqtt').classList.add('show');
                    else
                        document.getElementById('card-mqtt').classList.remove('show');
                    if (mqtt.ipServer != null)
                        document.getElementById('mqtt-hostname').value = mqtt.ipServer;
                    if (mqtt.portServer != null)
                        document.getElementById('mqtt-port').value = mqtt.portServer;
                    if (mqtt.username != null)
                        document.getElementById('mqtt-username').value = mqtt.username;
                    if (mqtt.password != null)
                        document.getElementById('mqtt-password').value = mqtt.password;
                    if (mqtt.topic != null)
                        document.getElementById('mqtt-topic').value = mqtt.topic;
                }

                // Get Line A informations
                if (config.lineA != null)
                {
                    lineA = config.lineA
                    if (lineA.enable != null)
                        document.getElementById('lineA-enable').checked = lineA.enable;
                    if (lineA.enable == true)
                        document.getElementById('card-lineA').classList.add('show');
                    else
                        document.getElementById('card-lineA').classList.remove('show');
                    if (lineA.name != null)
                        document.getElementById('lineA-name').value = lineA.name;
                    if (lineA.currentClamp != null)
                        document.getElementById('lineA-clamp').value = lineA.currentClamp;
                    if (lineA.conso != null)
                        document.getElementById('lineA-consumption').value = lineA.conso;
                }

                // Get Line B informations
                if (config.lineB != null)
                {
                    lineB = config.lineB
                    if (lineB.enable != null)
                        document.getElementById('lineB-enable').checked = lineB.enable;
                    if (lineB.enable == true)
                        document.getElementById('card-lineB').classList.add('show');
                    else
                        document.getElementById('card-lineB').classList.remove('show');
                    if (lineB.name != null)
                        document.getElementById('lineB-name').value = lineB.name;
                    if (lineB.currentClamp != null)
                        document.getElementById('lineB-clamp').value = lineB.currentClamp;
                    if (lineB.conso != null)
                        document.getElementById('lineB-consumption').value = lineB.conso;
                }

                // Get Line C informations
                if (config.lineC != null)
                {
                    lineC = config.lineC
                    if (lineC.enable != null)
                        document.getElementById('lineC-enable').checked = lineC.enable;
                    if (lineC.enable == true)
                        document.getElementById('card-lineC').classList.add('show');
                    else
                        document.getElementById('card-lineC').classList.remove('show');
                    if (lineC.name != null)
                        document.getElementById('lineC-name').value = lineC.name;
                    if (lineC.currentClamp != null)
                        document.getElementById('lineC-clamp').value = lineC.currentClamp;
                    if (lineC.conso != null)
                        document.getElementById('lineC-consumption').value = lineC.conso;
                }

            }
        }).catch((error) => {
            toastErrorShow("Unable to read the configuration !");
            console.error(error);
            // sleep(10000).then(() => getConfig());
        });
}

document.addEventListener('DOMContentLoaded', (function () {
    console.log("config ready !");

    getConfig();
}));