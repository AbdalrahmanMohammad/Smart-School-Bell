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
      updateButton("bulb-button", "bulb-status", data.bulb);
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

function toggleBulb() {
  fetch("/bulb/toggle", { method: "POST" })
    .then((response) => response.json())
    .then((data) => {
      updateButton("bulb-button", "bulb-status", data.bulb);
    })
    .catch((error) => {
      console.error("Error:", error);
    });
}

function sendTimeToNodeMCU() {
  const now = new Date();
  
  // Format time in local timezone (YYYY-MM-DDTHH:MM:SS)
  const year = now.getFullYear();
  const month = String(now.getMonth() + 1).padStart(2, '0');
  const day = String(now.getDate()).padStart(2, '0');
  const hours = String(now.getHours()).padStart(2, '0');
  const minutes = String(now.getMinutes()).padStart(2, '0');
  const seconds = String(now.getSeconds()).padStart(2, '0');
  
  const timeString = `${year}-${month}-${day}T${hours}:${minutes}:${seconds}`;
  const timestamp = now.getTime();
  
  console.log("Sending time to NodeMCU:", timeString, "timestamp:", timestamp);
  console.log("Local timezone offset:", now.getTimezoneOffset(), "minutes");
  
  fetch("/send-time", { 
    method: "POST",
    headers: {
      "Content-Type": "application/json",
    },
    body: JSON.stringify({ 
      time: timeString,
      timestamp: timestamp
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

// Load config immediately
loadConfig();

// Set up month/day selector interaction
document.addEventListener('DOMContentLoaded', function() {
  const monthSelect = document.getElementById('schedule-month');
  if (monthSelect) {
    monthSelect.addEventListener('change', updateDaySelector);
  }
});

// Update schedules every 5 seconds
// setInterval(loadSchedules, 5000);

function loadSchedules() {
  console.log("Loading schedules...");
  fetch("/schedules")
    .then((response) => {
      console.log("Response received:", response.status);
      return response.json();
    })
    .then((data) => {
      console.log("Data received:", data);
      console.log("Schedules count:", data.schedules ? data.schedules.length : "undefined");
      displaySchedules(data.schedules);
    })
    .catch((error) => {
      console.error("Error loading schedules:", error);
      document.getElementById("schedules-list").innerHTML =
        "<p>Error loading schedules</p>";
    });
}

function loadConfig() {
  // Bulb is a simple on/off device - no duration config needed
}

// Bulb duration function removed - bulb is simple on/off only

// Global variable to store all schedules
let allSchedules = [];

function displaySchedules(schedules) {
  console.log("displaySchedules called with:", schedules);
  
  if (!schedules) {
    console.error("Schedules is undefined or null");
    document.getElementById("schedules-list").innerHTML = "<p>Error: No schedules data</p>";
    return;
  }
  
  // Store all schedules globally for filtering
  allSchedules = schedules;
  window.currentSchedules = schedules;
  
  // Update total schedule counter
  const totalCounter = document.getElementById("total-schedule-count");
  if (totalCounter) {
    totalCounter.textContent = schedules.length;
  }

  if (schedules.length === 0) {
    document.getElementById("schedules-list").innerHTML = "<p>No schedules configured</p>";
    return;
  }

  // Apply current filter
  filterSchedulesByMonth();
}

function filterSchedulesByMonth() {
  const monthFilter = document.getElementById("month-filter");
  const selectedMonth = monthFilter.value;
  const container = document.getElementById("schedules-list");
  const scheduleCounter = document.getElementById("current-schedule-count");
  
  if (!allSchedules || allSchedules.length === 0) {
    container.innerHTML = "<p>No schedules found</p>";
    if (scheduleCounter) scheduleCounter.textContent = "0";
    return;
  }
  
  const targetMonth = parseInt(selectedMonth);
  const filteredSchedules = allSchedules.filter(schedule => {
    const month = getMonthFromDayOfYear(schedule.d);
    return month === targetMonth;
  });
  
  if (scheduleCounter) {
    scheduleCounter.textContent = filteredSchedules.length;
  }

  if (filteredSchedules.length === 0) {
    container.innerHTML = "<p>No schedules found for selected month</p>";
    return;
  }

  let html = "";
  filteredSchedules.forEach((schedule, originalIndex) => {
    // Find the original index in the full array
    const actualIndex = allSchedules.findIndex(s => s === schedule);
    
    const statusClass = schedule.e ? "enabled" : "disabled";
    const statusText = schedule.e ? "ENABLED" : "DISABLED";
    const formattedDate = formatDayOfYear(schedule.d);

    html += `
      <div class="schedule-item" id="schedule-${actualIndex}">
        <div class="schedule-index">#${actualIndex + 1}</div>
        <div class="schedule-info">
          <div class="schedule-date-row">
            <div class="schedule-date" id="date-display-${actualIndex}">${formattedDate}</div>
            <div class="schedule-status ${statusClass}" onclick="toggleScheduleStatus(${actualIndex})">
              <div class="toggle-switch ${statusClass}"></div>
            </div>
          </div>
          <div class="schedule-times">
            <div class="time-slot">
              <span class="time-label">ON:</span>
              <span class="time-value" id="on-time-display-${actualIndex}">${schedule.on}</span>
            </div>
            <div class="time-slot">
              <span class="time-label">OFF:</span>
              <span class="time-value" id="off-time-display-${actualIndex}">${schedule.off}</span>
            </div>
            <span class="type-badge">BULB</span>
          </div>
        </div>
        <div class="schedule-actions">
          <button class="edit-btn" onclick="editSchedule(${actualIndex})">Edit</button>
        </div>
      </div>
    `;
  });

  container.innerHTML = html;
}

function getMonthFromDayOfYear(dayOfYear) {
  const daysInMonth = [31, 29, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31];
  let month = 0;
  let day = dayOfYear;
  
  while (day > daysInMonth[month]) {
    day -= daysInMonth[month];
    month++;
  }
  
  return month + 1; // Return 1-based month
}

function formatDayOfYear(dayOfYear) {
  const monthNames = [
    "January", "February", "March", "April", "May", "June",
    "July", "August", "September", "October", "November", "December"
  ];
  
  // Calculate month and day from day of year
  let month = 0;
  let day = dayOfYear;
  
  const daysInMonth = [31, 29, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31]; // Using 29 for Feb to handle leap years
  
  while (day > daysInMonth[month]) {
    day -= daysInMonth[month];
    month++;
  }
  
  return `${monthNames[month]} ${day}`;
}

// Function to populate day selector based on selected month
function updateDaySelector() {
  const monthSelect = document.getElementById('schedule-month');
  const daySelect = document.getElementById('schedule-day');
  
  if (!monthSelect || !daySelect) return;
  
  const selectedMonth = parseInt(monthSelect.value);
  if (!selectedMonth) {
    daySelect.innerHTML = '<option value="">Select Day</option>';
    return;
  }
  
  const daysInMonth = [31, 29, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31];
  const maxDays = daysInMonth[selectedMonth - 1];
  
  daySelect.innerHTML = '<option value="">Select Day</option>';
  for (let day = 1; day <= maxDays; day++) {
    const option = document.createElement('option');
    option.value = day;
    option.textContent = day;
    daySelect.appendChild(option);
  }
}

// Function to calculate day of year from month and day
function calculateDayOfYear(month, day) {
  const daysInMonth = [31, 29, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31];
  let dayOfYear = 0;
  
  for (let i = 0; i < month - 1; i++) {
    dayOfYear += daysInMonth[i];
  }
  
  return dayOfYear + day;
}

// updateEditDaySelector function removed - date is now read-only

// updateAddButtonState function removed - no longer needed

// addSchedule function removed - only edit functionality available

function toggleScheduleStatus(index) {
  // This function will toggle the schedule status
  // For now, it just reloads the schedules to show the toggle effect
  // In a real implementation, you'd send a request to update the status
  console.log("Toggling schedule status for index:", index);
  
  // Simulate toggle by reloading (in real app, you'd update the backend)
  loadSchedules();
}

// deleteSchedule function removed - only edit functionality available

function editSchedule(index) {
  const scheduleItem = document.getElementById(`schedule-${index}`);
  const dateDisplay = document.getElementById(`date-display-${index}`);
  const onTimeDisplay = document.getElementById(`on-time-display-${index}`);
  const offTimeDisplay = document.getElementById(`off-time-display-${index}`);
  const actionsDiv = scheduleItem.querySelector('.schedule-actions');
  const statusDisplay = scheduleItem.querySelector('.schedule-status');
  
  // Get current schedule data from the global schedules array
  // We need to access the schedules data that was loaded
  if (!window.currentSchedules || !window.currentSchedules[index]) {
    console.error('Schedule data not available for index:', index);
    return;
  }
  
  const currentSchedule = window.currentSchedules[index];
  const currentEnabled = currentSchedule.e;
  const currentDayOfYear = currentSchedule.d;
  const currentOnTime = currentSchedule.on;
  const currentOffTime = currentSchedule.off;
  
  // Convert day of year to month and day
  const monthNames = ["January", "February", "March", "April", "May", "June",
                     "July", "August", "September", "October", "November", "December"];
  const daysInMonth = [31, 29, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31];
  
  let month = 0;
  let day = currentDayOfYear;
  while (day > daysInMonth[month]) {
    day -= daysInMonth[month];
    month++;
  }
  month += 1; // Convert to 1-based month
  
  // Create comprehensive edit form
  const editForm = document.createElement('div');
  editForm.className = 'edit-form';
  editForm.innerHTML = `
    <div class="edit-content">
      <div class="edit-section">
        <label>Date:</label>
        <div class="readonly-field">
          <span class="readonly-text">${monthNames[month - 1]} ${day}</span>
          <input type="hidden" id="edit-month-${index}" value="${month}">
          <input type="hidden" id="edit-day-${index}" value="${day}">
        </div>
      </div>
      
      <div class="edit-section">
        <label>ON Time:</label>
        <input type="time" id="edit-on-time-${index}" value="${currentOnTime}" required>
      </div>
      
      <div class="edit-section">
        <label>OFF Time:</label>
        <input type="time" id="edit-off-time-${index}" value="${currentOffTime}" required>
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
      <button class="save-btn" onclick="saveScheduleEdit(${index})">Save</button>
      <button class="cancel-btn" onclick="cancelScheduleEdit(${index})">Cancel</button>
    </div>
  `;
  
  // Hide original elements
  dateDisplay.style.display = 'none';
  onTimeDisplay.style.display = 'none';
  offTimeDisplay.style.display = 'none';
  statusDisplay.style.display = 'none';
  actionsDiv.style.display = 'none';
  
  // Insert edit form
  const scheduleInfo = scheduleItem.querySelector('.schedule-info');
  scheduleInfo.appendChild(editForm);
  
  // Month and day selectors removed - date is now read-only
  
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

function saveScheduleEdit(index) {
  const newMonth = parseInt(document.getElementById(`edit-month-${index}`).value);
  const newDay = parseInt(document.getElementById(`edit-day-${index}`).value);
  const newOnTime = document.getElementById(`edit-on-time-${index}`).value;
  const newOffTime = document.getElementById(`edit-off-time-${index}`).value;
  
  // Month and day validation removed - they are now read-only hidden fields
  
  if (!newOnTime) {
    alert("Please select a valid ON time");
    return;
  }
  
  if (!newOffTime) {
    alert("Please select a valid OFF time");
    return;
  }
  
  // Cross-midnight schedules are allowed (e.g., ON at 22:00, OFF at 08:00)
  // No validation needed - the NodeMCU handles this correctly
  
  // Get enabled status
  const newEnabled = document.getElementById(`edit-enabled-${index}`).checked;
  const newDayOfYear = calculateDayOfYear(newMonth, newDay);
  
  // Send update request to server
  fetch("/schedules/edit", {
    method: "POST",
    headers: {
      "Content-Type": "application/json",
    },
    body: JSON.stringify({ 
      index: index, 
      d: newDayOfYear,
      on: newOnTime,
      off: newOffTime,
      e: newEnabled
    }),
  })
    .then((response) => response.json())
    .then((data) => {
      if (data.success) {
        cancelScheduleEdit(index);
        loadSchedules(); // Reload to ensure consistency
      } else {
        alert("Error updating schedule: " + (data.message || "Unknown error"));
      }
    })
    .catch((error) => {
      console.error("Error:", error);
      alert("Error updating schedule");
    });
}

function cancelScheduleEdit(index) {
  const scheduleItem = document.getElementById(`schedule-${index}`);
  const dateDisplay = document.getElementById(`date-display-${index}`);
  const onTimeDisplay = document.getElementById(`on-time-display-${index}`);
  const offTimeDisplay = document.getElementById(`off-time-display-${index}`);
  const statusDisplay = scheduleItem.querySelector('.schedule-status');
  const actionsDiv = scheduleItem.querySelector('.schedule-actions');
  const editForm = scheduleItem.querySelector('.edit-form');
  
  // Show original elements again
  dateDisplay.style.display = 'block';
  onTimeDisplay.style.display = 'inline';
  offTimeDisplay.style.display = 'inline';
  statusDisplay.style.display = 'flex';
  actionsDiv.style.display = 'flex';
  
  // Remove edit form
  if (editForm) {
    editForm.remove();
  }
}
