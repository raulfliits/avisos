#!/bin/sh
# Compila la ROM de Game Boy. Necesita GBDK-2020 (lcc) y la fuente Press Start 2P en dl/ps2p.ttf.
# Uso: sh build.sh <carpeta_con_tool/gbdk_y_dl>   → build/fliits.gb
set -e
D=$(cd "$(dirname "$0")" && pwd); W=${1:-$D}
mkdir -p "$W/src" "$W/spr"; cp "$D/main.c" "$D/gen.py" "$W/src/"; cp "$D"/spr/*.png "$W/spr/"
python3 -I "$W/src/gen.py" "$W"
cd "$W/build" && "$W/tool/gbdk/bin/lcc" -Wm-yn"FLIITS WORLD" -o fliits.gb main.c assets.c
