# MQTT RBG Controller for the ESP8266

This project was based on the esp-mqtt-rgb-led project by Corban Mailloux:<br>
 https://github.com/corbanmailloux/esp-mqtt-rgb-led

 
v0.1.0 - Removed IOTAppStory and added HTML web status and config page<br>
v0.1.1 - Memory leak fix (?), random bug fixes, added random color on turn on if no color/transistion defined<br>
 *** Don't update to ESP Board 2.4.2  until confirm OTA is fixed<br>
 ***   https://github.com/esp8266/Arduino/issues/5023<br>

 
### *** ESP Core File Changes ***

File:<br>
  ~\AppData\Local\Arduino15\packages\esp8266\hardware\esp8266\2.4.2\libraries\ESP8266WebServer\src\detail\RequestHandlersImpl.h

 Change the following in function 'bool handle(...)'<br>
  -- this fixes serving gz files when they exist instead of the uncompressed file<br>


## ESP Version 4.2
```C
if (!path.endsWith(FPSTR(mimeTable[gz].endsWith)) && !_fs.exists(path))
{
  String pathWithGz = path + FPSTR(mimeTable[gz].endsWith);
  if(_fs.exists(pathWithGz))
  path += FPSTR(mimeTable[gz].endsWith);
}

if (!path.endsWith(FPSTR(mimeTable[gz].endsWith)) { //&& !_fs.exists(path))
{
  String pathWithGz = path + FPSTR(mimeTable[gz].endsWith);
  if(_fs.exists(pathWithGz))
  path += FPSTR(mimeTable[gz].endsWith);
}
```

## ESP Version 4.0
```C
if (!path.endsWith(".gz") && !_fs.exists(path))
{
  String pathWithGz = path + ".gz";
  if(_fs.exists(pathWithGz))
    path += ".gz";
}

if (!path.endsWith(".gz"))// && !_fs.exists(path))
{
  String pathWithGz = path + ".gz";
  if(_fs.exists(pathWithGz))
    path += ".gz";
}# mqtt_esp8266_light
```
