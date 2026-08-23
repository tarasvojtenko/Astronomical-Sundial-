# astronomical_sundial.php
#!/usr/bin/env php
<?php

define('DEG2RAD', M_PI / 180.0);
define('RAD2DEG', 180.0 / M_PI);

function julianDay($dt) {
    $year = (int)$dt->format('Y');
    $month = (int)$dt->format('m');
    $day = (float)$dt->format('d') + (float)$dt->format('H')/24 + (float)$dt->format('i')/1440 + (float)$dt->format('s')/86400;
    if ($month <= 2) { $year--; $month += 12; }
    $A = (int)($year / 100);
    $B = 2 - $A + (int)($A / 4);
    return (int)(365.25 * ($year + 4716)) + (int)(30.6001 * ($month + 1)) + $day + $B - 1524.5;
}

function solarDeclination($dayOfYear) {
    return 23.44 * DEG2RAD * sin((284 + $dayOfYear) * 360 * DEG2RAD / 365);
}

function equationOfTime($dayOfYear) {
    $B = (360.0 / 365) * ($dayOfYear - 81);
    $B_rad = $B * DEG2RAD;
    return 9.87 * sin(2 * $B_rad) - 7.53 * cos($B_rad) - 1.5 * sin($B_rad);
}

function sunPosition($latDeg, $lonDeg, $dt) {
    $latRad = $latDeg * DEG2RAD;
    $dayOfYear = (int)$dt->format('z') + 1;
    $decRad = solarDeclination($dayOfYear);
    $eot = equationOfTime($dayOfYear);
    $hourUTC = (float)$dt->format('G') + (float)$dt->format('i')/60 + (float)$dt->format('s')/3600;
    $solarTime = $hourUTC + (4 * $lonDeg) / 60 + $eot / 60;
    $haRad = ($solarTime - 12) * 15 * DEG2RAD;

    $altRad = asin(sin($latRad) * sin($decRad) + cos($latRad) * cos($decRad) * cos($haRad));
    $altDeg = $altRad * RAD2DEG;

    $aziRad = atan2(-sin($haRad) * cos($decRad),
                    sin($decRad) * cos($latRad) - cos($decRad) * sin($latRad) * cos($haRad));
    $aziDeg = fmod($aziRad * RAD2DEG + 360, 360);

    $cosHASunrise = -tan($latRad) * tan($decRad);
    if ($cosHASunrise < -1) $haSunrise = M_PI;
    elseif ($cosHASunrise > 1) $haSunrise = 0;
    else $haSunrise = acos($cosHASunrise);
    $dayLength = $haSunrise * 2 / (M_PI / 12);
    $noon = 12.0 - $lonDeg/15.0 - $eot/60.0;
    $sunrise = $noon - $dayLength/2;
    $sunset = $noon + $dayLength/2;

    return [
        'altitude' => $altDeg,
        'azimuth' => $aziDeg,
        'solarTime' => $solarTime,
        'eot' => $eot,
        'declination' => $decRad * RAD2DEG,
        'sunrise' => $sunrise,
        'sunset' => $sunset,
        'dayLength' => $dayLength
    ];
}

function drawSundial($azimuthDeg) {
    $dirNames = ['N', 'NE', 'E', 'SE', 'S', 'SW', 'W', 'NW'];
    $idx = (int)round($azimuthDeg / 45) % 8;
    $shadowDir = $dirNames[$idx];
    $lines = [
        "      N",
        "      |",
        "  W---+---E",
        "      |",
        "      S",
        "\nShadow direction: $shadowDir (" . round($azimuthDeg, 1) . "°)"
    ];
    return implode("\n", $lines);
}

function formatTime($hours) {
    $h = (int)($hours) % 24;
    $m = (int)(($hours - $h) * 60);
    return sprintf("%02d:%02d", $h, $m);
}

$opts = getopt("", ["lat:", "lon:", "tz:", "date:", "time:", "dial-only"]);
$lat = isset($opts['lat']) ? (float)$opts['lat'] : 0.0;
$lon = isset($opts['lon']) ? (float)$opts['lon'] : 0.0;
$tz = isset($opts['tz']) ? (float)$opts['tz'] : 0.0;
$dateStr = $opts['date'] ?? null;
$timeStr = $opts['time'] ?? null;
$dialOnly = isset($opts['dial-only']);

$dt = new DateTime('now', new DateTimeZone('UTC'));
if ($dateStr) {
    $dt = new DateTime($dateStr . ' 00:00:00', new DateTimeZone('UTC'));
}
if ($timeStr) {
    list($h, $m) = explode(':', $timeStr);
    $dt->setTime((int)$h, (int)$m, 0);
} else {
    $dt->setTime((int)$dt->format('H'), (int)$dt->format('i'), 0);
}

if ($dialOnly) {
    $data = sunPosition($lat, $lon, $dt);
    echo drawSundial($data['azimuth']) . "\n";
    exit(0);
}

$data = sunPosition($lat, $lon, $dt);
$alt = $data['altitude'];
$azi = $data['azimuth'];
$solarTime = $data['solarTime'];
$eot = $data['eot'];
$dec = $data['declination'];
$sunrise = $data['sunrise'] + $tz;
$sunset = $data['sunset'] + $tz;
$dayLen = $data['dayLength'];
$dayLenH = (int)$dayLen;
$dayLenM = (int)(($dayLen - $dayLenH) * 60);

$latStr = abs($lat) . "°" . ($lat >= 0 ? 'N' : 'S');
$lonStr = abs($lon) . "°" . ($lon >= 0 ? 'E' : 'W');
$tzSign = $tz >= 0 ? '+' : '-';
$localDT = clone $dt;
$localDT->modify(($tz * 3600) . ' seconds');

echo "\n☀️ Astronomical Sundial\n";
echo "Location: $latStr, $lonStr\n";
echo "Date: " . $dt->format('Y-m-d H:i') . " (UTC$tzSign" . abs($tz) . ")\n";
echo "Local Time: " . $localDT->format('H:i') . "\n";
echo "\nSolar Declination: " . round($dec, 1) . "°\n";
echo "Equation of Time: " . round($eot, 1) . " min\n";
$solarH = (int)$solarTime;
$solarM = (int)(($solarTime - $solarH) * 60);
echo "Solar Time: " . sprintf("%02d:%02d", $solarH, $solarM) . "\n";
echo "\nSolar Altitude: " . round($alt, 1) . "°\n";
echo "Solar Azimuth: " . round($azi, 1) . "°\n";
echo "\nSunrise: " . formatTime($sunrise) . " | Sunset: " . formatTime($sunset) . "\n";
echo "Day length: {$dayLenH}h {$dayLenM}m\n";
echo "\n" . drawSundial($azi) . "\n";
?>
