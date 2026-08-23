// astronomical_sundial.go
package main

import (
	"flag"
	"fmt"
	"math"
	"time"
)

const (
	DEG2RAD = math.Pi / 180.0
	RAD2DEG = 180.0 / math.Pi
)

type SolarData struct {
	Altitude    float64
	Azimuth     float64
	SolarTime   float64
	EOT         float64
	Declination float64
	HA          float64
	Sunrise     float64
	Sunset      float64
	DayLength   float64
}

func julianDay(t time.Time) float64 {
	year := t.Year()
	month := int(t.Month())
	day := float64(t.Day()) + float64(t.Hour())/24.0 + float64(t.Minute())/1440.0 + float64(t.Second())/86400.0
	if month <= 2 {
		year--
		month += 12
	}
	A := year / 100
	B := 2 - A + A/4
	return float64(int(365.25*float64(year+4716))) + float64(int(30.6001*float64(month+1))) + day + float64(B) - 1524.5
}

func solarDeclination(dayOfYear int) float64 {
	return 23.44 * DEG2RAD * math.Sin((284+dayOfYear)*360*DEG2RAD/365)
}

func equationOfTime(dayOfYear int) float64 {
	B := (360.0 / 365) * float64(dayOfYear-81)
	B_rad := B * DEG2RAD
	return 9.87*math.Sin(2*B_rad) - 7.53*math.Cos(B_rad) - 1.5*math.Sin(B_rad)
}

func sunPosition(latDeg, lonDeg float64, t time.Time) SolarData {
	latRad := latDeg * DEG2RAD
	dayOfYear := t.YearDay()
	decRad := solarDeclination(dayOfYear)
	eot := equationOfTime(dayOfYear)
	hourUTC := float64(t.Hour()) + float64(t.Minute())/60.0 + float64(t.Second())/3600.0
	solarTime := hourUTC + (4*lonDeg)/60.0 + eot/60.0
	haRad := (solarTime - 12) * 15 * DEG2RAD

	altRad := math.Asin(math.Sin(latRad)*math.Sin(decRad) + math.Cos(latRad)*math.Cos(decRad)*math.Cos(haRad))
	altDeg := altRad * RAD2DEG

	aziRad := math.Atan2(-math.Sin(haRad)*math.Cos(decRad),
		math.Sin(decRad)*math.Cos(latRad)-math.Cos(decRad)*math.Sin(latRad)*math.Cos(haRad))
	aziDeg := math.Mod(aziRad*RAD2DEG+360, 360)

	// Sunrise/sunset
	cosHASunrise := -math.Tan(latRad) * math.Tan(decRad)
	var haSunrise float64
	if cosHASunrise < -1 {
		haSunrise = math.Pi
	} else if cosHASunrise > 1 {
		haSunrise = 0
	} else {
		haSunrise = math.Acos(cosHASunrise)
	}
	dayLength := haSunrise * 2 / (math.Pi / 12)
	noon := 12.0 - lonDeg/15.0 - eot/60.0
	sunrise := noon - dayLength/2
	sunset := noon + dayLength/2

	return SolarData{
		Altitude:    altDeg,
		Azimuth:     aziDeg,
		SolarTime:   solarTime,
		EOT:         eot,
		Declination: decRad * RAD2DEG,
		HA:          haRad * RAD2DEG,
		Sunrise:     sunrise,
		Sunset:      sunset,
		DayLength:   dayLength,
	}
}

func drawSundial(azimuthDeg float64) string {
	dirNames := []string{"N", "NE", "E", "SE", "S", "SW", "W", "NW"}
	idx := int(math.Round(azimuthDeg/45)) % 8
	shadowDir := dirNames[idx]
	lines := []string{
		"      N",
		"      |",
		"  W---+---E",
		"      |",
		"      S",
		fmt.Sprintf("\nShadow direction: %s (%.1f°)", shadowDir, azimuthDeg),
	}
	return strings.Join(lines, "\n")
}

func formatTime(hours float64) string {
	h := int(hours) % 24
	m := int((hours - float64(h)) * 60)
	return fmt.Sprintf("%02d:%02d", h, m)
}

func main() {
	var (
		lat      = flag.Float64("lat", 0.0, "Latitude (degrees North)")
		lon      = flag.Float64("lon", 0.0, "Longitude (degrees East)")
		tz       = flag.Float64("tz", 0.0, "Timezone offset (hours from UTC)")
		dateStr  = flag.String("date", "", "YYYY-MM-DD")
		timeStr  = flag.String("time", "", "HH:MM")
		dialOnly = flag.Bool("dial-only", false, "Show only the dial")
	)
	flag.Parse()

	now := time.Now().UTC()
	t := now
	if *dateStr != "" {
		d, _ := time.Parse("2006-01-02", *dateStr)
		t = time.Date(d.Year(), d.Month(), d.Day(), t.Hour(), t.Minute(), 0, 0, time.UTC)
	}
	if *timeStr != "" {
		parsed, _ := time.Parse("15:04", *timeStr)
		t = time.Date(t.Year(), t.Month(), t.Day(), parsed.Hour(), parsed.Minute(), 0, 0, time.UTC)
	}

	if *dialOnly {
		data := sunPosition(*lat, *lon, t)
		fmt.Print(drawSundial(data.Azimuth))
		return
	}

	data := sunPosition(*lat, *lon, t)
	alt := data.Altitude
	azi := data.Azimuth
	solarTime := data.SolarTime
	eot := data.EOT
	dec := data.Declination
	sunrise := data.Sunrise + *tz
	sunset := data.Sunset + *tz
	dayLen := data.DayLength
	dayLenH := int(dayLen)
	dayLenM := int((dayLen - float64(dayLenH)) * 60)

	latStr := fmt.Sprintf("%.2f°%c", math.Abs(*lat), 'N')
	if *lat < 0 {
		latStr = fmt.Sprintf("%.2f°%c", math.Abs(*lat), 'S')
	}
	lonStr := fmt.Sprintf("%.2f°%c", math.Abs(*lon), 'E')
	if *lon < 0 {
		lonStr = fmt.Sprintf("%.2f°%c", math.Abs(*lon), 'W')
	}
	tzSign := '+'
	if *tz < 0 {
		tzSign = '-'
	}
	localTime := t.Add(time.Duration(*tz * float64(time.Hour)))

	fmt.Printf("\n☀️ Astronomical Sundial\n")
	fmt.Printf("Location: %s, %s\n", latStr, lonStr)
	fmt.Printf("Date: %s (UTC%c%.1f)\n", t.Format("2006-01-02 15:04"), tzSign, math.Abs(*tz))
	fmt.Printf("Local Time: %s\n", localTime.Format("15:04"))
	fmt.Printf("\nSolar Declination: %+.1f°\n", dec)
	fmt.Printf("Equation of Time: %+.1f min\n", eot)
	solarH := int(solarTime)
	solarM := int((solarTime - float64(solarH)) * 60)
	fmt.Printf("Solar Time: %02d:%02d\n", solarH, solarM)
	fmt.Printf("\nSolar Altitude: %.1f°\n", alt)
	fmt.Printf("Solar Azimuth: %.1f°\n", azi)
	fmt.Printf("\nSunrise: %s | Sunset: %s\n", formatTime(sunrise), formatTime(sunset))
	fmt.Printf("Day length: %dh %dm\n", dayLenH, dayLenM)
	fmt.Println("\n" + drawSundial(azi))
}
