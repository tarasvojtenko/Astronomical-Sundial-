# astronomical_sundial.py
import sys
import math
import argparse
from datetime import datetime, timezone, timedelta

DEG2RAD = math.pi / 180.0
RAD2DEG = 180.0 / math.pi

def julian_day(dt):
    """Julian Day Number for UTC datetime."""
    year = dt.year
    month = dt.month
    day = dt.day + dt.hour/24.0 + dt.minute/1440.0 + dt.second/86400.0
    if month <= 2:
        year -= 1
        month += 12
    A = int(year / 100)
    B = 2 - A + int(A / 4)
    return int(365.25 * (year + 4716)) + int(30.6001 * (month + 1)) + day + B - 1524.5

def solar_declination(day_of_year):
    """Solar declination in radians."""
    return 23.44 * DEG2RAD * math.sin((284 + day_of_year) * 360 * DEG2RAD / 365)

def equation_of_time(day_of_year):
    """Equation of time in minutes."""
    B = (360.0 / 365) * (day_of_year - 81)
    B_rad = B * DEG2RAD
    return 9.87 * math.sin(2 * B_rad) - 7.53 * math.cos(B_rad) - 1.5 * math.sin(B_rad)

def sun_position(lat_deg, lon_deg, dt):
    """Compute solar altitude, azimuth, solar time, etc."""
    lat_rad = lat_deg * DEG2RAD
    day_of_year = dt.timetuple().tm_yday
    dec_rad = solar_declination(day_of_year)
    eot = equation_of_time(day_of_year)

    # Local time (UTC + tz offset)
    # We'll use UTC and then apply timezone later; for now use UTC directly
    hour_utc = dt.hour + dt.minute/60.0 + dt.second/3600.0
    # Solar time (including longitude correction and equation of time)
    solar_time = hour_utc + (4 * lon_deg) / 60.0 + eot / 60.0
    ha_rad = (solar_time - 12) * 15 * DEG2RAD

    # Altitude
    alt_rad = math.asin(math.sin(lat_rad) * math.sin(dec_rad) +
                        math.cos(lat_rad) * math.cos(dec_rad) * math.cos(ha_rad))
    alt_deg = alt_rad * RAD2DEG

    # Azimuth (from north, clockwise)
    azi_rad = math.atan2(-math.sin(ha_rad) * math.cos(dec_rad),
                         math.sin(dec_rad) * math.cos(lat_rad) -
                         math.cos(dec_rad) * math.sin(lat_rad) * math.cos(ha_rad))
    azi_deg = (azi_rad * RAD2DEG) % 360.0

    # Sunrise/sunset (simplified, ignoring refraction)
    cos_ha_sunrise = -math.tan(lat_rad) * math.tan(dec_rad)
    if cos_ha_sunrise < -1:
        sunrise_ha = math.pi
    elif cos_ha_sunrise > 1:
        sunrise_ha = 0
    else:
        sunrise_ha = math.acos(cos_ha_sunrise)
    day_length = sunrise_ha * 2 / (math.pi / 12)  # hours
    # noon = 12 - lon/15 - eot/60 (in hours)
    noon = 12.0 - lon_deg/15.0 - eot/60.0
    sunrise = noon - day_length/2
    sunset = noon + day_length/2

    return {
        'altitude': alt_deg,
        'azimuth': azi_deg,
        'solar_time': solar_time,
        'eot': eot,
        'declination': dec_rad * RAD2DEG,
        'ha_deg': ha_rad * RAD2DEG,
        'sunrise': sunrise,
        'sunset': sunset,
        'day_length': day_length,
    }

def draw_sundial(azimuth_deg):
    """Draw a simple ASCII sundial with shadow direction."""
    dir_names = ['N', 'NE', 'E', 'SE', 'S', 'SW', 'W', 'NW']
    idx = int(round(azimuth_deg / 45)) % 8
    shadow_dir = dir_names[idx]

    # Simple compass rose
    lines = []
    lines.append("      N")
    lines.append("      |")
    lines.append("  W---+---E")
    lines.append("      |")
    lines.append("      S")
    lines.append(f"\nShadow direction: {shadow_dir} ({azimuth_deg:.1f}°)")
    return "\n".join(lines)

def format_time(hours):
    h = int(hours) % 24
    m = int((hours - int(hours)) * 60)
    return f"{h:02d}:{m:02d}"

def main():
    parser = argparse.ArgumentParser(description="Astronomical Sundial")
    parser.add_argument("--lat", type=float, default=0.0, help="Latitude (degrees North)")
    parser.add_argument("--lon", type=float, default=0.0, help="Longitude (degrees East)")
    parser.add_argument("--tz", type=float, default=0.0, help="Timezone offset (hours from UTC)")
    parser.add_argument("--date", help="YYYY-MM-DD")
    parser.add_argument("--time", help="HH:MM")
    parser.add_argument("--dial-only", action="store_true", help="Show only the dial")
    args = parser.parse_args()

    # Get current UTC time
    now = datetime.now(timezone.utc)
    if args.date:
        dt_date = datetime.strptime(args.date, "%Y-%m-%d").date()
    else:
        dt_date = now.date()
    if args.time:
        dt_time = datetime.strptime(args.time, "%H:%M").time()
    else:
        dt_time = now.time().replace(second=0, microsecond=0)
    dt_utc = datetime.combine(dt_date, dt_time, tzinfo=timezone.utc)

    # Local time for display
    local_time = dt_utc + timedelta(hours=args.tz)

    if args.dial_only:
        pos = sun_position(args.lat, args.lon, dt_utc)
        print(draw_sundial(pos['azimuth']))
        return

    pos = sun_position(args.lat, args.lon, dt_utc)
    alt = pos['altitude']
    azi = pos['azimuth']
    solar_time = pos['solar_time']
    eot = pos['eot']
    dec = pos['declination']
    sunrise = pos['sunrise']
    sunset = pos['sunset']
    day_len = pos['day_length']

    # Convert times to local timezone
    sunrise_local = sunrise + args.tz
    sunset_local = sunset + args.tz
    day_len_h = int(day_len)
    day_len_m = int((day_len - day_len_h) * 60)

    lat_str = f"{abs(args.lat):.2f}°{'N' if args.lat>=0 else 'S'}"
    lon_str = f"{abs(args.lon):.2f}°{'E' if args.lon>=0 else 'W'}"

    print("\n☀️ Astronomical Sundial")
    print(f"Location: {lat_str}, {lon_str}")
    print(f"Date: {dt_utc.strftime('%Y-%m-%d %H:%M')} (UTC{'+' if args.tz>=0 else ''}{args.tz:.1f})")
    print(f"Local Time: {local_time.strftime('%H:%M')}")
    print(f"\nSolar Declination: {dec:+.1f}°")
    print(f"Equation of Time: {eot:+.1f} min")
    solar_hours = int(solar_time)
    solar_min = int((solar_time - solar_hours) * 60)
    print(f"Solar Time: {solar_hours:02d}:{solar_min:02d}")
    print(f"\nSolar Altitude: {alt:.1f}°")
    print(f"Solar Azimuth: {azi:.1f}°")
    print(f"\nSunrise: {format_time(sunrise_local)} | Sunset: {format_time(sunset_local)}")
    print(f"Day length: {day_len_h}h {day_len_m}m")
    print("\n" + draw_sundial(azi))

if __name__ == "__main__":
    main()
