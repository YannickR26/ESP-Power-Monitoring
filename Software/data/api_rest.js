
async function api_rest_restart() {
    const response = await fetch("http://" + URI + "/restart", {
        method: 'GET',
    }).catch((error) => {
        throw "<b>Unable to send command !</b><br/>" + error;
    });

    if (response.status === 200) {
        if (response.headers.get('Content-Type') == 'application/json')
            return await response.json();
        else
            return await response.text();
    }
    else {
        throw "<b>Response Error !</b><br/>" + "message: " + await response.text() + "<br/>status: " + response.status;
    }
}

async function api_rest_reset_wifi_manager() {
    const response = await fetch("http://" + URI + "/resetWifiManager", {
        method: 'GET',
    }).catch((error) => {
        throw "<b>Unable to send command !</b><br/>" + error;
    });

    if (response.status === 200) {
        if (response.headers.get('Content-Type') == 'application/json')
            return await response.json();
        else
            return await response.text();
    }
    else {
        throw "<b>Response Error !</b><br/>" + "message: " + await response.text() + "<br/>status: " + response.status;
    }
}

async function api_rest_reset_configuration() {
    const response = await fetch("http://" + URI + "/resetConfiguration", {
        method: 'GET',
    }).catch((error) => {
        throw "<b>Unable to send command !</b><br/>" + error;
    });

    if (response.status === 200) {
        if (response.headers.get('Content-Type') == 'application/json')
            return await response.json();
        else
            return await response.text();
    }
    else {
        throw "<b>Response Error !</b><br/>" + "message: " + await response.text() + "<br/>status: " + response.status;
    }
}

async function api_rest_set_configuration(config) {
    const response = await fetch("http://" + URI + "/setConfiguration", {
        method: 'PUT',
        body: JSON.stringify(config),
        headers: {
            'Content-Type': 'application/json'
        }
    }).catch((error) => {
        throw "<b>Unable to send command !</b><br/>" + error;
    });

    if (response.status === 200) {
        if (response.headers.get('Content-Type') == 'application/json')
            return await response.json();
        else
            return await response.text();
    }
    else {
        throw "<b>Response Error !</b><br/>" + "message: " + await response.text() + "<br/>status: " + response.status;
    }
}

async function api_rest_get_status() {
    const response = await fetch("http://" + URI + "/status", {
        method: 'GET',
    }).catch((error) => {
        throw "<b>Unable to send command !</b><br/>" + error;
    });

    if (response.status === 200) {
        if (response.headers.get('Content-Type') == 'application/json')
            return await response.json();
        else
            return await response.text();
    }
    else {
        throw "<b>Response Error !</b><br/>" + "message: " + await response.text() + "<br/>status: " + response.status;
    }
}

async function api_rest_get_informations() {
    const response = await fetch("http://" + URI + "/informations", {
        method: 'GET'
    }).catch((error) => {
        throw "<b>Unable to send command !</b><br/>" + error;
    });

    if (response.status === 200) {
        if (response.headers.get('Content-Type') == 'application/json')
            return await response.json();
        else
            return await response.text();
    }
    else {
        throw "<b>Response Error !</b><br/>" + "message: " + await response.text() + "<br/>status: " + response.status;
    }
}

async function api_rest_get_configuration() {
    const response = await fetch("http://" + URI + "/config.json", {
        method: 'GET',
    }).catch((error) => {
        throw "<b>Unable to send command !</b><br/>" + error;
    });

    if (response.status === 200) {
        if (response.headers.get('Content-Type') == 'application/json')
            return await response.json();
        else
            return await response.text();
    }
    else {
        throw "<b>Response Error !</b><br/>" + "message: " + await response.text() + "<br/>status: " + response.status;
    }
}

async function api_rest_upload_firmware(file) {
    const response = await fetch("http://" + URI + "/update", {
        method: 'POST',
        body: file,
    }).catch((error) => {
        throw "<b>Unable to send command !</b><br/>" + error;
    });

    if (response.status === 200) {
        if (response.headers.get('Content-Type') == 'application/json')
            return await response.json();
        else
            return await response.text();
    }
    else {
        throw "<b>Response Error !</b><br/>" + "message: " + await response.text() + "<br/>status: " + response.status;
    }
}

// **************
// *** Others ***
// **************
function toastInfoShow(text) {
    /* Display Toast Info */
    document.getElementById('toastInfoText').innerHTML = text;
    new bootstrap.Toast(document.getElementById('toastInfo')).show();
}

function toastErrorShow(text) {
    /* Display Toast Error */
    document.getElementById('toastErrorText').innerHTML = text;
    new bootstrap.Toast(document.getElementById('toastError')).show();
}

function sleep(ms) {
    return new Promise(resolve => setTimeout(resolve, ms));
}