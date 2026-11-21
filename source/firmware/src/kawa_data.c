/* SPDX-License-Identifier: GPL-3.0-only */
/* KAWA: names, parameter descriptions, factory sounds, themes, chords, scales, the live effects and the
 * magic tools. Text on the screen is upper case without accents (the font is ASCII), in three languages:
 * French, English, Spanish (OPTIONS, KNOB 2). Tables hold [3] names in that order. */

enum { LANG_FR, LANG_EN, LANG_ES, LANG_N };
static uint8_t lang = LANG_EN;                  /* the screen's language (the settings sector) */
#define TR(fr, en, es) (lang == LANG_EN ? (en) : lang == LANG_ES ? (es) : (fr))
static int32_t clampi(int32_t v, int32_t lo, int32_t hi) { return v < lo ? lo : v > hi ? hi : v; }
static const char *const LANG_CODE[LANG_N] = {"FR", "EN", "ES"};

/* ---- engines: each of the eight tracks plays one, picked with its sound (SOUND, PRESETS) */

#define DX_NV 32u                               /* (Gabare) a DX7 bank's voices, after the FM sounds (kawa_edit.c) */
/* (Gabare) FM SOUND's value (a signed byte, as every sound setting): 0..11 KAWA's FM sounds, 12..43 the DX7 bank's voices
 * (as in 0.3), -128..-1 the FM-1's original voices 1..128 (kawa_mvave.h, 0.5). The knob goes KAWA, M-VAVE, the bank. */
#define FMPAT_N (FM_NPATCH + MV_N + DX_NV)
static int32_t fmpat_pos(int32_t v)             /* a value -> its place in the knob's order */
{
    return v < 0 ? (int32_t)FM_NPATCH + v + 128 : v < (int32_t)FM_NPATCH ? v : (int32_t)MV_N + v;
}
static int32_t fmpat_val(int32_t pos)           /* a place -> the value */
{
    return pos < (int32_t)FM_NPATCH ? pos : pos < (int32_t)(FM_NPATCH + MV_N) ? pos - (int32_t)FM_NPATCH - 128 : pos - (int32_t)MV_N;
}
static uint8_t dx_ok;                           /* (Gabare) a DX7 bank is in the flash */
static const char *dx_name(uint32_t k);
static const char *mv_name(uint32_t k);
#define NTRK 6u                                /* six tracks: the FM-1's audio interrupt keeps a wide margin */
enum { E_PULSE, E_WAVE, E_XNOISE, E_FM, E_KIT, E_XSAMPLE, E_ANALOG, E_PHASE, E_LOFI, E_VOICE, E_TRIO, E_WHEEL, E_PHYS, E_STRING, E_ADD,
       E_NOISE, E_DIGI, E_N };
/* (Gabare 0.9) E_XNOISE (KAWA's BRUIT) and E_XSAMPLE (SAMPLE) are retired: their numbers stay (they are in the songs),
 * nothing plays them, a song's sound on one becomes NOISE or KIT (prj_sanitize). NOISE and DIGITAL (FM4) are SLOOP's
 * (Felucca's), in Skiff's form like the engines from E_SK0 on (kawa_sk.c) */
#define E_RETIRED(e) ((e) == E_XNOISE || (e) == E_XSAMPLE)
/* (0.9) the engines as the strip shows them and ARP + PRESETS goes through them: KAWA's first, NOISE by them, FM4 by FM */
static const uint8_t ENG_ORDER[] = {E_PULSE, E_WAVE, E_NOISE, E_FM, E_DIGI, E_KIT, E_ANALOG, E_PHASE, E_LOFI, E_VOICE, E_TRIO,
                                    E_WHEEL, E_PHYS, E_STRING, E_ADD};
#define ENG_NORDER ((uint32_t)(sizeof ENG_ORDER / sizeof ENG_ORDER[0]))
typedef char eng_order_all[ENG_NORDER == E_N - 2u ? 1 : -1];
static uint32_t eng_pos(uint32_t e)             /* where an engine is in ENG_ORDER */
{
    uint32_t k;
    for (k = 0; k < ENG_NORDER; k++)
        if (ENG_ORDER[k] == e) return k;
    return 0;
}   /* (Fm1Studio) from E_SK0 on: Skiff's (kawa_sk.c) */
#define E_SK0 E_ANALOG
static const char *const ENGINE_NAMES[LANG_N][E_N] = {{"CARRE", "ONDE", "-", "FM", "KIT", "-", "ANALOG", "PHASE", "LOFI", "VOIX", "TRIO", "ORGUE", "PHYS", "CORDE", "ADD", "BRUIT", "FM4"},
                                                      {"PULSE", "WAVE", "-", "FM", "KIT", "-", "ANALOG", "PHASE", "LOFI", "VOICE", "TRIO", "WHEEL", "PHYS", "STRING", "ADD", "NOISE", "FM4"},
                                                      {"PULSO", "ONDA", "-", "FM", "KIT", "-", "ANALOG", "FASE", "LOFI", "VOZ", "TRIO", "ORGANO", "FISICO", "CUERDA", "ADD", "RUIDO", "FM4"}};
#define ENGINE_NAME ENGINE_NAMES[lang]
static const uint8_t ENGINE_ICON[E_N] = {ICO_PULSE, ICO_WAVE, ICO_NOISE, ICO_FM, ICO_KIT, ICO_SAMPLE, ICO_LPF, ICO_SHAPE, ICO_PIXEL, ICO_VIB, ICO_TRI3, ICO_DISC, ICO_RING, ICO_BEND, ICO_VOL,
                                        ICO_NOISE, ICO_FM};

/* ---- themes (OPTIONS, KNOB 1): the screen's colours. KAWA is the look of 0.1 */
typedef struct {
    const char *name;
    uint16_t bg, bg2, beat, rule, faint, mid, txt, gold, rec, go, ink;   /* ink: text on a lit tile */
    uint16_t trk[NTRK];
} theme_t;
static const theme_t THEMES[3] = {
    {"KAWA", RGB(18, 12, 36), RGB(26, 18, 50), RGB(38, 28, 72), RGB(56, 44, 98), RGB(118, 106, 170), RGB(176, 166, 224),
     RGB(240, 236, 255), RGB(255, 222, 110), RGB(255, 64, 96), RGB(96, 255, 150), RGB(18, 12, 36),
     {RGB(255, 95, 180), RGB(255, 176, 58), RGB(62, 216, 255), RGB(232, 240, 96), RGB(98, 242, 154), RGB(169, 139, 255)}},
    {"GAME BOY", RGB(15, 56, 15), RGB(24, 70, 24), RGB(40, 92, 36), RGB(56, 108, 48), RGB(110, 150, 70), RGB(160, 196, 60),
     RGB(214, 236, 150), RGB(236, 250, 170), RGB(250, 120, 90), RGB(200, 248, 120), RGB(15, 56, 15),
     {RGB(200, 240, 110), RGB(170, 220, 90), RGB(220, 248, 150), RGB(150, 200, 70), RGB(230, 250, 170), RGB(180, 230, 100)}},
    {"BONBON", RGB(255, 244, 250), RGB(250, 230, 244), RGB(240, 210, 234), RGB(222, 190, 218), RGB(150, 120, 160),
     RGB(100, 72, 112), RGB(50, 28, 62), RGB(200, 106, 10), RGB(226, 30, 84), RGB(20, 150, 80), RGB(255, 255, 255),
     {RGB(230, 50, 140), RGB(226, 120, 10), RGB(10, 140, 200), RGB(120, 150, 10), RGB(20, 160, 100), RGB(130, 70, 220)}},
};
#define NTHEME 3u
static uint8_t theme;
static uint16_t C_BG, C_BG2, C_BEAT, C_RULE, C_FAINT, C_MID, C_TXT, C_GOLD, C_REC, C_GO, C_INK;
static uint16_t TRK_COL[NTRK];
static void theme_set(uint32_t k)
{
    const theme_t *t = &THEMES[k % NTHEME];
    uint32_t i;
    theme = (uint8_t)(k % NTHEME);
    C_BG = t->bg; C_BG2 = t->bg2; C_BEAT = t->beat; C_RULE = t->rule; C_FAINT = t->faint; C_MID = t->mid;
    C_TXT = t->txt; C_GOLD = t->gold; C_REC = t->rec; C_GO = t->go; C_INK = t->ink;
    for (i = 0; i < NTRK; i++)
        TRK_COL[i] = t->trk[i];
}
/* one knob, one colour: KNOB 1..4 everywhere (the cards, the bottom line, the emulator's rings) */
static const uint16_t KNOB_COL[4] = {RGB(255, 92, 92), RGB(255, 204, 64), RGB(92, 226, 116), RGB(84, 164, 255)};
/* one page, one colour (the page's pill, the emulator's buttons) */
enum { PG_TRACK, PG_GRID, PG_SOUND, PG_ENV, PG_MIX, PG_MAGIC, PG_SCALE, PG_SONG, PG_PROJ, PG_OPT, PG_FX, PG_HELP, PG_BOOT,
       PG_GROOVE, PG_SEND, PG_FILT, PG_FXSEQ, PG_ARP, PG_PC, PG_N };   /* ((Gabare 0.10) PG_PC: the MODE PC page) */   /* (Gabare) PG_ARP: the arpeggiator (kawa_arp.c) */                       /* (GROOVE last: the emulator knows the others by their number) */
static const uint16_t PAGE_COL[PG_N] = {RGB(169, 139, 255), RGB(255, 96, 146), RGB(62, 216, 255), RGB(98, 242, 154),
                                        RGB(255, 160, 60), RGB(255, 210, 90), RGB(200, 240, 90), RGB(255, 160, 60),
                                        RGB(100, 150, 255), RGB(255, 160, 60), RGB(255, 120, 200), RGB(169, 139, 255),
                                        RGB(169, 139, 255), RGB(255, 160, 60), RGB(255, 120, 200), RGB(255, 70, 90), RGB(90, 200, 255),
                                        RGB(150, 235, 200), RGB(62, 216, 255)};

/* ---- the 24 parameters of a track: SOUND 1, SOUND 2 (the engine's), ENVELOPE, MIX, EFFECT, LFO */
#define NPARAM 28u
enum { P_S0, P_S1, P_S2, P_S3, P_S4, P_S5, P_S6, P_S7, P_ATT, P_DEC, P_SUS, P_REL, P_VOL, P_PAN, P_ECHO, P_VERB,
       P_FXT, P_FXA, P_FXB, P_FXM, P_LFOD, P_LFOR, P_LFOA, P_LFOW, P_MENVA, P_MVELA, P_MENVP, P_MLFRT };
/* SOUND for PULSE / WAVE / NOISE */
enum { P_SHAPE = 0, P_SWEEP, P_VIB, P_TON, P_CRUSH, P_DETUNE, P_ARPV, P_GLIDE };
/* FM */
enum { P_FMPAT = 0, P_FMALG, P_FMRAT, P_FMFB, P_FMBRI, P_FMMEG, P_FMDET, P_FMVIB };
/* KIT */
enum { P_KIT = 0, P_TUNE, P_DECAY, P_DRIVE, P_KCRUSH, P_KTON };
/* SAMPLE */
enum { P_SRC = 0, P_SMODE, P_STUNE, P_FIT, P_LEN, P_SDRIVE, P_SCRUSH, P_STON };
enum { K_NUM, K_SIGN, K_LIST, K_NAMES, K_PAN, K_MS, K_SRC };   /* K_LIST: n values x 3 languages, flat */
typedef struct {
    const char *name[LANG_N];
    int8_t lo, hi;
    uint8_t kind;
    const char *const *list;
} pdesc_t;
static const char *const L_DUTY[] = {"12%", "25%", "50%", "75%", "PWM", "12%", "25%", "50%", "75%", "PWM",
                                     "12%", "25%", "50%", "75%", "PWM"};
static const char *const L_WAVE[] = {"TRI", "SCIE", "SINUS", "DOUX", "GB", "ORGUE", "VOIX", "FM",
                                     "TRI", "SAW", "SINE", "SOFT", "GB", "ORGAN", "VOICE", "FM",
                                     "TRI", "SIERRA", "SENO", "SUAVE", "GB", "ORGANO", "VOZ", "FM"};
static const char *const L_DUR[] = {"1/4", "1/2", "3/4", "1", "1.5", "2", "3", "4", "6", "8", "12", "TOUT",
                                    "1/4", "1/2", "3/4", "1", "1.5", "2", "3", "4", "6", "8", "12", "ALL",
                                    "1/4", "1/2", "3/4", "1", "1.5", "2", "3", "4", "6", "8", "12", "TODO"};
