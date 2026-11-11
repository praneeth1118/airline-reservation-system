// State storage
let flightsData = [];
let perksData = [];
let selectedFlight = null;

// Initialize on page load
document.addEventListener('DOMContentLoaded', () => {
  setupNavigation();
  loadFlights();
  loadPerks();
  loadSystemStats();
});

// Setup Tab Switching
function setupNavigation() {
  const navLinks = document.querySelectorAll('.nav-link');
  navLinks.forEach(link => {
    link.addEventListener('click', () => {
      const targetTab = link.getAttribute('data-tab');
      switchTab(targetTab);
    });
  });
}

function switchTab(tabId) {
  // Update nav buttons
  document.querySelectorAll('.nav-link').forEach(btn => {
    btn.classList.toggle('active', btn.getAttribute('data-tab') === tabId);
  });

  // Update tabs
  document.querySelectorAll('.tab-pane').forEach(pane => {
    pane.classList.toggle('active', pane.id === tabId);
  });

  // Contextual refresh
  if (tabId === 'flights-tab') {
    loadFlights();
  } else if (tabId === 'manifest-tab') {
    populateManifestDropdown();
    loadFlightPassengers();
  } else if (tabId === 'stats-tab') {
    loadSystemStats();
    loadAllBookings();
  } else if (tabId === 'classes-tab') {
    loadPerks();
  }
}

// ----------------- 1. FLIGHTS API & RENDERING -----------------
async function loadFlights() {
  const container = document.getElementById('flightsContainer');
  try {
    const res = await fetch('/api/flights');
    flightsData = await res.json();

    populateBookingDropdown();
    populateManifestDropdown();
    renderFlightCards(flightsData);
    updateHeroStats();
  } catch (err) {
    console.error('Failed to load flights:', err);
    container.innerHTML = `<div class="empty-state"><h3>⚠️ Failed to connect to C++ backend.</h3><p>Make sure the backend executable is running on port 8080.</p></div>`;
  }
}

function renderFlightCards(flights) {
  const container = document.getElementById('flightsContainer');
  if (!flights || flights.length === 0) {
    container.innerHTML = `<div class="empty-state"><h3>No matching flights found.</h3><p>Try searching with another keyword.</p></div>`;
    return;
  }

  container.innerHTML = flights.map(f => `
    <div class="flight-card">
      <div class="card-top">
        <span class="flight-airline">${escapeHtml(f.airlineName)}</span>
        <span class="flight-badge">${escapeHtml(f.flightNumber)}</span>
      </div>

      <div class="route-display">
        <div class="route-endpoint">
          <div class="city-code">${escapeHtml(f.originCode)}</div>
          <div class="city-name">${escapeHtml(f.origin)}</div>
          <div class="time-val">🛫 ${escapeHtml(f.departureTime)}</div>
        </div>
        <div class="route-middle">
          <div class="flight-duration">${escapeHtml(f.duration)}</div>
          <div class="flight-line">── ✈ ──</div>
          <div class="aircraft-type">${escapeHtml(f.aircraft)}</div>
        </div>
        <div class="route-endpoint text-right">
          <div class="city-code">${escapeHtml(f.destinationCode)}</div>
          <div class="city-name">${escapeHtml(f.destination)}</div>
          <div class="time-val">🛬 ${escapeHtml(f.arrivalTime)}</div>
        </div>
      </div>

      <div class="card-classes-row">
        <div class="class-mini-box">
          <div class="class-mini-name" style="color: var(--economy-color)">Economy</div>
          <div class="class-mini-price">$${f.economy.price.toFixed(0)}</div>
          <div class="class-mini-avail">${f.economy.availableSeats} left</div>
        </div>
        <div class="class-mini-box">
          <div class="class-mini-name" style="color: var(--business-color)">Business</div>
          <div class="class-mini-price">$${f.business.price.toFixed(0)}</div>
          <div class="class-mini-avail">${f.business.availableSeats} left</div>
        </div>
        <div class="class-mini-box">
          <div class="class-mini-name" style="color: var(--first-color)">First Class</div>
          <div class="class-mini-price">$${f.firstClass.price.toFixed(0)}</div>
          <div class="class-mini-avail">${f.firstClass.availableSeats} left</div>
        </div>
      </div>

      <div class="card-actions">
        <button class="btn btn-secondary" onclick="viewFlightManifest('${f.flightNumber}')">Manifest</button>
        <button class="btn btn-primary" onclick="selectFlightForBooking('${f.flightNumber}')">Book Flight</button>
      </div>
    </div>
  `).join('');
}

