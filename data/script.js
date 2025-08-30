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
setInterval(loadSchedules, 5000000000000000);

function loadSchedules() {
  // fetch("/schedules")
  fetch("https://mocki.io/v1/0a97100d-fb9f-4508-add7-a19b6d1f52d5")
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
  
  // Create edit form
  const editForm = document.createElement('div');
  editForm.className = 'edit-form';
  editForm.innerHTML = `
    <div class="edit-time-input">
      <input type="time" id="edit-time-${index}" value="${timeDisplay.textContent}" required>
    </div>
    <div class="edit-actions">
      <button class="save-btn" onclick="saveAlarmEdit(${index})">Save</button>
      <button class="cancel-btn" onclick="cancelAlarmEdit(${index})">Cancel</button>
    </div>
  `;
  
  // Replace time display with edit form
  timeDisplay.style.display = 'none';
  timeDisplay.parentNode.insertBefore(editForm, timeDisplay.nextSibling);
  
  // Hide action buttons during edit
  actionsDiv.style.display = 'none';
}

function saveAlarmEdit(index) {
  const newTime = document.getElementById(`edit-time-${index}`).value;
  
  if (!newTime) {
    alert("Please select a valid time");
    return;
  }
  
  // Send update request to server
  fetch("/schedules/edit", {
    method: "POST",
    headers: {
      "Content-Type": "application/json",
    },
    body: JSON.stringify({ 
      index: index, 
      time: newTime 
    }),
  })
    .then((response) => response.json())
    .then((data) => {
      if (data.success) {
        // Update the display
        const timeDisplay = document.getElementById(`time-display-${index}`);
        timeDisplay.textContent = newTime;
        cancelAlarmEdit(index);
        loadSchedules(); // Reload to ensure consistency
      } else {
        alert("Error updating alarm: " + data.message);
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
  const actionsDiv = scheduleItem.querySelector('.schedule-actions');
  const editForm = scheduleItem.querySelector('.edit-form');
  
  // Show time display again
  timeDisplay.style.display = 'block';
  
  // Remove edit form
  if (editForm) {
    editForm.remove();
  }
  
  // Show action buttons again
  actionsDiv.style.display = 'flex';
}