static const uint8_t DUR_Q[12] = {1, 2, 3, 4, 6, 8, 12, 16, 24, 32, 48, 0};   /* quarter slices, 0 = to the end */
static const char *const L_ONOFF[] = {"NON", "OUI", "NO", "YES", "NO", "SI"};
static const char *const L_ALG[] = {"SON", "1", "2", "3", "4", "5", "6", "7", "8", "9", "10", "11", "12", "13", "14", "15", "16",
                                    "17", "18", "19", "20", "21", "22", "23", "24", "25", "26", "27", "28", "29", "30", "31", "32",
                                    "SOUND", "1", "2", "3", "4", "5", "6", "7", "8", "9", "10", "11", "12", "13", "14", "15", "16",
                                    "17", "18", "19", "20", "21", "22", "23", "24", "25", "26", "27", "28", "29", "30", "31", "32",
                                    "SONIDO", "1", "2", "3", "4", "5", "6", "7", "8", "9", "10", "11", "12", "13", "14", "15", "16",
                                    "17", "18", "19", "20", "21", "22", "23", "24", "25", "26", "27", "28", "29", "30", "31", "32"};
/* the effect of a track or of the master (EFFECT; MIX on MASTER) */
enum { FXT_OFF, FXT_LPF, FXT_HPF, FXT_BPF, FXT_DIST, FXT_CRUSH, FXT_DECIM, FXT_RING, FXT_TREM, FXT_WAH,
       FXT_SOFT, FXT_HARD, FXT_FOLD, FXT_FUZZ, FXT_PHASER, FXT_FLANGER, FXT_CHORUS, FXT_N };   /* ((Gabare 0.9) SLOOP's INSERT
                                                * types, Felucca 1.4's: kawa_snd.c fx_insert) */
static const char *const L_FXT[] = {"AUCUN", "FILT BAS", "FILT HAU", "PASS BND", "DISTO", "ECRASE", "DECIME", "ANNEAU", "TREMOLO", "WAH",
                                    "DOUCE", "DURE", "PLIE", "FUZZ", "PHASER", "FLANGER", "CHORUS",
                                    "NONE", "LOWPASS", "HIPASS", "BANDPASS", "DRIVE", "CRUSH", "LO-FI", "RING", "TREMOLO", "WAH",
                                    "SOFT", "HARD", "FOLD", "FUZZ", "PHASER", "FLANGER", "CHORUS",
                                    "NADA", "FILT BAJ", "FILT ALT", "PASA BND", "DISTOR", "TRITURA", "DIEZMA", "ANILLO", "TREMOLO", "WAH",
                                    "SUAVE", "DURA", "PLIEGA", "FUZZ", "PHASER", "FLANGER", "CHORUS"};
static const uint8_t FXT_ICON[FXT_N] = {ICO_ERASE, ICO_LPF, ICO_HPF, ICO_BPF, ICO_DIST, ICO_CRUSH, ICO_PIXEL, ICO_RING, ICO_TREM, ICO_WOB,
                                        ICO_DIST, ICO_DIST, ICO_SHAPE, ICO_DIST, ICO_WOB, ICO_WOB, ICO_UNI};
/* what the effect's A and B do, per type */
static const char *const FXA_NAMES[LANG_N][FXT_N] = {
    {"A", "COUPURE", "COUPURE", "CENTRE", "GAIN", "BITS", "TAUX", "FREQ", "VITESSE", "VITESSE", "GAIN", "GAIN", "GAIN", "GAIN",
     "VITESSE", "VITESSE", "VITESSE"},
    {"A", "CUTOFF", "CUTOFF", "CENTER", "GAIN", "BITS", "RATE", "FREQ", "RATE", "RATE", "GAIN", "GAIN", "GAIN", "GAIN", "RATE",
     "RATE", "RATE"},
    {"A", "CORTE", "CORTE", "CENTRO", "GANANC", "BITS", "TASA", "FREC", "VELOC", "VELOC", "GANANC", "GANANC", "GANANC",
     "GANANC", "VELOC", "VELOC", "VELOC"}};
static const char *const FXB_NAMES[LANG_N][FXT_N] = {
    {"B", "RESO", "RESO", "RESO", "TON", "-", "-", "-", "PROFOND", "PROFOND", "TON", "TON", "TON", "TON", "PROFOND",
     "PROFOND", "PROFOND"},
    {"B", "RESO", "RESO", "RESO", "TONE", "-", "-", "-", "DEPTH", "DEPTH", "TONE", "TONE", "TONE", "TONE", "DEPTH", "DEPTH",
     "DEPTH"},
    {"B", "RESO", "RESO", "RESO", "TONO", "-", "-", "-", "PROFUND", "PROFUND", "TONO", "TONO", "TONO", "TONO", "PROFUND",
     "PROFUND", "PROFUND"}};
/* the LFO */
enum { LFD_OFF, LFD_PITCH, LFD_VOL, LFD_FILT, LFD_PAN, LFD_SHAPE, LFD_N };
static const char *const L_LFOD[] = {"AUCUN", "HAUTEUR", "VOLUME", "EFFET A", "PANO", "FORME",
                                     "NONE", "PITCH", "VOLUME", "EFFECT A", "PAN", "SHAPE",
                                     "NADA", "ALTURA", "VOLUMEN", "EFECTO A", "PANEO", "FORMA"};
static const char *const L_LFOR[] = {"4 MES", "2 MES", "1 MES", "1/2", "1/4", "1/8", "1/16", "1/32", "LENT", "MOYEN", "VITE",
                                     "4 BARS", "2 BARS", "1 BAR", "1/2", "1/4", "1/8", "1/16", "1/32", "SLOW", "MEDIUM", "FAST",
                                     "4 COMP", "2 COMP", "1 COMP", "1/2", "1/4", "1/8", "1/16", "1/32", "LENTO", "MEDIO", "RAPIDO"};
static const char *const L_LFOW[] = {"SINUS", "TRIANGLE", "CARRE", "SCIE", "HASARD", "SINE", "TRIANGLE", "SQUARE", "SAW", "RANDOM",
                                     "SENO", "TRIANG", "CUADRADA", "SIERRA", "AZAR"};

/* (Fm1Studio) the value names of Skiff's engines' knobs (their own, in every language) */
static const char *const L_SK_ANALOG_0[] = {"SAW", "SQR", "TRI", "SIN", "PWM", "SYNC", "SUB", "SAW", "SQR", "TRI", "SIN", "PWM", "SYNC", "SUB", "SAW", "SQR", "TRI", "SIN", "PWM", "SYNC", "SUB"};
static const char *const L_SK_PHASE_0[] = {"SAW", "SQR", "PLS", "DSIN", "SPLS", "RSAW", "RTRI", "RTRP", "SAW", "SQR", "PLS", "DSIN", "SPLS", "RSAW", "RTRI", "RTRP", "SAW", "SQR", "PLS", "DSIN", "SPLS", "RSAW", "RTRI", "RTRP"};
static const char *const L_SK_PHASE_1[] = {"-", "SAW", "SQR", "PLS", "DSIN", "SPLS", "RSAW", "RTRI", "RTRP", "-", "SAW", "SQR", "PLS", "DSIN", "SPLS", "RSAW", "RTRI", "RTRP", "-", "SAW", "SQR", "PLS", "DSIN", "SPLS", "RSAW", "RTRI", "RTRP"};
static const char *const L_SK_PHASE_5[] = {"MIX", "RING", "MIX", "RING", "MIX", "RING"};
static const char *const L_SK_LOFI_0[] = {"4BIT", "4B/2", "8BIT", "1BIT", "STEP", "4BIT", "4B/2", "8BIT", "1BIT", "STEP", "4BIT", "4B/2", "8BIT", "1BIT", "STEP"};
static const char *const L_SK_LOFI_1[] = {"PLS", "TRI", "SAW", "NOIS", "WRAM", "PLS", "TRI", "SAW", "NOIS", "WRAM", "PLS", "TRI", "SAW", "NOIS", "WRAM"};
static const char *const L_SK_LOFI_6[] = {"OFF", "OCT", "MAJ", "MIN", "OFF", "OCT", "MAJ", "MIN", "OFF", "OCT", "MAJ", "MIN"};
static const char *const L_SK_TRIO_0[] = {"SAW3", "PLS3", "PPT", "SST", "TRI3", "S&T", "P&S", "P+N", "NOIS", "SYNC", "SYNCP", "SYNC3", "RING", "RING3", "R+S", "R+SP", "SAW3", "PLS3", "PPT", "SST", "TRI3", "S&T", "P&S", "P+N", "NOIS", "SYNC", "SYNCP", "SYNC3", "RING", "RING3", "R+S", "R+SP", "SAW3", "PLS3", "PPT", "SST", "TRI3", "S&T", "P&S", "P+N", "NOIS", "SYNC", "SYNCP", "SYNC3", "RING", "RING3", "R+S", "R+SP"};
static const char *const L_SK_TRIO_4[] = {"LP", "BP", "HP", "NOT", "LP", "BP", "HP", "NOT", "LP", "BP", "HP", "NOT"};
static const char *const L_SK_WHEEL_0[] = {"FLUTE", "MELLOW", "HOLLOW", "SMOOTH", "3BAR", "BLUES", "GOSPEL", "ROCK", "TOPS", "CLARI", "REED", "STRNG", "CHAPL", "BRITE", "BASS", "FULL", "FLUTE", "MELLOW", "HOLLOW", "SMOOTH", "3BAR", "BLUES", "GOSPEL", "ROCK", "TOPS", "CLARI", "REED", "STRNG", "CHAPL", "BRITE", "BASS", "FULL", "FLUTE", "MELLOW", "HOLLOW", "SMOOTH", "3BAR", "BLUES", "GOSPEL", "ROCK", "TOPS", "CLARI", "REED", "STRNG", "CHAPL", "BRITE", "BASS", "FULL"};
static const char *const L_SK_WHEEL_4[] = {"OFF", "2ND", "3RD", "2SOFT", "3SOFT", "2SLOW", "3SLOW", "OFF", "2ND", "3RD", "2SOFT", "3SOFT", "2SLOW", "3SLOW", "OFF", "2ND", "3RD", "2SOFT", "3SOFT", "2SLOW", "3SLOW"};
static const char *const L_SK_WHEEL_7[] = {"OFF", "SLOW", "FAST", "OFF", "SLOW", "FAST", "OFF", "SLOW", "FAST"};
static const char *const L_SK_PHYS_0[] = {"MODAL", "STRNG", "MEMB", "SYMP", "MODAL", "STRNG", "MEMB", "SYMP", "MODAL", "STRNG", "MEMB", "SYMP"};
static const char *const L_SK_STRING_0[] = {"NOISE", "PLUCK", "PICK", "HAMMR", "STICK", "FDBK", "NOISE", "PLUCK", "PICK", "HAMMR", "STICK", "FDBK", "NOISE", "PLUCK", "PICK", "HAMMR", "STICK", "FDBK"};
static const char *const L_SK_ADD_0[] = {"HARM", "ODD", "WIDE", "BELL", "BAR", "DRUM", "PRIME", "FIFTH", "SUB", "PLATE", "HARM", "ODD", "WIDE", "BELL", "BAR", "DRUM", "PRIME", "FIFTH", "SUB", "PLATE", "HARM", "ODD", "WIDE", "BELL", "BAR", "DRUM", "PRIME", "FIFTH", "SUB", "PLATE"};
static const char *const L_SK_NOISE_0[] = {"ANLG", "DUST", "LFSR", "META", "ANLG", "DUST", "LFSR", "META", "ANLG", "DUST", "LFSR", "META"};   /* (Gabare 0.9) */
static const char *const L_SK_DIGI_0[] = {"1", "2", "3", "4", "5", "6", "7", "8", "1", "2", "3", "4", "5", "6", "7", "8", "1", "2", "3", "4", "5", "6", "7", "8"};
static const char *const L_SK_DIGI_1[] = {".5", "1", "2", "3", "4", "5", "6", "7", "8", "9", "10", "11", "12", "14", "16", ".5", "1", "2", "3", "4", "5", "6", "7", "8", "9", "10", "11", "12", "14", "16", ".5", "1", "2", "3", "4", "5", "6", "7", "8", "9", "10", "11", "12", "14", "16"};
#define PDN(fr, en, es) {fr, en, es}
#define PD_NONE {{0, 0, 0}, 0, 0, 0, 0}
#define PD_ENV {PDN("ATTAQUE", "ATTACK", "ATAQUE"), 0, 15, K_MS, 0}, {PDN("DECLIN", "DECAY", "CAIDA"), 0, 15, K_MS, 0}, \
               {PDN("TENUE", "SUSTAIN", "SOSTEN"), 0, 15, K_NUM, 0}, {PDN("RELACHE", "RELEASE", "SUELTA"), 0, 15, K_MS, 0}
