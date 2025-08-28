function updateTime() {
  fetch('/time')
    .then(response => response.text())
    .then(time => {
      document.getElementById('current-time').textContent = time;
    })
    .catch(error => {
      document.getElementById('current-time').textContent = 'Error loading time';
      console.error('Error:', error);
    });
}

function updateDeviceStatus() {
  fetch('/status')
    .then(response => response.json())
    .then(data => {
      updateButton('led-button', 'led-status', data.led);
      updateButton('bell-button', 'bell-status', data.bell);
    })
    .catch(error => {
      console.error('Error:', error);
    });
}

function updateButton(buttonId, statusId, isOn) {
  const button = document.getElementById(buttonId);
  const status = document.getElementById(statusId);
  
  if (isOn) {
    button.className = 'control-btn on';
    status.textContent = 'ON';
  } else {
    button.className = 'control-btn off';
    status.textContent = 'OFF';
  }
}

function toggleLED() {
  fetch('/led/toggle', { method: 'POST' })
    .then(response => response.json())
    .then(data => {
      updateButton('led-button', 'led-status', data.led);
    })
    .catch(error => {
      console.error('Error:', error);
    });
}

function toggleBell() {
  fetch('/bell/toggle', { method: 'POST' })
    .then(response => response.json())
    .then(data => {
      updateButton('bell-button', 'bell-status', data.bell);
    })
    .catch(error => {
      console.error('Error:', error);
    });
}

// Update time immediately when page loads
updateTime();

// Update device status immediately when page loads
updateDeviceStatus();

// Update time every second
setInterval(updateTime, 1000);

// Update device status every second
setInterval(updateDeviceStatus, 1000);
