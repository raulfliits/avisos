/**
 * Buscador Ops FLiiTS
 * Webapp de Google Apps Script para que Ops Specialists y Ops Managers
 * encuentren el archivo o herramienta que necesitan escribiendo una pregunta.
 *
 * Fuente de datos:
 *  - Cada carpeta de rol (Ops Specialist / Ops Manager) tiene un Google Sheet
 *    cuyo nombre empieza por "Índice" con una fila por archivo/herramienta.
 *  - Cualquier archivo o acceso directo que se añada a esas carpetas (o a sus
 *    subcarpetas) se añade solo al índice al sincronizar, para que alguien le
 *    escriba la descripción.
 */

const CONFIG = {
  roles: [
    { rol: 'Ops Specialist', folderId: '1BczoFp9d92sofa1moZEDKUvZf8VOXRzm' },
    { rol: 'Ops Manager',    folderId: '1VtM3-SP6E_Vh2lCYyA4q_UYPAlV8OxZN' },
  ],
  indexPrefix: 'Índice',
  // Hoja con quién puede entrar y con qué rol (columnas: Email | Rol | Nombre).
  // Está en la carpeta raíz Q&A FLiiTS OPS. Quien no aparezca no puede usar el buscador.
  accesos: { folderId: '1FrYt1cad6b7wp7MzBNhcpjdCkKcyhYsS', nombre: 'Accesos Buscador Ops' },
  // Hoja donde se guarda el "¿Te ha resultado útil?". Misma carpeta raíz.
  feedback: {
    nombre: 'Feedback Buscador Ops',
    headers: ['Fecha', 'Email', 'Rol', 'Búsqueda', '¿Útil?', 'Comentario', 'Resultados mostrados',
              'Archivo que debía salir', 'Estado', 'Notas'],
  },
  headers: ['Nombre', 'Enlace', 'Tipo', 'Categoría', 'Descripción', 'Palabras clave', 'Origen', 'Añadido'],
  cacheKey: 'catalogo_v1',
  cacheSeconds: 600,
};

/* ---------- Webapp ---------- */

function doGet() {
  return HtmlService.createTemplateFromFile('Index')
    .evaluate()
    .setTitle('Buscador Ops FLiiTS')
    .addMetaTag('viewport', 'width=device-width, initial-scale=1')
    .setXFrameOptionsMode(HtmlService.XFrameOptionsMode.ALLOWALL);
}

/** Lo llama la página. Devuelve solo lo que el usuario puede ver según su rol. */
function getCatalogo() {
  asegurarActivador_();
  const usuario = usuarioActual_();
  if (!usuario.rol) return { sinAcceso: true, email: usuario.email };
  return filtrarPorRol_(catalogoCompleto_(), usuario);
}

/** Lo llama la página al pulsar Sí / No en "¿Te ha resultado útil?". */
function enviarFeedback(fb) {
  const usuario = usuarioActual_();
  if (!usuario.rol) throw new Error('Sin acceso');
  fb = fb || {};
  const corta = (v, n) => String(v || '').replace(/[\r\n]+/g, ' ').slice(0, n);
  const lock = LockService.getScriptLock();
  lock.waitLock(10000);
  try {
    const sh = obtenerHojaFeedback_();
    sh.appendRow([
      new Date(), usuario.email, usuario.rol, corta(fb.busqueda, 300), fb.util ? 'Sí' : 'No',
      corta(fb.comentario, 1000), corta((fb.resultados || []).join(' | '), 1000), '', fb.util ? 'OK' : 'Pendiente', '',
    ]);
  } finally {
    lock.releaseLock();
  }
  return true;
}

/** Fuerza recarga desde la página (botón "Actualizar"). */
function refrescarCatalogo() {
  const cache = CacheService.getScriptCache();
  cache.remove(CONFIG.cacheKey);
  cache.remove('accesos_v1');
  return getCatalogo();
}

