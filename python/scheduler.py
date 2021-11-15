#!/home/pi/shutters/venv/bin/python
# -*- coding: utf-8 -*-

# # # # # # # # # # # # # # # # # # # # #
# Ilya Mazlov https://github.com/wi1k1n
# # # # # # # # # # # # # # # # # # # # #

import requests, time, datetime as dt, pytz, configparser, sys, math, functools
from astral import LocationInfo
from astral.sun import sun, golden_hour, blue_hour, dusk, now as anow
from colorama import Fore
sign = functools.partial(math.copysign, 1)

CONFIGURATION_PATH = "config.ini"
# SR_ADD_HOURS: how many hours to add to 'sunrise' time (can be negative)
# SS_ADD_HOURS: how many hours to add to 'sunset' time (can be negative)
IP_ADDRESS = WEB_LOGIN = WEB_PASSWORD = WEATHER_API_KEY = ADDRESS_LATITUDE = ADDRESS_LONGITUDE = city = OVERCAST_HOURS = None
OUTDATED_WEATHER_TIME = 43200  # maximum time (in seconds) after which forecast considered to be outdated (too far in future)
CLOUDS_LOWER_BOUND = 0.35
CLOUDS_UPPER_BOUND = 0.75
SHUTTERS_REATTEMPT_DELAY = 5
SHUTTERS_REATTEMPT_N = 2  # number of sequential attempts
SHUTTERS_COOLDOWN_DELAY = 5
DEBUG = False


def MoveShutters(dir = 0):
    # Moves shutters: close if dir == 0, open if dir == 1
    print("{0} [Trigger] Trying to {1}".format(dt.datetime.now(), ("Open" if dir else "Close")))
    url = "http://" + IP_ADDRESS + "/" + ("right" if dir else "left")
    def makeRequest(n = 0):
        if n > SHUTTERS_REATTEMPT_N:
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
                reattemptDelay = SHUTTERS_REATTEMPT_DELAY
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
    sram, ssam = (abs(SR_ADD_HOURS) - abs(srah)) * 60 * sign(SR_ADD_HOURS), (abs(SS_ADD_HOURS) - abs(ssah)) * 60 * sign(SS_ADD_HOURS)

    sr, ss = gh[1] + dt.timedelta(hours=srah, minutes=sram), dsk + dt.timedelta(hours=ssah, minutes=ssam)

    # Consider overcast conditions as well
    def adjustClouds(date, dir):
        forecast = getWeatherInfo(date)
        if not ('clouds' in forecast) or not('dt' in forecast):
            return date, None
        if abs(forecast['dt'] - date.timestamp()) > OUTDATED_WEATHER_TIME:  # forecast is more than 12 hours far from requested datetime
            print(' ! Forecast returned outdated/too_far result! Forecast date: {0}'.format(dt.datetime.fromtimestamp(forecast['dt'])))
            return date, None
        cl = forecast['clouds'] / 100.0
        lb, hb = CLOUDS_LOWER_BOUND, CLOUDS_UPPER_BOUND
        mlt = 1.0
        if cl < lb:
            return date, 0
        elif cl < hb:
            mlt = 1 / (hb - lb) * (cl - lb)

        hours = OVERCAST_HOURS * mlt * dir
        minutes = (hours - int(hours)) * 60
        hours = int(hours)

        return date + dt.timedelta(hours=hours, minutes=minutes), cl


    src, clr = adjustClouds(sr, 1)
    ssc, cls = adjustClouds(ss, -1)

    print(Fore.CYAN+'Golden Hour'+Fore.RESET+' ends at {0}.\n'
          'Offsetting:         {1}.\n'
          'After forecasting:  {2}{3}.'.format(gh[1], sr, src, '' if clr is None else ' (Clouds {0:.0f}%)'.format(clr * 100.0)))
    print(Fore.YELLOW+'Dusk'+Fore.RESET+' starts at      {0}.\n'
          'Offsetting:         {1}.\n'
          'After forecasting:  {2}{3}.'.format(dsk, ss, ssc, '' if cls is None else ' (Clouds {0:.0f}%)'.format(cls * 100.0)))
    return src, ssc

