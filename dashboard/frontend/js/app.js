// SMART-GROW Dashboard Controller (Pure Vanilla JS + Chart.js)

document.addEventListener('DOMContentLoaded', () => {
    // Referencias al DOM
    const inputDeviceIp = document.getElementById('device-ip-input');
    const btnReconnect = document.getElementById('btn-reconnect');
    const badgeConnection = document.getElementById('connection-badge');
    const badgeMode = document.getElementById('mode-badge');
    const btnEmergencyStop = document.getElementById('btn-emergency-stop');
    const bannerOffline = document.getElementById('offline-banner');
    const bannerSimulation = document.getElementById('simulation-banner');

    // Métricas
    const valTemp = document.getElementById('val-temp');
    const valHumidity = document.getElementById('val-humidity');
    const valSoil = document.getElementById('val-soil');
    const valLight = document.getElementById('val-light');
    const lastUpdateTime = document.getElementById('last-update-time');

    const badgeTempStatus = document.getElementById('badge-temp-status');
    const badgeHumStatus = document.getElementById('badge-hum-status');
    const badgeSoilStatus = document.getElementById('badge-soil-status');
    const badgeLightStatus = document.getElementById('badge-light-status');

    const tagTempSim = document.getElementById('tag-temp-sim');
    const tagHumSim = document.getElementById('tag-hum-sim');
    const tagSoilSim = document.getElementById('tag-soil-sim');

    // Actuadores
    const btnSetAuto = document.getElementById('btn-set-auto');
    const btnSetManual = document.getElementById('btn-set-manual');
    const pumpStatusBadge = document.getElementById('pump-status-badge');
    const pumpCooldownText = document.getElementById('pump-cooldown-text');
    const inputPumpDuration = document.getElementById('pump-duration');
    const btnRunPump = document.getElementById('btn-run-pump');

    const fanStatusBadge = document.getElementById('fan-status-badge');
    const btnToggleFan = document.getElementById('btn-toggle-fan');

    const lightStatusBadge = document.getElementById('light-status-badge');
    const sliderLight = document.getElementById('slider-light');
    const lightValDisplay = document.getElementById('light-val-display');

    const windowStatusBadge = document.getElementById('window-status-badge');
    const sliderWindow = document.getElementById('slider-window');
    const windowValDisplay = document.getElementById('window-val-display');

    // Simulaciones
    const btnSimDrought = document.getElementById('btn-sim-drought');
    const btnSimHeat = document.getElementById('btn-sim-heat');
    const btnSimStop = document.getElementById('btn-sim-stop');

    // IA
    const aiRecommendationText = document.getElementById('ai-recommendation-text');
    const aiStatusBadge = document.getElementById('ai-status-badge');
    const btnRequestAiAnalysis = document.getElementById('btn-request-ai-analysis');

    // Estado local
    let currentHost = inputDeviceIp.value.trim() || '127.0.0.1:8000';
    let isOnline = false;
    let consecutiveFailures = 0;
    let pollIntervalId = null;
    let currentMode = 'AUTO'; // 'AUTO' o 'MANUAL'
    let currentActuators = {};

    // 1. Inicialización de Chart.js
    const ctx = document.getElementById('telemetryChart').getContext('2d');
    const maxDataPoints = 25;
    const chartData = {
        labels: [],
        datasets: [
            {
                label: 'Temperatura (°C)',
                data: [],
                borderColor: '#EF4444',
                backgroundColor: 'rgba(239, 68, 68, 0.1)',
                borderWidth: 2,
                tension: 0.35,
                yAxisID: 'y'
            },
            {
                label: 'Humedad Suelo (%)',
                data: [],
                borderColor: '#10B981',
                backgroundColor: 'rgba(16, 185, 129, 0.1)',
                borderWidth: 2,
                tension: 0.35,
                yAxisID: 'y1'
            },
            {
                label: 'Luz (%)',
                data: [],
                borderColor: '#F59E0B',
                backgroundColor: 'rgba(245, 158, 11, 0.1)',
                borderWidth: 2,
                borderDash: [4, 4],
                tension: 0.35,
                yAxisID: 'y1'
            }
        ]
    };

    const telemetryChart = new Chart(ctx, {
        type: 'line',
        data: chartData,
        options: {
            responsive: true,
            maintainAspectRatio: false,
            animation: { duration: 300 },
            scales: {
                x: {
                    grid: { color: 'rgba(255, 255, 255, 0.05)' },
                    ticks: { color: '#94A3B8', font: { family: 'JetBrains Mono', size: 10 } }
                },
                y: {
                    type: 'linear',
                    display: true,
                    position: 'left',
                    min: 10,
                    max: 45,
                    title: { display: true, text: 'Temp (°C)', color: '#EF4444' },
                    grid: { color: 'rgba(255, 255, 255, 0.05)' },
                    ticks: { color: '#94A3B8' }
                },
                y1: {
                    type: 'linear',
                    display: true,
                    position: 'right',
                    min: 0,
                    max: 100,
                    title: { display: true, text: 'Porcentaje (%)', color: '#10B981' },
                    grid: { drawOnChartArea: false },
                    ticks: { color: '#94A3B8' }
                }
            },
            plugins: {
                legend: { display: false }
            }
        }
    });

    // 2. Función de Actualización de Telemetría
    async function fetchTelemetry() {
        const protocol = window.location.protocol === 'https:' ? 'https:' : 'http:';
        const url = `${protocol}//${currentHost}/api/telemetry`;

        try {
            const controller = new AbortController();
            const timeoutId = setTimeout(() => controller.abort(), 2500);

            const res = await fetch(url, { signal: controller.signal });
            clearTimeout(timeoutId);

            if (!res.ok) throw new Error(`HTTP ${res.status}`);

            const data = await res.json();
            consecutiveFailures = 0;
            setOnlineStatus(true);
            updateUI(data);
        } catch (err) {
            consecutiveFailures++;
            if (consecutiveFailures >= 2) {
                setOnlineStatus(false);
            }
        }
    }

    function setOnlineStatus(online) {
        isOnline = online;
        if (online) {
            badgeConnection.textContent = 'EN LÍNEA';
            badgeConnection.className = 'badge badge-online';
            bannerOffline.classList.add('hidden');
        } else {
            badgeConnection.textContent = 'DESCONECTADO';
            badgeConnection.className = 'badge badge-offline';
            bannerOffline.classList.remove('hidden');
            // Deshabilitar botones manuales si no hay conexión
            setControlsEnabled(false);
        }
    }

    function updateUI(data) {
        const now = new Date();
        lastUpdateTime.textContent = `Última actualización: ${now.toLocaleTimeString()}`;

        // Modo del sistema
        currentMode = data.mode || 'AUTO';
        badgeMode.textContent = `MODO ${currentMode}`;
        if (currentMode === 'AUTO') {
            btnSetAuto.classList.add('btn-active');
            btnSetManual.classList.remove('btn-active');
            setControlsEnabled(false);
        } else {
            btnSetManual.classList.add('btn-active');
            btnSetAuto.classList.remove('btn-active');
            setControlsEnabled(true);
        }

        // Sensores (distinguiendo entre efectivos y reales)
        const sReal = data.sensors_real || {};
        const sEff = data.sensors_effective || sReal;
        const sim = data.simulation || {};

        // Banner de Simulación
        if (sim.is_active) {
            bannerSimulation.classList.remove('hidden');
        } else {
            bannerSimulation.classList.add('hidden');
        }

        // Temperatura
        valTemp.textContent = sEff.temperature_c !== undefined ? Number(sEff.temperature_c).toFixed(1) : '--.-';
        if (sim.heat_mode) {
            tagTempSim.classList.remove('hidden');
            badgeTempStatus.textContent = 'Ola de Calor (Simulada)';
            badgeTempStatus.className = 'status-pill pill-alert';
        } else {
            tagTempSim.classList.add('hidden');
            const t = Number(sReal.temperature_c);
            if (t > 28) {
                badgeTempStatus.textContent = 'Alta (>28°C)';
                badgeTempStatus.className = 'status-pill pill-alert';
            } else if (t < 18) {
                badgeTempStatus.textContent = 'Baja (<18°C)';
                badgeTempStatus.className = 'status-pill pill-alert';
            } else {
                badgeTempStatus.textContent = 'Óptimo (20-27°C)';
                badgeTempStatus.className = 'status-pill pill-ok';
            }
        }

        // Humedad del Aire
        valHumidity.textContent = sEff.humidity_air_pct !== undefined ? Number(sEff.humidity_air_pct).toFixed(1) : '--.-';
        if (sim.heat_mode) {
            tagHumSim.classList.remove('hidden');
        } else {
            tagHumSim.classList.add('hidden');
        }

        // Humedad del Suelo
        valSoil.textContent = sEff.soil_moisture_pct !== undefined ? Number(sEff.soil_moisture_pct).toFixed(1) : '--.-';
        if (sim.drought_mode) {
            tagSoilSim.classList.remove('hidden');
            badgeSoilStatus.textContent = 'Sequía Forzada (Simulada)';
            badgeSoilStatus.className = 'status-pill pill-danger';
        } else {
            tagSoilSim.classList.add('hidden');
            const sm = Number(sReal.soil_moisture_pct);
            if (sm < 35) {
                badgeSoilStatus.textContent = 'Sustrato Seco (<35%)';
                badgeSoilStatus.className = 'status-pill pill-danger';
            } else if (sm > 70) {
                badgeSoilStatus.textContent = 'Sustrato Saturado';
                badgeSoilStatus.className = 'status-pill pill-alert';
            } else {
                badgeSoilStatus.textContent = 'Humedad Correcta';
                badgeSoilStatus.className = 'status-pill pill-ok';
            }
        }

        // Luz
        valLight.textContent = sEff.light_level_pct !== undefined ? Number(sEff.light_level_pct).toFixed(0) : '--.-';

        // Actuadores
        const act = data.actuators || {};
        currentActuators = act;

        // Bomba
        if (act.pump_active) {
            pumpStatusBadge.textContent = 'REGANDO (ACTIVA)';
            pumpStatusBadge.className = 'state-badge badge-on';
        } else {
            pumpStatusBadge.textContent = 'APAGADA';
            pumpStatusBadge.className = 'state-badge badge-off';
        }

        if (act.pump_cooldown_sec > 0) {
            pumpCooldownText.textContent = `Cooldown de sustrato: ${act.pump_cooldown_sec} s`;
            btnRunPump.disabled = true;
        } else {
            pumpCooldownText.textContent = 'Cooldown: Listo';
            if (currentMode === 'MANUAL') btnRunPump.disabled = false;
        }

        // Ventilador
        if (act.fan_active) {
            fanStatusBadge.textContent = `ACTIVO (${act.fan_speed_pct || 100}%)`;
            fanStatusBadge.className = 'state-badge badge-on';
        } else {
            fanStatusBadge.textContent = 'APAGADO';
            fanStatusBadge.className = 'state-badge badge-off';
        }

        // Luz
        if (act.light_active) {
            lightStatusBadge.textContent = `ENCENDIDA (${act.light_duty_pct}%)`;
            lightStatusBadge.className = 'state-badge badge-on';
        } else {
            lightStatusBadge.textContent = 'APAGADA';
            lightStatusBadge.className = 'state-badge badge-off';
        }
        if (currentMode !== 'MANUAL') {
            sliderLight.value = act.light_duty_pct || 0;
            lightValDisplay.textContent = `${act.light_duty_pct || 0}%`;
        }

        // Ventana
        const angle = act.window_angle_deg || 0;
        windowStatusBadge.textContent = angle > 0 ? `ABIERTA (${angle}°)` : 'CERRADA (0°)';
        windowStatusBadge.className = angle > 0 ? 'state-badge badge-on' : 'state-badge badge-off';
        if (currentMode !== 'MANUAL') {
            sliderWindow.value = angle;
            windowValDisplay.textContent = `${angle}°`;
        }

        // Actualizar Gráfica
        const timeLabel = now.toLocaleTimeString([], { hour: '2-digit', minute: '2-digit', second: '2-digit' });
        chartData.labels.push(timeLabel);
        chartData.datasets[0].data.push(Number(sEff.temperature_c));
        chartData.datasets[1].data.push(Number(sEff.soil_moisture_pct));
        chartData.datasets[2].data.push(Number(sEff.light_level_pct));

        if (chartData.labels.length > maxDataPoints) {
            chartData.labels.shift();
            chartData.datasets.forEach(ds => ds.data.shift());
        }
        telemetryChart.update();
    }

    function setControlsEnabled(enabled) {
        btnRunPump.disabled = !enabled || (currentActuators.pump_cooldown_sec > 0);
        btnToggleFan.disabled = !enabled;
        sliderLight.disabled = !enabled;
        sliderWindow.disabled = !enabled;
    }

    // 3. Enviar Comandos HTTP al Dispositivo o Puente
    async function sendPost(endpoint, body) {
        const protocol = window.location.protocol === 'https:' ? 'https:' : 'http:';
        const url = `${protocol}//${currentHost}${endpoint}`;

        try {
            const res = await fetch(url, {
                method: 'POST',
                headers: { 'Content-Type': 'application/json' },
                body: JSON.stringify(body)
            });
            const result = await res.json();
            if (!res.ok) {
                alert(`Aviso: ${result.error || 'Operación no permitida'}`);
            }
            fetchTelemetry();
        } catch (err) {
            alert(`Error de comunicación con ${url}: ${err.message}`);
        }
    }

    // Eventos de botones de Modo
    btnSetAuto.addEventListener('click', () => sendPost('/api/mode', { mode: 'AUTO' }));
    btnSetManual.addEventListener('click', () => sendPost('/api/mode', { mode: 'MANUAL' }));

    // Parada de Emergencia
    btnEmergencyStop.addEventListener('click', () => {
        if (confirm('¿Confirmar PARADA DE EMERGENCIA? Todos los actuadores se apagarán de inmediato.')) {
            sendPost('/api/emergency', {});
        }
    });

    // Control Manual Bomba
    btnRunPump.addEventListener('click', () => {
        const durSec = Math.min(Math.max(parseInt(inputPumpDuration.value, 10) || 5, 2), 10);
        if (confirm(`¿Regar durante ${durSec} segundos? La seguridad física detendrá la bomba si supera 10s.`)) {
            sendPost('/api/actuators', { pump: true, duration_ms: durSec * 1000 });
        }
    });

    // Control Manual Ventilador
    btnToggleFan.addEventListener('click', () => {
        const newState = !currentActuators.fan_active;
        sendPost('/api/actuators', { fan: newState, fan_speed: 100 });
    });

    // Control Manual Luz
    sliderLight.addEventListener('input', (e) => {
        lightValDisplay.textContent = `${e.target.value}%`;
    });
    sliderLight.addEventListener('change', (e) => {
        sendPost('/api/actuators', { light: parseInt(e.target.value, 10) });
    });

    // Control Manual Ventana
    sliderWindow.addEventListener('input', (e) => {
        windowValDisplay.textContent = `${e.target.value}°`;
    });
    sliderWindow.addEventListener('change', (e) => {
        sendPost('/api/actuators', { window: parseInt(e.target.value, 10) });
    });

    // Simulaciones
    btnSimDrought.addEventListener('click', () => sendPost('/api/simulate', { drought: true }));
    btnSimHeat.addEventListener('click', () => sendPost('/api/simulate', { heat: true }));
    btnSimStop.addEventListener('click', () => sendPost('/api/simulate', { stop: true }));

    // Botón Reconectar
    btnReconnect.addEventListener('click', () => {
        currentHost = inputDeviceIp.value.trim();
        fetchTelemetry();
    });

    // Consultar IA Local
    btnRequestAiAnalysis.addEventListener('click', async () => {
        aiRecommendationText.textContent = 'Analizando tendencias agronómicas con modelo de lenguaje local...';
        aiStatusBadge.textContent = 'Consultando...';
        aiStatusBadge.className = 'status-pill pill-alert';

        const protocol = window.location.protocol === 'https:' ? 'https:' : 'http:';
        const url = `${protocol}//${currentHost}/api/ai/analyze`;

        try {
            const res = await fetch(url, { method: 'POST' });
            const result = await res.json();
            if (result.analysis) {
                aiRecommendationText.innerHTML = `<strong>Diagnóstico IA:</strong> ${result.analysis}`;
                aiStatusBadge.textContent = result.approved_action ? 'Acción Sugerida' : 'Solo Diagnóstico';
                aiStatusBadge.className = 'status-pill pill-ok';
            } else {
                aiRecommendationText.textContent = result.message || 'Sin diagnóstico disponible.';
                aiStatusBadge.textContent = 'Modo Observador';
                aiStatusBadge.className = 'status-pill pill-neutral';
            }
        } catch (err) {
            aiRecommendationText.textContent = 'Nota: El servicio puente de IA (LM Studio) no está respondiendo en este endpoint. Verifique que ai_bridge/main.py esté en ejecución.';
            aiStatusBadge.textContent = 'Sin IA Local';
            aiStatusBadge.className = 'status-pill pill-danger';
        }
    });

    // Inicio del ciclo de muestreo continuo
    fetchTelemetry();
    pollIntervalId = setInterval(fetchTelemetry, 2000);
});
