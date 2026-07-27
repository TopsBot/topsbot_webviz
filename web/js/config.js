// TOPSBOT WebViz runtime config
// Priority: URL query > js/runtime_config.js (from launch/yaml) > defaults
// Open http://<board>:8000/ — no query string required when launch injects runtime.

(function () {
  const params = new URLSearchParams(window.location.search);
  const rt = window.TopsbotWebvizRuntime || {};

  const mode = params.get('mode') || rt.mode || 'gateway';
  const host = params.get('host') || window.location.hostname || '127.0.0.1';
  const basePort = parseInt(
    params.get('ws_base_port') || String(rt.ws_base_port || 8080), 10);
  const gatewayPort = parseInt(
    params.get('gateway_ws_port') || String(rt.gateway_ws_port || 9090), 10);
  const maxChannels = parseInt(
    params.get('max_channels') || String(rt.maxChannels || 1), 10);

  window.TopsbotWebvizConfig = {
    mode,
    host,
    basePort,
    gatewayPort,
    maxChannels,
    wsUrls: mode === 'gateway'
      ? [`ws://${host}:${gatewayPort}`]
      : Array.from({ length: maxChannels }, (_, i) =>
        `ws://${host}:${basePort + i * 2}`),
  };
})();
