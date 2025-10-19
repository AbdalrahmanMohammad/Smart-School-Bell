function updateTime() {
  fetch("/time")
    .then((response) => response.text())
    .then((time) => {
      document.getElementById("current-time").textContent = time;
    })
    .catch((error) => {
      document.getElementById("current-time").textContent =
        "خطأ في تحميل الوقت";
      console.error("Error:", error);
    });
}

function updateDeviceStatus() {
  fetch("/status")
    .then((response) => response.json())
    .then((data) => {
      updateButton("led-button", "led-status", data.led);
      updateButton("bulb-button", "bulb-status", data.bulb);
      // Store LED state for manual button control
      window.ledState = data.led;
      // Store stopLedFlag state
      window.stopLedFlag = data.stopLedFlag;
      // Update button states based on stopLedFlag
      updateButtonStates();
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
    status.textContent = "تشغيل";
  } else {
    button.className = "control-btn off";
    status.textContent = "إيقاف";
  }
  
  // Update manual button state based on automatic mode
  updateManualButtonState();
}

function toggleLED() {
  // Check if stopLedFlag is true - if so, disable LED control
  if (window.stopLedFlag) {
    alert("تم إيقاف التحكم في الإضاءة - لا يمكن التحكم في LED");
    return;
  }
  
  fetch("/led/toggle", { method: "POST" })
    .then((response) => response.json())
    .then((data) => {
      updateButton("led-button", "led-status", data.led);
      // Update LED state for manual button control
      window.ledState = data.led;
    })
    .catch((error) => {
      console.error("Error:", error);
    });
}

function toggleBulb() {
  // Check if stopLedFlag is true - if so, disable bulb control
  if (window.stopLedFlag) {
    alert("تم إيقاف التحكم في الإضاءة - لا يمكن التحكم في المصباح");
    return;
  }
  
  // Check if automatic mode is on - if so, disable manual control
  if (window.ledState) {
    alert("التحكم التلقائي مفعل - لا يمكن التحكم اليدوي");
    return;
  }
  
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
        console.log("تم إرسال الوقت بنجاح إلى NodeMCU");
        // Show a brief success message
        const button = document.getElementById("send-time-btn");
        const originalText = button.textContent;
        button.textContent = "تم الإرسال!";
        button.style.background = "#4caf50";
        setTimeout(() => {
          button.textContent = originalText;
          button.style.background = "";
        }, 2000);
      } else {
        console.error("Error sending time:", data.message);
        alert("خطأ في إرسال الوقت إلى NodeMCU: " + data.message);
      }
    })
    .catch((error) => {
      console.error("Error:", error);
      alert("خطأ في إرسال الوقت إلى NodeMCU");
    });
}

// Function to update button states based on stopLedFlag and automatic mode
function updateButtonStates() {
  const ledButton = document.getElementById("led-button");
  const bulbButton = document.getElementById("bulb-button");
  const warningMessage = document.getElementById("stop-led-warning");
  
  if (!ledButton || !bulbButton) return;
  
  if (window.stopLedFlag) {
    // stopLedFlag is true - disable both buttons and show warning
    ledButton.disabled = true;
    ledButton.className = "control-btn disabled";
    ledButton.style.opacity = "0.5";
    ledButton.style.cursor = "not-allowed";
    
    bulbButton.disabled = true;
    bulbButton.className = "control-btn disabled";
    bulbButton.style.opacity = "0.5";
    bulbButton.style.cursor = "not-allowed";
    
    // Show warning message
    if (warningMessage) {
      warningMessage.style.display = "block";
    }
  } else {
    // stopLedFlag is false - restore normal behavior and hide warning
    if (warningMessage) {
      warningMessage.style.display = "none";
    }
    
    // LED button is always enabled
    ledButton.disabled = false;
    ledButton.style.opacity = "1";
    ledButton.style.cursor = "pointer";
    // Restore LED button class based on LED state
    const ledStatus = document.getElementById("led-status");
    if (ledStatus) {
      const isOn = ledStatus.textContent === "تشغيل";
      ledButton.className = isOn ? "control-btn on" : "control-btn off";
    }
    
    // Bulb button depends on automatic mode
    if (window.ledState) {
      // Automatic mode is ON - disable manual button
      bulbButton.disabled = true;
      bulbButton.className = "control-btn disabled";
      bulbButton.style.opacity = "0.5";
      bulbButton.style.cursor = "not-allowed";
    } else {
      // Automatic mode is OFF - enable manual button
      bulbButton.disabled = false;
      bulbButton.style.opacity = "1";
      bulbButton.style.cursor = "pointer";
      // Restore original button class based on bulb state
      const bulbStatus = document.getElementById("bulb-status");
      if (bulbStatus) {
        const isOn = bulbStatus.textContent === "تشغيل";
        bulbButton.className = isOn ? "control-btn on" : "control-btn off";
      }
    }
  }
}