function catalogoCompleto_() {
  const cache = CacheService.getScriptCache();
  const hit = cache.get(CONFIG.cacheKey);
  if (hit) return JSON.parse(hit);
  sincronizar_();
  aplicarFeedback_();
  const payload = { items: leerCatalogo_(), actualizado: new Date().toISOString() };
  try { cache.put(CONFIG.cacheKey, JSON.stringify(payload), CONFIG.cacheSeconds); } catch (e) { /* >100KB: sin caché */ }
  return payload;
}

/* ---------- Accesos por rol ---------- */

/**
 * Un Ops Manager ve todo. Un Ops Specialist ve completo lo que está en el índice de
 * Ops Specialist; de lo que es solo de manager recibe únicamente la categoría y las
 * palabras clave (para poder decirle "consulta con tu manager"), nunca el nombre,
 * el enlace ni la descripción.
 */
function filtrarPorRol_(payload, usuario) {
  const esManager = usuario.rol === 'Ops Manager';
  const items = payload.items.map(i => {
    if (esManager || i.roles.indexOf('Ops Specialist') !== -1) return i;
    return { restringido: true, categoria: i.categoria, keywords: i.keywords, roles: i.roles };
  });
  return { items: items, actualizado: payload.actualizado, usuario: usuario };
}

function usuarioActual_() {
  const email = String(Session.getActiveUser().getEmail() || '').trim().toLowerCase();
  const tabla = leerAccesos_();
  const fila = tabla[email];
  return { email: email, rol: fila ? fila.rol : '', nombre: fila ? fila.nombre : '' };
}

function leerAccesos_() {
  const cache = CacheService.getScriptCache();
  const hit = cache.get('accesos_v1');
  if (hit) return JSON.parse(hit);
  const sheet = obtenerHojaAccesos_();
  const rows = sheet.getDataRange().getValues();
  const tabla = {};
  for (let i = 1; i < rows.length; i++) {
    const email = String(rows[i][0] || '').trim().toLowerCase();
    const rol = normalizarRol_(rows[i][1]);
    if (email && rol) tabla[email] = { rol: rol, nombre: String(rows[i][2] || '') };
  }
  cache.put('accesos_v1', JSON.stringify(tabla), 300);
  return tabla;
}

function normalizarRol_(v) {
  const t = String(v || '').toLowerCase();
  if (t.indexOf('manager') !== -1) return 'Ops Manager';
  if (t.indexOf('specialist') !== -1) return 'Ops Specialist';
  return '';
}

function obtenerHojaAccesos_() {
  const folder = DriveApp.getFolderById(CONFIG.accesos.folderId);
  const it = folder.getFilesByName(CONFIG.accesos.nombre);
  if (it.hasNext()) return SpreadsheetApp.openById(it.next().getId()).getSheets()[0];
  const ss = SpreadsheetApp.create(CONFIG.accesos.nombre);
  DriveApp.getFileById(ss.getId()).moveTo(folder);
  const sh = ss.getSheets()[0];
  sh.getRange(1, 1, 2, 3).setValues([['Email', 'Rol', 'Nombre'], [Session.getEffectiveUser().getEmail(), 'Ops Manager', '']]);
  sh.getRange(1, 1, 1, 3).setFontWeight('bold');
  sh.setFrozenRows(1);
  return sh;
}

/* ---------- Sincronización carpetas -> índices ---------- */

/** Ejecutar a mano o con el activador horario (ver instalarActivador). */
function sincronizar() {
  sincronizar_();
  aplicarFeedback_();
  CacheService.getScriptCache().remove(CONFIG.cacheKey);
}

/** Crea un activador que sincroniza cada hora. Ejecutar UNA vez. */
function instalarActivador() {
  ScriptApp.getProjectTriggers()
    .filter(t => t.getHandlerFunction() === 'sincronizar')
    .forEach(t => ScriptApp.deleteTrigger(t));
  ScriptApp.newTrigger('sincronizar').timeBased().everyHours(1).create();
}