#define PD_MIX {PDN("VOLUME", "VOLUME", "VOLUMEN"), 0, 15, K_NUM, 0}, {PDN("PANO", "PAN", "PANEO"), -7, 7, K_PAN, 0}, \
               {PDN("ECHO", "ECHO", "ECO"), 0, 15, K_NUM, 0}, {PDN("REVERB", "REVERB", "REVERB"), 0, 15, K_NUM, 0}
#define PD_FX {PDN("EFFET", "EFFECT", "EFECTO"), 0, FXT_N - 1, K_LIST, L_FXT}, {PDN("A", "A", "A"), 0, 15, K_NUM, 0}, \
              {PDN("B", "B", "B"), 0, 15, K_NUM, 0}, {PDN("DOSAGE", "MIX", "MEZCLA"), 0, 15, K_NUM, 0}
#define PD_LFO {PDN("CIBLE", "TARGET", "DESTINO"), 0, LFD_N - 1, K_LIST, L_LFOD}, {PDN("VITESSE", "RATE", "VELOC"), 0, 10, K_LIST, L_LFOR}, \
               {PDN("PROFOND", "DEPTH", "PROFUND"), 0, 15, K_NUM, 0}, {PDN("ONDE", "WAVE", "ONDA"), 0, 4, K_LIST, L_LFOW}
#define PD_MOD {PDN("ENV>EFF", "ENV>FX", "ENV>EFE"), -15, 15, K_SIGN, 0}, {PDN("VEL>EFF", "VEL>FX", "VEL>EFE"), 0, 15, K_NUM, 0}, \
               {PDN("ENV>HAUT", "ENV>PIT", "ENV>ALT"), -12, 12, K_SIGN, 0}, {PDN("LFO NOTE", "LFO KEY", "LFO NOTA"), 0, 1, K_LIST, L_ONOFF}
#define PD_TONAL_REST {PDN("BALAYE", "SWEEP", "BARRIDO"), -15, 15, K_SIGN, 0}, {PDN("VIBRATO", "VIBRATO", "VIBRATO"), 0, 15, K_NUM, 0}, \
                      {PDN("TON", "TONE", "TONO"), 0, 15, K_NUM, 0}, {PDN("ECRASE", "CRUSH", "TRITURA"), 0, 7, K_NUM, 0}, \
                      {PDN("DESACC", "DETUNE", "DESAFIN"), 0, 15, K_NUM, 0}, {PDN("ARP VIT", "ARP SPD", "ARP VEL"), 1, 8, K_NUM, 0}, \
                      {PDN("GLISSE", "GLIDE", "GLISA"), 0, 15, K_NUM, 0}
#define PD_SHAPE PDN("FORME", "SHAPE", "FORMA")
static const pdesc_t PDESC[E_N][NPARAM] = {
    {{PD_SHAPE, 0, 4, K_LIST, L_DUTY}, PD_TONAL_REST, PD_ENV, PD_MIX, PD_FX, PD_LFO, PD_MOD},
    {{PD_SHAPE, 0, 7, K_LIST, L_WAVE}, PD_TONAL_REST, PD_ENV, PD_MIX, PD_FX, PD_LFO, PD_MOD},
    {PD_NONE, PD_NONE, PD_NONE, PD_NONE, PD_NONE, PD_NONE, PD_NONE, PD_NONE, PD_ENV, PD_MIX, PD_FX, PD_LFO, PD_MOD},   /* (retired) */
    {{PDN("SON FM", "PATCH", "SONIDO"), -128, FM_NPATCH + DX_NV - 1, K_NAMES, &FM_PATCH_NAME[0][0]},
     {PDN("ALGO", "ALGO", "ALGO"), 0, 32, K_LIST, L_ALG}, {PDN("RATIO", "RATIO", "RATIO"), -8, 8, K_SIGN, 0},
     {PDN("RETOUR", "FEEDBK", "RETORNO"), -7, 7, K_SIGN, 0}, {PDN("BRILLE", "BRIGHT", "BRILLO"), -15, 15, K_SIGN, 0},
     {PDN("MOD ENV", "MOD ENV", "MOD ENV"), -8, 8, K_SIGN, 0}, {PDN("DESACC", "DETUNE", "DESAFIN"), 0, 15, K_NUM, 0},
     {PDN("VIBRATO", "VIBRATO", "VIBRATO"), 0, 15, K_NUM, 0}, PD_ENV, PD_MIX, PD_FX, PD_LFO, PD_MOD},
    {{PDN("KIT", "KIT", "KIT"), 0, NKIT - 1, K_LIST, L_KITS}, {PDN("ACCORD", "TUNE", "AFINA"), -12, 12, K_SIGN, 0},
     {PDN("DECLIN", "DECAY", "CAIDA"), -8, 8, K_SIGN, 0}, {PDN("SATURE", "DRIVE", "SATURA"), 0, 15, K_NUM, 0},
     {PDN("ECRASE", "CRUSH", "TRITURA"), 0, 7, K_NUM, 0}, {PDN("TON", "TONE", "TONO"), 0, 15, K_NUM, 0}, PD_NONE, PD_NONE,
     PD_ENV, PD_MIX, PD_FX, PD_LFO, PD_MOD},
    {PD_NONE, PD_NONE, PD_NONE, PD_NONE, PD_NONE, PD_NONE, PD_NONE, PD_NONE, PD_ENV, PD_MIX, PD_FX, PD_LFO, PD_MOD},   /* (retired) */
    /* (Fm1Studio) ANALOG, Skiff's (eng_analog.c) */
    {{PDN("WAVE", "WAVE", "WAVE"), 0, 6, K_LIST, L_SK_ANALOG_0},
     {PDN("DETUNE", "DETUNE", "DETUNE"), 0, 127, K_NUM, 0},
     {PDN("MIX", "MIX", "MIX"), 0, 127, K_NUM, 0},
     {PDN("NOISE", "NOISE", "NOISE"), 0, 127, K_NUM, 0},
     {PDN("CUTOFF", "CUTOFF", "CUTOFF"), 0, 127, K_NUM, 0},
     {PDN("RES", "RES", "RES"), 0, 127, K_NUM, 0},
     {PDN("DRIVE", "DRIVE", "DRIVE"), 0, 127, K_NUM, 0},
     {PDN("KEYTRK", "KEYTRK", "KEYTRK"), 0, 127, K_NUM, 0},
     PD_ENV, PD_MIX, PD_FX, PD_LFO, PD_MOD},
    /* (Fm1Studio) PHASE, Skiff's (eng_phase.c) */
    {{PDN("WAVE", "WAVE", "WAVE"), 0, 7, K_LIST, L_SK_PHASE_0},
     {PDN("WAVE2", "WAVE2", "WAVE2"), 0, 8, K_LIST, L_SK_PHASE_1},
     {PDN("DCW", "DCW", "DCW"), 0, 127, K_NUM, 0},
     {PDN("ENV", "ENV", "ENV"), 0, 127, K_NUM, 0},
     {PDN("DETUNE", "DETUNE", "DETUNE"), 0, 127, K_NUM, 0},
     {PDN("LINE", "LINE", "LINE"), 0, 1, K_LIST, L_SK_PHASE_5},
     {PDN("SUB", "SUB", "SUB"), 0, 127, K_NUM, 0},
     {PDN("FOLD", "FOLD", "FOLD"), 0, 127, K_NUM, 0},
     PD_ENV, PD_MIX, PD_FX, PD_LFO, PD_MOD},
    /* (Fm1Studio) LOFI, Skiff's (eng_lofi.c) */
    {{PDN("CHIP", "CHIP", "CHIP"), 0, 4, K_LIST, L_SK_LOFI_0},
     {PDN("WAVE", "WAVE", "WAVE"), 0, 4, K_LIST, L_SK_LOFI_1},
     {PDN("DUTY", "DUTY", "DUTY"), 0, 127, K_NUM, 0},
     {PDN("CRUSH", "CRUSH", "CRUSH"), 0, 127, K_NUM, 0},
     {PDN("SWEEP", "SWEEP", "SWEEP"), 0, 127, K_NUM, 0},
     {PDN("VIBRATO", "VIBRATO", "VIBRATO"), 0, 127, K_NUM, 0},
     {PDN("ARP", "ARP", "ARP"), 0, 3, K_LIST, L_SK_LOFI_6},
     {PDN("TONE", "TONE", "TONE"), 0, 127, K_NUM, 0},
     PD_ENV, PD_MIX, PD_FX, PD_LFO, PD_MOD},
    /* (Fm1Studio) VOICE, Skiff's (eng_formant.c) */
    {{PDN("VOWEL", "VOWEL", "VOWEL"), 0, 127, K_NUM, 0},
     {PDN("VOWEL2", "VOWEL2", "VOWEL2"), 0, 127, K_NUM, 0},
     {PDN("TALK", "TALK", "TALK"), 0, 127, K_NUM, 0},
     {PDN("PITCH", "PITCH", "PITCH"), -12, 12, K_SIGN, 0},
     {PDN("BUZZ", "BUZZ", "BUZZ"), 0, 127, K_NUM, 0},
     {PDN("BREATH", "BREATH", "BREATH"), 0, 127, K_NUM, 0},
     {PDN("Q", "Q", "Q"), 0, 127, K_NUM, 0},
     {PDN("RANDOM", "RANDOM", "RANDOM"), 0, 127, K_NUM, 0},
     PD_ENV, PD_MIX, PD_FX, PD_LFO, PD_MOD},
    /* (Fm1Studio) TRIO, Skiff's (eng_trio.c) */
    {{PDN("WAVE", "WAVE", "WAVE"), 0, 15, K_LIST, L_SK_TRIO_0},
     {PDN("PITCH2", "PITCH2", "PITCH2"), -24, 24, K_SIGN, 0},
     {PDN("PITCH3", "PITCH3", "PITCH3"), -24, 24, K_SIGN, 0},
     {PDN("DETUNE", "DETUNE", "DETUNE"), 0, 50, K_NUM, 0},
     {PDN("MODE", "MODE", "MODE"), 0, 3, K_LIST, L_SK_TRIO_4},
     {PDN("CUTOFF", "CUTOFF", "CUTOFF"), 0, 127, K_NUM, 0},
     {PDN("RES", "RES", "RES"), 0, 127, K_NUM, 0},
     {PDN("PW", "PW", "PW"), 0, 127, K_NUM, 0},
     PD_ENV, PD_MIX, PD_FX, PD_LFO, PD_MOD},
    /* (Fm1Studio) WHEEL, Skiff's (eng_wheel.c) */
    {{PDN("REG", "REG", "REG"), 0, 15, K_LIST, L_SK_WHEEL_0},
     {PDN("SUB", "SUB", "SUB"), -8, 8, K_SIGN, 0},
     {PDN("BODY", "BODY", "BODY"), -8, 8, K_SIGN, 0},
     {PDN("TOP", "TOP", "TOP"), -8, 8, K_SIGN, 0},
     {PDN("PERC", "PERC", "PERC"), 0, 6, K_LIST, L_SK_WHEEL_4},
     {PDN("CLICK", "CLICK", "CLICK"), 0, 127, K_NUM, 0},
     {PDN("DRIVE", "DRIVE", "DRIVE"), 0, 127, K_NUM, 0},
     {PDN("ROTOR", "ROTOR", "ROTOR"), 0, 2, K_LIST, L_SK_WHEEL_7},
     PD_ENV, PD_MIX, PD_FX, PD_LFO, PD_MOD},
    /* (Fm1Studio) PHYS, Skiff's (eng_phys.c) */
    {{PDN("MODEL", "MODEL", "MODEL"), 0, 3, K_LIST, L_SK_PHYS_0},
     {PDN("STRUCT", "STRUCT", "STRUCT"), 0, 127, K_NUM, 0},
     {PDN("BRIGHT", "BRIGHT", "BRIGHT"), 0, 127, K_NUM, 0},
     {PDN("DAMP", "DAMP", "DAMP"), 0, 127, K_NUM, 0},
     {PDN("POS", "POS", "POS"), 0, 127, K_NUM, 0},
     {PDN("ACCENT", "ACCENT", "ACCENT"), 0, 127, K_NUM, 0},
     {PDN("BOW", "BOW", "BOW"), 0, 127, K_NUM, 0},
     {PDN("EXCITE", "EXCITE", "EXCITE"), 0, 127, K_NUM, 0},
     PD_ENV, PD_MIX, PD_FX, PD_LFO, PD_MOD},
    /* (Gabare) STRING, JIANT's (skiff/eng_string.c) */
    {{PDN("EXCITE", "EXCITE", "EXCITE"), 0, 5, K_LIST, L_SK_STRING_0},
     {PDN("DECAY", "DECAY", "DECAY"), 0, 127, K_NUM, 0},
     {PDN("BRIGHT", "BRIGHT", "BRIGHT"), 0, 127, K_NUM, 0},
     {PDN("POS", "POS", "POS"), 0, 127, K_NUM, 0},
     {PDN("STRETCH", "STRETCH", "STRETCH"), 0, 127, K_NUM, 0},
     {PDN("BODY", "BODY", "BODY"), 0, 127, K_NUM, 0},
     {PDN("SIZE", "SIZE", "SIZE"), 0, 127, K_NUM, 0},
     {PDN("BOW", "BOW", "BOW"), 0, 127, K_NUM, 0},
     PD_ENV, PD_MIX, PD_FX, PD_LFO, PD_MOD},
    /* (Gabare) ADD, JIANT's (skiff/eng_add.c) */
    {{PDN("MODE", "MODE", "MODE"), 0, 9, K_LIST, L_SK_ADD_0},
     {PDN("MORPH", "MORPH", "MORPH"), 0, 127, K_NUM, 0},
     {PDN("BRIGHT", "BRIGHT", "BRIGHT"), 0, 127, K_NUM, 0},
     {PDN("PARTIAL", "PARTIAL", "PARTIAL"), 0, 127, K_NUM, 0},
     {PDN("SHIFT", "SHIFT", "SHIFT"), -64, 63, K_SIGN, 0},
     {PDN("COMB", "COMB", "COMB"), 0, 127, K_NUM, 0},
     {PDN("DECAY", "DECAY", "DECAY"), 0, 127, K_NUM, 0},
     {PDN("MOTION", "MOTION", "MOTION"), -64, 63, K_SIGN, 0},
     PD_ENV, PD_MIX, PD_FX, PD_LFO, PD_MOD},
    /* (Gabare 0.9) NOISE, SLOOP's (skiff/eng_noise.c) */
    {{PDN("MODE", "MODE", "MODO"), 0, 3, K_LIST, L_SK_NOISE_0},
     {PDN("COULEUR", "COLOR", "COLOR"), 0, 127, K_NUM, 0},
     {PDN("FREQ", "FREQ", "FREC"), 0, 127, K_NUM, 0},
     {PDN("RES", "RES", "RES"), 0, 127, K_NUM, 0},
     {PDN("SUIVI", "KEYTRK", "SEGUIM"), 0, 127, K_NUM, 0},
     {PDN("DENSITE", "DENSITY", "DENSID"), 0, 127, K_NUM, 0},
     {PDN("DERIVE", "DRIFT", "DERIVA"), 0, 127, K_NUM, 0},
     {PDN("ECRASE", "CRUSH", "TRITURA"), 0, 127, K_NUM, 0},
     PD_ENV, PD_MIX, PD_FX, PD_LFO, PD_MOD},
    /* (Gabare 0.9) DIGITAL, FM4: SLOOP's (skiff/eng_digital.c) */
    {{PDN("ALGO", "ALGO", "ALGO"), 0, 7, K_LIST, L_SK_DIGI_0},
     {PDN("RATIO 2", "RATIO 2", "RATIO 2"), 0, 14, K_LIST, L_SK_DIGI_1},
     {PDN("RATIO 3", "RATIO 3", "RATIO 3"), 0, 14, K_LIST, L_SK_DIGI_1},
     {PDN("RATIO 4", "RATIO 4", "RATIO 4"), 0, 14, K_LIST, L_SK_DIGI_1},
     {PDN("INDEX", "INDEX", "INDICE"), 0, 127, K_NUM, 0},
     {PDN("DECL MOD", "MOD DEC", "CAID MOD"), 0, 127, K_NUM, 0},
     {PDN("RETOUR", "FEEDBK", "RETORNO"), 0, 127, K_NUM, 0},
     PD_NONE,
     PD_ENV, PD_MIX, PD_FX, PD_LFO, PD_MOD},
};
/* the pages of the SOUND page (EDIT again: the next one); the FM track has two more: its operators; a SAMPLE
 * track one: its sample's edit. Every track has its TABLE last */