// Function to update manual button state based on automatic mode (legacy function)
function updateManualButtonState() {
  // This function is now handled by updateButtonStates()
  updateButtonStates();
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
  
  // Set current month when page loads
  setCurrentMonth();

  // Emergency panel is now always visible
  setupEmergencyModal();
});

// Function to set the current month in the dropdown
function setCurrentMonth() {
  const monthSelect = document.getElementById('month-filter');
  if (monthSelect) {
    try {
      const currentDate = new Date();
      const currentMonth = currentDate.getMonth() + 1; // getMonth() returns 0-11, we need 1-12
      
      // Set the dropdown to current month
      monthSelect.value = currentMonth.toString();
      
      // Trigger the filter to show current month's schedules
      filterSchedulesByMonth();
    } catch (error) {
      // If anything goes wrong, default to January
      monthSelect.value = '1';
      filterSchedulesByMonth();
    }
  }
}

function setupEmergencyModal() {
  const trigger = document.getElementById('emergency-trigger');
  const modal = document.getElementById('emergency-modal');
  if (!trigger || !modal) return;

  let clickCount = 0;
  let clickTimer = null;

  const reset = () => {
    clickCount = 0;
    clearTimeout(clickTimer);
    clickTimer = null;
  };

  const open = () => {
    modal.style.display = 'flex';
    reset();
  };

  // Require a quick double-click to open (within 500ms)
  trigger.addEventListener('click', () => {
    clickCount += 1;
    if (clickCount === 1) {
      clickTimer = setTimeout(reset, 500);
    } else if (clickCount === 2) {
      open();
    }
  });

  // Also support long-press (1.5s)
  let pressTimer = null;
  const startPress = () => {
    clearTimeout(pressTimer);
    pressTimer = setTimeout(open, 1500);
  };
  const cancelPress = () => clearTimeout(pressTimer);
  trigger.addEventListener('mousedown', startPress);
  trigger.addEventListener('mouseup', cancelPress);
  trigger.addEventListener('mouseleave', cancelPress);
  trigger.addEventListener('touchstart', startPress, { passive: true });
  trigger.addEventListener('touchend', cancelPress);
  trigger.addEventListener('touchcancel', cancelPress);

  // Close when clicking backdrop
  modal.addEventListener('click', (e) => {
    if (e.target === modal) closeEmergencyModal();
  });
}

function closeEmergencyModal() {
  const modal = document.getElementById('emergency-modal');
  if (modal) modal.style.display = 'none';
}