/** La webapp se ejecuta con la cuenta del dueño: la primera visita deja instalado el activador horario. */
function asegurarActivador_() {
  const props = PropertiesService.getScriptProperties();
  if (props.getProperty('activador') === 'ok') return;
  try {
    const existe = ScriptApp.getProjectTriggers().some(t => t.getHandlerFunction() === 'sincronizar');
    if (!existe) ScriptApp.newTrigger('sincronizar').timeBased().everyHours(1).create();
    props.setProperty('activador', 'ok');
  } catch (e) { /* se reintenta en la siguiente visita */ }
}

function sincronizar_() {
  CONFIG.roles.forEach(cfg => {
    const folder = DriveApp.getFolderById(cfg.folderId);
    const sheet = obtenerHojaIndice_(folder, cfg.rol);
    const data = sheet.getDataRange().getValues();
    const conocidos = new Set();
    for (let i = 1; i < data.length; i++) {
      const id = extraerId_(String(data[i][1] || ''));
      if (id) conocidos.add(id);
      if (data[i][1]) conocidos.add(String(data[i][1]).trim());
    }
    const nuevos = [];
    recorrerCarpeta_(folder, '', (file, ruta) => {
      if (file.getName().indexOf(CONFIG.indexPrefix) === 0 && file.getMimeType() === MimeType.GOOGLE_SHEETS) return;
      const destino = resolverAcceso_(file);
      if (!destino || conocidos.has(destino.id)) return;
      conocidos.add(destino.id);
      nuevos.push([
        destino.name,
        destino.url,
        tipoDesdeMime_(destino.mime),
        ruta || 'Sin clasificar',
        destino.description || '',
        '',
        'Carpeta (auto)',
        new Date(),
      ]);
    });
    if (nuevos.length) {
      sheet.getRange(sheet.getLastRow() + 1, 1, nuevos.length, CONFIG.headers.length).setValues(nuevos);
    }
  });
}

function recorrerCarpeta_(folder, ruta, fn) {
  const files = folder.getFiles();
  while (files.hasNext()) fn(files.next(), ruta);
  const subs = folder.getFolders();
  while (subs.hasNext()) {
    const sub = subs.next();
    recorrerCarpeta_(sub, ruta ? ruta + ' / ' + sub.getName() : sub.getName(), fn);
  }
}

/** Si es un acceso directo, devuelve el archivo al que apunta. */
function resolverAcceso_(file) {
  try {
    if (file.getMimeType() === MimeType.SHORTCUT) {
      const targetId = file.getTargetId();
      try {
        const t = DriveApp.getFileById(targetId);
        return { id: t.getId(), name: t.getName(), url: t.getUrl(), mime: t.getMimeType(), description: t.getDescription() };
      } catch (e) {
        // Destino sin permiso o carpeta: usamos los datos del acceso directo.
        return { id: targetId, name: file.getName(), url: 'https://drive.google.com/open?id=' + targetId, mime: file.getTargetMimeType(), description: file.getDescription() };
      }
    }
    return { id: file.getId(), name: file.getName(), url: file.getUrl(), mime: file.getMimeType(), description: file.getDescription() };
  } catch (e) {
    return null;
  }
}

function obtenerHojaIndice_(folder, rol) {
  const it = folder.getFilesByType(MimeType.GOOGLE_SHEETS);
  while (it.hasNext()) {
    const f = it.next();
    if (f.getName().indexOf(CONFIG.indexPrefix) === 0) return SpreadsheetApp.openById(f.getId()).getSheets()[0];
  }
  const ss = SpreadsheetApp.create(CONFIG.indexPrefix + ' ' + rol);
  DriveApp.getFileById(ss.getId()).moveTo(folder);
  const sh = ss.getSheets()[0];
  sh.getRange(1, 1, 1, CONFIG.headers.length).setValues([CONFIG.headers]).setFontWeight('bold');
  sh.setFrozenRows(1);
  return sh;
}