enum { SP_SON1, SP_SON2, SP_OP, SP_OPENV, SP_SMP, SP_FX, SP_LFO, SP_MOD, SP_TABLE, SP_N };
static const uint8_t SP_FIRST[SP_N] = {P_S0, P_S4, 0, 0, 0, P_FXT, P_LFOD, P_MENVA, 0};
static const char *const SP_NAMES[LANG_N][SP_N] = {
    {"SON", "SON 2", "OPERATEUR", "ENV OPERAT", "SAMPLE", "EFFET", "LFO", "MODULATION", "TABLE"},
    {"SOUND", "SOUND 2", "OPERATOR", "OP ENVELOPE", "SAMPLE EDIT", "EFFECT", "LFO", "MODULATION", "TABLE"},
    {"SONIDO", "SONIDO 2", "OPERADOR", "ENV OPERAD", "EDITAR SAMPLE", "EFECTO", "LFO", "MODULACION", "TABLA"}};
static const uint8_t SP_LIST_FM[8] = {SP_SON1, SP_SON2, SP_OP, SP_OPENV, SP_FX, SP_LFO, SP_MOD, SP_TABLE};
static const uint8_t SP_LIST_SMP[7] = {SP_SON1, SP_SON2, SP_SMP, SP_FX, SP_LFO, SP_MOD, SP_TABLE};
static const uint8_t SP_LIST[6] = {SP_SON1, SP_SON2, SP_FX, SP_LFO, SP_MOD, SP_TABLE};
/* an FM operator's knobs (OPERATOR, OP ENV): what they change in the patch is in kawa_edit.c (op_get, op_set) */
enum { OPK_LEVEL, OPK_RATIO, OPK_FINE, OPK_DET, OPK_ATT, OPK_DEC, OPK_SUS, OPK_REL, OPK_N };
static const char *const OPK_NAMES[LANG_N][OPK_N] = {
    {"NIVEAU", "RATIO", "FIN", "DESACC", "ATTAQUE", "DECLIN", "TENUE", "RELACHE"},
    {"LEVEL", "RATIO", "FINE", "DETUNE", "ATTACK", "DECAY", "SUSTAIN", "RELEASE"},
    {"NIVEL", "RATIO", "FINO", "DESAFIN", "ATAQUE", "CAIDA", "SOSTEN", "SUELTA"}};
static const int8_t OPK_LO[OPK_N] = {0, 0, 0, -7, 0, 0, 0, 0}, OPK_HI[OPK_N] = {99, 31, 99, 7, 99, 99, 99, 99};

