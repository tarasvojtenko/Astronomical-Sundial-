// astronomical_sundial.cpp
#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <cmath>
#include <ctime>
#include <iomanip>
#include <sstream>
#include <nlohmann/json.hpp>
#include <getopt.h>

using namespace std;
using json = nlohmann::json;

const double DEG2RAD = M_PI / 180.0;
const double RAD2DEG = 180.0 / M_PI;

double julianDay(const tm& dt) {
    int year = dt.tm_year + 1900;
    int month = dt.tm_mon + 1;
    double day = dt.tm_mday + dt.tm_hour/24.0 + dt.tm_min/1440.0 + dt.tm_sec/86400.0;
    if (month <= 2) { year--; month += 12; }
    int A = year / 100;
    int B = 2 - A + A / 4;
    return (int)(365.25 * (year + 4716)) + (int)(30.6001 * (month + 1)) + day + B - 1524.5;
}

double solarDeclination(int dayOfYear) {
    return 23.44 * DEG2RAD * sin((284 + dayOfYear) * 360 * DEG2RAD / 365);
}

double equationOfTime(int dayOfYear) {
    double B = (360.0 / 365) * (dayOfYear - 81);
    double B_rad = B * DEG2RAD;
    return 9.87 * sin(2 * B_rad) - 7.53 * cos(B_rad) - 1.5 * sin(B_rad);
}

struct SolarData {
    double altitude, azimuth, solarTime, eot, declination, sunrise, sunset, dayLength;
};

SolarData sunPosition(double latDeg, double lonDeg, const tm& dt) {
    double latRad = latDeg * DEG2RAD;
    int dayOfYear = dt.tm_yday + 1;
    double decRad = solarDeclination(dayOfYear);
    double eot = equationOfTime(dayOfYear);
    double hourUTC = dt.tm_hour + dt.tm_min/60.0 + dt.tm_sec/3600.0;
    double solarTime = hourUTC + (4 * lonDeg) / 60.0 + eot / 60.0;
    double haRad = (solarTime - 12) * 15 * DEG2RAD;

    double altRad = asin(sin(latRad)*sin(decRad) + cos(latRad)*cos(decRad)*cos(haRad));
    double altDeg = altRad * RAD2DEG;

    double aziRad = atan2(-sin(haRad)*cos(decRad),
                          sin(decRad)*cos(latRad) - cos(decRad)*sin(latRad)*cos(haRad));
    double aziDeg = fmod(aziRad * RAD2DEG + 360, 360);

    double cosHASunrise = -tan(latRad) * tan(decRad);
    double haSunrise;
    if (cosHASunrise < -1) haSunrise = M_PI;
    else if (cosHASunrise > 1) haSunrise = 0;
    else haSunrise = acos(cosHASunrise);
    double dayLength = haSunrise * 2 / (M_PI / 12);
    double noon = 12.0 - lonDeg/15.0 - eot/60.0;
    double sunrise = noon - dayLength/2;
    double sunset = noon + dayLength/2;

    return {altDeg, aziDeg, solarTime, eot, decRad * RAD2DEG, sunrise, sunset, dayLength};
}

string drawSundial(double azimuthDeg) {
    vector<string> dirNames = {"N", "NE", "E", "SE", "S", "SW", "W", "NW"};
    int idx = (int)round(azimuthDeg / 45) % 8;
    string shadowDir = dirNames[idx];
    stringstream ss;
    ss << "      N\n";
    ss << "      |\n";
    ss << "  W---+---E\n";
    ss << "      |\n";
    ss << "      S\n";
    ss << "\nShadow direction: " << shadowDir << " (" << fixed << setprecision(1) << azimuthDeg << "°)\n";
    return ss.str();
}

string formatTime(double hours) {
    int h = (int)hours % 24;
    int m = (int)((hours - h) * 60);
    char buf[6];
    snprintf(buf, sizeof(buf), "%02d:%02d", h, m);
    return string(buf);
}