function filterFlights() {
  const query = document.getElementById('flightSearchInput').value.toLowerCase().trim();
  if (!query) {
    renderFlightCards(flightsData);
    return;
  }

  const filtered = flightsData.filter(f => 
    f.flightNumber.toLowerCase().includes(query) ||
    f.origin.toLowerCase().includes(query) ||
    f.destination.toLowerCase().includes(query) ||
    f.originCode.toLowerCase().includes(query) ||
    f.destinationCode.toLowerCase().includes(query) ||
    f.airlineName.toLowerCase().includes(query)
  );

  renderFlightCards(filtered);
}

// ----------------- 2. BOOKING SYSTEM -----------------
function populateBookingDropdown() {
  const select = document.getElementById('bookFlightSelect');
  const currentVal = select.value;
  select.innerHTML = '<option value="">-- Choose a Flight --</option>' +
    flightsData.map(f => `
      <option value="${f.flightNumber}" ${f.flightNumber === currentVal ? 'selected' : ''}>
        ${f.flightNumber} : ${f.origin} ➔ ${f.destination} ($${f.basePrice.toFixed(0)} base)
      </option>
    `).join('');
}

function selectFlightForBooking(flightNo) {
  switchTab('book-tab');
  const select = document.getElementById('bookFlightSelect');
  select.value = flightNo;
  onFlightSelectionChange();
}

function onFlightSelectionChange() {
  const flightNo = document.getElementById('bookFlightSelect').value;
  selectedFlight = flightsData.find(f => f.flightNumber === flightNo) || null;
  updateFarePreview();
}

function updateFarePreview() {
  const summaryBox = document.getElementById('summaryFlightInfo');
  const breakdownSec = document.getElementById('fareBreakdownSection');

  if (!selectedFlight) {
    summaryBox.innerHTML = '<p class="placeholder-text">Please select a flight route on the left to preview fares and perks.</p>';
    breakdownSec.style.display = 'none';
    return;
  }

  const selectedClass = document.querySelector('input[name="seatClass"]:checked').value;
  let multiplier = 1.0;
  let baggage = "20 kg";
  let meal = "Complimentary Snack & Drink";
  let lounge = "Not Included";
  let availSeats = selectedFlight.economy.availableSeats;

  if (selectedClass === 'Business') {
    multiplier = 2.2;
    baggage = "40 kg (2 Pieces)";
    meal = "Chef-curated 3-course Hot Meal & Wine";
    lounge = "Executive Airport Business Lounge";
    availSeats = selectedFlight.business.availableSeats;
  } else if (selectedClass === 'First Class') {
    multiplier = 3.8;
    baggage = "60 kg Priority Baggage";
    meal = "Michelin-Inspired A La Carte & Champagne";
    lounge = "VIP First Class Royal Lounge & Spa";
    availSeats = selectedFlight.firstClass.availableSeats;
  }

  const totalFare = selectedFlight.basePrice * multiplier;

  summaryBox.innerHTML = `
    <div style="font-weight: 800; font-size: 1.05rem; color: #0f172a;">${selectedFlight.flightNumber} - ${selectedFlight.airlineName}</div>
    <div style="margin-top: 4px; color: #475569;">${selectedFlight.origin} ➔ ${selectedFlight.destination}</div>
    <div style="font-size: 0.8rem; color: #64748b; margin-top: 6px;">
      Departure: <strong>${selectedFlight.departureTime}</strong> | Duration: <strong>${selectedFlight.duration}</strong>
    </div>
    <div style="margin-top: 8px; font-size: 0.82rem; font-weight: 600; color: ${availSeats > 0 ? '#16a34a' : '#dc2626'};">
      Available in ${selectedClass}: ${availSeats} seats left
    </div>
  `;

  document.getElementById('summaryBasePrice').textContent = `$${selectedFlight.basePrice.toFixed(2)}`;
  document.getElementById('summaryClassLabel').textContent = `${selectedClass} Multiplier:`;
  document.getElementById('summaryClassMult').textContent = `${multiplier.toFixed(1)}x`;
  document.getElementById('summaryBaggage').textContent = baggage;
  document.getElementById('summaryMeal').textContent = meal;
  document.getElementById('summaryLounge').textContent = lounge;
  document.getElementById('summaryTotalFare').textContent = `$${totalFare.toFixed(2)}`;

  breakdownSec.style.display = 'block';
}