/* ---- factory sounds: a name, SOUND 1-2 and the envelope (the mix, the effect and the LFO stay) */
typedef struct {
    const char *name[LANG_N];
    uint8_t engine;
    int8_t s[8], env[4];
    int8_t fenv;                                /* (Fm1Studio) Skiff's engines: ENV -> FILTER (-64..63), into ENV>EFF */
} preset_t;
static const preset_t PRESETS[] = {
    {{"LEAD 25", "LEAD 25", "LEAD 25"}, E_PULSE, {1, 0, 3, 15, 0, 0, 3, 0}, {0, 6, 10, 5}},
    {{"BIP 12", "BEEP 12", "BIP 12"}, E_PULSE, {0, 0, 0, 15, 0, 0, 3, 0}, {0, 4, 0, 3}},
    {{"CARRE 50", "SQUARE 50", "CUADRADA 50"}, E_PULSE, {2, 0, 0, 13, 0, 0, 3, 0}, {0, 8, 12, 5}},
    {{"SUPER CARRE", "SUPER PULSE", "SUPER PULSO"}, E_PULSE, {1, 0, 2, 14, 0, 9, 3, 0}, {0, 8, 11, 6}},
    {{"PLUCK", "PLUCK", "PLUCK"}, E_PULSE, {1, 0, 0, 12, 0, 0, 3, 0}, {0, 5, 0, 4}},
    {{"LASER", "LASER", "LASER"}, E_PULSE, {0, 12, 0, 15, 0, 0, 3, 0}, {0, 7, 0, 4}},
    {{"PWM PAD", "PWM PAD", "PWM PAD"}, E_PULSE, {4, 0, 2, 11, 0, 4, 3, 0}, {7, 9, 11, 9}},
    {{"BASSE", "BASS", "BAJO"}, E_PULSE, {2, 0, 0, 9, 0, 0, 3, 0}, {0, 7, 9, 3}},
    {{"SALE", "DIRTY", "SUCIO"}, E_PULSE, {1, 0, 0, 15, 5, 0, 3, 0}, {0, 6, 10, 4}},
    {{"BASSE NES", "NES BASS", "BAJO NES"}, E_WAVE, {0, 0, 0, 15, 0, 0, 3, 0}, {0, 8, 13, 3}},
    {{"BASSE GB", "GB BASS", "BAJO GB"}, E_WAVE, {4, 0, 0, 12, 0, 0, 3, 0}, {0, 6, 10, 3}},
    {{"SCIE", "SAW", "SIERRA"}, E_WAVE, {1, 0, 2, 13, 0, 6, 3, 0}, {0, 7, 9, 5}},
    {{"FLUTE", "FLUTE", "FLAUTA"}, E_WAVE, {2, 0, 4, 12, 0, 0, 3, 3}, {4, 8, 12, 7}},
    {{"ORGUE", "ORGAN", "ORGANO"}, E_WAVE, {5, 0, 0, 13, 0, 0, 3, 0}, {1, 9, 13, 6}},
    {{"VOIX", "VOICE", "VOZ"}, E_WAVE, {6, 0, 5, 12, 0, 3, 3, 0}, {3, 8, 11, 7}},
    {{"KICK TRI", "KICK TRI", "KICK TRI"}, E_WAVE, {0, 15, 0, 15, 0, 0, 3, 0}, {0, 4, 0, 2}},
    {{"PIANO TINE", "TINE EP", "PIANO TINE"}, E_FM, {0, 0, 0, 0, 0, 0, 0, 0}, {0, 15, 15, 5}},
    {{"CLOCHE", "BELL", "CAMPANA"}, E_FM, {1, 0, 0, 0, 0, 0, 0, 0}, {0, 15, 15, 9}},
    {{"BASSE FM", "FM BASS", "BAJO FM"}, E_FM, {2, 0, 0, 0, 0, 0, 0, 0}, {0, 15, 15, 3}},
    {{"CUIVRES", "BRASS", "METALES"}, E_FM, {3, 0, 0, 0, 0, 0, 0, 0}, {0, 15, 15, 5}},
    {{"NAPPE", "PAD", "COLCHON"}, E_FM, {4, 0, 0, 0, 0, 0, 3, 0}, {0, 15, 15, 9}},
    {{"MARIMBA", "MARIMBA", "MARIMBA"}, E_FM, {5, 0, 0, 0, 0, 0, 0, 0}, {0, 15, 15, 5}},
    {{"ORGUE FM", "FM ORGAN", "ORGANO FM"}, E_FM, {6, 0, 0, 0, 0, 0, 0, 0}, {0, 15, 15, 4}},
    {{"GUITARE", "PLUCK", "PUNTEO"}, E_FM, {7, 0, 0, 0, 0, 0, 0, 0}, {0, 15, 15, 4}},
    {{"LEAD CHIP", "CHIP LEAD", "LEAD CHIP"}, E_FM, {8, 0, 0, 0, 0, 0, 0, 4}, {0, 15, 15, 4}},
    {{"BASSE ACIDE", "ACID BASS", "BAJO ACIDO"}, E_FM, {9, 0, 0, 0, 0, 0, 0, 0}, {0, 15, 15, 2}},
    {{"HOOVER", "HOOVER", "HOOVER"}, E_FM, {10, 0, 0, 0, 0, 0, 0, 0}, {0, 15, 15, 6}},
    {{"KALIMBA", "KALIMBA", "KALIMBA"}, E_FM, {11, 0, 0, 0, 0, 0, 0, 0}, {0, 15, 15, 6}},
    {{"KIT ACOUSTIQUE", "ACOUSTIC KIT", "KIT ACUSTICO"}, E_KIT, {0, 0, 0, 0, 0, 15, 0, 0}, {0, 15, 15, 15}},
    {{"KIT HIP HOP", "HIP HOP KIT", "KIT HIP HOP"}, E_KIT, {1, 0, 0, 1, 0, 13, 0, 0}, {0, 15, 15, 15}},
    {{"KIT TECHNO", "TECHNO KIT", "KIT TECHNO"}, E_KIT, {2, 0, 0, 1, 0, 15, 0, 0}, {0, 15, 15, 15}},
    {{"KIT DUB REGGAE", "DUB REGGAE KIT", "KIT DUB REGGAE"}, E_KIT, {3, 0, 0, 0, 0, 13, 0, 0}, {0, 15, 15, 15}},
    {{"KIT LO-FI", "LO-FI KIT", "KIT LO-FI"}, E_KIT, {4, 0, 0, 1, 0, 11, 0, 0}, {0, 15, 15, 15}},
    {{"KIT GAME BOY", "GAME BOY KIT", "KIT GAME BOY"}, E_KIT, {5, 0, 0, 0, 0, 15, 0, 0}, {0, 15, 15, 15}},
    {{"KIT SNES", "SNES KIT", "KIT SNES"}, E_KIT, {6, 0, 0, 0, 0, 15, 0, 0}, {0, 15, 15, 15}},
    {{"KIT PERCUS", "PERCUSSION KIT", "KIT PERCUSION"}, E_KIT, {7, 0, 0, 0, 0, 15, 0, 0}, {0, 15, 15, 15}},
    {{"KIT GABBER", "GABBER KIT", "KIT GABBER"}, E_KIT, {2, 2, 2, 12, 0, 13, 0, 0}, {0, 15, 15, 15}},
    /* (Fm1Studio) Skiff 2.5's engines: their factory sounds (skiff/eng_*.c), the envelope on KAWA's scale */
    {{"SAW LEAD", "SAW LEAD", "SAW LEAD"}, E_ANALOG, {0, 12, 64, 0, 90, 30, 10, 64}, {1, 9, 12, 5}, 20},
    {{"SOFT PAD", "SOFT PAD", "SOFT PAD"}, E_ANALOG, {0, 20, 64, 4, 60, 10, 0, 32}, {11, 13, 13, 14}, 10},
    {{"SQR BASS", "SQR BASS", "SQR BASS"}, E_ANALOG, {1, 0, 0, 0, 50, 70, 40, 64}, {0, 7, 5, 3}, 30},
    {{"PWM STR", "PWM STR", "PWM STR"}, E_ANALOG, {4, 8, 40, 0, 75, 20, 0, 48}, {7, 11, 13, 12}, 8},
    {{"ACID", "ACID", "ACID"}, E_ANALOG, {0, 0, 0, 0, 50, 100, 25, 64}, {0, 6, 2, 3}, 48},
    {{"SINE KEY", "SINE KEY", "SINE KEY"}, E_ANALOG, {3, 6, 50, 0, 127, 0, 0, 0}, {1, 11, 4, 9}, 0},
    {{"RAVE", "RAVE", "RAVE"}, E_ANALOG, {4, 30, 64, 0, 85, 20, 30, 64}, {2, 11, 13, 7}, 10},
    {{"SUB BASS", "SUB BASS", "SUB BASS"}, E_ANALOG, {3, 0, 0, 0, 40, 0, 20, 0}, {0, 7, 12, 2}, 0},
    {{"SYNTH PLUCK", "SYNTH PLUCK", "SYNTH PLUCK"}, E_ANALOG, {0, 8, 50, 0, 30, 40, 0, 64}, {0, 12, 0, 7}, 55},
    {{"SYNTH BRASS", "SYNTH BRASS", "SYNTH BRASS"}, E_ANALOG, {0, 10, 64, 0, 45, 20, 10, 64}, {3, 9, 11, 4}, 40},
    {{"WIND SWEEP", "WIND SWEEP", "WIND SWEEP"}, E_ANALOG, {0, 0, 0, 90, 30, 90, 0, 0}, {7, 13, 7, 11}, 50},
    {{"STRINGS", "STRINGS", "STRINGS"}, E_ANALOG, {4, 6, 64, 0, 92, 8, 0, 48}, {5, 13, 14, 9}, 6},
    {{"BONE FLUTE", "BONE FLUTE", "BONE FLUTE"}, E_ANALOG, {2, 3, 30, 22, 84, 15, 0, 64}, {6, 11, 13, 8}, 6},
    {{"WAR HORN", "WAR HORN", "WAR HORN"}, E_ANALOG, {0, 5, 64, 4, 42, 12, 20, 64}, {9, 11, 13, 11}, 30},
    {{"DARK STRINGS", "DARK STRINGS", "DARK STRINGS"}, E_ANALOG, {1, 4, 64, 6, 40, 34, 22, 40}, {14, 13, 14, 15}, 10},
    {{"NIGHT BASS", "NIGHT BASS", "NIGHT BASS"}, E_ANALOG, {0, 7, 64, 0, 40, 35, 15, 64}, {0, 8, 6, 3}, 45},
    {{"NEON LEAD", "NEON LEAD", "NEON LEAD"}, E_ANALOG, {0, 14, 64, 0, 92, 25, 12, 64}, {1, 9, 13, 6}, 14},
    {{"SUNSET PAD", "SUNSET PAD", "SUNSET PAD"}, E_ANALOG, {4, 9, 55, 0, 74, 15, 0, 48}, {10, 13, 14, 13}, 10},
    {{"HERO BRASS", "HERO BRASS", "HERO BRASS"}, E_ANALOG, {0, 16, 64, 0, 50, 18, 8, 56}, {5, 10, 11, 6}, 44},
    {{"SYN TOM", "SYN TOM", "SYN TOM"}, E_ANALOG, {3, 0, 0, 6, 127, 0, 25, 0}, {0, 10, 0, 7}, 0},
    {{"QUANTA", "QUANTA", "QUANTA"}, E_ANALOG, {3, 0, 0, 0, 127, 0, 0, 0}, {0, 8, 15, 5}, 0},
    {{"REESE", "REESE", "REESE"}, E_ANALOG, {0, 40, 64, 0, 52, 15, 30, 32}, {0, 8, 15, 3}, 10},
    {{"DRIVE BASS", "DRIVE BASS", "DRIVE BASS"}, E_ANALOG, {0, 14, 64, 0, 62, 20, 10, 64}, {0, 8, 8, 2}, 24},
    {{"HARD SYNC", "HARD SYNC", "HARD SYNC"}, E_ANALOG, {5, 36, 96, 0, 88, 24, 12, 64}, {1, 9, 11, 4}, 16},
    {{"SUB SAW", "SUB SAW", "SUB SAW"}, E_ANALOG, {6, 0, 80, 0, 70, 22, 6, 64}, {0, 7, 13, 2}, 20},
    {{"PD BRASS", "PD BRASS", "PD BRASS"}, E_PHASE, {0, 0, 30, 90, 0, 0, 0, 0}, {1, 9, 11, 4}, 0},
    {{"PD ORGAN", "PD ORGAN", "PD ORGAN"}, E_PHASE, {3, 0, 40, 0, 0, 0, 0, 0}, {0, 15, 15, 3}, 0},
    {{"PD STRINGS", "PD STRINGS", "PD STRINGS"}, E_PHASE, {0, 4, 50, 40, 12, 0, 0, 0}, {4, 13, 12, 9}, 0},
    {{"RESO", "RESO", "RESO"}, E_PHASE, {5, 0, 60, 60, 0, 0, 0, 0}, {0, 9, 4, 7}, 0},
    {{"PD BELL", "PD BELL", "PD BELL"}, E_PHASE, {6, 0, 80, 50, 0, 0, 0, 0}, {0, 14, 0, 13}, 0},
    {{"WIRE", "WIRE", "WIRE"}, E_PHASE, {4, 7, 70, 40, 7, 0, 0, 0}, {1, 11, 9, 7}, 0},
    {{"NEUTRINO", "NEUTRINO", "NEUTRINO"}, E_PHASE, {3, 0, 20, 70, 0, 0, 0, 0}, {0, 11, 0, 10}, 0},
    {{"PD BASS", "PD BASS", "PD BASS"}, E_PHASE, {0, 0, 50, 80, 4, 0, 60, 0}, {0, 7, 7, 2}, 0},
    {{"SOFT KEYS", "SOFT KEYS", "SOFT KEYS"}, E_PHASE, {3, 0, 30, 60, 6, 0, 0, 0}, {0, 12, 4, 7}, 0},
    {{"PULSE LD", "PULSE LD", "PULSE LD"}, E_LOFI, {0, 0, 32, 0, 0, 20, 0, 127}, {0, 7, 11, 3}, 0},
    {{"WAVE BASS", "WAVE BASS", "WAVE BASS"}, E_LOFI, {1, 1, 0, 0, 0, 0, 0, 90}, {0, 5, 8, 2}, 0},
    {{"ARP 8BIT", "ARP 8BIT", "ARP 8BIT"}, E_LOFI, {2, 0, 96, 0, 0, 0, 2, 110}, {0, 7, 9, 4}, 0},
    {{"WAVE LEAD", "WAVE LEAD", "WAVE LEAD"}, E_LOFI, {0, 4, 92, 0, 0, 18, 0, 110}, {0, 9, 11, 3}, 0},
    {{"STEP LEAD", "STEP LEAD", "STEP LEAD"}, E_LOFI, {4, 0, 40, 34, 0, 16, 0, 127}, {0, 8, 15, 6}, 0},
    {{"PHOTON", "PHOTON", "PHOTON"}, E_LOFI, {2, 1, 64, 0, 16, 0, 0, 120}, {0, 9, 0, 4}, 0},
    {{"TRI BASS", "TRI BASS", "TRI BASS"}, E_LOFI, {0, 1, 64, 0, 0, 0, 0, 100}, {0, 7, 11, 2}, 0},
    {{"CHIP BELL", "CHIP BELL", "CHIP BELL"}, E_LOFI, {2, 0, 20, 0, 30, 0, 0, 110}, {0, 9, 0, 5}, 0},
    {{"CHOIR AAH", "CHOIR AAH", "CHOIR AAH"}, E_VOICE, {0, 0, 0, 0, 40, 22, 60, 0}, {12, 13, 14, 14}, 0},
    {{"VOX LEAD", "VOX LEAD", "VOX LEAD"}, E_VOICE, {32, 0, 0, 2, 90, 8, 72, 0}, {1, 9, 12, 6}, 0},
    {{"WOW BASS", "WOW BASS", "WOW BASS"}, E_VOICE, {95, 0, 68, 0, 100, 0, 80, 0}, {0, 9, 8, 3}, 0},
    {{"WHISPER", "WHISPER", "WHISPER"}, E_VOICE, {0, 95, 88, 3, 50, 120, 50, 0}, {5, 13, 13, 13}, 0},
    {{"CRYPT CHOIR", "CRYPT CHOIR", "CRYPT CHOIR"}, E_VOICE, {112, 0, 0, -3, 34, 30, 64, 14}, {13, 13, 14, 14}, 0},
    {{"MONK CHANT", "MONK CHANT", "MONK CHANT"}, E_VOICE, {95, 0, 96, -5, 80, 12, 80, 0}, {5, 11, 14, 12}, 0},
    {{"DREAM CHOIR", "DREAM CHOIR", "DREAM CHOIR"}, E_VOICE, {0, 20, 0, 3, 55, 45, 55, 8}, {11, 13, 14, 14}, 0},
    {{"ROBOT TALK", "ROBOT TALK", "ROBOT TALK"}, E_VOICE, {20, 100, 40, -12, 110, 4, 90, 30}, {0, 9, 12, 3}, 0},
    {{"FAT BASS", "FAT BASS", "FAT BASS"}, E_TRIO, {0, 0, -12, 9, 0, 62, 45, 64}, {0, 7, 7, 2}, 40},
    {{"ARP LEAD", "ARP LEAD", "ARP LEAD"}, E_TRIO, {2, 12, 0, 4, 0, 88, 25, 32}, {0, 7, 11, 3}, 8},
    {{"SYNC LEAD", "SYNC LEAD", "SYNC LEAD"}, E_TRIO, {9, 9, 0, 0, 0, 82, 30, 64}, {1, 9, 12, 4}, 18},
    {{"RING BELL", "RING BELL", "RING BELL"}, E_TRIO, {13, 0, 18, 6, 1, 96, 30, 64}, {0, 13, 0, 11}, 0},
    {{"CHIP CHOIR", "CHIP CHOIR", "CHIP CHOIR"}, E_TRIO, {1, 0, 12, 7, 1, 62, 95, 40}, {9, 13, 13, 12}, 30},
    {{"TURBO SYNC", "TURBO SYNC", "TURBO SYNC"}, E_TRIO, {9, 7, 0, 3, 0, 92, 28, 64}, {1, 9, 12, 4}, 20},
    {{"NEON ARP", "NEON ARP", "NEON ARP"}, E_TRIO, {1, 12, 0, 5, 0, 70, 35, 40}, {0, 8, 0, 4}, 36},
    {{"COLLIDER", "COLLIDER", "COLLIDER"}, E_TRIO, {13, 0, 23, 9, 2, 60, 50, 64}, {0, 9, 0, 6}, 0},
    {{"DIRTY BASS", "DIRTY BASS", "DIRTY BASS"}, E_TRIO, {1, -12, 0, 8, 1, 62, 85, 30}, {0, 5, 6, 2}, 35},
    {{"FULL ORGAN", "FULL ORGAN", "FULL ORGAN"}, E_WHEEL, {15, 0, 0, 0, 0, 30, 20, 1}, {0, 8, 15, 4}, 0},
    {{"JAZZ PERC", "JAZZ PERC", "JAZZ PERC"}, E_WHEEL, {4, 0, 0, 0, 2, 50, 8, 1}, {0, 8, 15, 4}, 0},
    {{"GOSPEL", "GOSPEL", "GOSPEL"}, E_WHEEL, {6, 0, 0, 0, 1, 60, 40, 2}, {0, 8, 15, 4}, 0},
    {{"SOFT FLUTE", "SOFT FLUTE", "SOFT FLUTE"}, E_WHEEL, {1, 0, -2, 0, 0, 10, 0, 1}, {0, 8, 15, 6}, 0},
    {{"ROCK DRIVE", "ROCK DRIVE", "ROCK DRIVE"}, E_WHEEL, {7, 0, 0, 0, 0, 70, 100, 2}, {0, 8, 15, 4}, 0},
    {{"CATHEDRAL", "CATHEDRAL", "CATHEDRAL"}, E_WHEEL, {12, 2, 0, 0, 0, 6, 0, 0}, {5, 8, 15, 9}, 0},
    {{"PEDAL BASS", "PEDAL BASS", "PEDAL BASS"}, E_WHEEL, {14, 4, 0, -4, 0, 20, 10, 0}, {0, 8, 15, 3}, 0},
    {{"CLARINET", "CLARINET", "CLARINET"}, E_WHEEL, {9, 0, 2, -5, 0, 0, 14, 1}, {2, 8, 14, 4}, 0},
    {{"BELL TREE", "BELL TREE", "BELL TREE"}, E_PHYS, {0, 70, 100, 106, 12, 100, 0, 6}, {0, 14, 15, 14}, 0},
    {{"MARIMBA", "MARIMBA", "MARIMBA"}, E_PHYS, {0, 116, 38, 40, 24, 90, 0, 30}, {0, 13, 15, 6}, 0},
    {{"PLUCK", "PLUCK", "PLUCK"}, E_PHYS, {1, 56, 72, 84, 40, 96, 0, 0}, {0, 14, 15, 7}, 0},
    {{"BOWED METAL", "BOWED METAL", "BOWED METAL"}, E_PHYS, {0, 98, 70, 112, 10, 80, 90, 0}, {7, 13, 15, 13}, 0},
    {{"KALIMBA", "KALIMBA", "KALIMBA"}, E_PHYS, {0, 34, 16, 56, 100, 90, 0, 48}, {0, 14, 15, 7}, 0},
    {{"HAND DRUM", "HAND DRUM", "HAND DRUM"}, E_PHYS, {2, 112, 76, 56, 44, 100, 14, 24}, {0, 14, 15, 8}, 0},
    {{"TOMS", "TOMS", "TOMS"}, E_PHYS, {2, 0, 52, 40, 30, 100, 40, 30}, {0, 14, 15, 9}, 0},
    {{"DRONE STRING", "DRONE STRING", "DRONE STRING"}, E_PHYS, {3, 127, 80, 88, 64, 100, 92, 0}, {0, 14, 15, 13}, 0},
    {{"HARP", "HARP", "HARP"}, E_PHYS, {3, 8, 62, 74, 60, 90, 0, 0}, {0, 14, 15, 11}, 0},
    {{"HARPSICHORD", "HARPSICHORD", "HARPSICHORD"}, E_PHYS, {1, 44, 112, 76, 10, 127, 0, 24}, {0, 14, 15, 5}, 0},
    {{"UPRIGHT", "UPRIGHT", "UPRIGHT"}, E_PHYS, {1, 60, 60, 90, 20, 127, 0, 60}, {0, 14, 15, 4}, 0},
    /* (Gabare) JIANT 0.9.9's STRING and ADD: their factory sounds (in JIANT's browsing order), the envelope on KAWA's scale */
    {{"NYLON", "NYLON", "NYLON"}, E_STRING, {1, 72, 52, 28, 0, 60, 40, 0}, {0, 8, 15, 15}, 0},
    {{"STEEL GTR", "STEEL GTR", "STEEL GTR"}, E_STRING, {2, 86, 88, 24, 6, 40, 52, 0}, {0, 8, 15, 15}, 0},
    {{"HARP", "HARP", "HARP"}, E_STRING, {1, 96, 62, 64, 0, 30, 30, 0}, {0, 8, 15, 15}, 0},
    {{"KOTO", "KOTO", "KOTO"}, E_STRING, {2, 78, 98, 14, 16, 30, 62, 0}, {0, 8, 15, 15}, 0},
    {{"BANJO", "BANJO", "BANJO"}, E_STRING, {2, 52, 112, 10, 4, 90, 92, 0}, {0, 8, 15, 14}, 0},
    {{"PIANO", "PIANO", "PIANO"}, E_STRING, {3, 104, 76, 18, 42, 20, 22, 0}, {0, 8, 15, 13}, 0},
    {{"DULCIMER", "DULCIMER", "DULCIMER"}, E_STRING, {3, 92, 100, 20, 26, 36, 60, 0}, {0, 8, 15, 15}, 0},
    {{"MARIMBA", "MARIMBA", "MARIMBA"}, E_STRING, {4, 56, 40, 64, 72, 80, 70, 0}, {0, 8, 15, 11}, 0},
    {{"PLUCK BASS", "PLUCK BASS", "PLUCK BASS"}, E_STRING, {1, 62, 36, 20, 0, 50, 10, 0}, {0, 8, 15, 11}, 0},
    {{"BOWED", "BOWED", "BOWED"}, E_STRING, {0, 118, 46, 30, 0, 50, 40, 70}, {3, 8, 15, 14}, 0},
    {{"EBOW", "EBOW", "EBOW"}, E_STRING, {5, 127, 44, 40, 50, 20, 64, 90}, {2, 8, 8, 14}, 0},
    {{"FEEDBACK", "FEEDBACK", "FEEDBACK"}, E_STRING, {5, 90, 70, 24, 60, 70, 40, 60}, {0, 8, 15, 13}, 0},
    {{"SHOEGAZE", "SHOEGAZE", "SHOEGAZE"}, E_STRING, {5, 110, 50, 30, 55, 100, 90, 70}, {4, 8, 15, 15}, 0},
    {{"SQUEAL", "SQUEAL", "SQUEAL"}, E_STRING, {5, 80, 96, 16, 70, 110, 12, 30}, {0, 8, 15, 11}, 0},
    {{"METAL STR", "METAL STR", "METAL STR"}, E_STRING, {0, 100, 120, 10, 112, 0, 0, 0}, {0, 8, 15, 15}, 0},
    {{"INIT STRING", "INIT STRING", "INIT STRING"}, E_STRING, {1, 72, 60, 28, 0, 40, 40, 0}, {0, 8, 15, 13}, 0},
    {{"ADD ORGAN", "ADD ORGAN", "ADD ORGAN"}, E_ADD, {0, 0, 76, 127, 0, 0, 127, 0}, {0, 8, 15, 4}, 0},
    {{"PRIME ORG", "PRIME ORG", "PRIME ORG"}, E_ADD, {6, 0, 84, 127, 0, 0, 127, 0}, {1, 8, 15, 4}, 0},
    {{"ADD SAW", "ADD SAW", "ADD SAW"}, E_ADD, {0, 0, 92, 127, 0, 0, 127, 0}, {1, 9, 12, 3}, 30},
    {{"HOLLOW", "HOLLOW", "HOLLOW"}, E_ADD, {1, 0, 84, 127, 0, 30, 127, 0}, {3, 9, 14, 9}, 0},
    {{"AIR PAD", "AIR PAD", "AIR PAD"}, E_ADD, {2, 64, 100, 127, -10, 20, 127, 50}, {8, 8, 15, 11}, 0},
    {{"QUINT", "QUINT", "QUINT"}, E_ADD, {7, 0, 80, 127, 0, 24, 127, 20}, {1, 8, 15, 5}, 0},
    {{"BELL", "BELL", "BELL"}, E_ADD, {3, 0, 100, 127, 0, 0, 74, 0}, {0, 8, 15, 9}, 0},
    {{"VIBES", "VIBES", "VIBES"}, E_ADD, {4, 0, 72, 127, 0, 0, 76, 0}, {0, 8, 15, 6}, 0},
    {{"GLASS", "GLASS", "GLASS"}, E_ADD, {2, 0, 118, 127, -20, 0, 80, 30}, {0, 8, 15, 9}, 0},
    {{"SUB ADD", "SUB ADD", "SUB ADD"}, E_ADD, {1, 0, 20, 40, 0, 0, 127, 0}, {0, 8, 9, 2}, 0},
    {{"SKIN TOM", "SKIN TOM", "SKIN TOM"}, E_ADD, {5, 0, 92, 127, 0, 0, 46, 0}, {0, 8, 15, 3}, 0},
    {{"SWELL", "SWELL", "SWELL"}, E_ADD, {0, 0, 104, 127, 0, 0, 127, -50}, {2, 8, 15, 7}, 0},
    {{"DRONE", "DRONE", "DRONE"}, E_ADD, {8, 50, 70, 127, 0, 30, 127, 40}, {4, 8, 15, 13}, 0},
    {{"GONG", "GONG", "GONG"}, E_ADD, {9, 30, 104, 127, -12, 0, 88, 30}, {0, 8, 15, 13}, 0},
    {{"RADIO", "RADIO", "RADIO"}, E_ADD, {0, 0, 96, 127, 40, 0, 127, 20}, {0, 8, 15, 11}, 0},
    {{"INIT ADD", "INIT ADD", "INIT ADD"}, E_ADD, {0, 0, 70, 127, 0, 0, 127, 0}, {0, 8, 15, 4}, 0},
    /* (Gabare 0.9) NOISE (SLOOP 2.8.5's, Felucca's): BRUIT's six first, then SLOOP's sounds */
    {{"CHARLEY", "HI-HAT", "CHARLES"}, E_NOISE, {0, 0, 118, 10, 0, 0, 0, 0}, {0, 2, 0, 2}, 0},
    {{"CAISSE", "SNARE", "CAJA"}, E_NOISE, {0, 20, 90, 30, 64, 0, 0, 0}, {0, 5, 0, 4}, 0},
    {{"METAL", "METAL", "METAL"}, E_NOISE, {3, 0, 100, 40, 100, 32, 0, 0}, {0, 4, 0, 3}, 0},
    {{"VAPEUR", "STEAM", "VAPOR"}, E_NOISE, {0, 10, 100, 20, 0, 0, 40, 0}, {8, 10, 8, 9}, 0},
    {{"EXPLOSION", "EXPLOSION", "EXPLOSION"}, E_NOISE, {0, 110, 50, 20, 0, 0, 0, 30}, {0, 12, 0, 10}, 0},
    {{"CRASH", "CRASH", "CRASH"}, E_NOISE, {0, 0, 110, 5, 0, 30, 0, 0}, {0, 11, 0, 10}, 0},
    {{"WIND", "WIND", "WIND"}, E_NOISE, {0, 64, 70, 100, 127, 0, 70, 0}, {13, 13, 13, 13}, 0},
    {{"RAIN", "RAIN", "RAIN"}, E_NOISE, {1, 30, 88, 80, 64, 105, 40, 0}, {1, 13, 15, 9}, 0},
    {{"NZ ARCADE", "NZ ARCADE", "NZ ARCADE"}, E_NOISE, {2, 0, 127, 0, 127, 112, 0, 84}, {0, 11, 0, 5}, 0},
    {{"NZ METAL", "NZ METAL", "NZ METAL"}, E_NOISE, {3, 0, 100, 40, 100, 32, 0, 0}, {0, 10, 5, 7}, 10},
    {{"OCEAN", "OCEAN", "OCEAN"}, E_NOISE, {0, 100, 50, 20, 0, 0, 110, 0}, {14, 13, 15, 14}, 0},
    {{"VINYL", "VINYL", "VINYL"}, E_NOISE, {1, 0, 110, 10, 0, 70, 20, 0}, {0, 8, 15, 4}, 0},
    {{"HISS", "HISS", "HISS"}, E_NOISE, {0, 0, 115, 0, 0, 0, 0, 0}, {2, 8, 15, 4}, 0},
    {{"RISER", "RISER", "RISER"}, E_NOISE, {0, 30, 60, 110, 127, 0, 0, 0}, {14, 13, 15, 7}, 0},
    {{"NZ SNARE", "NZ SNARE", "NZ SNARE"}, E_NOISE, {0, 20, 90, 30, 64, 0, 0, 0}, {0, 7, 0, 4}, 0},
    {{"BITCRUSH", "BITCRUSH", "BITCRUSH"}, E_NOISE, {0, 40, 90, 40, 64, 0, 0, 100}, {0, 9, 13, 4}, 0},
    {{"RADIO", "RADIO", "RADIO"}, E_NOISE, {2, 20, 70, 90, 64, 0, 60, 48}, {1, 9, 14, 5}, 0},
    /* (Gabare 0.9) DIGITAL, FM4 on the screen (SLOOP 2.8.5's, Felucca's four-operator FM) */
    {{"RHODES", "RHODES", "RHODES"}, E_DIGI, {4, 1, 1, 14, 26, 35, 0, 0}, {0, 12, 4, 6}, 0},
    {{"DX RHODES", "DX RHODES", "DX RHODES"}, E_DIGI, {4, 1, 1, 14, 85, 45, 0, 0}, {0, 11, 5, 6}, 0},
    {{"WURLI", "WURLI", "WURLI"}, E_DIGI, {4, 1, 2, 3, 62, 35, 8, 0}, {0, 10, 4, 4}, 0},
    {{"CLAV", "CLAV", "CLAV"}, E_DIGI, {3, 1, 3, 5, 90, 25, 20, 0}, {0, 7, 4, 2}, 0},
    {{"M1 PIANO", "M1 PIANO", "M1 PIANO"}, E_DIGI, {4, 1, 2, 13, 74, 30, 12, 0}, {0, 12, 4, 5}, 0},
    {{"AFRO KEYS", "AFRO KEYS", "AFRO KEYS"}, E_DIGI, {4, 1, 1, 7, 40, 50, 0, 0}, {0, 11, 5, 7}, 0},
    {{"TRAP BELL", "TRAP BELL", "TRAP BELL"}, E_DIGI, {4, 8, 1, 4, 80, 70, 0, 0}, {0, 13, 0, 9}, 0},
    {{"MUSIC BOX", "MUSIC BOX", "MUSIC BOX"}, E_DIGI, {4, 4, 3, 9, 45, 75, 0, 0}, {0, 13, 0, 10}, 0},
    {{"MARIMBA", "MARIMBA", "MARIMBA"}, E_DIGI, {4, 4, 1, 1, 60, 30, 0, 0}, {0, 11, 0, 7}, 0},
    {{"KALIMBA", "KALIMBA", "KALIMBA"}, E_DIGI, {4, 4, 1, 6, 52, 24, 0, 0}, {0, 11, 0, 7}, 0},
    {{"PLUGG BELL", "PLUGG BELL", "PLUGG BELL"}, E_DIGI, {4, 3, 1, 4, 56, 60, 0, 0}, {0, 13, 0, 11}, 0},
    {{"GLASS PAD", "GLASS PAD", "GLASS PAD"}, E_DIGI, {4, 2, 1, 5, 34, 100, 0, 0}, {7, 13, 13, 13}, 0},
    {{"FM BASS", "FM BASS", "FM BASS"}, E_DIGI, {0, 1, 1, 1, 34, 40, 8, 0}, {0, 7, 8, 3}, 0},
    {{"STEEL DRUM", "STEEL DRUM", "STEEL DRUM"}, E_DIGI, {4, 2, 2, 2, 45, 45, 0, 0}, {0, 13, 0, 8}, 0},
    {{"GLOCKEN", "GLOCKEN", "GLOCKEN"}, E_DIGI, {4, 5, 1, 8, 40, 60, 0, 0}, {0, 14, 0, 12}, 0},
    {{"CELESTA", "CELESTA", "CELESTA"}, E_DIGI, {4, 2, 1, 4, 30, 50, 0, 0}, {0, 12, 0, 9}, 0},
    {{"XYLOPHONE", "XYLOPHONE", "XYLOPHONE"}, E_DIGI, {4, 4, 3, 3, 70, 18, 0, 0}, {0, 10, 0, 7}, 0},
    {{"HARP", "HARP", "HARP"}, E_DIGI, {4, 1, 1, 2, 45, 30, 10, 0}, {0, 13, 0, 11}, 0},
    {{"BASS GTR", "BASS GTR", "BASS GTR"}, E_DIGI, {4, 1, 1, 2, 40, 30, 6, 0}, {0, 10, 5, 2}, 0},
    {{"TUBE BELL", "TUBE BELL", "TUBE BELL"}, E_DIGI, {4, 6, 1, 13, 50, 90, 0, 0}, {0, 14, 0, 13}, 0},
    {{"FM PLUCK", "FM PLUCK", "FM PLUCK"}, E_DIGI, {4, 2, 1, 3, 70, 15, 0, 0}, {0, 11, 0, 6}, 0},
    {{"FM GRAND", "FM GRAND", "FM GRAND"}, E_DIGI, {4, 1, 2, 12, 60, 40, 10, 0}, {0, 13, 2, 7}, 0},
};
#define KIT_DEMO 6                              /* the demo's kit (its first knob): SNES */
#define NPRESET ((uint8_t)(sizeof(PRESETS) / sizeof(PRESETS[0])))
/* each track's sound on a new project (an index in PRESETS) */
static const uint8_t TRK_ENGINE0[NTRK] = {E_PULSE, E_FM, E_WAVE, E_NOISE, E_KIT, E_KIT};   /* ((Gabare 0.9) 6: percussion) */   /* a new project: lead, */
static const int8_t TRK_S0[NTRK] = {1, 0, 0, 0, 0, 0};   /* FM, bass, hats, kit, break (each its engine's sound with this */
                                                          /* first knob: LEAD 25, TINE EP, NES BASS, HI-HAT, ACOUSTIC, SOLEIL) */

