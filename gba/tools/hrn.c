// Arnés headless de mGBA: lee órdenes por stdin.
//  k <mask>        teclas (A=1 B=2 SEL=4 START=8 R=16 L=32 U=64 D=128 RB=256 LB=512)
//  f <n>           avanza n frames
//  s <file.ppm>    captura
//  r <addr> <len>  lee memoria (hex)
//  q               salir
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <mgba/core/core.h>
#include <mgba/core/log.h>
static void nolog(struct mLogger *l, int c, enum mLogLevel lv, const char *f, va_list a) { (void)l; (void)c; (void)lv; (void)f; (void)a; }
static struct mLogger LG = { .log = nolog };
int main(int argc, char **argv) {
    mLogSetDefaultLogger(&LG);
    struct mCore *core = mCoreFind(argv[1]);
    if (!core || !core->init(core)) { fprintf(stderr, "no core\n"); return 1; }
    mCoreInitConfig(core, NULL);
    unsigned w, h; core->desiredVideoDimensions(core, &w, &h);
    color_t *buf = malloc(w * h * sizeof(color_t));
    core->setVideoBuffer(core, buf, w);
    if (!mCoreLoadFile(core, argv[1])) { fprintf(stderr, "no load\n"); return 1; }
    core->reset(core);
    char line[512]; unsigned keys = 0; unsigned long fr = 0;
    while (fgets(line, sizeof line, stdin)) {
        char c = line[0];
        if (c == 'k') { keys = strtoul(line + 2, 0, 0); core->setKeys(core, keys); }
        else if (c == 'f') { int n = atoi(line + 2); for (int i = 0; i < n; i++) { core->setKeys(core, keys); core->runFrame(core); fr++; } }
        else if (c == 's') {
            char *p = line + 2; p[strcspn(p, "\r\n")] = 0;
            FILE *o = fopen(p, "wb"); fprintf(o, "P6\n%u %u\n255\n", w, h);
            for (unsigned i = 0; i < w * h; i++) { uint32_t v = buf[i]; unsigned char px[3] = { v & 0xFF, (v >> 8) & 0xFF, (v >> 16) & 0xFF }; fwrite(px, 1, 3, o); }
            fclose(o);
        } else if (c == 'r') {
            unsigned a, n; sscanf(line + 2, "%x %u", &a, &n);
            for (unsigned i = 0; i < n; i++) printf("%02x", core->busRead8(core, a + i));
            printf("\n");
        } else if (c == 'q') break;
        printf("ok %lu\n", fr); fflush(stdout);
    }
    core->deinit(core);
    return 0;
}
