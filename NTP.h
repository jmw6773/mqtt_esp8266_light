#ifndef NTP_H_
#define NTP_H_

#include <ESP8266WiFi.h>
#include <time.h>                       // time() ctime()
#include <sys/time.h>                   // struct timeval
#include <coredecls.h> // settimeofday_cb()

//#define NTP_DEBUG

class NtpClient 
{
  public:
    static void ICACHE_FLASH_ATTR Ntp(const char * _ntpServer, int8_t _tz);

    static time_t ICACHE_FLASH_ATTR getUptimeSec();
    static time_t ICACHE_FLASH_ATTR getUtcTimeNow();
    static int8_t ICACHE_FLASH_ATTR getTZ();
  
  private:
	  static void ICACHE_FLASH_ATTR time_is_set(void);
    
    static timeval cbtime; // time set in callback
    static bool cbtime_set;
    static int8_t timeZone;
    
    //timeval tv;
    static time_t now;
};

#endif /* NTP_H_ */