/* ---- notes */
static const char *const NOTE_NAME[12] = {"C-", "C#", "D-", "D#", "E-", "F-", "F#", "G-", "G#", "A-", "A#", "B-"};
#define NOTE_NONE 0u
#define NOTE_OFF 255u
#define KEY0 53u                                /* key 0 = F3 */
static int is_black(uint32_t note) { return (0x54Au >> (note % 12u)) & 1u; }
static int8_t WHITE_OF_KEY[27], BLACK_OF_KEY[27];    /* key -> its index among the white / black keys, or -1 */
static uint8_t KEY_OF_WHITE[16], KEY_OF_BLACK[11];
static void keys_init(void)
{
    uint32_t k, w = 0, b = 0;
    for (k = 0; k < 27u; k++) {
        WHITE_OF_KEY[k] = BLACK_OF_KEY[k] = -1;
        if (is_black(KEY0 + k)) { BLACK_OF_KEY[k] = (int8_t)b; KEY_OF_BLACK[b++] = (uint8_t)k; }
        else { WHITE_OF_KEY[k] = (int8_t)w; KEY_OF_WHITE[w++] = (uint8_t)k; }
    }
}

/* ---- the cell tricks (FX column): a type and a value */
enum { FX_NONE, FX_ARP, FX_GLIDE, FX_VIB, FX_ROLL, FX_BEND, FX_CHANCE, FX_DELAY, FX_CUT, FX_REV, FX_PITCH, FX_SHAPE,
       FX_CRUSH, FX_VSL, FX_PAN, FX_OFS, FX_HOP, FX_END, FX_CHORD, FX_N };   /* (HOP and END: the sequencer's, format 5;
                                                                                * (Gabare) CHORD: ACC, 0.6) */
