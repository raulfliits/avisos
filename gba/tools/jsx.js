// Extrae constantes de datos del juego web y las imprime como JSON.
// node jsx.js <html> NOMBRE1 NOMBRE2 ...   (evalúa en orden; las de antes quedan disponibles)
const fs = require('fs'), vm = require('vm');
const [, , html, ...names] = process.argv;
const src = fs.readFileSync(html, 'utf8');
const ctx = vm.createContext({ Math, Object, Array, JSON, String, Number });
function grab(name) {
  const re = new RegExp('(?:^|[\\s;])const ' + name + '\\s*=', 'm'); const m = re.exec(src);
  if (!m) throw new Error('no encuentro ' + name);
  let i = m.index + m[0].length, depth = 0, q = null;
  for (; i < src.length; i++) {
    const c = src[i];
    if (q) { if (c === '\\') { i++; continue; } if (c === q) q = null; continue; }
    if (c === '"' || c === "'" || c === '`') { q = c; continue; }
    if ('([{'.includes(c)) depth++; else if (')]}'.includes(c)) depth--;
    else if ((c === ';' || c === '\n') && depth === 0) break;
  }
  return src.slice(m.index + m[0].length, i);
}
const out = {};
for (const n of names) { const e = grab(n); vm.runInContext('var ' + n + '=(' + e + ');', ctx); out[n] = vm.runInContext(n, ctx); }
console.log(JSON.stringify(out));
