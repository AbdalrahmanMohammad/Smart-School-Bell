function updateTime() {
  fetch("/time")
    .then((response) => response.text())
    .then((time) => {
      document.getElementById("current-time").textContent = time;
    })
    .catch((error) => {
      document.getElementById("current-time").textContent =
        "Error loading time";
      console.error("Error:", error);
    });
}

function updateDeviceStatus() {
  fetch("/status")
    .then((response) => response.json())
    .then((data) => {
      updateButton("led-button", "led-status", data.led);
      updateButton("bell-button", "bell-status", data.bell);
    })
    .catch((error) => {
      console.error("Error:", error);
    });
}

function updateButton(buttonId, statusId, isOn) {
  const button = document.getElementById(buttonId);
  const status = document.getElementById(statusId);

  if (isOn) {
    button.className = "control-btn on";
    status.textContent = "ON";
  } else {
    button.className = "control-btn off";
    status.textContent = "OFF";
  }
}

function toggleLED() {
  fetch("/led/toggle", { method: "POST" })
    .then((response) => response.json())
    .then((data) => {
      updateButton("led-button", "led-status", data.led);
    })
    .catch((error) => {
      console.error("Error:", error);
    });
}

function toggleBell() {
  fetch("/bell/toggle", { method: "POST" })
    .then((response) => response.json())
    .then((data) => {
      updateButton("bell-button", "bell-status", data.bell);
    })
    .catch((error) => {
      console.error("Error:", error);
    });
}

