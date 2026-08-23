# astronomical_sundial.rb
#!/usr/bin/env ruby
require 'optparse'
require 'date'

DEG2RAD = Math::PI / 180.0
RAD2DEG = 180.0 / Math::PI

def julian_day(dt)
  year = dt.year
  month = dt.month
  day = dt.day + dt.hour/24.0 + dt.min/1440.0 + dt.sec/86400.0
  if month <= 2
    year -= 1
    month += 12
  end
  a = (year / 100).to_i
  b = 2 - a + (a / 4).to_i
  (365.25 * (year + 4716)).to_i + (30.6001 * (month + 1)).to_i + day + b - 1524.5
end

def solar_declination(day_of_year)
  23.44 * DEG2RAD * Math.sin((284 + day_of_year) * 360 * DEG2RAD / 365)
end

def equation_of_time(day_of_year)
  b = (360.0 / 365) * (day_of_year - 81)
  b_rad = b * DEG2RAD
  9.87 * Math.sin(2 * b_rad) - 7.53 * Math.cos(b_rad) - 1.5 * Math.sin(b_rad)
end

def sun_position(lat_deg, lon_deg, dt)
  lat_rad = lat_deg * DEG2RAD
  day_of_year = dt.yday
  dec_rad = solar_declination(day_of_year)
  eot = equation_of_time(day_of_year)
  hour_utc = dt.hour + dt.min/60.0 + dt.sec/3600.0
  solar_time = hour_utc + (4 * lon_deg) / 60.0 + eot / 60.0
  ha_rad = (solar_time - 12) * 15 * DEG2RAD

  alt_rad = Math.asin(Math.sin(lat_rad) * Math.sin(dec_rad) + Math.cos(lat_rad) * Math.cos(dec_rad) * Math.cos(ha_rad))
  alt_deg = alt_rad * RAD2DEG

  azi_rad = Math.atan2(-Math.sin(ha_rad) * Math.cos(dec_rad),
                       Math.sin(dec_rad) * Math.cos(lat_rad) -
                       Math.cos(dec_rad) * Math.sin(lat_rad) * Math.cos(ha_rad))
  azi_deg = (azi_rad * RAD2DEG) % 360.0

  cos_ha_sunrise = -Math.tan(lat_rad) * Math.tan(dec_rad)
  ha_sunrise = if cos_ha_sunrise < -1
                 Math::PI
               elsif cos_ha_sunrise > 1
                 0
               else
                 Math.acos(cos_ha_sunrise)
               end
  day_length = ha_sunrise * 2 / (Math::PI / 12)
  noon = 12.0 - lon_deg/15.0 - eot/60.0
  sunrise = noon - day_length/2
  sunset = noon + day_length/2

  {
    altitude: alt_deg,
    azimuth: azi_deg,
    solar_time: solar_time,
    eot: eot,
    declination: dec_rad * RAD2DEG,
    sunrise: sunrise,
    sunset: sunset,
    day_length: day_length
  }
end

def draw_sundial(azimuth_deg)
  dir_names = ['N', 'NE', 'E', 'SE', 'S', 'SW', 'W', 'NW']
  idx = (azimuth_deg / 45).round % 8
  shadow_dir = dir_names[idx]
  lines = [
    "      N",
    "      |",
    "  W---+---E",
    "      |",
    "      S",
    "\nShadow direction: #{shadow_dir} (#{azimuth_deg.round(1)}°)"
  ]
  lines.join("\n")
end

def format_time(hours)
  h = hours.to_i % 24
  m = ((hours - h) * 60).to_i
  sprintf("%02d:%02d", h, m)
end

options = {}
OptionParser.new do |opts|
  opts.banner = "Usage: astronomical_sundial.rb [options]"
  opts.on("--lat LAT", Float, "Latitude (positive North)") { |v| options[:lat] = v }
  opts.on("--lon LON", Float, "Longitude (positive East)") { |v| options[:lon] = v }
  opts.on("--tz TZ", Float, "Timezone offset (hours from UTC)") { |v| options[:tz] = v }
  opts.on("--date DATE", "YYYY-MM-DD") { |v| options[:date] = v }
  opts.on("--time TIME", "HH:MM") { |v| options[:time] = v }
  opts.on("--dial-only", "Show only the dial") { options[:dial_only] = true }
end.parse!

now = DateTime.now
dt = now
if options[:date]
  dt = DateTime.parse(options[:date])
end
if options[:time]
  h, m = options[:time].split(':').map(&:to_i)
  dt = DateTime.new(dt.year, dt.month, dt.day, h, m, 0, dt.zone)
else
  dt = DateTime.new(dt.year, dt.month, dt.day, now.hour, now.min, 0, dt.zone)
end
# Convert to UTC
dt = dt.new_offset(0)

lat = options[:lat] || 0.0
lon = options[:lon] || 0.0
tz = options[:tz] || 0.0

if options[:dial_only]
  data = sun_position(lat, lon, dt)
  puts draw_sundial(data[:azimuth])
  exit
end

data = sun_position(lat, lon, dt)
alt = data[:altitude]
azi = data[:azimuth]
solar_time = data[:solar_time]
eot = data[:eot]
dec = data[:declination]
sunrise = data[:sunrise] + tz
sunset = data[:sunset] + tz
day_len = data[:day_length]
day_len_h = day_len.to_i
day_len_m = ((day_len - day_len_h) * 60).to_i

lat_str = "#{lat.abs.round(2)}°#{lat >= 0 ? 'N' : 'S'}"
lon_str = "#{lon.abs.round(2)}°#{lon >= 0 ? 'E' : 'W'}"
tz_sign = tz >= 0 ? '+' : '-'
local_time = dt + tz/24.0

puts "\n☀️ Astronomical Sundial"
puts "Location: #{lat_str}, #{lon_str}"
puts "Date: #{dt.strftime('%Y-%m-%d %H:%M')} (UTC#{tz_sign}#{tz.abs.round(1)})"
puts "Local Time: #{local_time.strftime('%H:%M')}"
puts "\nSolar Declination: #{dec.round(1)}°"
puts "Equation of Time: #{eot.round(1)} min"
solar_h = solar_time.to_i
solar_m = ((solar_time - solar_h) * 60).to_i
puts "Solar Time: #{sprintf('%02d:%02d', solar_h, solar_m)}"
puts "\nSolar Altitude: #{alt.round(1)}°"
puts "Solar Azimuth: #{azi.round(1)}°"
puts "\nSunrise: #{format_time(sunrise)} | Sunset: #{format_time(sunset)}"
puts "Day length: #{day_len_h}h #{day_len_m}m"
puts "\n" + draw_sundial(azi)
