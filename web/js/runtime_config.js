// Default when opening http://<host>:8000/ without query params.
// Launch overwrites this via a temp HTTP root (see web_http_utils.py).
window.TopsbotWebvizRuntime = {
  mode: 'gateway',
  maxChannels: 1,
  ws_base_port: 8080,
  gateway_ws_port: 9090,
};