function sendTimeToNodeMCU() {
  const now = new Date();
  const timeString = now.toLocaleTimeString();
  const dateString = now.toLocaleDateString();
  const fullDateTime = `${dateString} ${timeString}`;
  
  console.log("Sending time to NodeMCU:", fullDateTime);
  
  fetch("/send-time", { 
    method: "POST",
    headers: {
      "Content-Type": "application/json",
    },
    body: JSON.stringify({ 
      time: fullDateTime,
      timestamp: now.getTime()
    }),
  })
    .then((response) => response.json())
    .then((data) => {
      if (data.success) {
        console.log("Time sent successfully to NodeMCU");
        // Show a brief success message
        const button = document.getElementById("send-time-btn");
        const originalText = button.textContent;
        button.textContent = "Time Sent!";
        button.style.background = "#4caf50";
        setTimeout(() => {
          button.textContent = originalText;
          button.style.background = "";
        }, 2000);
      } else {
        console.error("Error sending time:", data.message);
        alert("Error sending time to NodeMCU: " + data.message);
      }
    })
    .catch((error) => {
      console.error("Error:", error);
      alert("Error sending time to NodeMCU");
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

// Load schedules immediately when page loads
loadSchedules();

// Update schedules every 5 seconds
// setInterval(loadSchedules, 5000);

function loadSchedules() {
  fetch("/schedules")
  // fetch("https://mocki.io/v1/0a97100d-fb9f-4508-add7-a19b6d1f52d5")
    .then((response) => response.json())
    .then((data) => {
      displaySchedules(data.schedules);
    })
    .catch((error) => {
      console.error("Error loading schedules:", error);
      document.getElementById("schedules-list").innerHTML =
        "<p>Error loading schedules</p>";
    });
}

function displaySchedules(schedules) {
  const container = document.getElementById("schedules-list");

  if (schedules.length === 0) {
    container.innerHTML = "<p>No alarms scheduled</p>";
    return;
  }

  let html = "";
  schedules.forEach((schedule, index) => {
    const daysHtml = getDaysHTML(schedule.days);
    const statusClass = schedule.enabled ? "enabled" : "disabled";
    const statusText = schedule.enabled ? "ENABLED" : "DISABLED";

    html += `
      <div class="schedule-item" id="schedule-${index}">
        <div class="schedule-info">
          <div class="schedule-time-row">
            <div class="schedule-time" id="time-display-${index}">${schedule.time}</div>
            <div class="schedule-status ${statusClass}" onclick="toggleAlarmStatus(${index})">
              <div class="toggle-switch ${statusClass}"></div>
            </div>
          </div>
          <div class="schedule-days">       
            ${daysHtml}
          </div>
        </div>
        <div class="schedule-actions">
          <button class="edit-btn" onclick="editAlarm(${index})">Edit</button>
          <button class="delete-btn" onclick="deleteAlarm(${index})">Delete</button>
        </div>
      </div>
    `;
  });

  container.innerHTML = html;
}

function getDaysHTML(selectedDays) {
  const dayNames = ["Sat", "Sun", "Mon", "Tue", "Wed", "Thu", "Fri"];

  return dayNames
    .map((day, index) => {
      const isSelected = selectedDays.includes(index);
      return `<div class="days${isSelected ? " selected" : ""}">${day}</div>`;
    })
    .join("");
}

function addAlarm() {
  const time = document.getElementById("alarm-time").value;

  if (!time) {
    alert("Please select a time");
    return;
  }

  const selectedDays = [];
  document
    .querySelectorAll('.day-checkboxes input[type="checkbox"]:checked')
    .forEach((checkbox) => {
      selectedDays.push(parseInt(checkbox.value));
    });

  if (selectedDays.length === 0) {
    alert("Please select at least one day");
    return;
  }

  const newAlarm = {
    time: time,
    days: selectedDays,
    enabled: true,
  };

  fetch("/schedules/add", {
    method: "POST",
    headers: {
      "Content-Type": "application/json",
    },
    body: JSON.stringify(newAlarm),
  })
    .then((response) => response.json())
    .then((data) => {
      if (data.success) {
        // Clear form
        document.getElementById("alarm-time").value = "";
        document
          .querySelectorAll('.day-checkboxes input[type="checkbox"]')
          .forEach((checkbox) => {
            checkbox.checked = false;
          });

        // Reload schedules
        loadSchedules();
      } else {
        alert("Error adding alarm: " + data.message);
      }
    })
    .catch((error) => {
      console.error("Error:", error);
      alert("Error adding alarm");
    });
}

function toggleAlarmStatus(index) {
  // This function will toggle the alarm status
  // For now, it just reloads the schedules to show the toggle effect
  // In a real implementation, you'd send a request to update the status
  console.log("Toggling alarm status for index:", index);
  
  // Simulate toggle by reloading (in real app, you'd update the backend)
  loadSchedules();
}

function deleteAlarm(index) {
  console.log("Attempting to delete alarm at index:", index);
  if (confirm("Are you sure you want to delete this alarm?")) {
    console.log("Deleting alarm at index:", index);
    fetch("/schedules/delete", {
      method: "POST",
      headers: {
        "Content-Type": "application/json",
      },
      body: JSON.stringify({ index: index }),
    })
      .then((response) => response.json())
      .then((data) => {
        if (data.success) {
          loadSchedules();
        } else {
          alert("Error deleting alarm: " + data.message);
        }
      })
      .catch((error) => {
        console.error("Error:", error);
        alert("Error deleting alarm");
      });
  }
}

function editAlarm(index) {
  const scheduleItem = document.getElementById(`schedule-${index}`);
  const timeDisplay = document.getElementById(`time-display-${index}`);
  const actionsDiv = scheduleItem.querySelector('.schedule-actions');
  const daysDisplay = scheduleItem.querySelector('.schedule-days');
  const statusDisplay = scheduleItem.querySelector('.schedule-status');
  
  // Get current schedule data
  const currentTime = timeDisplay.textContent;
  const currentDays = Array.from(daysDisplay.querySelectorAll('.days.selected')).map(day => {
    const dayText = day.textContent;
    const dayNames = ["Sat", "Sun", "Mon", "Tue", "Wed", "Thu", "Fri"];
    return dayNames.indexOf(dayText);
  });
  const currentEnabled = statusDisplay.querySelector('.toggle-switch').classList.contains('enabled');
  
  // Create comprehensive edit form
  const editForm = document.createElement('div');
  editForm.className = 'edit-form';
  editForm.innerHTML = `
    <div class="edit-content">
      <div class="edit-section">
        <label>Time:</label>
        <input type="time" id="edit-time-${index}" value="${currentTime}" required>
      </div>
      
      <div class="edit-section">
        <label>Days:</label>
        <div class="edit-day-checkboxes">
          <input type="checkbox" id="edit-day-0-${index}" value="0" ${currentDays.includes(0) ? 'checked' : ''}>
          <label for="edit-day-0-${index}">Sun</label>
          <input type="checkbox" id="edit-day-1-${index}" value="1" ${currentDays.includes(1) ? 'checked' : ''}>
          <label for="edit-day-1-${index}">Mon</label>
          <input type="checkbox" id="edit-day-2-${index}" value="2" ${currentDays.includes(2) ? 'checked' : ''}>
          <label for="edit-day-2-${index}">Tue</label>
          <input type="checkbox" id="edit-day-3-${index}" value="3" ${currentDays.includes(3) ? 'checked' : ''}>
          <label for="edit-day-3-${index}">Wed</label>
          <input type="checkbox" id="edit-day-4-${index}" value="4" ${currentDays.includes(4) ? 'checked' : ''}>
          <label for="edit-day-4-${index}">Thu</label>
          <input type="checkbox" id="edit-day-5-${index}" value="5" ${currentDays.includes(5) ? 'checked' : ''}>
          <label for="edit-day-5-${index}">Fri</label>
          <input type="checkbox" id="edit-day-6-${index}" value="6" ${currentDays.includes(6) ? 'checked' : ''}>
          <label for="edit-day-6-${index}">Sat</label>
        </div>
      </div>
      
             <div class="edit-section">
         <label>Status:</label>
         <div class="edit-toggle-container">
           <input type="checkbox" id="edit-enabled-${index}" ${currentEnabled ? 'checked' : ''}>
           <div class="toggle-switch ${currentEnabled ? 'enabled' : 'disabled'}" id="edit-toggle-${index}"></div>
           <span class="edit-toggle-text">${currentEnabled ? 'Enabled' : 'Disabled'}</span>
         </div>
       </div>
    </div>
    
    <div class="edit-actions">
      <button class="save-btn" onclick="saveAlarmEdit(${index})">Save</button>
      <button class="cancel-btn" onclick="cancelAlarmEdit(${index})">Cancel</button>
    </div>
  `;
  
  // Hide original elements
  timeDisplay.style.display = 'none';
  daysDisplay.style.display = 'none';
  statusDisplay.style.display = 'none';
  actionsDiv.style.display = 'none';
  
  // Insert edit form
  const scheduleInfo = scheduleItem.querySelector('.schedule-info');
  scheduleInfo.appendChild(editForm);
  
  // Add event listener for toggle text update and visual feedback
  const toggleCheckbox = document.getElementById(`edit-enabled-${index}`);
  const toggleText = editForm.querySelector('.edit-toggle-text');
  const toggleSwitch = document.getElementById(`edit-toggle-${index}`);
  
  function updateToggleState() {
    toggleText.textContent = toggleCheckbox.checked ? 'Enabled' : 'Disabled';
    
    // Update the toggle switch classes
    if (toggleCheckbox.checked) {
      toggleSwitch.className = 'toggle-switch enabled';
    } else {
      toggleSwitch.className = 'toggle-switch disabled';
    }
  }
  
  toggleCheckbox.addEventListener('change', updateToggleState);
  
  // Set initial state
  updateToggleState();
  
  // Make the toggle switch clickable
  toggleSwitch.addEventListener('click', function() {
    toggleCheckbox.checked = !toggleCheckbox.checked;
    updateToggleState();
  });
}

function saveAlarmEdit(index) {
  const newTime = document.getElementById(`edit-time-${index}`).value;
  
  if (!newTime) {
    alert("Please select a valid time");
    return;
  }
  
  // Get selected days
  const selectedDays = [];
  document.querySelectorAll(`#schedule-${index} .edit-day-checkboxes input[type="checkbox"]:checked`).forEach(checkbox => {
    selectedDays.push(parseInt(checkbox.value));
  });
  
  if (selectedDays.length === 0) {
    alert("Please select at least one day");
    return;
  }
  
  // Get enabled status
  const newEnabled = document.getElementById(`edit-enabled-${index}`).checked;
  
  // Send update request to server
  fetch("/schedules/edit", {
    method: "POST",
    headers: {
      "Content-Type": "application/json",
    },
    body: JSON.stringify({ 
      index: index, 
      time: newTime,
      days: selectedDays,
      enabled: newEnabled
    }),
  })
    .then((response) => response.json())
    .then((data) => {
      if (data.success) {
        cancelAlarmEdit(index);
        loadSchedules(); // Reload to ensure consistency
      } else {
        alert("Error updating alarm: " + (data.message || "Unknown error"));
      }
    })
    .catch((error) => {
      console.error("Error:", error);
      alert("Error updating alarm");
    });
}

function cancelAlarmEdit(index) {
  const scheduleItem = document.getElementById(`schedule-${index}`);
  const timeDisplay = document.getElementById(`time-display-${index}`);
  const daysDisplay = scheduleItem.querySelector('.schedule-days');
  const statusDisplay = scheduleItem.querySelector('.schedule-status');
  const actionsDiv = scheduleItem.querySelector('.schedule-actions');
  const editForm = scheduleItem.querySelector('.edit-form');
  
  // Show original elements again
  timeDisplay.style.display = 'block';
  daysDisplay.style.display = 'flex';
  statusDisplay.style.display = 'flex';
  actionsDiv.style.display = 'flex';
  
  // Remove edit form
  if (editForm) {
    editForm.remove();
  }
}