/* ---------- Feedback: aprender de las búsquedas ---------- */

function obtenerHojaFeedback_() {
  const folder = DriveApp.getFolderById(CONFIG.accesos.folderId);
  const it = folder.getFilesByName(CONFIG.feedback.nombre);
  if (it.hasNext()) return SpreadsheetApp.openById(it.next().getId()).getSheets()[0];
  const ss = SpreadsheetApp.create(CONFIG.feedback.nombre);
  DriveApp.getFileById(ss.getId()).moveTo(folder);
  const sh = ss.getSheets()[0];
  sh.setName('Feedback');
  const h = CONFIG.feedback.headers;
  sh.getRange(1, 1, 1, h.length).setValues([h]).setFontWeight('bold').setBackground('#F2F1EC');
  sh.setFrozenRows(1);
  sh.getRange(2, 9, 999, 1).setDataValidation(
    SpreadsheetApp.newDataValidation().requireValueInList(['Pendiente', 'OK', 'Aplicado', 'Descartado'], true).build());
  [140, 190, 110, 240, 60, 320, 320, 260, 100, 240].forEach((w, i) => sh.setColumnWidth(i + 1, w));
  sh.getRange('A:A').setNumberFormat('dd/mm/yyyy HH:mm');
  sh.getRange('H1').setNote('Escribe aquí el nombre exacto (o el enlace) del archivo que tendría que haber salido. ' +
    'Al sincronizar, las palabras de la búsqueda se añaden a sus palabras clave y el Estado pasa a "Aplicado".');
  return sh;
}

/**
 * Para cada fila con "Archivo que debía salir" relleno y Estado distinto de Aplicado/Descartado:
 * añade las palabras de la búsqueda a las palabras clave de ese archivo en los índices.
 */
function aplicarFeedback_() {
  let sh;
  try { sh = obtenerHojaFeedback_(); } catch (e) { return; }
  const rows = sh.getDataRange().getValues();
  if (rows.length < 2) return;
  const indices = CONFIG.roles.map(cfg => obtenerHojaIndice_(DriveApp.getFolderById(cfg.folderId), cfg.rol));
  let cambios = false;
  for (let r = 1; r < rows.length; r++) {
    const busqueda = String(rows[r][3] || '');
    const objetivo = String(rows[r][7] || '').trim();
    const estado = String(rows[r][8] || '');
    if (!objetivo || estado === 'Aplicado' || estado === 'Descartado') continue;
    const palabras = palabrasUtiles_(busqueda);
    let encontrado = false;
    indices.forEach(ix => {
      const data = ix.getDataRange().getValues();
      const head = data[0].map(h => String(h).trim().toLowerCase());
      const cN = head.indexOf('nombre'), cE = head.indexOf('enlace'), cK = head.indexOf('palabras clave');
      const objId = extraerId_(objetivo);
      for (let i = 1; i < data.length; i++) {
        const coincide = String(data[i][cN]).trim().toLowerCase() === objetivo.toLowerCase() ||
          (objId && extraerId_(String(data[i][cE])) === objId) || String(data[i][cE]).trim() === objetivo;
        if (!coincide) continue;
        encontrado = true;
        const actuales = String(data[i][cK] || '');
        const set = new Set(actuales.split(',').map(x => normalizar_(x)).filter(Boolean));
        const nuevas = palabras.filter(w => !set.has(w));
        if (busqueda && !set.has(normalizar_(busqueda))) nuevas.push(busqueda.trim().toLowerCase());
        if (nuevas.length) ix.getRange(i + 1, cK + 1).setValue(actuales ? actuales + ', ' + nuevas.join(', ') : nuevas.join(', '));
      }
    });
    sh.getRange(r + 1, 9).setValue(encontrado ? 'Aplicado' : 'Pendiente');
    sh.getRange(r + 1, 10).setValue(encontrado
      ? 'Palabras añadidas el ' + Utilities.formatDate(new Date(), 'Europe/Madrid', 'dd/MM/yyyy')
      : 'No encuentro ese archivo en los índices: revisa el nombre');
    cambios = cambios || encontrado;
  }
  if (cambios) CacheService.getScriptCache().remove(CONFIG.cacheKey);
}

