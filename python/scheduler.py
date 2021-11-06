#!/home/pi/shutters/venv/bin/python
# -*- coding: utf-8 -*-

# # # # # # # # # # # # # # # # # # # # #
# Ilya Mazlov https://github.com/wi1k1n
# # # # # # # # # # # # # # # # # # # # #

import requests, time, datetime as dt, pytz, configparser
from astral import LocationInfo
from astral.sun import sun, golden_hour, blue_hour, dusk, now as anow

CONFIGURATION_PATH = "config.ini"
SR_ADD_HOURS = 0.0  # how many hours to add to 'sunrise' time (can be negative)
SS_ADD_HOURS = 0.0  # how many hours to add to 'sunset' time (can be negative)
WEB_LOGIN = WEB_PASSWORD = IP_ADDRESS = city = None
DEBUG = False


def MoveShutters(dir = 0):
    # Moves shutters: close if dir == 0, open if dir == 1
    print("{0} [Trigger] Trying to {1}".format(dt.datetime.now(), ("Open" if dir else "Close")))
    url = "http://" + IP_ADDRESS + "/" + ("right" if dir else "left")
    def makeRequest(n = 0):
        if n > 2:
            return
        if DEBUG:
            print("[DEBUG] Getting URL: {0}".format(url))
            return
        res = requests.get(url, auth=requests.auth.HTTPDigestAuth(WEB_LOGIN, WEB_PASSWORD))
        if res.status_code != 200:
            print("{0} [Error] Error occurred while sending move request:".format(dt.datetime.now()))
            print("[{0}] {1}".format(res.status_code, res.text))
            print(res.headers)
            return
        elif res.text != "success":
            if res.text == "try again later":
                reattemptDelay = 5
                print("{0} [Wait] The client is busy. Attempt #{1} in {2}s".format(dt.datetime.now(), n + 2, reattemptDelay))
                time.sleep(reattemptDelay)
                makeRequest(n + 1)
            else:
                print("{1} [Error] Unknown return statement: {0}".format(res.text, dt.datetime.now()))
                return
        else:
            print("{1} [Success] Successfully {0} shutters!".format("opened" if dir else "closed", dt.datetime.now()))
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

def loadConfigAttrOrExit(config, section, attr, obfuscate = False):
    if not config.has_option(section, attr):
        print("> Error parsing config file. No attribute '{1}' found in section [{0}]".format(section, attr))
        exit(2)
    val = config[section][attr]
    print("> [{0}].{1} = {2}".format(section, attr, '*****' if obfuscate else val))
    return val

if __name__ == '__main__':
    print("===> Welcome to Electric Shutter Scheduler! <===")
    print("It's {0} now".format(dt.datetime.now()))
    print("\nLoading configuration from {0}".format(CONFIGURATION_PATH))

    config = configparser.ConfigParser()
    if not config.read(CONFIGURATION_PATH):
        print("> Error loading configuration file {0}! Exiting..".format(CONFIGURATION_PATH))
        exit(1)

    IP_ADDRESS = loadConfigAttrOrExit(config, 'DEFAULT', 'IP')
    WEB_LOGIN = loadConfigAttrOrExit(config, 'DEFAULT', 'WEB_LOGIN')
    WEB_PASSWORD = loadConfigAttrOrExit(config, 'DEFAULT', 'WEB_PASSWORD', True)
    SR_ADD_HOURS = float(loadConfigAttrOrExit(config, 'DEFAULT', 'SR_ADD_HOURS'))
    SS_ADD_HOURS = float(loadConfigAttrOrExit(config, 'DEFAULT', 'SS_ADD_HOURS'))
    ADDRESS_LATITUDE = float(loadConfigAttrOrExit(config, 'DEFAULT', 'LATITUDE'))
    ADDRESS_LONGITUDE = float(loadConfigAttrOrExit(config, 'DEFAULT', 'LONGITUDE'))
    city = LocationInfo(latitude=ADDRESS_LATITUDE, longitude=ADDRESS_LONGITUDE)
    print("")

    # print("Use 'help' to check available commands")

    while True:
        now = dt.datetime.now().replace(tzinfo=pytz.UTC)
        if DEBUG:
            now = dt.datetime(2021, 11, 7, 8, 13, 30, tzinfo=pytz.UTC)
            print("=>  {0}".format(now))

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
        print("   > {4} at {0}. Shutters will be {2} in {1} seconds ({3} hours)".format(baseDT, duration, "OPENED" if dir else "CLOSED", duration / 3600, sunwhat))
        time.sleep(duration)
        MoveShutters(dir)
        time.sleep(5)
