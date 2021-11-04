#!/home/pi/shutters/venv/bin/python
# -*- coding: utf-8 -*-

# # # # # # # # # # # # # # # # # # # # #
# Ilya Mazlov https://github.com/wi1k1n
# # # # # # # # # # # # # # # # # # # # #

import requests, time, datetime as dt, pytz
from astral import LocationInfo
from astral.sun import sun, golden_hour, blue_hour, dusk, now as anow

from credentials import *

IP_ADDRESS = "192.168.0.105"
city = LocationInfo(ADDRESS_CITY, ADDRESS_COUNTRY, ADDRESS_TIMEZONE, ADDRESS_LATITUDE, ADDRESS_LONGITUTE)
SR_ADD_HOURS = 1.0  # how many hours to add to 'sunrise' time (can be negative)
SS_ADD_HOURS = 1.0  # how many hours to add to 'sunset' time (can be negative)


def MoveShutters(dir = 0):
    # Moves shutters: close if dir == 0, open if dir == 1
    print("[SHUTTERS] " + ("Open" if dir else "Close"))
    url = "http://" + IP_ADDRESS + "/" + ("right" if dir else "left")
    def makeRequest(n = 0):
        if n > 2: return
        res = requests.get(url, auth=requests.auth.HTTPDigestAuth(WEB_LOGIN, WEB_PASSWORD))
        if res.status_code != 200:
            print("{0}[Error] Error occurred while sending move request:".format(dt.datetime.now()))
            print("[{0}] {1}".format(res.status_code, res.text))
            print(res.headers)
            return
        elif res.text != "success":
            if res.text == "try again later":
                time.sleep(5)
                makeRequest(n + 1)
            else:
                print("{1}[Error] Unknown return statement: {0}".format(res.text, dt.datetime.now()))
                return
        else:
            print("{1}[Success] Successfully {0} shutters!".format("opened" if dir else "closed", dt.datetime.now()))
            return
    makeRequest()

def GetSunriseSet(date, observer):
    # s = sun(observer, date)
    # return s['sunrise'], s['sunset']

    gh = golden_hour(observer, date)
    dsk = dusk(observer, date)
    srah, ssah = int(SR_ADD_HOURS), int(SS_ADD_HOURS)
    sram, ssam = (srah - SR_ADD_HOURS) * 60, (ssah - SS_ADD_HOURS) * 60
    return gh[1] + dt.timedelta(hours=srah, minutes=sram), dsk + dt.timedelta(hours=ssah, minutes=ssam)


if __name__ == '__main__':
    print("Welcome to Electric Shutter Scheduler!")
    print("It's {0}".format(dt.datetime.now()))
    # print("Use 'help' to check available commands")

    while True:
        now = dt.datetime.now().replace(tzinfo=pytz.UTC)
        # # DEBUG
        # now = dt.datetime(2021, 11, 4, 8, 7, 40, tzinfo=pytz.UTC)
        # print("=>  {0}".format(now))
        # # /DEBUG
        sr, ss = GetSunriseSet(now.date(), city.observer)

        if now.timestamp() < sr.timestamp():
            baseDT = sr
            dir = 1
            sunwhat = "Sunrise"
        elif now.timestamp() < ss.timestamp():
            baseDT = ss
            dir = 0
            sunwhat = "Sunset"
        else:
            sr, _ = GetSunriseSet(now + dt.timedelta(days=1), city.observer)
            baseDT = sr
            dir = 1
            sunwhat = "Sunrise"

        duration = baseDT.timestamp() - now.timestamp()
        print("{4} at {0}. Shutters will be {2} in {1} seconds ({3} hours)".format(baseDT, duration, "OPENED" if dir else "CLOSED", duration / 3600, sunwhat))
        time.sleep(duration)
        MoveShutters(dir)
        time.sleep(10)