async function handleBookingSubmit(event) {
  event.preventDefault();

  const flightNo = document.getElementById('bookFlightSelect').value;
  if (!flightNo) {
    showToast("Please choose a flight to book.");
    return;
  }

  const payload = {
    flightNumber: flightNo,
    passengerName: document.getElementById('passName').value.trim(),
    passport: document.getElementById('passPassport').value.trim(),
    email: document.getElementById('passEmail').value.trim(),
    phone: document.getElementById('passPhone').value.trim(),
    age: parseInt(document.getElementById('passAge').value) || 25,
    gender: document.getElementById('passGender').value,
    seatClass: document.querySelector('input[name="seatClass"]:checked').value,
    requestedSeat: document.getElementById('passSeatReq').value.trim().toUpperCase()
  };

  const submitBtn = document.getElementById('bookSubmitBtn');
  submitBtn.disabled = true;
  submitBtn.textContent = "Processing via C++ Engine...";

  try {
    const res = await fetch('/api/book', {
      method: 'POST',
      headers: { 'Content-Type': 'application/json' },
      body: JSON.stringify(payload)
    });

    const data = await res.json();
    submitBtn.disabled = false;
    submitBtn.textContent = "Confirm & Book Ticket";

    if (data.success && data.ticket) {
      showToast("🎉 " + data.message);
      openBoardingPassModal(data.ticket);
      loadFlights(); // refresh seat availability
      loadSystemStats();
    } else {
      showToast("❌ " + (data.message || "Failed to book ticket."));
    }
  } catch (err) {
    submitBtn.disabled = false;
    submitBtn.textContent = "Confirm & Book Ticket";
    console.error('Booking error:', err);
    showToast("⚠️ Network error communicating with C++ backend.");
  }
}

// ----------------- 3. BOARDING PASS MODAL -----------------
function openBoardingPassModal(ticket) {
  document.getElementById('modalClassBadge').textContent = ticket.seatClass.toUpperCase();
  document.getElementById('modalFlightNo').textContent = ticket.flightNumber;
  document.getElementById('modalOriginCode').textContent = ticket.origin.match(/\(([A-Z]{3})\)/)?.[1] || "DEP";
  document.getElementById('modalOriginName').textContent = ticket.origin.split('(')[0].trim();
  document.getElementById('modalDestCode').textContent = ticket.destination.match(/\(([A-Z]{3})\)/)?.[1] || "ARR";
  document.getElementById('modalDestName').textContent = ticket.destination.split('(')[0].trim();

  document.getElementById('modalPassengerName').textContent = ticket.passengerName;
  document.getElementById('modalTicketId').textContent = ticket.ticketId;
  document.getElementById('modalSeatNumber').textContent = ticket.seatNumber;
  document.getElementById('modalDepartureTime').textContent = ticket.departureTime || "08:30 AM";
  document.getElementById('modalBaggage').textContent = `${ticket.baggageAllowanceKg} kg Baggage`;
  document.getElementById('modalFarePaid').textContent = `$${ticket.farePaid.toFixed(2)}`;
  document.getElementById('modalMealService').textContent = ticket.mealService;
  document.getElementById('modalBarcodeText').textContent = `${ticket.ticketId} • SKY-VERIFIED`;

  document.getElementById('boardingPassModal').style.display = 'flex';
}

