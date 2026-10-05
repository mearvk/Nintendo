/* ============================================================================
 * audio_build — original NES audio data builder.
 * ----------------------------------------------------------------------------
 * Our own code. Converts a simple text score into a compact byte stream that
 * our own playback engine (which we write in 6502) can step through, one event
 * per frame-group. Not derived from any third-party tracker or sound engine.
 *
 * The byte stream we define (our own format, "NTSND v1"):
 *   byte 0      : 'S'  (0x53) magic tag
 *   byte 1      : version (0x01)
 *   then a sequence of events, each 3 bytes:
 *     [channel] [note]  [duration_frames]
 *   channel: 0=pulse1 1=pulse2 2=triangle 3=noise
 *   note:    0 = rest, else a semitone index 1..96 (our engine maps to timer)
 *   duration: number of frames to hold (1..255)
 *   terminated by a single 0xFF byte.
 *
 * Input text:
 *   lines of: <channel> <note|R> <frames>     e.g.  0 A4 15
 *   notes are name+octave (C,C#,D,...,B) + octave 0..7; R = rest
 *   '#' comment to end of line; blank lines ignored
 *
 * Usage:
 *   audio_build <score.txt> <out.snd>
 * ============================================================================ */
#include <ctype.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* semitone offsets within an octave, our canonical table */
static int note_semitone(const char *name, int *ok) {
    static const char *names[] = {"C","C#","D","D#","E","F",
                                  "F#","G","G#","A","A#","B"};
    *ok = 0;
    char base[4] = {0};
    int i = 0;
    base[i++] = (char)toupper((unsigned char)name[0]);
    if (name[1] == '#') base[i++] = '#';
    int octdigit = name[i];
    if (!isdigit((unsigned char)octdigit)) return 0;
    int oct = octdigit - '0';
    for (int s = 0; s < 12; ++s) {
        if (strncmp(base, names[s], 2) == 0 || (strlen(names[s])==1 &&
            base[1]==0 && base[0]==names[s][0])) {
            *ok = 1;
            /* semitone index 1-based, C0 => 1 */
            return oct * 12 + s + 1;
        }
    }
    return 0;
}

int main(int argc, char **argv) {
    if (argc < 3) { fprintf(stderr, "usage: audio_build <score.txt> <out.snd>\n"); return 2; }
    FILE *in = fopen(argv[1], "r");
    if (!in) { perror("open"); return 1; }
    FILE *out = fopen(argv[2], "wb");
    if (!out) { perror("out"); fclose(in); return 1; }

    fputc('S', out); fputc(0x01, out);     /* magic + version */

    char line[256];
    int events = 0, lineno = 0;
    while (fgets(line, sizeof(line), in)) {
        ++lineno;
        char *h = strchr(line, '#'); if (h) *h = 0;
        char chs[16], note[16], durs[16];
        int n = sscanf(line, "%15s %15s %15s", chs, note, durs);
        if (n <= 0) continue;          /* blank or comment-only line (EOF/0) */
        if (n != 3) {
            fprintf(stderr, "line %d: need '<chan> <note|R> <frames>'\n", lineno);
            fclose(in); fclose(out); return 1;
        }
        int ch = atoi(chs);
        int dur = atoi(durs);
        if (ch < 0 || ch > 3 || dur < 1 || dur > 255) {
            fprintf(stderr, "line %d: channel 0-3, frames 1-255\n", lineno);
            fclose(in); fclose(out); return 1;
        }
        int semitone = 0;
        if (!(note[0] == 'R' && note[1] == 0)) {
            int ok; semitone = note_semitone(note, &ok);
            if (!ok || semitone < 1 || semitone > 96) {
                fprintf(stderr, "line %d: bad note '%s'\n", lineno, note);
                fclose(in); fclose(out); return 1;
            }
        }
        fputc((uint8_t)ch, out);
        fputc((uint8_t)semitone, out);
        fputc((uint8_t)dur, out);
        ++events;
    }
    fputc(0xFF, out);       /* terminator */
    fclose(in); fclose(out);
    printf("built %d audio events -> %s (format NTSND v1)\n", events, argv[2]);
    return 0;
}
