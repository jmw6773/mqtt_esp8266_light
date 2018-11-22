#include "NTP.h"
//#include <sys/time.h>

//Initialize class variables
int8_t NtpClient::timeZone;
bool NtpClient::cbtime_set = false;
time_t NtpClient::now;
timeval NtpClient::cbtime;

void ICACHE_FLASH_ATTR NtpClient::Ntp(const char * _ntpServer, int8_t _tz)
{
  timeZone = _tz;
  
  configTime( timeZone * 3600, 0, _ntpServer);
  //setenv("TZ", "JST-9", 0);  
  //tzset();
}

int8_t ICACHE_FLASH_ATTR NtpClient::getTZ()
{
    return timeZone;
}

time_t ICACHE_FLASH_ATTR NtpClient::getUptimeSec() 
{
	return (millis () / 1000);
}

time_t ICACHE_FLASH_ATTR NtpClient::getUtcTimeNow()
{
  time_t currTime = 0;
  int8_t loopCount = 0;
  
  while (currTime < 5000000 && loopCount < 20)  //loop until we get a valid time or a timeout (50ms * 20 = 1 sec)
  {
    currTime = time(nullptr);
    loopCount++;
    delay(50);
  }
  
#ifdef NTP_DEBUG
  Serial.print("getUTCTimeNow: ");
  Serial.println(currTime);
#endif //NTP_DEBUG

  return currTime;
}

void ICACHE_FLASH_ATTR NtpClient::time_is_set(void)
{
  gettimeofday(&cbtime, NULL);
  cbtime_set = true;
}
