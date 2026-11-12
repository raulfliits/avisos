# Buscador Ops FLiiTS

Webapp (Google Apps Script) donde Ops Specialists y Ops Managers escriben una pregunta
("cómo entrego una chaqueta", "cambiar horas de contrato") y ven los archivos y
herramientas útiles con una descripción de para qué sirve cada uno.

## De dónde salen los datos

- `Q&A FLiiTS OPS / Ops Specialist / Índice Ops Specialist` (Google Sheet)
- `Q&A FLiiTS OPS / Ops Manager / Índice Ops Manager` (Google Sheet)

Cada fila es un archivo o herramienta: Nombre, Enlace, Tipo, Categoría, Descripción,
Palabras clave. Lo que esté en los dos índices sale una sola vez con las dos etiquetas.

**Se actualiza solo:** si alguien deja un archivo o un acceso directo en cualquiera de las
dos carpetas (o en subcarpetas), la sincronización lo añade al índice de esa carpeta con
Origen "Carpeta (auto)". Solo hay que rellenarle la descripción y las palabras clave.
Para añadir una web que no está en Drive (Metabase, portales…), se añade una fila a mano.

## Quién ve qué

La hoja **`Q&A FLiiTS OPS / Accesos Buscador Ops`** tiene una fila por persona: Email | Rol | Nombre.

- **Ops Manager**: ve todo (lo de los dos índices) y puede filtrar por rol.
- **Ops Specialist**: ve completo solo lo que está en el índice de Ops Specialist. Si busca algo
  que solo es de manager, le sale el aviso *"Esto lo gestiona tu Ops Manager. Consulta con tu
  manager"*, sin nombre, enlace ni descripción del archivo. El filtrado se hace en el servidor:
  los datos de manager no llegan nunca a su navegador.
- **Quien no esté en la hoja** ve "Todavía no tienes acceso".

Los cambios en la hoja se aplican en 5 minutos como mucho (o al pulsar "Actualizar").
Para que un archivo lo vea un specialist, ponlo en el índice de Ops Specialist (o deja el
archivo/acceso directo en esa carpeta).

## Publicarla (una sola vez, ~5 min)

1. Entra en https://script.google.com con la cuenta de FLiiTS → **Nuevo proyecto**. Nómbralo "Buscador Ops FLiiTS".
2. Pega el contenido de `Code.gs` en el archivo `Código.gs`.
3. **+ → HTML**, nómbralo `Index` (sin .html) y pega el contenido de `Index.html`.
4. En el selector de funciones elige `sincronizar` → **Ejecutar** y acepta los permisos (Drive y Sheets).
5. Elige `instalarActivador` → **Ejecutar**. Desde ahora sincroniza cada hora (la web además sincroniza cada 10 min cuando alguien la abre, y el botón "Actualizar" fuerza una recarga).
6. **Implementar → Nueva implementación → Aplicación web**:
   - Ejecutar como: **Yo**
   - Quién tiene acceso: **Cualquier usuario de fliits.com**
7. Copia la URL `/exec`.
8. **Página con el estilo FLiiTS (GitHub Pages)**, igual que *Panel Citas* y *Stock FLiiTS*: copia `web/index.html`
   a una carpeta `buscador/` del repo de `opsfliits.github.io`, pega la URL `/exec` en la variable `APP` y súbelo.
   Quedará en `https://opsfliits.github.io/buscador/` con la pantalla de carga negra y el logo, y dentro el buscador.

Para publicar cambios del código: **Implementar → Gestionar implementaciones → editar → Nueva versión**.

## Permisos

La web lee las carpetas con tu cuenta, así que el equipo no necesita acceso a los índices.
Para **abrir** cada archivo, cada persona sí necesita permiso sobre ese archivo en Drive.