// Emergency API calls
function emergencyStopLed(turnOn) {
  const endpoint = turnOn ? '/stop-led/on' : '/stop-led/off';
  fetch(endpoint, { method: 'POST' })
    .then(r => r.json().catch(() => ({})))
    .then(() => {
      // refresh status UI after change
      updateDeviceStatus();
      // small visual feedback on buttons
      const onBtn = document.getElementById('stop-led-on');
      const offBtn = document.getElementById('stop-led-off');
      if (turnOn) {
        if (onBtn) onBtn.className = 'control-btn off';
        if (offBtn) offBtn.className = 'control-btn on';
      } else {
        if (onBtn) onBtn.className = 'control-btn off';
        if (offBtn) offBtn.className = 'control-btn on';
      }
    })
    .catch(err => {
      console.error('Emergency API error', err);
      alert('تعذر تنفيذ إجراء الطوارئ');
    });
}

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
        "<p>خطأ في تحميل الجداول</p>";
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
    document.getElementById("schedules-list").innerHTML = "<p>خطأ: لا توجد بيانات جداول</p>";
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
    document.getElementById("schedules-list").innerHTML = "<p>لا توجد جداول مُعدة</p>";
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
    container.innerHTML = "<p>لا توجد جداول</p>";
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
    container.innerHTML = "<p>لا توجد جداول للشهر المحدد</p>";
    return;
  }

  let html = "";
  filteredSchedules.forEach((schedule, originalIndex) => {
    // Find the original index in the full array
    const actualIndex = allSchedules.findIndex(s => s === schedule);
    
    // Enable/disable logic removed - all schedules are always active
    const formattedDate = formatDayOfYear(schedule.d);

    html += `
      <div class="schedule-item" id="schedule-${actualIndex}">
        <div class="schedule-index">#${actualIndex + 1}</div>
        <div class="schedule-info">
          <div class="schedule-date-row">
            <div class="schedule-date" id="date-display-${actualIndex}">${formattedDate}</div>
          </div>
          <div class="schedule-times">
            <div class="time-slot">
              <span class="time-label">تشغيل:</span>
              <span class="time-value" id="on-time-display-${actualIndex}">${schedule.on}</span>
            </div>
            <div class="time-slot">
              <span class="time-label">إيقاف:</span>
              <span class="time-value" id="off-time-display-${actualIndex}">${schedule.off}</span>
            </div>
            <span class="type-badge">ضوء الشارع</span>
          </div>
        </div>
        <div class="schedule-actions">
          <button class="edit-btn" onclick="editSchedule(${actualIndex})">تعديل</button>
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
    "يناير", "فبراير", "مارس", "أبريل", "مايو", "يونيو",
    "يوليو", "أغسطس", "سبتمبر", "أكتوبر", "نوفمبر", "ديسمبر"
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

// toggleScheduleStatus function removed - enable/disable logic removed

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
  const currentDayOfYear = currentSchedule.d;
  const currentOnTime = currentSchedule.on;
  const currentOffTime = currentSchedule.off;
  
  // Convert day of year to month and day
  const monthNames = ["يناير", "فبراير", "مارس", "أبريل", "مايو", "يونيو",
                     "يوليو", "أغسطس", "سبتمبر", "أكتوبر", "نوفمبر", "ديسمبر"];
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
        <label>التاريخ:</label>
        <div class="readonly-field">
          <span class="readonly-text">${monthNames[month - 1]} ${day}</span>
          <input type="hidden" id="edit-month-${index}" value="${month}">
          <input type="hidden" id="edit-day-${index}" value="${day}">
        </div>
      </div>
      
      <div class="edit-section">
        <label>وقت التشغيل:</label>
        <input type="time" id="edit-on-time-${index}" value="${currentOnTime}" required>
      </div>
      
      <div class="edit-section">
        <label>وقت الإيقاف:</label>
        <input type="time" id="edit-off-time-${index}" value="${currentOffTime}" required>
      </div>
      
      <!-- Status section removed - all schedules are always active -->
    </div>
    
    <div class="edit-actions">
      <button class="save-btn" onclick="saveScheduleEdit(${index})">حفظ</button>
      <button class="cancel-btn" onclick="cancelScheduleEdit(${index})">إلغاء</button>
    </div>
  `;
  
  // Hide original elements
  dateDisplay.style.display = 'none';
  onTimeDisplay.style.display = 'none';
  offTimeDisplay.style.display = 'none';
  actionsDiv.style.display = 'none';
  
  // Insert edit form
  const scheduleInfo = scheduleItem.querySelector('.schedule-info');
  scheduleInfo.appendChild(editForm);
  
  // Month and day selectors removed - date is now read-only
  
  // Toggle event listeners removed - enable/disable logic removed
}

function saveScheduleEdit(index) {
  const newMonth = parseInt(document.getElementById(`edit-month-${index}`).value);
  const newDay = parseInt(document.getElementById(`edit-day-${index}`).value);
  const newOnTime = document.getElementById(`edit-on-time-${index}`).value;
  const newOffTime = document.getElementById(`edit-off-time-${index}`).value;
  
  // Month and day validation removed - they are now read-only hidden fields
  
  if (!newOnTime) {
    alert("يرجى اختيار وقت تشغيل صحيح");
    return;
  }
  
  if (!newOffTime) {
    alert("يرجى اختيار وقت إيقاف صحيح");
    return;
  }
  
  // Cross-midnight schedules are allowed (e.g., ON at 22:00, OFF at 08:00)
  // No validation needed - the NodeMCU handles this correctly
  
  // All schedules are always enabled - no status needed
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
      off: newOffTime
    }),
  })
    .then((response) => response.json())
    .then((data) => {
      if (data.success) {
        cancelScheduleEdit(index);
        loadSchedules(); // Reload to ensure consistency
      } else {
        alert("خطأ في تحديث الجدول: " + (data.message || "خطأ غير معروف"));
      }
    })
    .catch((error) => {
      console.error("Error:", error);
      alert("خطأ في تحديث الجدول");
    });
}

function cancelScheduleEdit(index) {
  const scheduleItem = document.getElementById(`schedule-${index}`);
  const dateDisplay = document.getElementById(`date-display-${index}`);
  const onTimeDisplay = document.getElementById(`on-time-display-${index}`);
  const offTimeDisplay = document.getElementById(`off-time-display-${index}`);
  const actionsDiv = scheduleItem.querySelector('.schedule-actions');
  const editForm = scheduleItem.querySelector('.edit-form');
  
  // Show original elements again
  dateDisplay.style.display = 'block';
  onTimeDisplay.style.display = 'inline';
  offTimeDisplay.style.display = 'inline';
  actionsDiv.style.display = 'flex';
  
  // Remove edit form
  if (editForm) {
    editForm.remove();
  }
}