typedef struct {
    const char *tag[LANG_N];                    /* three letters */
    uint8_t icon;
    int8_t lo, hi, def;
} fxdesc_t;
static const fxdesc_t FXD[FX_N] = {
    {{"---", "---", "---"}, ICO_ERASE, 0, 0, 0},
    {{"ARP", "ARP", "ARP"}, ICO_ARP, 0, 13, 1},             /* arpeggio */
    {{"GLI", "GLI", "GLI"}, ICO_GLIDE, 1, 15, 6},           /* glide */
    {{"VIB", "VIB", "VIB"}, ICO_VIB, 1, 15, 5},             /* vibrato */
    {{"ROL", "ROL", "RED"}, ICO_ROLL, 0, 8, 2},             /* roll (redoble) */
    {{"BND", "BND", "BND"}, ICO_BEND, -15, 15, -6},         /* bend */
    {{"CHN", "CHN", "AZR"}, ICO_DICE, 1, 9, 5},             /* chance (azar) */
    {{"RET", "DLY", "RET"}, ICO_CLOCK, 1, 23, 12},          /* delay (retard, retraso) */
    {{"CUT", "CUT", "CRT"}, ICO_CUT, 1, 23, 12},            /* cut (coupe, corte) */
    {{"ENV", "REV", "REV"}, ICO_REV, 0, 0, 0},              /* reverse (envers, reves) */
    {{"HAU", "PIT", "ALT"}, ICO_SHARP, -12, 12, 12},        /* pitch (hauteur, altura) */
    {{"FOR", "SHP", "FOR"}, ICO_SHAPE, 0, 7, 0},            /* shape (forme, forma) */
    {{"ECR", "CRU", "TRI"}, ICO_CRUSH, 1, 7, 4},            /* crush (ecrase, tritura) */
    {{"VOL", "VSL", "VOL"}, ICO_VOL, -15, 15, -4},          /* volume slide over the row (an empty cell: the note playing) */
    {{"PAN", "PAN", "PAN"}, ICO_MIRROR, -7, 7, -5},         /* this note's pan */
    {{"DEP", "OFS", "INI"}, ICO_RIGHT, 0, 15, 8},           /* SAMPLE: start further in (n / 16 of the slice; depart, inicio) */
    {{"SAU", "HOP", "SAL"}, ICO_LOOP, 0, 31, 0},            /* after this row, row n + 1 (saut, salto) */
    {{"FIN", "END", "FIN"}, ICO_STOP, 0, 0, 0},             /* the pattern ends after this row */
    {{"ACC", "CHD", "ACO"}, ICO_UNI, 0, 13, 0},             /* (Gabare) a chord, its notes together (CHORDS; accord, acorde) */
};
/* the tricks of a table's steps (TABLE): each acts on the note playing when its step comes */
enum { TB_NONE, TB_HIT, TB_CUT, TB_VIB, TB_BEND, TB_SHAPE, TB_CRUSH, TB_VSL, TB_PAN, TB_HOP, TB_N };
static const fxdesc_t TBD[TB_N] = {
    {{"---", "---", "---"}, ICO_ERASE, 0, 0, 0},
    {{"FRP", "HIT", "GOL"}, ICO_AGAIN, 0, 0, 0},            /* the note again (frappe, golpe), its step's pitch */
    {{"CUT", "CUT", "CRT"}, ICO_CUT, 0, 0, 0},              /* the note stops */
    {{"VIB", "VIB", "VIB"}, ICO_VIB, 0, 15, 5},             /* vibrato from here (0: none) */
    {{"BND", "BND", "BND"}, ICO_BEND, -15, 15, -6},         /* the pitch bends from here (0: no more) */
    {{"FOR", "SHP", "FOR"}, ICO_SHAPE, 0, 7, 0},            /* the wave's shape */
    {{"ECR", "CRU", "TRI"}, ICO_CRUSH, 0, 7, 4},            /* crush (0: none) */
    {{"VOL", "VSL", "VOL"}, ICO_VOL, -15, 15, -4},          /* the volume slides (0: it stops) */
    {{"PAN", "PAN", "PAN"}, ICO_MIRROR, -7, 7, -5},         /* the note's pan */
    {{"SAU", "HOP", "SAL"}, ICO_LOOP, 0, 15, 0},            /* to step n + 1 (after this one plays its time) */
};
static const uint8_t GROOVE_PRESET[6][17] = {   /* the groove's presets (GROOVE, KNOB 3): length, then the ticks */
    {2, 6, 6}, {2, 7, 5}, {2, 8, 4}, {3, 4, 4, 4}, {4, 7, 5, 6, 6}, {4, 9, 3, 8, 4}};
static const char *const GROOVE_NAMES[LANG_N][6] = {
    {"DROIT", "SWING 58", "SWING 67", "TRIOLET", "DECALE", "SHUFFLE"},
    {"STRAIGHT", "SWING 58", "SWING 67", "TRIPLET", "LAZY", "SHUFFLE"},
    {"RECTO", "SWING 58", "SWING 67", "TRESILLO", "PEREZOSO", "SHUFFLE"}};
