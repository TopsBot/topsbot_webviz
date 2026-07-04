// TOPSBOT WebViz runtime config (override via query string)

(function () {
  const params = new URLSearchParams(window.location.search);
  const mode = params.get('mode') || 'multi_instance';
  const host = params.get('host') || window.location.hostname || '127.0.0.1';
  const basePort = parseInt(params.get('ws_base_port') || '8080', 10);
  const gatewayPort = parseInt(params.get('gateway_ws_port') || '9090', 10);
  const maxChannels = parseInt(params.get('max_channels') || '4', 10);

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
