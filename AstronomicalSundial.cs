// AstronomicalSundial.cs
using System;
using System.Collections.Generic;

class SolarData
{
    public double Altitude, Azimuth, SolarTime, EOT, Declination, Sunrise, Sunset, DayLength;
}

class AstronomicalSundial
{
    const double DEG2RAD = Math.PI / 180.0;
    const double RAD2DEG = 180.0 / Math.PI;

    static double JulianDay(DateTime dt)
    {
        int year = dt.Year;
        int month = dt.Month;
        double day = dt.Day + dt.Hour/24.0 + dt.Minute/1440.0 + dt.Second/86400.0;
        if (month <= 2) { year--; month += 12; }
        int A = year / 100;
        int B = 2 - A + A / 4;
        return (int)(365.25 * (year + 4716)) + (int)(30.6001 * (month + 1)) + day + B - 1524.5;
    }

    static double SolarDeclination(int dayOfYear)
        => 23.44 * DEG2RAD * Math.Sin((284 + dayOfYear) * 360 * DEG2RAD / 365);

    static double EquationOfTime(int dayOfYear)
    {
        double B = (360.0 / 365) * (dayOfYear - 81);
        double B_rad = B * DEG2RAD;
        return 9.87 * Math.Sin(2 * B_rad) - 7.53 * Math.Cos(B_rad) - 1.5 * Math.Sin(B_rad);
    }

    static SolarData SunPosition(double latDeg, double lonDeg, DateTime dt)
    {
        double latRad = latDeg * DEG2RAD;
        int dayOfYear = dt.DayOfYear;
        double decRad = SolarDeclination(dayOfYear);
        double eot = EquationOfTime(dayOfYear);
        double hourUTC = dt.Hour + dt.Minute/60.0 + dt.Second/3600.0;
        double solarTime = hourUTC + (4 * lonDeg) / 60.0 + eot / 60.0;
        double haRad = (solarTime - 12) * 15 * DEG2RAD;

        double altRad = Math.Asin(Math.Sin(latRad)*Math.Sin(decRad) + Math.Cos(latRad)*Math.Cos(decRad)*Math.Cos(haRad));
        double altDeg = altRad * RAD2DEG;

        double aziRad = Math.Atan2(-Math.Sin(haRad)*Math.Cos(decRad),
                                   Math.Sin(decRad)*Math.Cos(latRad) - Math.Cos(decRad)*Math.Sin(latRad)*Math.Cos(haRad));
        double aziDeg = (aziRad * RAD2DEG + 360) % 360;

        double cosHASunrise = -Math.Tan(latRad) * Math.Tan(decRad);
        double haSunrise;
        if (cosHASunrise < -1) haSunrise = Math.PI;
        else if (cosHASunrise > 1) haSunrise = 0;
        else haSunrise = Math.Acos(cosHASunrise);
        double dayLength = haSunrise * 2 / (Math.PI / 12);
        double noon = 12.0 - lonDeg/15.0 - eot/60.0;
        double sunrise = noon - dayLength/2;
        double sunset = noon + dayLength/2;

        return new SolarData
        {
            Altitude = altDeg,
            Azimuth = aziDeg,
            SolarTime = solarTime,
            EOT = eot,
            Declination = decRad * RAD2DEG,
            Sunrise = sunrise,
            Sunset = sunset,
            DayLength = dayLength
        };
    }

    static string DrawSundial(double azimuthDeg)
    {
        string[] dirNames = {"N", "NE", "E", "SE", "S", "SW", "W", "NW"};
        int idx = (int)Math.Round(azimuthDeg / 45) % 8;
        string shadowDir = dirNames[idx];
        return "      N\n" +
               "      |\n" +
               "  W---+---E\n" +
               "      |\n" +
               "      S\n" +
               $"\nShadow direction: {shadowDir} ({azimuthDeg:F1}°)";
    }

