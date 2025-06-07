#include <websocket_server.h>
#include <my_blynk.h>

ESP32WebSocketServer socket_server(ssid, pass);

void setup() 
{
  socket_server.begin();
#if USE_BLYNK == true
  Blynk.begin(BLYNK_AUTH_TOKEN, ssid, pass);
#endif
}

void loop()
{
#if USE_BLYNK == true
    Blynk.run();
#endif
    socket_server.loop();
delay(50);  // Required for ESP32 to run properly
}
