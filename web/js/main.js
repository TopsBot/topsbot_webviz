(function () {
  const cfg = window.TopsbotWebvizConfig;
  let WebFrameMessage = null;
  const cells = new Map();
  const channelMetrics = new Map();
  let focusChannel = -1;
  let layoutSlots = 4;
  let frameCount = 0;
  let lastFrameTime = 0;

  const LAYOUT_MAP = { 1: 'grid-1x1', 4: 'grid-2x2', 9: 'grid-3x3', 16: 'grid-4x4' };

  function camName(id) {
    return `Cam-${String(id + 1).padStart(2, '0')}`;
  }

  function getOrCreateMetrics(channelId) {
    if (!channelMetrics.has(channelId)) {
      channelMetrics.set(channelId, {
        lastSeq: null,
        fps: 0,
        width: 0,
        height: 0,
        arrivalTimes: [],
        live: false,
      });
    }
    return channelMetrics.get(channelId);
  }

  function recordFrameArrival(channelId, frame) {
    const metrics = getOrCreateMetrics(channelId);
    const seq = frame.sequenceId ?? frame.sequence_id;
    if (seq !== undefined && seq !== null && seq === metrics.lastSeq) {
      return;
    }
    if (seq !== undefined && seq !== null) {
      metrics.lastSeq = seq;
    }

    const now = performance.now();
    if (lastFrameTime > 0) {
      const latency = now - lastFrameTime;
      const latEl = document.getElementById('statLatency');
      if (latEl && latency < 2000) {
        latEl.textContent = Math.round(latency);
      }
    }
    lastFrameTime = now;

    metrics.arrivalTimes.push(now);
    metrics.arrivalTimes = metrics.arrivalTimes.filter((t) => now - t <= 1000);
    if (metrics.arrivalTimes.length >= 2) {
      const span = metrics.arrivalTimes[metrics.arrivalTimes.length - 1] - metrics.arrivalTimes[0];
      metrics.fps = span > 0
        ? ((metrics.arrivalTimes.length - 1) * 1000) / span
        : 0;
    }
    metrics.live = true;

    const image = frame.image || frame.Image;
    const protoW = image && (image.width || image.Width);
    const protoH = image && (image.height || image.Height);
    if (protoW > 0) metrics.width = protoW;
    if (protoH > 0) metrics.height = protoH;
  }

  function updateResolutionFallback(channelId, frame, imgW, imgH) {
    const metrics = getOrCreateMetrics(channelId);
    if (metrics.width <= 0 && imgW > 0) metrics.width = imgW;
    if (metrics.height <= 0 && imgH > 0) metrics.height = imgH;
  }

  function statsChannelId(preferred) {
    if (focusChannel >= 0) return focusChannel;
    return preferred;
  }

  function avgFps() {
    let sum = 0;
    let n = 0;
    channelMetrics.forEach((m) => {
      if (m.live && m.fps > 0) {
        sum += m.fps;
        n += 1;
      }
    });
    return n > 0 ? sum / n : 0;
  }

  function onlineCount() {
    let n = 0;
    cells.forEach((state, id) => {
      if (state.active && getOrCreateMetrics(id).live) n += 1;
    });
    return n;
  }

  function refreshHeaderStatus() {
    const online = onlineCount();
    const total = cfg.maxChannels;
    document.getElementById('onlineCount').textContent = String(online);
    document.getElementById('totalChannels').textContent = String(total);
    const dot = document.getElementById('statusDot');
    dot.className = 'status-dot';
    if (online === 0) dot.classList.add('offline');
    else if (online < total) dot.classList.add('partial');
    else dot.classList.add('online');
  }

  function refreshStatsPanel(channelId) {
    const metrics = channelMetrics.get(statsChannelId(channelId));
    const fpsEl = document.getElementById('statFps');
    const resEl = document.getElementById('statResolution');
    const avg = avgFps();
    fpsEl.textContent = avg > 0 ? avg.toFixed(1) : (metrics && metrics.fps > 0 ? metrics.fps.toFixed(1) : '-');
    if (metrics && metrics.width > 0 && metrics.height > 0) {
      resEl.textContent = `${metrics.width}×${metrics.height}`;
    } else {
      resEl.textContent = '-';
    }
    refreshHeaderStatus();
  }

  function refreshCellLabel(channelId) {
    const state = cells.get(channelId);
    const metrics = getOrCreateMetrics(channelId);
    if (!state) return;
    const label = state.cell.querySelector('.cell-label');
    const badge = state.cell.querySelector('.cell-fps-badge');
    if (label) {
      const parts = [camName(channelId)];
      if (metrics.width > 0 && metrics.height > 0) {
        parts.push(`${metrics.width}×${metrics.height}`);
      }
      label.textContent = parts.join(' ');
    }
    if (badge) {
      badge.textContent = metrics.fps > 0 ? `${metrics.fps.toFixed(1)} FPS` : '- FPS';
    }
  }

  function refreshSidebarThumb(channelId, imgEl) {
    const thumb = document.querySelector(`.channel-thumb[data-channel="${channelId}"]`);
    if (!thumb || !imgEl || !imgEl.src) return;
    if (thumb.tagName === 'IMG') {
      thumb.src = imgEl.src;
    }
  }

  function refreshSidebarDot(channelId) {
    const dot = document.querySelector(`.channel-dot[data-channel="${channelId}"]`);
    if (!dot) return;
    dot.classList.toggle('live', getOrCreateMetrics(channelId).live);
  }

  const layers = {
    boxes: document.getElementById('layerBoxes').checked,
    labels: document.getElementById('layerLabels').checked,
    trackId: document.getElementById('layerTrackId').checked,
    keypoints: document.getElementById('layerKeypoints').checked,
  };

  ['layerBoxes', 'layerLabels', 'layerTrackId', 'layerKeypoints'].forEach((id) => {
    document.getElementById(id).addEventListener('change', (e) => {
      const key = id.replace('layer', '');
      const map = { Boxes: 'boxes', Labels: 'labels', TrackId: 'trackId', Keypoints: 'keypoints' };
      layers[map[key] || key.toLowerCase()] = e.target.checked;
    });
  });

  function sendControl(payload) {
    cells.forEach((cell) => {
      if (cell.ws && cell.ws.readyState === WebSocket.OPEN) {
        cell.ws.send(JSON.stringify(payload));
      }
    });
  }

  function setStatus(text) {
    document.getElementById('statusText').textContent = text;
  }

  function normalizeJpegBytes(raw) {
    if (!raw) return null;
    if (raw instanceof Uint8Array) return raw;
    if (ArrayBuffer.isView(raw)) {
      return new Uint8Array(raw.buffer, raw.byteOffset, raw.byteLength);
    }
    if (raw instanceof ArrayBuffer) return new Uint8Array(raw);
    if (Array.isArray(raw)) return new Uint8Array(raw);
    return null;
  }

  function noSignalHtml() {
    return `<div class="cell-inner">
      <svg class="no-signal-icon" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="1.5">
        <path d="M17 10.5V7a5 5 0 00-10 0v3.5M5 10.5h14v8a2 2 0 01-2 2H7a2 2 0 01-2-2v-8z"/>
        <line x1="4" y1="4" x2="20" y2="20"/>
      </svg>
      <span>无信号</span>
    </div>`;
  }

  function buildCell(channelId) {
    const cell = document.createElement('div');
    cell.className = 'cell empty';
    cell.dataset.channelId = String(channelId);
    cell.innerHTML = `${noSignalHtml()}
      <div class="cell-label">${camName(channelId)}</div>
      <div class="cell-fps-badge">- FPS</div>`;
    cell.addEventListener('click', () => {
      if (cell.classList.contains('empty')) return;
      focusChannel = channelId;
      applyLayout();
      refreshStatsPanel(channelId);
      sendControl({ cmd: 'focus', channel_id: channelId });
    });
    const canvas = document.createElement('canvas');
    canvas.style.display = 'none';
    const img = document.createElement('img');
    img.style.display = 'none';
    cell.appendChild(img);
    cell.appendChild(canvas);
    return { cell, img, canvas, ws: null, active: channelId < cfg.maxChannels };
  }

  function buildSidebarItem(channelId) {
    const li = document.createElement('li');
    li.className = 'channel-item selected';
    li.dataset.channelId = String(channelId);
    li.innerHTML = `
      <input type="checkbox" checked data-channel="${channelId}" />
      <div class="channel-thumb empty" data-channel="${channelId}">📷</div>
      <div class="channel-meta">
        <div class="channel-name">${camName(channelId)}</div>
        <div class="channel-sub">通道 ${String(channelId + 1).padStart(2, '0')}</div>
      </div>
      <span class="channel-dot" data-channel="${channelId}"></span>`;
    const cb = li.querySelector('input');
    cb.addEventListener('change', (e) => {
      const state = cells.get(channelId);
      if (!state) return;
      state.active = e.target.checked;
      state.cell.classList.toggle('hidden', !state.active);
      li.classList.toggle('selected', state.active);
      applyLayout();
      refreshHeaderStatus();
    });
    li.addEventListener('click', (e) => {
      if (e.target.tagName === 'INPUT') return;
      cb.checked = !cb.checked;
      cb.dispatchEvent(new Event('change'));
    });
    return li;
  }

  function buildGrid() {
    const grid = document.getElementById('videoGrid');
    grid.innerHTML = '';
    cells.clear();
    const channelList = document.getElementById('channelList');
    channelList.innerHTML = '';

    const slotCount = layoutSlots;
    for (let i = 0; i < slotCount; i += 1) {
      const state = buildCell(i);
      state.active = i < cfg.maxChannels;
      grid.appendChild(state.cell);
      cells.set(i, state);
    }

    for (let i = 0; i < cfg.maxChannels; i += 1) {
      channelList.appendChild(buildSidebarItem(i));
    }

    document.getElementById('totalChannels').textContent = String(cfg.maxChannels);
    applyLayout();
    refreshHeaderStatus();
  }

  function applyLayout() {
    const grid = document.getElementById('videoGrid');
    grid.className = 'video-grid';
    cells.forEach((c) => c.cell.classList.remove('focused'));

    if (focusChannel >= 0 && cells.has(focusChannel)) {
      grid.classList.add('grid-1x1');
      cells.get(focusChannel).cell.classList.remove('hidden');
      cells.get(focusChannel).cell.classList.add('focused');
      return;
    }

    const cls = LAYOUT_MAP[layoutSlots] || 'grid-2x2';
    grid.classList.add(cls);
  }

  function setLayout(slots) {
    layoutSlots = slots;
    focusChannel = -1;
    document.querySelectorAll('.layout-btn').forEach((btn) => {
      btn.classList.toggle('active', parseInt(btn.dataset.layout, 10) === slots);
    });
    buildGrid();
    reconnectAll();
  }

  function reconnectAll() {
    if (!WebFrameMessage) return;
    if (cfg.mode === 'gateway') {
      const ws = connectWs(cfg.wsUrls[0], 0);
      cells.forEach((state) => { state.ws = ws; });
    } else {
      cfg.wsUrls.forEach((url, index) => {
        if (index < cfg.maxChannels && cells.has(index)) {
          cells.get(index).ws = connectWs(url, index);
        }
      });
    }
  }

  function onFrame(channelId, frame) {
    const state = cells.get(channelId);
    if (!state || !state.active) return;

    const image = frame.image || frame.Image;
    const jpegBytes = normalizeJpegBytes(image && (image.jpeg || image.Jpeg));
    if (!jpegBytes || !jpegBytes.length) return;

    frameCount += 1;
    setStatus(`已连接 · 累计 ${frameCount} 帧 · ${cfg.wsUrls[0] || ''}`);

    recordFrameArrival(channelId, frame);
    refreshStatsPanel(channelId);
    refreshCellLabel(channelId);
    refreshSidebarDot(channelId);

    const blob = new Blob([jpegBytes], { type: 'image/jpeg' });
    const url = URL.createObjectURL(blob);
    if (state.img.dataset.url) URL.revokeObjectURL(state.img.dataset.url);
    state.img.dataset.url = url;
    state.img.src = url;
    state.img.style.display = 'block';
    state.cell.classList.remove('empty');
    state.cell.classList.add('has-signal');

    let inner = state.cell.querySelector('.cell-inner');
    if (inner) inner.remove();

    state.img.onerror = () => {
      setStatus(`JPEG 解码失败 (${camName(channelId)})`);
    };

    state.img.onload = () => {
      refreshSidebarThumb(channelId, state.img);
      const sidebarThumb = document.querySelector(`.channel-thumb[data-channel="${channelId}"]`);
      if (sidebarThumb && sidebarThumb.classList.contains('empty')) {
        const timg = document.createElement('img');
        timg.className = 'channel-thumb';
        timg.dataset.channel = String(channelId);
        sidebarThumb.replaceWith(timg);
      }
      refreshSidebarThumb(channelId, state.img);

      const canvas = state.canvas;
      canvas.width = state.img.clientWidth;
      canvas.height = state.img.clientHeight;
      canvas.style.display = 'block';
      const ctx = canvas.getContext('2d');
      const imgW = image.width || state.img.naturalWidth;
      const imgH = image.height || state.img.naturalHeight;
      updateResolutionFallback(channelId, frame, imgW, imgH);
      refreshStatsPanel(channelId);
      refreshCellLabel(channelId);
      TopsbotRender.drawOverlay(ctx, frame.overlay, imgW, imgH, layers);
    };

    const stats = frame.stats || frame.Stats;
    if (stats && stats.items) {
      stats.items.forEach((item) => {
        const type = item.type || '';
        if (type === 'fps' || type.startsWith('fps_')) {
          const metrics = getOrCreateMetrics(channelId);
          const v = item.value_string || item.value;
          const parsed = parseFloat(v, 10);
          if (!Number.isNaN(parsed) && parsed > 0) {
            metrics.fps = parsed;
            metrics.arrivalTimes = [];
            refreshStatsPanel(channelId);
            refreshCellLabel(channelId);
          }
        }
      });
    }
  }

  function connectWs(url, channelHint) {
    const ws = new ReconnectingWebSocket(url, [], { binaryType: 'arraybuffer' });
    ws.onopen = () => setStatus(`已连接 ${url}，等待视频…`);
    ws.onclose = () => setStatus('连接断开，重连中…');
    ws.onerror = () => setStatus(`WebSocket 错误: ${url}`);
    ws.onmessage = (event) => {
      if (!WebFrameMessage) return;
      try {
        const buf = event.data instanceof ArrayBuffer
          ? new Uint8Array(event.data)
          : new Uint8Array(event.data);
        const frame = WebFrameMessage.decode(buf);
        const channelId = cfg.mode === 'gateway'
          ? (frame.channelId || frame.channel_id || 0)
          : channelHint;
        onFrame(channelId, frame);
      } catch (err) {
        setStatus(`帧解析失败: ${err.message}`);
        console.error('WebFrame decode error', err);
      }
    };
    return ws;
  }

  document.getElementById('layoutBtns').addEventListener('click', (e) => {
    const btn = e.target.closest('.layout-btn');
    if (!btn) return;
    setLayout(parseInt(btn.dataset.layout, 10));
    sendControl({ cmd: 'grid' });
  });

  document.getElementById('btnFullscreen').addEventListener('click', () => {
    const el = document.querySelector('.grid-wrap');
    if (document.fullscreenElement) {
      document.exitFullscreen();
    } else if (el && el.requestFullscreen) {
      el.requestFullscreen();
    }
  });

  document.getElementById('btnSnapshot').addEventListener('click', () => {
    const id = focusChannel >= 0 ? focusChannel : 0;
    const state = cells.get(id);
    if (!state || !state.img.src) return;
    const a = document.createElement('a');
    a.href = state.img.src;
    a.download = `${camName(id)}_${Date.now()}.jpg`;
    a.click();
  });

  document.getElementById('btnSelectAll').addEventListener('click', () => {
    document.querySelectorAll('#channelList input[type="checkbox"]').forEach((cb) => {
      cb.checked = true;
      cb.dispatchEvent(new Event('change'));
    });
  });

  document.addEventListener('keydown', (e) => {
    if (e.key === 'Escape') {
      focusChannel = -1;
      applyLayout();
      refreshStatsPanel(0);
      sendControl({ cmd: 'grid' });
    }
  });

  protobuf.load('protos/topsbot_web.proto', (err, root) => {
    if (err) {
      setStatus('Protobuf 加载失败: ' + err.message);
      return;
    }
    WebFrameMessage = root.lookupType('topsbot.web.WebFrameMessage');
    buildGrid();
    reconnectAll();
  });
})();