function closeBoardingPassModal() {
  document.getElementById('boardingPassModal').style.display = 'none';
}

// ----------------- 4. PASSENGER MANIFEST -----------------
function populateManifestDropdown() {
  const select = document.getElementById('manifestFlightSelect');
  if (!select) return;
  const currentVal = select.value;
  select.innerHTML = flightsData.map(f => `
    <option value="${f.flightNumber}" ${f.flightNumber === currentVal ? 'selected' : ''}>
      ${f.flightNumber} : ${f.origin} ➔ ${f.destination}
    </option>
  `).join('');
}

function viewFlightManifest(flightNo) {
  switchTab('manifest-tab');
  const select = document.getElementById('manifestFlightSelect');
  select.value = flightNo;
  loadFlightPassengers();
}

async function loadFlightPassengers() {
  const flightNo = document.getElementById('manifestFlightSelect').value;
  if (!flightNo) return;

  const tbody = document.getElementById('passengerTableBody');
  const emptyState = document.getElementById('manifestEmptyState');

  try {
    const res = await fetch(`/api/flights/${flightNo}/passengers`);
    const passengers = await res.json();

    if (!passengers || passengers.length === 0) {
      tbody.innerHTML = '';
      emptyState.style.display = 'block';
      return;
    }

    emptyState.style.display = 'none';
    tbody.innerHTML = passengers.map(p => {
      let badgeClass = 'badge-economy';
      if (p.seatClass === 'Business') badgeClass = 'badge-business';
      if (p.seatClass === 'First Class') badgeClass = 'badge-first';

      return `
        <tr>
          <td><strong class="mono" style="color: var(--primary);">${escapeHtml(p.ticketId)}</strong></td>
          <td><strong>${escapeHtml(p.passengerName)}</strong> (${p.age}y, ${p.gender})</td>
          <td><span class="mono">${escapeHtml(p.passport)}</span></td>
          <td><span class="class-badge ${badgeClass}">${escapeHtml(p.seatClass)}</span></td>
          <td><span class="seat-pill">${escapeHtml(p.seatNumber)}</span></td>
          <td><strong>$${p.fare.toFixed(2)}</strong></td>
          <td><small>${escapeHtml(p.email)}<br>${escapeHtml(p.phone)}</small></td>
          <td><small style="color: var(--text-muted);">${escapeHtml(p.bookingDate)}</small></td>
        </tr>
      `;
    }).join('');
  } catch (err) {
    console.error('Error loading passengers:', err);
  }
}

// ----------------- 5. CANCEL TICKET & LOOKUP -----------------
function quickFillTicket(id) {
  document.getElementById('lookupTicketId').value = id;
  handleTicketLookup(new Event('submit'));
}

