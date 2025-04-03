#include <websocket_server.h>

ESP32WebSocketServer socket_server("Galaxy", "pnml6201");

void setup() 
{
  socket_server.begin();
}

void loop()
{
  socket_server.loop();
}
