const fs = require('fs');
const path = require('path');

const pkgPath = path.join(__dirname, '..', 'package.json');
const cPath = path.join(__dirname, '..', 'src', 'SteamLite.c');

const pkg = JSON.parse(fs.readFileSync(pkgPath, 'utf8'));
const version = pkg.version;

let c = fs.readFileSync(cPath, 'utf8');
c = c.replace(/#define STEAMLITE_VERSION L"v[^"]+"/, `#define STEAMLITE_VERSION L"v${version}"`);
fs.writeFileSync(cPath, c);

console.log(`SteamLite.c updated to v${version}`);