async function handleTicketLookup(event) {
  if (event) event.preventDefault();
  const ticketId = document.getElementById('lookupTicketId').value.trim();
  const resultCard = document.getElementById('ticketResultCard');

  if (!ticketId) return;

  try {
    const res = await fetch(`/api/bookings/${ticketId}`);
    const data = await res.json();

    if (!data.found) {
      resultCard.style.display = 'block';
      resultCard.innerHTML = `
        <div style="text-align: center; color: var(--danger);">
          <h3>Ticket Not Found</h3>
          <p>No booking found with Reference ID: <strong>${escapeHtml(ticketId)}</strong>.</p>
        </div>
      `;
      return;
    }

    const isCancelled = data.status === 'CANCELLED';
    let refundRate = 70;
    if (data.seatClass === 'Business') refundRate = 85;
    if (data.seatClass === 'First Class') refundRate = 100;
    const estRefund = data.farePaid * (refundRate / 100.0);

    resultCard.style.display = 'block';
    resultCard.innerHTML = `
      <div style="display: flex; justify-content: space-between; align-items: center; border-bottom: 1px solid var(--border-color); padding-bottom: 1rem; margin-bottom: 1.25rem;">
        <div>
          <span class="detail-label">BOOKING REFERENCE</span>
          <span style="font-size: 1.4rem; font-weight: 800; font-family: 'JetBrains Mono', monospace; color: var(--primary);">${escapeHtml(data.ticketId)}</span>
        </div>
        <span class="class-badge" style="background: ${isCancelled ? 'var(--danger-bg)' : 'var(--success-bg)'}; color: ${isCancelled ? 'var(--danger)' : 'var(--success)'}; font-size: 0.85rem; padding: 6px 12px;">
          ${isCancelled ? 'CANCELLED' : 'CONFIRMED'}
        </span>
      </div>

      <div class="pass-details-grid">
        <div>
          <span class="detail-label">PASSENGER NAME</span>
          <span class="detail-val">${escapeHtml(data.passengerName)}</span>
        </div>
        <div>
          <span class="detail-label">FLIGHT ROUTE</span>
          <span class="detail-val">${escapeHtml(data.flightNumber)} (${escapeHtml(data.origin)} ➔ ${escapeHtml(data.destination)})</span>
        </div>
        <div>
          <span class="detail-label">SEAT CLASS</span>
          <span class="detail-val">${escapeHtml(data.seatClass)} (Seat: ${escapeHtml(data.seatNumber)})</span>
        </div>
        <div>
          <span class="detail-label">FARE PAID</span>
          <span class="detail-val" style="color: var(--primary);">$${data.farePaid.toFixed(2)}</span>
        </div>
        <div>
          <span class="detail-label">CLASS REFUND POLICY</span>
          <span class="detail-val" style="color: #d97706;">${refundRate}% Refund Policy</span>
        </div>
        <div>
          <span class="detail-label">${isCancelled ? 'REFUNDED AMOUNT' : 'ESTIMATED REFUND'}</span>
          <span class="detail-val" style="color: var(--success); font-size: 1.2rem;">$${isCancelled ? data.refundAmount.toFixed(2) : estRefund.toFixed(2)}</span>
        </div>
      </div>

      ${!isCancelled ? `
        <div style="margin-top: 1.5rem; background: #fff1f2; border: 1px solid #fecdd3; padding: 1rem; border-radius: var(--radius-md);">
          <h4 style="color: #9f1239; margin-bottom: 4px;">Cancellation Confirmation</h4>
          <p style="font-size: 0.85rem; color: #be123c;">
            Cancelling this ticket will release seat <strong>${escapeHtml(data.seatNumber)}</strong> back to flight ${escapeHtml(data.flightNumber)}.
            A refund of <strong>$${estRefund.toFixed(2)}</strong> will be credited according to the <strong>${escapeHtml(data.seatClass)}</strong> refund policy.
          </p>
          <button class="btn btn-danger" style="margin-top: 0.85rem;" onclick="executeCancellation('${escapeHtml(data.ticketId)}')">
            Confirm & Cancel Ticket Now
          </button>
        </div>
      ` : `
        <div style="margin-top: 1.25rem; background: #f1f5f9; padding: 1rem; border-radius: var(--radius-md); text-align: center; color: var(--text-secondary);">
          This ticket was cancelled. A refund of <strong>$${data.refundAmount.toFixed(2)}</strong> has already been settled.
        </div>
      `}
    `;
  } catch (err) {
    console.error('Error looking up ticket:', err);
  }
}

