☀️ Astronomical Sundial — Multi‑Language Solar Position Calculator
8 languages, one precise solar calculator – compute solar altitude, azimuth, equation of time, sunrise/sunset, and draw an ASCII sundial based on your location and time.

✨ Features
🌞 Solar position – altitude, azimuth, declination, hour angle

🕐 Equation of time – difference between solar and mean time

🌅 Sunrise / Sunset – accurate times for any date and location

📐 Day length – total daylight hours

🎨 ASCII sundial – draw a simple horizontal sundial with shadow direction

📍 Custom location – latitude, longitude, timezone

📅 Any date/time – use current or specified date/time

🧰 Supported Languages & Files
Language	File
Python	astronomical_sundial.py
Go	astronomical_sundial.go
JavaScript (Node)	astronomical_sundial.js
Ruby	astronomical_sundial.rb
PHP	astronomical_sundial.php
Java	AstronomicalSundial.java
C#	AstronomicalSundial.cs
C++	astronomical_sundial.cpp
🚀 Quick Start
All implementations follow the same CLI pattern:

bash
# Show solar data for current time and location (lat=0, lon=0, UTC)
<command>

# Specify location
<command> --lat 48.8584 --lon 2.2945

# Specify timezone offset (hours from UTC)
<command> --lat 40.7128 --lon -74.0060 --tz -4

# Custom date and time
<command> --date 2026-08-23 --time 14:30

# Show only the ASCII sundial
<command> --dial-only
Arguments:

--lat – latitude in degrees (positive North)

--lon – longitude in degrees (positive East)

--tz – timezone offset in hours (default: 0)

--date – YYYY-MM-DD (default: today)

--time – HH:MM (default: current time)

--dial-only – output only the ASCII sundial

📸 Example Output
text
☀️ Astronomical Sundial
Location: 48.86°N, 2.29°E
Date: 2026-08-23 14:30 (UTC+0)
Local Time: 14:30

Solar Declination: +11.8°
Equation of Time: -2.5 min
Solar Time: 14:28

Solar Altitude: 45.2°
Solar Azimuth: 210.7° (SW)

Sunrise: 05:32 | Sunset: 20:15
Day length: 14h 43m

      N
      |
  W---+---E
      |
      S
Shadow direction: SW (210.7°)
📁 Repository Structure
text
.
├── README.md
├── python/
│   └── astronomical_sundial.py
├── go/
│   └── astronomical_sundial.go
├── javascript/
│   └── astronomical_sundial.js
├── ruby/
│   └── astronomical_sundial.rb
├── php/
│   └── astronomical_sundial.php
├── java/
│   └── AstronomicalSundial.java
├── csharp/
│   └── AstronomicalSundial.cs
└── cpp/
    └── astronomical_sundial.cpp