static const uint8_t ROLL_N[9] = {2, 3, 4, 6, 8, 12, 16, 24, 32};
typedef struct {
    const char *name[LANG_N];
    int8_t n, off[4];
} chord_t;
static const chord_t CHORDS[14] = {
    {{"MAJ", "MAJ", "MAY"}, 3, {0, 4, 7, 0}}, {{"MIN", "MIN", "MEN"}, 3, {0, 3, 7, 0}}, {{"OCT", "OCT", "OCT"}, 2, {0, 12, 0, 0}},
    {{"5TE", "5TH", "5TA"}, 2, {0, 7, 0, 0}}, {{"SUS", "SUS", "SUS"}, 3, {0, 5, 7, 0}}, {{"7", "7", "7"}, 4, {0, 4, 7, 10}},
    {{"MA7", "MA7", "MA7"}, 4, {0, 4, 7, 11}}, {{"MI7", "MI7", "ME7"}, 4, {0, 3, 7, 10}}, {{"DIM", "DIM", "DIS"}, 3, {0, 3, 6, 0}},
    {{"AUG", "AUG", "AUM"}, 3, {0, 4, 8, 0}}, {{"2OC", "2OC", "2OC"}, 3, {0, 12, 24, 0}}, {{"AD9", "AD9", "AD9"}, 4, {0, 4, 7, 14}},
    {{"TRI", "TRL", "TRI"}, 2, {0, 1, 0, 0}}, {{"BAS", "LOW", "BAJ"}, 2, {0, -12, 0, 0}},
};
/* (Gabare) ACC (FX_CHORD): a chord's notes played together on one track, sharing its envelope, volume and effect
 * (kawa_snd.c track_chunk, kawa_sk.c). CHORD_CUSTOM: a chord a computer gives (chord_custom, kawa_bridge.c). The sum of
 * n notes is brought down by 1/sqrt(n) */
#define CHORD_MAX 4u
#define CHORD_CUSTOM 127
static const int32_t CHORD_GAIN[CHORD_MAX] = {32767, 23170, 18919, 16384};

/* ---- scales (bit n: semitone n above the root) */
typedef struct {
    const char *name[LANG_N];
    uint16_t mask;
} scale_t;
static const scale_t SCALES[12] = {
    {{"CHROMA", "CHROMATIC", "CROMATICA"}, 0xFFF}, {{"MAJEUR", "MAJOR", "MAYOR"}, 0xAB5},
    {{"MINEUR", "MINOR", "MENOR"}, 0x5AD}, {{"PENTA", "PENTA", "PENTA"}, 0x295},
    {{"PENTA MIN", "PENTA MIN", "PENTA MEN"}, 0x4A9}, {{"BLUES", "BLUES", "BLUES"}, 0x4E9},
    {{"DORIEN", "DORIAN", "DORICA"}, 0x6AD}, {{"PHRYGIEN", "PHRYGIAN", "FRIGIA"}, 0x5AB},
    {{"HARMONIQ", "HARMONIC", "ARMONICA"}, 0x9AD}, {{"JAPON", "JAPANESE", "JAPONESA"}, 0x1A3},
    {{"ARABE", "ARABIC", "ARABE"}, 0x9B3}, {{"PAR TONS", "WHOLE TONE", "TONOS"}, 0x555},
};

/* ---- live effects: hold FX (or tap it to keep the page open), then the white keys. Two banks of 16:
 * ids 0..31 (bit id of a mask); the LIVE lane of a pattern stores id + 1 (0: none). They play on the
 * master or on one track (FX page, KNOB 4: TARGET) */
enum { LF_LOOP16, LF_LOOP12, LF_LOOP32, LF_LOOP64, LF_UNI, LF_UNILO, LF_OCTUP, LF_OCTDN,
       LF_STUT4, LF_STUT3, LF_SCR, LF_SCRF, LF_TRIP, LF_AGAIN, LF_REV, LF_ROLLUP,
       LF_CRUSH, LF_DECIM, LF_LPF, LF_HPF, LF_TAPE, LF_DIVE, LF_GATE16, LF_GATE32,
       LF_FREEZE, LF_RING, LF_ECHO, LF_DIST, LF_WOB, LF_VINYL, LF_NOMEL, LF_NOBEAT, LF_N };
typedef struct {
    const char *name[LANG_N], *sub[LANG_N];
    uint8_t icon;
} lfx_t;
static const lfx_t LFX[LF_N] = {
    {{"BOUCLE", "LOOP", "BUCLE"}, {"1/16", "1/16", "1/16"}, ICO_LOOP},
    {{"BOUCLE", "LOOP", "BUCLE"}, {"1/12", "1/12", "1/12"}, ICO_LOOP},
    {{"BOUCLE", "LOOP", "BUCLE"}, {"1/32", "1/32", "1/32"}, ICO_LOOP},
    {{"BOUCLE", "LOOP", "BUCLE"}, {"1/64", "1/64", "1/64"}, ICO_LOOP},
    {{"UNISSON", "UNISON", "UNISONO"}, {"", "", ""}, ICO_UNI},
    {{"UNISSON", "UNISON", "UNISONO"}, {"BAS", "LOW", "GRAVE"}, ICO_UNI},
    {{"OCTAVE", "OCTAVE", "OCTAVA"}, {"+1", "+1", "+1"}, ICO_UP},
    {{"OCTAVE", "OCTAVE", "OCTAVA"}, {"-1", "-1", "-1"}, ICO_DOWN},
    {{"BEGAIE", "STUTTER", "TARTAMU"}, {"X4", "X4", "X4"}, ICO_STUT},
    {{"BEGAIE", "STUTTER", "TARTAMU"}, {"X3", "X3", "X3"}, ICO_STUT},
    {{"SCRATCH", "SCRATCH", "SCRATCH"}, {"", "", ""}, ICO_DISC},
    {{"SCRATCH", "SCRATCH", "SCRATCH"}, {"VITE", "FAST", "RAPIDO"}, ICO_DISC},
    {{"6/8", "6/8", "6/8"}, {"TRIOLET", "TRIPLET", "SWING"}, ICO_TRI3},
    {{"RELANCE", "RETRIG", "REPITE"}, {"1 TEMPS", "1 BEAT", "1 PULSO"}, ICO_AGAIN},
    {{"ENVERS", "REVERSE", "REVES"}, {"", "", ""}, ICO_REV},
    {{"ROULEAU", "BUILDUP", "REDOBLE"}, {"ACCEL", "ROLL", "ACELERA"}, ICO_ACCEL},
    {{"ECRASE", "CRUSH", "TRITURA"}, {"4 BITS", "4 BITS", "4 BITS"}, ICO_CRUSH},
    {{"DECIME", "LO-FI", "DIEZMA"}, {"", "RATE", ""}, ICO_PIXEL},
    {{"FILTRE", "LOWPASS", "FILTRO"}, {"BAS", "SWEEP", "GRAVES"}, ICO_LPF},
    {{"FILTRE", "HIPASS", "FILTRO"}, {"HAUT", "SWEEP", "AGUDOS"}, ICO_HPF},
    {{"STOP", "TAPE", "CINTA"}, {"K7", "STOP", "PARADA"}, ICO_TAPE},
    {{"PLONGEE", "DIVE", "PICADO"}, {"", "", ""}, ICO_DIVE},
    {{"HACHE", "GATE", "GATE"}, {"1/16", "1/16", "1/16"}, ICO_GATE},
    {{"HACHE", "GATE", "GATE"}, {"1/32", "1/32", "1/32"}, ICO_GATE},
    {{"GEL", "FREEZE", "CONGELA"}, {"", "", ""}, ICO_SNOW},
    {{"ANNEAU", "RING", "ANILLO"}, {"", "", ""}, ICO_RING},
    {{"ECHO", "ECHO", "ECO"}, {"", "", ""}, ICO_ECHO},
    {{"DISTO", "DISTORT", "DISTOR"}, {"", "", ""}, ICO_DIST},
    {{"WOBBLE", "WOBBLE", "WOBBLE"}, {"", "", ""}, ICO_WOB},
    {{"VINYLE", "VINYL", "VINILO"}, {"", "", ""}, ICO_DISC},
    {{"SANS", "NO", "SIN"}, {"MELO", "MELODY", "MELODIA"}, ICO_NOMEL},
    {{"SANS", "NO", "SIN"}, {"BEAT", "BEAT", "RITMO"}, ICO_NOBEAT},
};
static const char *const LBANK_NAMES[LANG_N][4] = {{"POCHE", "CASSE", "DETRUIT", "BEAT"}, {"POCKET", "WRECK", "DESTROY", "BEAT"},
                                                   {"BOLSA", "ROTO", "DESTRUYE", "BEAT"}};   /* (DESTROY, BEAT: K-GLITCH's, kawa_kg.c) */
#define LBANK_NAME LBANK_NAMES[lang]
static const uint16_t LBANK_COL[4] = {RGB(255, 120, 200), RGB(255, 110, 70), RGB(255, 70, 90), RGB(90, 200, 255)};
#define NLBANK 4u

/* ---- the magic tools: tap LFO (MAGIE), then a white key; one undo each (SAVE held) */
enum { MG_CHOP, MG_BEAT, MG_MELO, MG_BASS, MG_ARP, MG_EUCL, MG_VARY, MG_ROLLS, MG_NUANCE, MG_DOUBLE, MG_SHIFT,
       MG_CLONE, MG_TRANSP, MG_COPY, MG_PASTE, MG_CLEAR, MG_N };
static const lfx_t MAGIC[MG_N] = {
    {{"HACHOIR", "CHOPPER", "PICADOR"}, {"TEMPS", "BEATS", "TIEMPOS"}, ICO_KNIFE},   /* ((Gabare 0.9) the beats shuffled) */
    {{"BEAT", "BEAT", "RITMO"}, {"HASARD", "RANDOM", "AZAR"}, ICO_KIT},
    {{"MELODIE", "MELODY", "MELODIA"}, {"GAMME", "SCALE", "ESCALA"}, ICO_NOTE},
    {{"BASSE", "BASS", "BAJO"}, {"GAMME", "SCALE", "ESCALA"}, ICO_BASS},
    {{"ARPEGE", "ARP", "ARPEGIO"}, {"ACCORDS", "CHORDS", "ACORDES"}, ICO_ARP},
    {{"EUCLIDE", "EUCLID", "EUCLIDE"}, {"N COUPS", "N HITS", "GOLPES"}, ICO_EUCL},
    {{"VARIER", "VARY", "VARIAR"}, {"UN PEU", "A BIT", "UN POCO"}, ICO_DICE},
    {{"ROULES", "ROLLS", "REDOBLE"}, {"FINS", "ENDS", "FINAL"}, ICO_ROLL},
    {{"NUANCES", "ACCENTS", "MATICES"}, {"VOLUMES", "VOLUMES", "VOLUMEN"}, ICO_VOL},
    {{"DOUBLER", "DOUBLE", "DUPLICA"}, {"X2", "X2", "X2"}, ICO_X2},
    {{"DECALER", "SHIFT", "MOVER"}, {"1 PAS", "1 STEP", "1 PASO"}, ICO_RIGHT},
    {{"CLONER", "CLONE", "CLONAR"}, {"MOTIF", "PATTERN", "PATRON"}, ICO_CLONE},
    {{"TRANSPO", "TRANSP", "TRANSP"}, {"+1 NOTE", "+1 NOTE", "+1 NOTA"}, ICO_UPDN},
    {{"COPIER", "COPY", "COPIAR"}, {"", "", ""}, ICO_COPY},
    {{"COLLER", "PASTE", "PEGAR"}, {"", "", ""}, ICO_PASTE},
    {{"EFFACER", "CLEAR", "BORRAR"}, {"", "", ""}, ICO_ERASE},
};
static const char *const L_SCOPES[LANG_N][2] = {{"PISTE", "MOTIF"}, {"TRACK", "PATTERN"}, {"PISTA", "PATRON"}};
#define L_SCOPE L_SCOPES[lang]

/* ---- names for new projects */
static const char *const NAME_A[8] = {"KAWA", "MIETTE", "PIXEL", "BOUM", "ZINZIN", "CHIP", "RAGE", "CANDY"};
static const char *const NAME_B[8] = {"ROSE", "VOLT", "NOIRE", "FOLLE", "MAGIK", "TURBO", "DOUCE", "SALE"};

/* a list's value in the language (K_LIST: n values x 3 languages; K_NAMES: [3][n], here FM_PATCH_NAME) */
static const char *pd_value(const pdesc_t *d, int32_t v)
{
    uint32_t n = (uint32_t)(d->hi - d->lo + 1);
    v = clampi(v, d->lo, d->hi);
    if (d->kind == K_NAMES)                     /* ((Gabare) FM SOUND: KAWA's, the FM-1's original voices, the DX7 bank's) */
        return v < 0 ? mv_name((uint32_t)(v + 128)) : v >= (int32_t)FM_NPATCH ? dx_name((uint32_t)v - FM_NPATCH)
                                                                             : d->list[(uint32_t)lang * FM_NPATCH + (uint32_t)v];
    return d->list[(uint32_t)lang * n + (uint32_t)(v - d->lo)];
}
