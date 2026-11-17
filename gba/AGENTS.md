# Port de FLiiTS Catalunya World a Game Boy Advance — guía para cada agente

El juego original es un único HTML con JavaScript: `/workspace/avisos/FLiiTS_Catalunya_World.html`
(el código del juego va de la línea ~1162 a la ~3990). Lo estamos portando a una ROM de GBA en C para una
consola Anbernic RG35XX con GarlicOS (emulador de GBA). Objetivo: que cada modo/minijuego se juegue y se vea
**igual que en el web** (misma lógica, mismos textos, mismos sprites y colores, mismas reglas de ganar/perder),
adaptado a la pantalla de 240x160 y a los botones de la consola (cruceta, A, B, START, SELECT, L, R).

NO hay paneles de datos: es la versión "solo juego" (`GAME_ONLY`): ignora todo lo de `openPanel`, `DEMO_ONLY`,
QR, modo edición y similares.

## Estructura
- `eng/` motor (NO lo modifiques salvo un fallo claro; si lo cambias, dilo en tu informe con el diff exacto):
  `eng.h` documenta toda la API: modo 4 (8 bpp con paleta), primitivas `rect`, `pset`, `line`, `circle`,
  `ellipse`, `poly`, `blit`, `rows` (sprites de cadenas como en el JS), texto UTF-8 `text`/`text_small`/`text_wrap`,
  `origin()` (= ctx.translate), `clip()`, bitmaps en EWRAM (`bmp_new`, `tgt_bmp`/`tgt_back`, pool con
  `pool_mark`/`pool_reset`), entrada (`key_held`, `key_hit`, `K_*`), `frame_end()`, sonido (`sfx_*`, `tone`, `slide`, `noise`),
  azar (`rnd`, `rndi`, `rndf`), trigonometría (`fsin`, `fcos`, `isin`…), `fade_level`.
- `game/people.h|.c` + `people_data.*`: sprites y dibujo compartido ya portados: `CREW[4]` (0 IVAN, 1 RAÜL,
  2 GERARD, 3 AZLAN), `draw_spr`, `draw_dog`, `compose_rider` (= composeRider), `rep_crew`, `draw_person`,
  `person_bmp` (= personCv), `draw_worker`, `draw_item`, `big_coin`, `draw_stand`, `wheel`, `fbox`, `helmet`,
  MARTI / MARTI_B / FRANCO / MOTO / BOSS / CAND_T / ITEM_* con sus paletas `P_*`.
- `game/modes.h`: la interfaz con el menú principal (qué función implementa cada modo y qué devuelve).
- `game/arcade.h|.c`: la máquina común de los arcade (= arcStart/arcTick/arcPress/arcDrawBanner/arcHire/arcWin/arcLose).
- `game/main.c`: menú principal. **En tu copia es tuya**: cámbiala para arrancar directamente en tu modo y probar.
  No se fusionará (lo hace el coordinador).

## Colores
Escribe los colores SIEMPRE como `C(0xRRGGBB)` literal en el código (exactamente los del JS). `tools/build.py`
los recoge todos y monta la paleta de 256. Los `rgba(...)` semitransparentes del JS: usa `rect_dark`/`rect_light`
(LUT) o `rect_dither`, o un color opaco equivalente. Colores calculados (shade(), mezclas): `col_shade`, `col_mix`, `col_rgb`.

## Compilar y probar
```
cd <tu carpeta>
python3 -I /tmp/claude-0/-workspace/8dc91dcf-bfc0-5e40-b28a-bc016414b30c/scratchpad/gba/tools/build.py .     # → build/fliits.gba
```
Pruebas sin ventana con mGBA (Python):
```python
import sys; sys.path.insert(0, '/tmp/claude-0/-workspace/8dc91dcf-bfc0-5e40-b28a-bc016414b30c/scratchpad/gba/tools')
from drv import Gba
g = Gba('build/fliits.gba'); g.frames(120); g.press('A'); g.hold('RIGHT', 60); g.shot('shots/a.png')
print(g.s32('mi_variable'))   # lee variables globales por nombre (no static) para que un bot sepa el estado
```
Mira las capturas (Read sobre el .png) y compáralas con lo que dibuja el JS. Haz un bot que **gane** tu modo de
principio a fin (y que también pierda, para probar GAME OVER/reintentar) sin cuelgues.

## Rendimiento (importante: GBA a 16,78 MHz, sin FPU)
- La lógica va a 60 pasos/s: en tu bucle `int n = frame_end(); for (i<n) step();` y dibuja una vez. Si el dibujo
  tarda más de un frame, n será 2 y el juego sigue a la misma velocidad (a 30 fps). Intenta que n sea 1–2; mide
  el máximo de n en una variable global y dilo en el informe.
- Pre-dibuja en bitmaps lo que no cambia (fondos, sprites de cadenas → `rows_bmp`, personas → `person_bmp`) y
  usa `blit`. `rows()`/`draw_spr()` por píxel cada frame es lento para muchos personajes.
- `float` funciona (por software) pero es lento: vale para unos pocos objetos; en bucles grandes (raycaster,
  carretera pseudo-3D) usa enteros/punto fijo y tablas.
- IWRAM es pequeña (32 KB): arrays grandes como `static ... EWRAM_BSS`. Funciones muy calientes: `IWRAM_CODE`.
- Usa `pool_mark()` al entrar en tu modo y `pool_reset()` + `people_cache_reset()` al salir.

## Pantalla y controles
- 240x160. El JS de plataforma dibuja a 256x144 (VW x VH, GY = VH-24): adáptalo a 240x160 (GY = 136).
- Mapea las teclas del JS: flechas → cruceta; Enter/Espacio/Z → A; X/Backspace → B; Escape → START (pausa con
  `pause_menu()`, que devuelve 1 si el jugador quiere salir al selector).
- Los bocadillos/textos que en el web son HTML (DOM) dibújalos en pantalla con la fuente (`text`, `text_small`,
  `ui_box`, `arc_bubble`): cortos y que no tapen el juego.

## Entrega
Escribe tu código SOLO en archivos nuevos `game/<prefijo>_*.c|h` (prefijo indicado en tu encargo). No toques
`eng/`, `people*`, `modes.h`, `arcade.*` (si necesitas algo de ellos, cópialo a tu archivo con otro nombre o
explícalo en el informe). Al terminar, informa: archivos, qué funciona, qué falta o difiere del web, rendimiento
(n máximo), y las capturas más representativas (rutas).
