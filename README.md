This project was based on the esp-mqtt-rgb-led project by Corban Mailloux:
 https://github.com/corbanmailloux/esp-mqtt-rgb-led

 
v0.1.0 - Removed IOTAppStory and added HTML web status and config page
v0.1.1 - Memory leak fix (?), random bug fixes, added random color on turn on if no color/transistion defined
 *** Don't update to ESP Board 2.4.2  until confirm OTA is fixed
 ***   https://github.com/esp8266/Arduino/issues/5023

 
** System Changes **

File:
  ~\AppData\Local\Arduino15\packages\esp8266\hardware\esp8266\2.4.2\libraries\ESP8266WebServer\src\detail\RequestHandlersImpl.h

//change following in function 'bool handle(...)'
//this fixes serving gz files when they exist instead of the uncompressed file


##ESP Version 4.2

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



#ESP Version 4.0

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