    static string FormatTime(double hours)
    {
        int h = (int)hours % 24;
        int m = (int)((hours - h) * 60);
        return $"{h:D2}:{m:D2}";
    }

    static void Main(string[] args)
    {
        var parsed = ParseArgs(args);
        double lat = parsed.ContainsKey("lat") ? double.Parse(parsed["lat"]) : 0.0;
        double lon = parsed.ContainsKey("lon") ? double.Parse(parsed["lon"]) : 0.0;
        double tz = parsed.ContainsKey("tz") ? double.Parse(parsed["tz"]) : 0.0;
        bool dialOnly = parsed.ContainsKey("dial-only");

        DateTime dt = DateTime.UtcNow;
        if (parsed.ContainsKey("date"))
        {
            dt = DateTime.Parse(parsed["date"] + " 00:00:00").ToUniversalTime();
        }
        if (parsed.ContainsKey("time"))
        {
            var parts = parsed["time"].Split(':');
            dt = new DateTime(dt.Year, dt.Month, dt.Day, int.Parse(parts[0]), int.Parse(parts[1]), 0, DateTimeKind.Utc);
        }
        else
        {
            dt = new DateTime(dt.Year, dt.Month, dt.Day, dt.Hour, dt.Minute, 0, DateTimeKind.Utc);
        }

        if (dialOnly)
        {
            var data = SunPosition(lat, lon, dt);
            Console.Write(DrawSundial(data.Azimuth));
            return;
        }

        var sunData = SunPosition(lat, lon, dt);
        double alt = sunData.Altitude, azi = sunData.Azimuth;
        double solarTime = sunData.SolarTime, eot = sunData.EOT;
        double dec = sunData.Declination;
        double sunrise = sunData.Sunrise + tz;
        double sunset = sunData.Sunset + tz;
        double dayLen = sunData.DayLength;
        int dayLenH = (int)dayLen;
        int dayLenM = (int)((dayLen - dayLenH) * 60);

        string latStr = $"{Math.Abs(lat):F2}°{(lat >= 0 ? 'N' : 'S')}";
        string lonStr = $"{Math.Abs(lon):F2}°{(lon >= 0 ? 'E' : 'W')}";
        char tzSign = tz >= 0 ? '+' : '-';
        var localTime = dt.AddSeconds(tz * 3600);

        Console.WriteLine("\n☀️ Astronomical Sundial");
        Console.WriteLine($"Location: {latStr}, {lonStr}");
        Console.WriteLine($"Date: {dt:yyyy-MM-dd HH:mm} (UTC{tzSign}{Math.Abs(tz):F1})");
        Console.WriteLine($"Local Time: {localTime:HH:mm}");
        Console.WriteLine($"\nSolar Declination: {dec:+0.0;-0.0}°");
        Console.WriteLine($"Equation of Time: {eot:+0.0;-0.0} min");
        int solarH = (int)solarTime;
        int solarM = (int)((solarTime - solarH) * 60);
        Console.WriteLine($"Solar Time: {solarH:D2}:{solarM:D2}");
        Console.WriteLine($"\nSolar Altitude: {alt:F1}°");
        Console.WriteLine($"Solar Azimuth: {azi:F1}°");
        Console.WriteLine($"\nSunrise: {FormatTime(sunrise)} | Sunset: {FormatTime(sunset)}");
        Console.WriteLine($"Day length: {dayLenH}h {dayLenM}m");
        Console.WriteLine("\n" + DrawSundial(azi));
    }

    static Dictionary<string, string> ParseArgs(string[] args)
    {
        var dict = new Dictionary<string, string>();
        for (int i=0; i<args.Length; i++)
        {
            if (args[i].StartsWith("--"))
            {
                string key = args[i].Substring(2);
                if (i+1 < args.Length && !args[i+1].StartsWith("--"))
                    dict[key] = args[++i];
                else
                    dict[key] = "";
            }
        }
        return dict;
    }
}