def getWeatherInfo(date, closest=True):
    # Gets date and returns (el, em) - entries from the forecast for the time period that contains given date
    # Returns (None, em) if there is no entry earlier than date and (el, None) if there is no entry later than date
    url = 'https://api.openweathermap.org/data/2.5/onecall?units=metric&appid='+WEATHER_API_KEY+'&lat='+str(ADDRESS_LATITUDE)+'&lon='+str(ADDRESS_LONGITUDE)
    resp = requests.get(url).json()

    l = []
    if 'current' in resp:
        l.append(resp['current'])
    if 'hourly' in resp:
        l = l + resp['hourly']

    if not len(l):
        print("> Error occurred when getting weather info. Check your API-key or https://openweathermap.org/ servers uptime")
        return None if closest else None, None

    l = sorted(l, key=lambda d: d['dt'])

    el = em = None
    for entry in l:
        el, em, cd = em, entry, dt.datetime.fromtimestamp(entry['dt']).replace(tzinfo=pytz.UTC)
        if cd > date:
            break
    if date > dt.datetime.fromtimestamp(em['dt']).replace(tzinfo=pytz.UTC):
        el, em = em, None
    if closest:
        dl = abs(date.timestamp() - el['dt']) if el else sys.maxsize
        dm = abs(date.timestamp() - em['dt']) if em else sys.maxsize
        return el if dl < dm else em
    return el, em

def loadConfigAttrOrExit(config, section, attr, obfuscate = False):
    if not config.has_option(section, attr):
        print("> Error parsing config file. No attribute '{1}' found in section [{0}]".format(section, attr))
        exit(2)
    val = config[section][attr]
    print("> [{0}].{1} = {2}".format(section, attr, '*****' if obfuscate else val))
    return val

def initConfig():
    config = configparser.ConfigParser()
    if not config.read(CONFIGURATION_PATH):
        print("> Error loading configuration file {0}! Exiting..".format(CONFIGURATION_PATH))
        exit(1)
    global IP_ADDRESS, WEB_LOGIN, WEB_PASSWORD, SR_ADD_HOURS, SS_ADD_HOURS, WEATHER_API_KEY, ADDRESS_LATITUDE, ADDRESS_LONGITUDE, city, OVERCAST_HOURS
    IP_ADDRESS = loadConfigAttrOrExit(config, 'DEFAULT', 'IP')
    WEB_LOGIN = loadConfigAttrOrExit(config, 'DEFAULT', 'WEB_LOGIN')
    WEB_PASSWORD = loadConfigAttrOrExit(config, 'DEFAULT', 'WEB_PASSWORD', True)
    SR_ADD_HOURS = float(loadConfigAttrOrExit(config, 'DEFAULT', 'SR_ADD_HOURS'))
    SS_ADD_HOURS = float(loadConfigAttrOrExit(config, 'DEFAULT', 'SS_ADD_HOURS'))
    WEATHER_API_KEY = loadConfigAttrOrExit(config, 'DEFAULT', 'WEATHER_API_KEY', True)
    OVERCAST_HOURS = float(loadConfigAttrOrExit(config, 'DEFAULT', 'OVERCAST_HOURS'))
    ADDRESS_LATITUDE = float(loadConfigAttrOrExit(config, 'DEFAULT', 'LATITUDE'))
    ADDRESS_LONGITUDE = float(loadConfigAttrOrExit(config, 'DEFAULT', 'LONGITUDE'))
    city = LocationInfo(latitude=ADDRESS_LATITUDE, longitude=ADDRESS_LONGITUDE)


if __name__ == '__main__':
    print("===> Welcome to Electric Shutter Scheduler! <===")
    print("It's {0} now".format(dt.datetime.now()))
    print("\nLoading configuration from {0}".format(CONFIGURATION_PATH))

    initConfig()
    print("")

    # print("Use 'help' to check available commands")

    while True:
        now = dt.datetime.now().replace(tzinfo=pytz.UTC)
        if DEBUG:
            now = dt.datetime(2021, 11, 16, 15, 13, 30, tzinfo=pytz.UTC)
            print("=>  {0}".format(now))

        sr, ss = GetSunriseSet(now.date(), city.observer)

        if now.timestamp() < sr.timestamp():
            baseDT, dir, sunwhat = sr, 1, "Sunrise"
        elif now.timestamp() < ss.timestamp():
            baseDT, dir, sunwhat = ss, 0, "Sunset"
        else:
            sr, _ = GetSunriseSet(now + dt.timedelta(days=1), city.observer)
            baseDT, dir, sunwhat = sr, 1, "Sunrise"

        duration = baseDT.timestamp() - now.timestamp()
        print("   > {3} at {0}. Shutters will be {2} in {1}".format(baseDT, dt.timedelta(seconds=int(duration)), "OPENED" if dir else "CLOSED", sunwhat))

        time.sleep(duration)
        MoveShutters(dir)
        time.sleep(SHUTTERS_COOLDOWN_DELAY)