async function executeCancellation(ticketId) {
  if (!confirm(`Are you sure you want to cancel booking ${ticketId}?`)) return;

  try {
    const res = await fetch('/api/cancel', {
      method: 'POST',
      headers: { 'Content-Type': 'application/json' },
      body: JSON.stringify({ ticketId })
    });

    const data = await res.json();
    if (data.success) {
      showToast(`✅ ${data.message} Refund: $${data.refundAmount.toFixed(2)}`);
      handleTicketLookup();
      loadFlights();
      loadSystemStats();
    } else {
      showToast(`❌ ${data.message}`);
    }
  } catch (err) {
    console.error('Error cancelling ticket:', err);
    showToast("⚠️ Failed to cancel ticket.");
  }
}

// ----------------- 6. SEAT CLASSES & PERKS TAB -----------------
async function loadPerks() {
  const container = document.getElementById('perksContainer');
  if (!container) return;

  try {
    const res = await fetch('/api/perks');
    perksData = await res.json();

    container.innerHTML = perksData.map(c => {
      let badgeClass = 'badge-economy';
      if (c.className === 'Business') badgeClass = 'badge-business';
      if (c.className === 'First Class') badgeClass = 'badge-first';

      return `
        <div class="class-detail-card">
          <div class="class-card-header">
            <span class="class-badge ${badgeClass}">${escapeHtml(c.className.toUpperCase())}</span>
            <h3 style="margin-top: 6px;">${escapeHtml(c.className)} Class</h3>
            <div style="font-size: 1.35rem; font-weight: 800; color: var(--primary); margin-top: 4px;">
              ${c.priceMultiplier.toFixed(1)}x Base Fare
            </div>
            <p style="font-size: 0.8rem; color: var(--text-muted); margin-top: 2px;">
              🧳 ${c.baggageKg} kg Baggage • 🔄 ${c.refundPercentage}% Refund Policy
            </p>
          </div>

          <div style="margin-bottom: 0.75rem;">
            <div style="font-size: 0.75rem; font-weight: 700; color: var(--text-muted);">SEAT TYPE</div>
            <div style="font-size: 0.85rem; font-weight: 600; color: #0f172a;">${escapeHtml(c.seatType)}</div>
          </div>

          <div style="margin-bottom: 0.75rem;">
            <div style="font-size: 0.75rem; font-weight: 700; color: var(--text-muted);">DINING & BEVERAGE</div>
            <div style="font-size: 0.85rem; color: #334155;">${escapeHtml(c.mealService)}</div>
          </div>

          <div style="margin-bottom: 0.75rem;">
            <div style="font-size: 0.75rem; font-weight: 700; color: var(--text-muted);">LOUNGE ACCESS</div>
            <div style="font-size: 0.85rem; color: #334155;">${escapeHtml(c.loungeAccess)}</div>
          </div>

          <div style="font-size: 0.75rem; font-weight: 700; color: var(--text-muted); margin-top: 0.5rem;">KEY PERKS</div>
          <ul class="perks-list">
            ${c.perks.map(p => `<li>${escapeHtml(p)}</li>`).join('')}
          </ul>
        </div>
      `;
    }).join('');
  } catch (err) {
    console.error('Error loading perks:', err);
  }
}