int main(int argc, char* argv[]) {
    static struct option long_options[] = {
        {"lat", required_argument, 0, 'a'},
        {"lon", required_argument, 0, 'o'},
        {"tz", required_argument, 0, 'z'},
        {"date", required_argument, 0, 'd'},
        {"time", required_argument, 0, 't'},
        {"dial-only", no_argument, 0, 'p'},
        {0,0,0,0}
    };
    int opt;
    double lat = 0.0, lon = 0.0, tz = 0.0;
    string dateStr, timeStr;
    bool dialOnly = false;

    while ((opt = getopt_long(argc, argv, "a:o:z:d:t:p", long_options, nullptr)) != -1) {
        switch (opt) {
            case 'a': lat = stod(optarg); break;
            case 'o': lon = stod(optarg); break;
            case 'z': tz = stod(optarg); break;
            case 'd': dateStr = optarg; break;
            case 't': timeStr = optarg; break;
            case 'p': dialOnly = true; break;
            default:
                cerr << "Usage: astronomical_sundial --lat LAT --lon LON --tz TZ --date YYYY-MM-DD --time HH:MM --dial-only\n";
                return 1;
        }
    }

    time_t now = time(nullptr);
    tm dt = *gmtime(&now);
    if (!dateStr.empty()) {
        strptime(dateStr.c_str(), "%Y-%m-%d", &dt);
    }
    if (!timeStr.empty()) {
        strptime(timeStr.c_str(), "%H:%M", &dt);
    }

    if (dialOnly) {
        SolarData data = sunPosition(lat, lon, dt);
        cout << drawSundial(data.azimuth);
        return 0;
    }

    SolarData data = sunPosition(lat, lon, dt);
    double alt = data.altitude, azi = data.azimuth;
    double solarTime = data.solarTime, eot = data.eot;
    double dec = data.declination;
    double sunrise = data.sunrise + tz;
    double sunset = data.sunset + tz;
    double dayLen = data.dayLength;
    int dayLenH = (int)dayLen;
    int dayLenM = (int)((dayLen - dayLenH) * 60);

    string latStr = to_string(abs(lat)).substr(0,5) + "°" + (lat >= 0 ? "N" : "S");
    string lonStr = to_string(abs(lon)).substr(0,5) + "°" + (lon >= 0 ? "E" : "W");
    char tzSign = tz >= 0 ? '+' : '-';
    char dateBuf[20];
    strftime(dateBuf, sizeof(dateBuf), "%Y-%m-%d %H:%M", &dt);
    double localTime = dt.tm_hour + dt.tm_min/60.0 + tz;

    cout << "\n☀️ Astronomical Sundial\n";
    cout << "Location: " << latStr << ", " << lonStr << "\n";
    cout << "Date: " << dateBuf << " (UTC" << tzSign << abs(tz) << ")\n";
    cout << "Local Time: " << formatTime(localTime) << "\n";
    cout << "\nSolar Declination: " << fixed << setprecision(1) << showpos << dec << "\xE2\x81\xB0\n";
    cout << "Equation of Time: " << fixed << setprecision(1) << showpos << eot << " min\n";
    int solarH = (int)solarTime;
    int solarM = (int)((solarTime - solarH) * 60);
    cout << "Solar Time: " << setw(2) << setfill('0') << solarH << ":" << setw(2) << solarM << "\n";
    cout << "\nSolar Altitude: " << fixed << setprecision(1) << noshowpos << alt << "\xE2\x81\xB0\n";
    cout << "Solar Azimuth: " << fixed << setprecision(1) << azi << "\xE2\x81\xB0\n";
    cout << "\nSunrise: " << formatTime(sunrise) << " | Sunset: " << formatTime(sunset) << "\n";
    cout << "Day length: " << dayLenH << "h " << dayLenM << "m\n";
    cout << "\n" << drawSundial(azi);

    return 0;
}
