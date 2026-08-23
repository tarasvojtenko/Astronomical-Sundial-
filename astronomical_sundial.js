// astronomical_sundial.js
#!/usr/bin/env node
const { program } = require('commander');

const DEG2RAD = Math.PI / 180;
const RAD2DEG = 180 / Math.PI;

function julianDay(dt) {
    const year = dt.getFullYear();
    const month = dt.getMonth() + 1;
    const day = dt.getDate() + dt.getHours()/24 + dt.getMinutes()/1440 + dt.getSeconds()/86400;
    let y = year, m = month;
    if (month <= 2) { y--; m += 12; }
    const A = Math.floor(y / 100);
    const B = 2 - A + Math.floor(A / 4);
    return Math.floor(365.25 * (y + 4716)) + Math.floor(30.6001 * (m + 1)) + day + B - 1524.5;
}

function solarDeclination(dayOfYear) {
    return 23.44 * DEG2RAD * Math.sin((284 + dayOfYear) * 360 * DEG2RAD / 365);
}

function equationOfTime(dayOfYear) {
    const B = (360.0 / 365) * (dayOfYear - 81);
    const B_rad = B * DEG2RAD;
    return 9.87 * Math.sin(2 * B_rad) - 7.53 * Math.cos(B_rad) - 1.5 * Math.sin(B_rad);
}

function sunPosition(latDeg, lonDeg, dt) {
    const latRad = latDeg * DEG2RAD;
    const dayOfYear = Math.floor((dt - new Date(dt.getFullYear(), 0, 0)) / (1000*60*60*24));
    const decRad = solarDeclination(dayOfYear);
    const eot = equationOfTime(dayOfYear);
    const hourUTC = dt.getHours() + dt.getMinutes()/60 + dt.getSeconds()/3600;
    const solarTime = hourUTC + (4 * lonDeg) / 60 + eot / 60;
    const haRad = (solarTime - 12) * 15 * DEG2RAD;

    const altRad = Math.asin(Math.sin(latRad)*Math.sin(decRad) + Math.cos(latRad)*Math.cos(decRad)*Math.cos(haRad));
    const altDeg = altRad * RAD2DEG;

    const aziRad = Math.atan2(-Math.sin(haRad)*Math.cos(decRad),
                              Math.sin(decRad)*Math.cos(latRad) - Math.cos(decRad)*Math.sin(latRad)*Math.cos(haRad));
    const aziDeg = ((aziRad * RAD2DEG) % 360 + 360) % 360;

    const cosHASunrise = -Math.tan(latRad) * Math.tan(decRad);
    let haSunrise;
    if (cosHASunrise < -1) haSunrise = Math.PI;
    else if (cosHASunrise > 1) haSunrise = 0;
    else haSunrise = Math.acos(cosHASunrise);
    const dayLength = haSunrise * 2 / (Math.PI / 12);
    const noon = 12.0 - lonDeg/15.0 - eot/60.0;
    const sunrise = noon - dayLength/2;
    const sunset = noon + dayLength/2;

    return { altitude: altDeg, azimuth: aziDeg, solarTime, eot, declination: decRad * RAD2DEG, sunrise, sunset, dayLength };
}

function drawSundial(azimuthDeg) {
    const dirNames = ['N', 'NE', 'E', 'SE', 'S', 'SW', 'W', 'NW'];
    const idx = Math.round(azimuthDeg / 45) % 8;
    const shadowDir = dirNames[idx];
    const lines = [
        "      N",
        "      |",
        "  W---+---E",
        "      |",
        "      S",
        `\nShadow direction: ${shadowDir} (${azimuthDeg.toFixed(1)}°)`
    ];
    return lines.join('\n');
}

function formatTime(hours) {
    const h = Math.floor(hours) % 24;
    const m = Math.floor((hours - h) * 60);
    return `${String(h).padStart(2,'0')}:${String(m).padStart(2,'0')}`;
}

program
    .option('--lat <lat>', 'Latitude (positive North)', parseFloat, 0)
    .option('--lon <lon>', 'Longitude (positive East)', parseFloat, 0)
    .option('--tz <tz>', 'Timezone offset (hours from UTC)', parseFloat, 0)
    .option('--date <date>', 'YYYY-MM-DD')
    .option('--time <time>', 'HH:MM')
    .option('--dial-only', 'Show only the dial')
    .parse(process.argv);

const opts = program.opts();

let dt = new Date();
if (opts.date) {
    const parts = opts.date.split('-').map(Number);
    dt.setFullYear(parts[0], parts[1]-1, parts[2]);
}
if (opts.time) {
    const [h, m] = opts.time.split(':').map(Number);
    dt.setHours(h, m, 0, 0);
}
// Keep in UTC
dt = new Date(dt.toISOString());

if (opts.dialOnly) {
    const data = sunPosition(opts.lat, opts.lon, dt);
    console.log(drawSundial(data.azimuth));
    process.exit(0);
}

const data = sunPosition(opts.lat, opts.lon, dt);
const alt = data.altitude, azi = data.azimuth;
const solarTime = data.solarTime, eot = data.eot;
const dec = data.declination;
const sunrise = data.sunrise + opts.tz;
const sunset = data.sunset + opts.tz;
const dayLen = data.dayLength;
const dayLenH = Math.floor(dayLen);
const dayLenM = Math.round((dayLen - dayLenH) * 60);

const latStr = `${Math.abs(opts.lat).toFixed(2)}°${opts.lat >= 0 ? 'N' : 'S'}`;
const lonStr = `${Math.abs(opts.lon).toFixed(2)}°${opts.lon >= 0 ? 'E' : 'W'}`;
const tzSign = opts.tz >= 0 ? '+' : '-';
const localTime = new Date(dt.getTime() + opts.tz * 3600000);

console.log(`\n☀️ Astronomical Sundial`);
console.log(`Location: ${latStr}, ${lonStr}`);
console.log(`Date: ${dt.toISOString().slice(0,16).replace('T',' ')} (UTC${tzSign}${Math.abs(opts.tz).toFixed(1)})`);
console.log(`Local Time: ${localTime.toISOString().slice(11,16)}`);
console.log(`\nSolar Declination: ${dec.toFixed(1)}°`);
console.log(`Equation of Time: ${eot.toFixed(1)} min`);
const solarH = Math.floor(solarTime);
const solarM = Math.round((solarTime - solarH) * 60);
console.log(`Solar Time: ${String(solarH).padStart(2,'0')}:${String(solarM).padStart(2,'0')}`);
console.log(`\nSolar Altitude: ${alt.toFixed(1)}°`);
console.log(`Solar Azimuth: ${azi.toFixed(1)}°`);
console.log(`\nSunrise: ${formatTime(sunrise)} | Sunset: ${formatTime(sunset)}`);
console.log(`Day length: ${dayLenH}h ${dayLenM}m`);
console.log(`\n${drawSundial(azi)}`);