const STOPWORDS_ = new Set(('a al como de del donde el en es esta este hay la las lo los me mi para por que se si sobre su ' +
  'un una uno y o tengo quiero necesito puedo hacer hago saber ver busco algo cual cuando con le les').split(' '));

function normalizar_(s) {
  return String(s || '').toLowerCase().normalize('NFD').replace(/[\u0300-\u036f]/g, '').trim();
}

function palabrasUtiles_(texto) {
  return Array.from(new Set(normalizar_(texto).split(/[^a-z0-9ñ]+/).filter(w => w.length > 2 && !STOPWORDS_.has(w))));
}

/* ---------- Lectura ---------- */

function leerCatalogo_() {
  const porClave = new Map();
  CONFIG.roles.forEach(cfg => {
    const folder = DriveApp.getFolderById(cfg.folderId);
    const sheet = obtenerHojaIndice_(folder, cfg.rol);
    const rows = sheet.getDataRange().getValues();
    const head = rows[0].map(h => String(h).trim().toLowerCase());
    const col = name => head.indexOf(name.toLowerCase());
    const c = {
      nombre: col('Nombre'), enlace: col('Enlace'), tipo: col('Tipo'), cat: col('Categoría'),
      desc: col('Descripción'), kw: col('Palabras clave'),
    };
    for (let i = 1; i < rows.length; i++) {
      const r = rows[i];
      const nombre = String(r[c.nombre] || '').trim();
      const enlace = String(r[c.enlace] || '').trim();
      if (!nombre && !enlace) continue;
      const clave = extraerId_(enlace) || enlace || nombre;
      const existente = porClave.get(clave);
      if (existente) {
        if (existente.roles.indexOf(cfg.rol) === -1) existente.roles.push(cfg.rol);
        if (!existente.descripcion && r[c.desc]) existente.descripcion = String(r[c.desc]);
        continue;
      }
      porClave.set(clave, {
        nombre: nombre,
        enlace: enlace,
        tipo: String(r[c.tipo] || ''),
        categoria: String(r[c.cat] || ''),
        descripcion: String(r[c.desc] || ''),
        keywords: String(r[c.kw] || ''),
        roles: [cfg.rol],
      });
    }
  });
  return Array.from(porClave.values());
}

/* ---------- Utilidades ---------- */

function extraerId_(url) {
  const m = url.match(/\/d\/([a-zA-Z0-9_-]{20,})/) || url.match(/[?&]id=([a-zA-Z0-9_-]{20,})/) ||
            url.match(/\/projects\/([a-zA-Z0-9_-]{20,})/) || url.match(/\/folders\/([a-zA-Z0-9_-]{20,})/);
  return m ? m[1] : '';
}

function tipoDesdeMime_(mime) {
  const map = {
    'application/vnd.google-apps.spreadsheet': 'Google Sheet',
    'application/vnd.google-apps.document': 'Google Doc',
    'application/vnd.google-apps.presentation': 'Google Slides',
    'application/vnd.google-apps.form': 'Google Form',
    'application/vnd.google-apps.script': 'Apps Script',
    'application/vnd.google-apps.folder': 'Carpeta',
    'application/pdf': 'PDF',
  };
  if (map[mime]) return map[mime];
  if (/^image\//.test(mime || '')) return 'Imagen';
  if (/^video\//.test(mime || '')) return 'Vídeo';
  return 'Archivo';
}