// ----------------- 7. SYSTEM STATS & ALL BOOKINGS -----------------
async function loadSystemStats() {
  try {
    const res = await fetch('/api/stats');
    const stats = await res.json();

    // Hero stats
    document.getElementById('hero-active-flights').textContent = stats.totalFlights;
    document.getElementById('hero-total-passengers').textContent = stats.activeBookings;
    document.getElementById('hero-total-revenue').textContent = `$${stats.totalRevenue.toFixed(2)}`;

    // Dashboard boxes
    document.getElementById('stat-flights').textContent = stats.totalFlights;
    document.getElementById('stat-bookings').textContent = stats.activeBookings;
    document.getElementById('stat-cancelled').textContent = stats.cancelledBookings;
    document.getElementById('stat-revenue').textContent = `$${stats.totalRevenue.toFixed(2)}`;

    // Distribution progress bars
    const totalBookings = stats.economyCount + stats.businessCount + stats.firstClassCount || 1;
    const econPct = Math.round((stats.economyCount / totalBookings) * 100);
    const bizPct = Math.round((stats.businessCount / totalBookings) * 100);
    const firstPct = Math.round((stats.firstClassCount / totalBookings) * 100);

    document.getElementById('dist-econ-count').textContent = `${stats.economyCount} tickets (${econPct}%)`;
    document.getElementById('dist-econ-bar').style.width = `${econPct}%`;

    document.getElementById('dist-biz-count').textContent = `${stats.businessCount} tickets (${bizPct}%)`;
    document.getElementById('dist-biz-bar').style.width = `${bizPct}%`;

    document.getElementById('dist-first-count').textContent = `${stats.firstClassCount} tickets (${firstPct}%)`;
    document.getElementById('dist-first-bar').style.width = `${firstPct}%`;
  } catch (err) {
    console.error('Error loading stats:', err);
  }
}

function updateHeroStats() {
  const activeFlights = flightsData.length;
  document.getElementById('hero-active-flights').textContent = activeFlights;
}

async function loadAllBookings() {
  const tbody = document.getElementById('allBookingsTableBody');
  if (!tbody) return;

  try {
    const res = await fetch('/api/bookings');
    const bookings = await res.json();

    if (!bookings || bookings.length === 0) {
      tbody.innerHTML = '<tr><td colspan="8" style="text-align: center; padding: 2rem;">No bookings recorded.</td></tr>';
      return;
    }

    tbody.innerHTML = bookings.map(b => {
      const isCancelled = b.status === 'CANCELLED';
      let badgeClass = 'badge-economy';
      if (b.seatClass === 'Business') badgeClass = 'badge-business';
      if (b.seatClass === 'First Class') badgeClass = 'badge-first';

      return `
        <tr>
          <td><strong class="mono" style="color: var(--primary);">${escapeHtml(b.ticketId)}</strong></td>
          <td><strong>${escapeHtml(b.flightNumber)}</strong></td>
          <td><small>${escapeHtml(b.origin)} ➔ ${escapeHtml(b.destination)}</small></td>
          <td><strong>${escapeHtml(b.passengerName)}</strong></td>
          <td><span class="class-badge ${badgeClass}">${escapeHtml(b.seatClass)}</span></td>
          <td><span class="seat-pill">${escapeHtml(b.seatNumber)}</span></td>
          <td><strong>$${b.farePaid.toFixed(2)}</strong></td>
          <td>
            <span class="class-badge" style="background: ${isCancelled ? 'var(--danger-bg)' : 'var(--success-bg)'}; color: ${isCancelled ? 'var(--danger)' : 'var(--success)'};">
              ${isCancelled ? 'CANCELLED' : 'CONFIRMED'}
            </span>
          </td>
        </tr>
      `;
    }).join('');
  } catch (err) {
    console.error('Error loading all bookings:', err);
  }
}

async function confirmResetSystem() {
  if (!confirm("Are you sure you want to reset the system to default demo state? This will restore original flights and sample bookings.")) return;

  try {
    const res = await fetch('/api/reset', { method: 'POST' });
    const data = await res.json();
    showToast(data.message);
    loadFlights();
    loadSystemStats();
    loadAllBookings();
  } catch (err) {
    showToast("Failed to reset system.");
  }
}

// ----------------- UTILITIES -----------------
function escapeHtml(str) {
  if (!str) return '';
  return String(str)
    .replace(/&/g, '&amp;')
    .replace(/</g, '&lt;')
    .replace(/>/g, '&gt;')
    .replace(/"/g, '&quot;')
    .replace(/'/g, '&#039;');
}

function showToast(message) {
  const toast = document.getElementById('toastNotification');
  toast.textContent = message;
  toast.style.display = 'block';
  setTimeout(() => {
    toast.style.display = 'none';
  }, 3500);
}

