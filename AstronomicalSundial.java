// AstronomicalSundial.java
import java.io.*;
import java.time.*;
import java.time.format.*;
import java.util.*;
import java.time.temporal.ChronoField;

public class AstronomicalSundial {
    private static final double DEG2RAD = Math.PI / 180.0;
    private static final double RAD2DEG = 180.0 / Math.PI;

    public static class SolarData {
        public double altitude, azimuth, solarTime, eot, declination, sunrise, sunset, dayLength;
    }

    public static double julianDay(LocalDateTime dt) {
        int year = dt.getYear();
        int month = dt.getMonthValue();
        double day = dt.getDayOfMonth() + dt.getHour()/24.0 + dt.getMinute()/1440.0 + dt.getSecond()/86400.0;
        if (month <= 2) { year--; month += 12; }
        int A = year / 100;
        int B = 2 - A + A / 4;
        return (int)(365.25 * (year + 4716)) + (int)(30.6001 * (month + 1)) + day + B - 1524.5;
    }

    public static double solarDeclination(int dayOfYear) {
        return 23.44 * DEG2RAD * Math.sin((284 + dayOfYear) * 360 * DEG2RAD / 365);
    }

    public static double equationOfTime(int dayOfYear) {
        double B = (360.0 / 365) * (dayOfYear - 81);
        double B_rad = B * DEG2RAD;
        return 9.87 * Math.sin(2 * B_rad) - 7.53 * Math.cos(B_rad) - 1.5 * Math.sin(B_rad);
    }

    public static SolarData sunPosition(double latDeg, double lonDeg, LocalDateTime dt) {
        double latRad = latDeg * DEG2RAD;
        int dayOfYear = dt.getDayOfYear();
        double decRad = solarDeclination(dayOfYear);
        double eot = equationOfTime(dayOfYear);
        double hourUTC = dt.getHour() + dt.getMinute()/60.0 + dt.getSecond()/3600.0;
        double solarTime = hourUTC + (4 * lonDeg) / 60.0 + eot / 60.0;
        double haRad = (solarTime - 12) * 15 * DEG2RAD;

        double altRad = Math.asin(Math.sin(latRad)*Math.sin(decRad) + Math.cos(latRad)*Math.cos(decRad)*Math.cos(haRad));
        double altDeg = altRad * RAD2DEG;

        double aziRad = Math.atan2(-Math.sin(haRad)*Math.cos(decRad),
                                   Math.sin(decRad)*Math.cos(latRad) - Math.cos(decRad)*Math.sin(latRad)*Math.cos(haRad));
        double aziDeg = (aziRad * RAD2DEG + 360) % 360;

        double cosHASunrise = -Math.tan(latRad) * Math.tan(decRad);
        double haSunrise;
        if (cosHASunrise < -1) haSunrise = Math.PI;
        else if (cosHASunrise > 1) haSunrise = 0;
        else haSunrise = Math.acos(cosHASunrise);
        double dayLength = haSunrise * 2 / (Math.PI / 12);
        double noon = 12.0 - lonDeg/15.0 - eot/60.0;
        double sunrise = noon - dayLength/2;
        double sunset = noon + dayLength/2;

        SolarData data = new SolarData();
        data.altitude = altDeg;
        data.azimuth = aziDeg;
        data.solarTime = solarTime;
        data.eot = eot;
        data.declination = decRad * RAD2DEG;
        data.sunrise = sunrise;
        data.sunset = sunset;
        data.dayLength = dayLength;
        return data;
    }

    public static String drawSundial(double azimuthDeg) {
        String[] dirNames = {"N", "NE", "E", "SE", "S", "SW", "W", "NW"};
        int idx = (int)Math.round(azimuthDeg / 45) % 8;
        String shadowDir = dirNames[idx];
        return "      N\n" +
               "      |\n" +
               "  W---+---E\n" +
               "      |\n" +
               "      S\n" +
               String.format("\nShadow direction: %s (%.1f°)", shadowDir, azimuthDeg);
    }

    public static String formatTime(double hours) {
        int h = (int)hours % 24;
        int m = (int)((hours - h) * 60);
        return String.format("%02d:%02d", h, m);
    }

    public static void main(String[] args) {
        Map<String, String> params = new HashMap<>();
        for (int i=0; i<args.length; i++) {
            if (args[i].startsWith("--")) {
                String key = args[i].substring(2);
                if (i+1 < args.length && !args[i+1].startsWith("--")) {
                    params.put(key, args[++i]);
                } else {
                    params.put(key, "");
                }
            }
        }
        double lat = Double.parseDouble(params.getOrDefault("lat", "0.0"));
        double lon = Double.parseDouble(params.getOrDefault("lon", "0.0"));
        double tz = Double.parseDouble(params.getOrDefault("tz", "0.0"));
        boolean dialOnly = params.containsKey("dial-only");

        LocalDateTime now = LocalDateTime.now(ZoneOffset.UTC);
        LocalDateTime dt = now;
        if (params.containsKey("date")) {
            LocalDate date = LocalDate.parse(params.get("date"));
            dt = LocalDateTime.of(date, dt.toLocalTime());
        }
        if (params.containsKey("time")) {
            LocalTime time = LocalTime.parse(params.get("time") + ":00");
            dt = LocalDateTime.of(dt.toLocalDate(), time);
        }
        // Force UTC
        dt = dt.atZone(ZoneOffset.UTC).toLocalDateTime();

        if (dialOnly) {
            SolarData data = sunPosition(lat, lon, dt);
            System.out.print(drawSundial(data.azimuth));
            return;
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

        String latStr = String.format("%.2f°%c", Math.abs(lat), lat >= 0 ? 'N' : 'S');
        String lonStr = String.format("%.2f°%c", Math.abs(lon), lon >= 0 ? 'E' : 'W');
        char tzSign = tz >= 0 ? '+' : '-';
        LocalDateTime localTime = dt.plusSeconds((long)(tz * 3600));

        System.out.printf("\n☀️ Astronomical Sundial\n");
        System.out.printf("Location: %s, %s\n", latStr, lonStr);
        System.out.printf("Date: %s (UTC%c%.1f)\n", dt.format(DateTimeFormatter.ofPattern("yyyy-MM-dd HH:mm")), tzSign, Math.abs(tz));
        System.out.printf("Local Time: %s\n", localTime.format(DateTimeFormatter.ofPattern("HH:mm")));
        System.out.printf("\nSolar Declination: %+.1f°\n", dec);
        System.out.printf("Equation of Time: %+.1f min\n", eot);
        int solarH = (int)solarTime;
        int solarM = (int)((solarTime - solarH) * 60);
        System.out.printf("Solar Time: %02d:%02d\n", solarH, solarM);
        System.out.printf("\nSolar Altitude: %.1f°\n", alt);
        System.out.printf("Solar Azimuth: %.1f°\n", azi);
        System.out.printf("\nSunrise: %s | Sunset: %s\n", formatTime(sunrise), formatTime(sunset));
        System.out.printf("Day length: %dh %dm\n", dayLenH, dayLenM);
        System.out.println("\n" + drawSundial(azi));
    }
}
