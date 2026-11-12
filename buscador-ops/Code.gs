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

/** Lo llama la página. Devuelve el catálogo de los dos roles, unificado. */
function getCatalogo() {
  const cache = CacheService.getScriptCache();
  const hit = cache.get(CONFIG.cacheKey);
  if (hit) return JSON.parse(hit);

  sincronizar_();
  const items = leerCatalogo_();
  const payload = { items: items, actualizado: new Date().toISOString() };
  try { cache.put(CONFIG.cacheKey, JSON.stringify(payload), CONFIG.cacheSeconds); } catch (e) { /* >100KB: sin caché */ }
  return payload;
}

/** Fuerza recarga desde la página (botón "Actualizar"). */
function refrescarCatalogo() {
  CacheService.getScriptCache().remove(CONFIG.cacheKey);
  return getCatalogo();
}

/* ---------- Sincronización carpetas -> índices ---------- */

/** Ejecutar a mano o con el activador horario (ver instalarActivador). */
function sincronizar() {
  sincronizar_();
  CacheService.getScriptCache().remove(CONFIG.cacheKey);
}

/** Crea un activador que sincroniza cada hora. Ejecutar UNA vez. */
function instalarActivador() {
  ScriptApp.getProjectTriggers()
    .filter(t => t.getHandlerFunction() === 'sincronizar')
    .forEach(t => ScriptApp.deleteTrigger(t));
  ScriptApp.newTrigger('sincronizar').timeBased().everyHours(1).create();
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
