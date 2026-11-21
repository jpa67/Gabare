/* SPDX-License-Identifier: GPL-3.0-only */
/* KAWA: the voices. Runs in the audio interrupt: integers only, no division per frame.
 *
 * Each of the eight tracks plays one engine (its sound picks it):
 *   PULSE   pulse waves 12 / 25 / 50 / 75 % or PWM, band-limited (polyBLEP), a detuned second oscillator
 *   WAVE    a 32-step 4-bit wave table (Game Boy style), stepped as the real thing
 *   NOISE   a 15-bit LFSR (NES style), LONG or METAL (93 steps), clocked by the note
 *   FM      six operators, 32 algorithms: msfa (Dexed's core) as Felucca ported it (fm6_core.c), two voices
 *   KIT     16 synthesised drums, two voices
 *   SAMPLE  a break (16 slices on the white keys) or a sample of the user's, sliced or played on the notes
 * Then, for every track: the ADSR envelope (ENV page), the track's effect (a filter, drive, crush,
 * decimate, ring, tremolo or wah, with its mix), the LFO (pitch, volume, the effect's A, pan or shape).
 * Control (envelopes, pitch, LFO) runs once per chunk of 16 frames; FM renders 32 frames at a time. */
#include "fm6_core.c"

static const uint32_t INC_C9[12] = {            /* MIDI 120..131 (C9..B9): f * 2^32 / 44100; lower octaves shift right */
    815363807u, 863847862u, 915214929u, 969636441u, 1027294024u, 1088380105u,
    1153098554u, 1221665363u, 1294309365u, 1371273005u, 1452813141u, 1539201906u};
static const uint32_t SEMI_Q16[25] = {          /* 2^(k/12), k = 0..24, Q16 */
    65536u, 69433u, 73562u, 77936u, 82570u, 87480u, 92682u, 98193u, 104032u, 110218u, 116772u, 123715u,
    131072u, 138866u, 147123u, 155872u, 165140u, 174959u, 185364u, 196386u, 208064u, 220436u, 233544u, 247431u, 262144u};
static const uint32_t SEMI_INV_Q16[25] = {      /* 2^(-k/12), Q16 */
    65536u, 61858u, 58386u, 55109u, 52016u, 49097u, 46341u, 43740u, 41285u, 38968u, 36781u, 34716u,
    32768u, 30929u, 29193u, 27554u, 26008u, 24548u, 23170u, 21870u, 20643u, 19484u, 18390u, 17358u, 16384u};
static uint32_t semi_ratio(int32_t k)           /* 2^(k/12), Q16, k = -36..+36 */
{
    k = clampi(k, -36, 36);
    if (k >= 0)
        return k > 24 ? SEMI_Q16[k - 12] << 1 : SEMI_Q16[k];
    k = -k;
    return k > 24 ? SEMI_INV_Q16[k - 12] >> 1 : SEMI_INV_Q16[k];
}

static uint32_t pitch_inc(int32_t p)            /* p: semitones Q8 (MIDI note * 256) -> phase step per frame */
{
    uint32_t semi, base;
    p = clampi(p, 0, 131 * 256 + 255);
    semi = (uint32_t)p >> 8;
    base = INC_C9[semi % 12u] >> (10u - semi / 12u);
    return (uint32_t)(((uint64_t)base * FINE[((uint32_t)p & 255u) >> 2]) >> 30);
}

static int32_t sine(uint32_t ph)                /* a full cycle over 2^32 -> -32767..32767 (refined parabola, ~0.1 %) */
{
    int32_t x = (int32_t)ph >> 16, ax = x < 0 ? -x : x, y, ay;
    y = (x * (32768 - ax)) >> 13;
    ay = y < 0 ? -y : y;
    y += ((((y * ay) >> 15) - y) * 7373) >> 15;
    return y > 32767 ? 32767 : y < -32767 ? -32767 : y;
}

static int32_t blep(uint32_t t, uint32_t dt)    /* polyBLEP for a rising edge of height 2, Q15; t: phase since it */
{
    uint32_t tq = t >> 16, dq = dt >> 16;
    int32_t x;
    if (!dq)
        return 0;
    if (tq < dq) {
        x = (int32_t)((tq << 15) / dq);
        return 2 * x - ((x * x) >> 15) - 32768;
    }
    if (tq > 65535u - dq) {
        x = -(int32_t)(((65536u - tq) << 15) / dq);
        return ((x * x) >> 15) + 2 * x + 32768;
    }
    return 0;
}

static int32_t sat(int32_t x)                   /* a soft knee from 16384, flat at 32767 */
{
    int32_t a = x < 0 ? -x : x;
    if (a > 16384) {
        a = 16384 + ((a - 16384) >> 1);
        if (a > 32767)
            a = 32767;
        x = x < 0 ? -a : a;
    }
    return x;
}

static int32_t crush_mask(int32_t c) { return c > 0 ? ~((1 << (2 * c)) - 1) : -1; }

/* what the live layer tells the voices (kawa_fx.c sets it once per block) */
static int32_t lv_transpose;                    /* semitones: OCTAVE +1 / -1 */
static uint8_t lv_uni;                          /* 1 unison, 2 unison an octave down */
static uint8_t lv_mute;                         /* tracks silenced, bit per track */
static uint8_t lv_mask = 0xFF;                  /* the tracks the live layer's pitch effects reach (the target's) */
static uint32_t lfo_inc[11];                    /* the LFO's rates per chunk, from the tempo (kawa_seq.c) */
static volatile int16_t midi_bend[NTRK];        /* (Gabare 0.9) MIDI pitch bend, Q8 semitones (main loop: kawa_ui.c) */

/* ---- the effect of a track or of the master */
#define FXL_LEN 256u                            /* (Gabare 0.9) FLANGER / CHORUS: the line (a power of 2) */
typedef struct {
    int32_t lp, bp, hold, tone;
    uint32_t n, ph;
    int32_t hp, dc, fb, px[4], py[4];           /* (0.9) SLOOP's INSERT: low cut, DC blocker, the phaser's stages */
    uint16_t wp;
    int16_t line[FXL_LEN];
} fxs_t;
/* (Gabare 0.9) SLOOP's INSERT (fx.c ins_*, Felucca 1.4): PHASER's all-pass coefficient over its sweep (Q14, 33 points) */
static const int16_t FX_PH_A[33] = {
    -16038, -15998, -15955, -15906, -15853, -15793, -15727, -15653, -15572, -15481, -15381, -15270, -15147, -15012,
    -14862, -14696, -14514, -14313, -14092, -13849, -13582, -13290, -12970, -12621, -12240, -11825, -11375, -10886,
    -10356, -9784, -9166, -8500, -7783,
};
static uint32_t fx_tri(uint32_t q) { uint32_t t = q >> 16; return t < 32768u ? t << 1 : (65535u - t) << 1; }
static int32_t fx_shape(int32_t type, int32_t v)   /* SOFT .. FUZZ: the drive's shape (v: a voice's full scale 32768) */
{
    if (type == FXT_SOFT) return sat(v);
    if (type == FXT_HARD) return clampi(v, -24000, 24000);
    if (type == FXT_FOLD) {                     /* a triangle of period 65536: +-16384, x 1.5 */
        int32_t u = (int32_t)((uint32_t)(v + 16384) & 0xFFFFu) - 32768;
        u = 16384 - (u < 0 ? -u : u);
        return u + (u >> 1);
    }
    return v > 0 ? (v > 20000 ? 20000 : v) : sat(v) >> 1;   /* FUZZ: lopsided */
}
static void fx_insert(fxs_t *s, int32_t type, int32_t a, int32_t b, int32_t mix, int32_t *x, uint32_t n)
{
    uint32_t i, rinc = 0;
    int32_t wet, f = 0, q = 0, m = clampi(mix, 0, 15) * 2185, bq;
    if (type <= FXT_OFF || type >= FXT_N || !m)
        return;
    a = clampi(a, 0, 15);
    b = clampi(b, 0, 15);
    bq = b * 2185;                              /* b / 15, Q15 (no division per frame) */
    if (type == FXT_RING)
        rinc = pitch_inc((40 + a * 5) * 256);
    if (type >= FXT_PHASER)                     /* (0.9) the sweep: A its rate (~0.02 .. 5 Hz), B its depth */
        rinc = (uint32_t)((a + 1) * (a + 1)) * 2000u;
    if (type == FXT_PHASER) {                   /* (its coefficient once a chunk, at its middle) */
        uint32_t tr = fx_tri(s->ph + (n >> 1) * rinc), qq = 32768u + (uint32_t)(((int32_t)tr - 32768) * b / 15);
        f = FX_PH_A[qq >> 11] + (((FX_PH_A[(qq >> 11) + 1u] - FX_PH_A[qq >> 11]) * (int32_t)(qq & 2047u)) >> 11);
    }
    if (type == FXT_LPF || type == FXT_HPF || type == FXT_BPF) {
        f = SVF_F[30 + a * 6];
        q = 30000 - b * 1800;
    } else if (type == FXT_WAH) {
        s->ph += lfo_inc[clampi(4 + a / 4, 0, 10)];
        f = SVF_F[clampi(64 + ((sine(s->ph) * (b + 4) * 2) >> 15), 30, 110)];
        q = 9000;
    }
    for (i = 0; i < n; i++) {
        int32_t d = x[i], y;
        switch (type) {
        case FXT_LPF: case FXT_HPF: case FXT_BPF: case FXT_WAH: {
            int32_t hp;
            s->lp += (int32_t)(((int64_t)f * s->bp) >> 15);   /* (64-bit products: a loud KIT or SAMPLE track */
            s->lp = clampi(s->lp, -400000, 400000);            /* would wrap in 32 and lock the filter) */
            hp = d - s->lp - (int32_t)(((int64_t)q * s->bp) >> 15);
            s->bp += (int32_t)(((int64_t)f * hp) >> 15);
            s->bp = clampi(s->bp, -400000, 400000);
            y = type == FXT_LPF ? s->lp : type == FXT_HPF ? hp : s->bp;
            break;
        }
        case FXT_DIST:
            y = sat(d * (1 + a * 2));
            s->tone += ((y - s->tone) * TON_A[b]) >> 15;
            y = (s->tone * 3) >> 2;
            break;
        case FXT_CRUSH:
            y = d & ~((1 << clampi(a, 0, 14)) - 1);
            break;
        case FXT_DECIM:
            if (++s->n > (uint32_t)a) { s->n = 0; s->hold = d; }
            y = s->hold;
            break;
        case FXT_RING:
            s->ph += rinc;
            y = (int32_t)(((int64_t)d * sine(s->ph)) >> 15);
            break;
        case FXT_TREM:
            if ((i & 15u) == 0u) s->ph += lfo_inc[clampi(4 + a / 4, 0, 10)] ;
            y = (int32_t)(((int64_t)d * (32767 - ((((32767 - sine(s->ph)) >> 1) * bq) >> 15))) >> 15);
            break;
        case FXT_SOFT: case FXT_HARD: case FXT_FOLD: case FXT_FUZZ: {   /* (0.9) low cut, GAIN, shape, TONE, DC */
            int32_t g = 4096 + a * a * 192, v;  /* 1x .. ~11.5x, Q12 (SLOOP's DRIVE over A 0..15) */
            s->hp += ((d << 8) - s->hp) >> 8;
            v = clampi(d - (s->hp >> 8), -230000, 230000);
            y = fx_shape(type, ((v >> 3) * g) >> 9);
            s->tone += (int32_t)(((int64_t)(y - s->tone) * TON_A[b]) >> 15);
            y = s->tone;
            s->dc += ((y << 8) - s->dc) >> 9;
            y -= s->dc >> 8;
            break;
        }
        case FXT_PHASER: {                      /* (0.9) four all-pass stages, fed back */
            int32_t u = clampi((d + 4) >> 3, -16383, 16383) + ((s->fb * 11520 + 16384) >> 15), k;
            for (k = 0; k < 4; k++) {
                int32_t z = clampi(s->px[k] + ((f * (u - s->py[k]) + 8192) >> 14), -65535, 65535);
                s->px[k] = u;
                s->py[k] = z;
                u = z;
            }
            s->fb = clampi(u, -32767, 32767);
            s->ph += rinc;
            u <<= 2;
            s->dc += ((u << 8) - s->dc) >> 9;
            y = (d >> 1) + u - (s->dc >> 8);
            break;
        }
        case FXT_FLANGER: case FXT_CHORUS: {    /* (0.9) a swept delay, fed back */
            int32_t ch = type == FXT_CHORUS, fbk = ch ? 3296 : 14016, r0, r1, v, fr;
            uint32_t dl;
            if (ch) dl = (uint32_t)((140 << 16) + ((sine(s->ph) * (b * 8) * 50) >> 5));   /* 3.2 +- 2.3 ms */
            else {
                uint32_t tr = fx_tri(s->ph), sw = (tr * tr) >> 16;
                dl = (11u << 16) + 209u * (((sw >> 1) * ((uint32_t)b * 8u * 516u)) >> 15);   /* 0.25 .. 5 ms */
            }
            s->ph += rinc;
            r0 = (int32_t)(s->wp - (dl >> 16)) & (int32_t)(FXL_LEN - 1u);
            r1 = (r0 - 1) & (int32_t)(FXL_LEN - 1u);
            fr = (int32_t)((dl >> 1) & 0x7FFFu);
            v = s->line[r0] + (((s->line[r1] - s->line[r0]) * fr) >> 15);
            s->line[s->wp] = (int16_t)clampi((d >> 2) + sat((v * fbk) >> 15), -32767, 32767);
            s->wp = (uint16_t)((s->wp + 1u) & (FXL_LEN - 1u));
            v <<= 1;
            s->dc += ((v << 8) - s->dc) >> 9;
            y = (d >> 1) + v - (s->dc >> 8);
            break;
        }
        default:
            y = d;
            break;
        }
        wet = y;
        x[i] = d + (int32_t)(((int64_t)(wet - d) * m) >> 15);
    }
}

/* ---- every track's common state: the envelope, pitch, tricks, LFO, effect */
typedef struct {
    uint8_t gate, stage, note, vol, shape, crush, vib, arp_n, arp_i;
    int8_t arp_off[4];
    uint8_t ch_n;                               /* (Gabare) ACC: the chord's notes (0: the note alone), their semitones */
    int8_t ch_off[CHORD_MAX];
    uint32_t cph[CHORD_MAX - 1u];               /* (Gabare) PULSE, WAVE: the chord's other notes' phases */
    int32_t env;                                /* the ADSR, Q16 */
    int32_t amp;                                /* the gain now, Q15 (ramps over a chunk) */
    int32_t pitch, ptgt, glide, sweep, bend;    /* Q8 semitones */
    uint32_t vph, arp_cnt, arp_len;
    uint32_t ph, ph2, pwm, lfsr;                /* PULSE, WAVE, NOISE */
    int32_t lp, dc;
    uint32_t lfo_ph, lfo_rnd;
    int32_t lfo;                                /* the LFO now, -32767..32767 x depth / 15 */
    int32_t volq;                               /* the note's volume, Q16 (VSL slides it) */
    int8_t vslide, npan;                        /* VSL: volume a row; PAN: this note's pan */
    uint8_t tb_run, tb_i, tb_note;              /* the table: running, its step, the note it started from */
    int8_t tb_tr, tb_vsl;                       /* its step's semitones, its volume slide */
    uint32_t tb_cnt;                            /* frames into its step */
    fxs_t fx;
} trk_t;
static uint32_t vsl_k = 272u;                   /* a VSL unit per chunk: 65536 x CHUNK / the row's frames (seq_tempo) */
static trk_t tk[NTRK];
/* (Gabare) a track's chord from a computer (FX_CHORD's CHORD_CUSTOM: kawa_bridge.c), its semitones over the note */
static int8_t chord_custom[NTRK][CHORD_MAX];
static uint8_t chord_custom_n[NTRK];
static int chord_engine(uint32_t e)             /* the engines that play an ACC chord */
{
    return e == E_PULSE || e == E_WAVE || e == E_FM || e == E_ANALOG || e == E_PHASE || e == E_LOFI || e == E_TRIO ||
           e == E_DIGI;                         /* ((Gabare 0.9) DIGITAL too) */
}

/* ---- FM: one track (fm_trk), two voices (the new note and the last one's release). The patch is the
 * project's (prj.fmp: its operators, edited in EDIT) through the track's knobs; the main loop builds it */
static fm6_note_t fmn[2];
static fm6_note_t fmc[CHORD_MAX - 1u];          /* (Gabare) ACC on the FM track: the chord's other notes */
static int8_t fmc_off[CHORD_MAX - 1u];
static uint8_t fmc_rel[CHORD_MAX - 1u];         /* a last chord's note, releasing at its own pitch (fmc_rpit) */
static int32_t fmc_rpit[CHORD_MAX - 1u];
static uint8_t fm_cur, fm_pos, fm_seen;
static int32_t fm_vpit[2];                     /* a voice's pitch once it releases (Q8 semitones) */
static int32_t fm_buf[CTL];
static uint8_t fm_patch[156];
static volatile uint8_t fm_gen;
static uint8_t fm_alg, fm_fb;
static fm6_lfo_t fm_lfo;
static int32_t fm_dt[6];

static void fm_build(uint32_t t)                /* main loop: the FM track's patch from its operators and knobs */
{
    const int8_t *p = prj.snd[t].p;
    uint8_t d[156];
    uint32_t k, car, alg;
    for (k = 0; k < 156u; k++)
        d[k] = prj.fmp[k];
    alg = p[P_FMALG] ? (uint32_t)(p[P_FMALG] - 1) : d[FP_ALG] & 31u;
    d[FP_ALG] = (uint8_t)(alg & 31u);
    car = fm6_carriers(alg & 31u);
    for (k = 0; k < 6u; k++) {
        uint8_t *op = d + k * FP_OP;
        uint32_t r;
        if ((car >> k) & 1u)
            continue;                           /* the modulators: ratio and envelope speed */
        op[FP_FC] = (uint8_t)clampi(op[FP_FC] + p[P_FMRAT], 0, 31);
        for (r = 0; r < 4u; r++)
            op[FP_R1 + r] = (uint8_t)clampi(op[FP_R1 + r] - p[P_FMMEG] * 6, 1, 99);
    }
    if (p[P_FMVIB]) {
        d[FP_LPMD] = (uint8_t)clampi(p[P_FMVIB] * 6, 0, 99);
        d[FP_LPMS] = 4;
        if (d[FP_LFS] < 30u) d[FP_LFS] = 34;
    }
    fm_alg = (uint8_t)(alg & 31u);
    fm_fb = (uint8_t)clampi((d[FP_FB] & 7) + p[P_FMFB], 0, 7);
    for (k = 0; k < 6u; k++)                    /* DETUNE: the carriers apart, up to ~25 cents */
        fm_dt[k] = ((car >> k) & 1u) ? (int32_t)((k & 1u) ? 1 : -1) * p[P_FMDET] * 23000 : 0;
    for (k = 0; k < 156u; k++)                  /* (byte by byte: the interrupt sees at worst one block half new) */
        fm_patch[k] = d[k];
    fm_trk = (uint8_t)t;
    fm_gen++;
}

/* (Gabare) ACC on the FM track: the chord's other notes start with its note; the last chord's that this one has not
 * release at their pitch */
static void fm_chord_on(const trk_t *v, uint32_t note, uint32_t vol, int32_t old_pit)
{
    const uint8_t *fp = fm_patch;
    uint32_t i;
    for (i = 0; i + 1u < CHORD_MAX; i++) {
        fm6_note_t *c = &fmc[i];
        if (i + 1u < v->ch_n) {
            fmc_off[i] = v->ch_off[i + 1u];
            fmc_rel[i] = 0;
            fm6_note_init(c, fp, clampi((int32_t)note + fmc_off[i], 0, 127) + fp[FP_TRNSP] - 24, (int32_t)(vol * 8u + 7u), 1);
        } else if (c->live && !fmc_rel[i]) {
            if (c->down) fm6_note_key(c, fp, 0);
            fmc_rel[i] = 1;
            fmc_rpit[i] = old_pit + fmc_off[i] * 256;
        }
    }
}
static void fm_chord_off(void)
{
    uint32_t i;
    for (i = 0; i + 1u < CHORD_MAX; i++)
        if (fmc[i].live && fmc[i].down) fm6_note_key(&fmc[i], fm_patch, 0);
}

#include "kawa_drums.c"

/* ---- a note on track t (k: the cell's two tricks) */
static const tricks_t NO_TRICKS = {FX_NONE, FX_NONE, 0, 0};
static uint8_t tb_inner;                        /* a table's HIT plays the note: the table goes on */
static void tb_apply(uint32_t t, int hit);
static void voice_on(uint32_t t, uint32_t note, uint32_t vol, const tricks_t *k, uint32_t arp_len)
{
    trk_t *v = &tk[t];
    const int8_t *p = prj.snd[t].p;
    const table_t *tb = &prj.tbl[t];
    uint32_t e = prj.snd[t].engine, nm = note_mode(t), i, table = 0;
    int32_t sv, pt = (int32_t)note << 8, old_pit = v->pitch;   /* (old_pit: an FM release's note) */
    if (note == NOTE_OFF) {
        v->gate = 0;
        if (v->stage && v->stage < 4u) v->stage = 4;
        if (is_sk(e)) sk_note_off(t);           /* (Fm1Studio) */
        if (e == E_FM && t == fm_trk) {
            fm6_note_t *n = &fmn[fm_cur];
            if (n->live && n->down) fm6_note_key(n, fm_patch, 0);
            fm_chord_off();                     /* (Gabare) */
        }
        return;
    }
    if (!tb_inner) {                            /* a new note: its table from the first step */
        v->tb_run = 0;
        v->tb_tr = 0;
        v->tb_vsl = 0;
        tb_now[t] = NTSTEP;
        if (tb->on) {
            table = 1;
            v->tb_run = 1;
            v->tb_i = 0;
            v->tb_cnt = 0;
            v->tb_note = (uint8_t)note;
            v->tb_tr = (int8_t)clampi(tb->s[0].tr, -48, 48);
            if (tb->s[0].vol) vol = tb->s[0].vol & 15u;
        }
    }
    trig_ms[t] = fm1_ms;
    v->vol = (uint8_t)vol;
    v->volq = (int32_t)vol << 16;
    v->vslide = (int8_t)(trick(k, FX_VSL, &sv) ? sv : 0);
    v->npan = (int8_t)(trick(k, FX_PAN, &sv) ? sv : 0);
    if (p[P_MLFRT]) {                           /* LFO KEY: the LFO starts again with each note */
        v->lfo_ph = 0;
        v->lfo_rnd = xr(&rnd_isr);
    }
    v->bend = 0;
    v->arp_n = 0;
    v->ch_n = 0;                                /* (Gabare) no ACC: the note alone */
    v->shape = (uint8_t)p[P_SHAPE];
    v->crush = (uint8_t)p[P_CRUSH];
    v->vib = (uint8_t)(e == E_FM || is_sk(e) ? 0 : p[P_VIB]);   /* ((Fm1Studio) a Skiff engine: P_S2 is its own knob) */
    if (nm == NM_NOTE) {
        int32_t gv;
        int fg = trick(k, FX_GLIDE, &gv);
        int glide = (fg || (e <= E_WAVE && p[P_GLIDE])) && v->stage && v->stage < 4u;
        if (glide) {
            v->ptgt = pt;
            v->glide = fg ? 1 + gv * gv / 6 : 1 + p[P_GLIDE] * p[P_GLIDE] / 6;
        } else {
            v->pitch = v->ptgt = pt;
            v->glide = 0;
            v->sweep = e <= E_WAVE ? p[P_SWEEP] * 512 : 0;
            if (v->env < 6000 && e <= E_WAVE)
                v->ph = v->ph2 = v->cph[0] = v->cph[1] = v->cph[2] = 0;   /* the same start every time (chip punch), when it cannot click */
        }
        if (trick(k, FX_ARP, &sv) && sv >= 0 && sv < 14) {
            v->arp_n = (uint8_t)CHORDS[sv].n;
            for (i = 0; i < 4u; i++)
                v->arp_off[i] = CHORDS[sv].off[i];
            v->arp_i = 0;
            v->arp_cnt = 0;
            v->arp_len = arp_len ? arp_len * 3u / (uint32_t)clampi(e <= E_WAVE ? p[P_ARPV] : 3, 1, 8) : 700u;
        }
        if (chord_engine(e) && trick(k, FX_CHORD, &sv)) {   /* (Gabare) ACC: the chord, its notes together */
            const int8_t *o = sv == CHORD_CUSTOM ? chord_custom[t] : sv >= 0 && sv < 14 ? CHORDS[sv].off : 0;
            uint32_t n = sv == CHORD_CUSTOM ? chord_custom_n[t] : sv >= 0 && sv < 14 ? (uint32_t)CHORDS[sv].n : 0u;
            if (o && n > 1u) {
                v->ch_n = (uint8_t)(n < CHORD_MAX ? n : CHORD_MAX);
                for (i = 0; i < CHORD_MAX; i++)
                    v->ch_off[i] = i < v->ch_n ? o[i] : 0;
            }
        }
        if (trick(k, FX_VIB, &sv)) v->vib = (uint8_t)sv;
        if (trick(k, FX_BEND, &sv)) v->bend = sv;
        if (trick(k, FX_SHAPE, &sv)) v->shape = (uint8_t)sv;
        if (trick(k, FX_CRUSH, &sv)) v->crush = (uint8_t)sv;
        if (trick(k, FX_PITCH, &sv)) v->pitch = v->ptgt = pt + sv * 256;
        if (!v->lfsr) v->lfsr = 1;
        if (e == E_FM && t == fm_trk && !(v->glide && v->stage)) {   /* a new FM voice; the last one releases */
            fm6_note_t *old = &fmn[fm_cur], *n;
            const uint8_t *fp = fm_patch;
            if (old->live && old->down) fm6_note_key(old, fp, 0);
            fm_vpit[fm_cur] = old_pit;          /* the note it was playing (before this one) */
            fm_cur ^= 1u;
            n = &fmn[fm_cur];
            fm6_note_init(n, fp, (int32_t)note + fp[FP_TRNSP] - 24, (int32_t)(vol * 8u + 7u), 1);
            fm6_lfo_key(&fm_lfo);
            fm_chord_on(v, note, vol, old_pit);   /* (Gabare) ACC */
        }
        if (is_sk(e) && !(v->glide && v->stage))   /* (Fm1Studio) Skiff's engine: its voice starts again (kawa_sk.c) */
            sk_note_on(t, note, vol, v->ch_n, v->ch_off);
    } else if (e == E_KIT)
        drum_on(t, cell_idx(note) & 15u, (trick(k, FX_PITCH, &sv) ? sv : 0) + v->tb_tr + (cell_idx(note) >= 16u ? 12 : 0), trick(k, FX_CRUSH, &sv) ? sv : -1);   /* (17..32: the same drum, tuned up) */
    v->note = (uint8_t)note;
    if (!(v->glide && v->stage) || v->stage == 4u) {
        v->stage = 1;
        if (p[P_ATT] == 0) v->env = 65535, v->stage = 2;
    }
    v->gate = 1;
    if (table)
        tb_apply(t, 0);                         /* the first step's trick (its pitch and volume are in) */
}

static void voice_off(uint32_t t)
{
    trk_t *v = &tk[t];
    v->gate = 0;
    if (v->stage && v->stage < 4u)
        v->stage = 4;
    if (is_sk(prj.snd[t].engine))               /* (Fm1Studio) */
        sk_note_off(t);
    if (prj.snd[t].engine == E_FM && t == fm_trk) {
        fm6_note_t *n = &fmn[fm_cur];
        if (n->live && n->down) fm6_note_key(n, fm_patch, 0);
        fm_chord_off();                         /* (Gabare) */
    }
}

static void voice_cut(uint32_t t)                /* CUT: the note stops now, whatever plays it */
{
    voice_off(t);
    drum_choke(&drum[t][0]);
    drum_choke(&drum[t][1]);
}

static void tb_stop(uint32_t t)
{
    tk[t].tb_run = 0;
    tk[t].tb_vsl = 0;
    tb_now[t] = NTSTEP;
}

static void voices_silence(void)                 /* (the stop: the tables stop too, no HIT starts a note again) */
{
    uint32_t t;
    for (t = 0; t < NTRK; t++) {
        voice_off(t);
        tb_stop(t);
    }
}

static void voice_kill(uint32_t t)               /* the engine changed (t NTRK: all tracks): the old voices go now, */
{                                                /* nothing would play them out (the gain fades in a chunk) */
    uint32_t a = t < NTRK ? t : 0u, b = t < NTRK ? t + 1u : NTRK;
    for (t = a; t < b; t++) {
        tk[t].gate = 0;
        tk[t].stage = 0;
        tk[t].env = 0;
        tk[t].tb_tr = 0;
        tb_stop(t);
        drum[t][0].on = drum[t][1].on = 0;
        if (t == fm_trk)
            fmn[0].live = fmn[1].live = fmc[0].live = fmc[1].live = fmc[2].live = 0;
    }
}

/* ---- the table of a track (TABLE): from each note, a step every SPEED ticks; each step its pitch over the
 * note, its volume, its trick. HIT plays the note again (only while it is held: an OFF, a stop end it) */
static void tb_apply(uint32_t t, int hit)
{
    trk_t *v = &tk[t];
    const tstep_t *s = &prj.tbl[t].s[v->tb_i % NTSTEP];
    uint32_t e = prj.snd[t].engine;
    int32_t fv = s->fxv;
    v->tb_tr = (int8_t)clampi(s->tr, -48, 48);
    tb_now[t] = v->tb_i;
    if (s->vol) {
        v->vol = (uint8_t)(s->vol & 15u);
        v->volq = (int32_t)v->vol << 16;
    }
    switch (s->fx) {
    case TB_HIT:
        if (hit && v->gate) {
            tb_inner = 1;
            voice_on(t, v->tb_note, v->vol, &NO_TRICKS, 0);
            tb_inner = 0;
        }
        break;
    case TB_CUT: voice_cut(t); break;
    case TB_VIB: v->vib = (uint8_t)clampi(fv, 0, 15); break;
    case TB_BEND: v->bend = clampi(fv, -15, 15); break;
    case TB_SHAPE: v->shape = (uint8_t)clampi(fv, 0, 7); break;
    case TB_CRUSH:
        v->crush = (uint8_t)clampi(fv, 0, 7);
        if (e == E_KIT) drum[t][drum_next[t] ^ 1u].crush = v->crush;
        break;
    case TB_VSL: v->tb_vsl = (int8_t)clampi(fv, -15, 15); break;
    case TB_PAN: v->npan = (int8_t)clampi(fv, -7, 7); break;
    }
}

static void tb_chunk(uint32_t t)                /* once a chunk, before the track renders */
{
    trk_t *v = &tk[t];
    const table_t *tb = &prj.tbl[t];
    const tstep_t *s;
    uint32_t len, n, next;
    if (!v->tb_run)
        return;
    if (!tb->on) {                              /* turned off while it ran */
        tb_stop(t);
        v->tb_tr = 0;
        return;
    }
    n = g_ticklen * (uint32_t)clampi(tb->speed, 1, 24);
    v->tb_cnt += CHUNK;
    if (v->tb_cnt < n)
        return;
    v->tb_cnt -= n;                             /* (a step lasts at least a tick: 367 frames, never two a chunk) */
    if (v->tb_cnt >= n)
        v->tb_cnt = 0;
    len = (uint32_t)clampi(tb->len, 1, NTSTEP);
    s = &tb->s[v->tb_i % NTSTEP];
    if (s->fx == TB_HOP && (uint32_t)s->fxv < len)
        next = (uint32_t)s->fxv;
    else if (v->tb_i + 1u < len)
        next = v->tb_i + 1u;
    else if (tb->loop < len)
        next = tb->loop;
    else {
        v->tb_run = 0;                          /* the end, no loop: the last step stays */
        return;
    }
    v->tb_i = (uint8_t)next;
    tb_apply(t, 1);
}

/* ---- the LFO of a track, once a chunk: -32767..32767 x depth / 15 */
static void lfo_chunk(trk_t *v, const int8_t *p)
{
    int32_t w;
    uint32_t o = v->lfo_ph;
    if (!p[P_LFOD] || !p[P_LFOA]) {
        v->lfo = 0;
        return;
    }
    v->lfo_ph += lfo_inc[clampi(p[P_LFOR], 0, 10)];
    switch (p[P_LFOW]) {
    case 1: w = (int32_t)(v->lfo_ph >> 16); w = (w < 32768 ? w : 65535 - w) * 2 - 32767; break;
    case 2: w = v->lfo_ph < 0x80000000u ? 32767 : -32767; break;
    case 3: w = (int32_t)(v->lfo_ph >> 16) - 32768; break;
    case 4:
        if (v->lfo_ph < o) v->lfo_rnd = xr(&rnd_isr);
        w = (int32_t)(v->lfo_rnd >> 17) - 16384;
        w *= 2;
        break;
    default: w = sine(v->lfo_ph); break;
    }
    v->lfo = (w * p[P_LFOA]) / 15;
}

/* ---- one track, one chunk: the engine, the envelope, the effect -> out (mono) */
static void track_chunk(uint32_t t, int32_t *out)
{
    trk_t *v = &tk[t];
    const int8_t *p = prj.snd[t].p;
    uint32_t e = prj.snd[t].engine, i, dest = (uint32_t)p[P_LFOD];
    int32_t raw[CHUNK], pit, gain, tgt, da, amp, fxa = p[P_FXA];
    int32_t ton = TON_A[clampi(e == E_KIT ? p[P_KTON] : p[P_TON], 0, 15)];
    int active = v->stage != 0 || v->amp != 0;
    lfo_chunk(v, p);
    switch (v->stage) {                         /* ADSR, once a chunk */
    case 1:
        v->env += ENV_RATE[p[P_ATT] & 15];
        if (v->env >= 65535) { v->env = 65535; v->stage = 2; }
        break;
    case 2:
        tgt = p[P_SUS] * 4369;
        v->env -= ENV_RATE[p[P_DEC] & 15] >> 1;
        if (v->env <= tgt) { v->env = tgt; v->stage = tgt ? 3 : 0; }
        break;
    case 4:
        v->env -= ENV_RATE[p[P_REL] & 15] >> 1;
        if (v->env <= 0) { v->env = 0; v->stage = 0; }
        break;
    }
    env_now[t] = (uint16_t)v->env;
    env_stage[t] = v->stage;
    for (i = 0; i < CHUNK; i++)
        raw[i] = 0;
    /* pitch: glide, bend, sweep, vibrato, arpeggio, the live octave, the LFO */
    if (v->glide) {
        if (v->pitch < v->ptgt) { v->pitch += v->glide; if (v->pitch > v->ptgt) v->pitch = v->ptgt; }
        else if (v->pitch > v->ptgt) { v->pitch -= v->glide; if (v->pitch < v->ptgt) v->pitch = v->ptgt; }
    }
    v->pitch += v->bend;
    v->pitch = clampi(v->pitch, -48 * 256, 160 * 256);   /* (a long BEND, a table's, stays in range) */
    v->sweep = (int32_t)(((int64_t)v->sweep * 64880) >> 16);   /* tau ~35 ms */
    pit = v->pitch + v->sweep + ((lv_mask >> t & 1u) ? lv_transpose * 256 : 0) + v->tb_tr * 256 + midi_bend[t];
    if (v->vib) {
        v->vph += 9350000u;                     /* ~6 Hz */
        pit += (sine(v->vph) * v->vib * 14) >> 15;
    }
    if (v->arp_n) {
        v->arp_cnt += CHUNK;
        if (v->arp_cnt >= v->arp_len) {
            v->arp_cnt -= v->arp_len;
            if (++v->arp_i >= v->arp_n) v->arp_i = 0;
        }
        pit += v->arp_off[v->arp_i] * 256;
    }
    if (dest == LFD_PITCH)
        pit += (v->lfo * p[P_LFOA] * 4) >> 15;
    if (dest == LFD_FILT)
        fxa = clampi(fxa + ((v->lfo * 8) >> 15), 0, 15);
    if (p[P_MENVA] || p[P_MVELA])               /* MODULATION: the envelope and the velocity open the effect */
        fxa = clampi(fxa + ((v->env * p[P_MENVA]) >> 16) + ((v->vol * p[P_MVELA]) >> 4), 0, 15);
    if (p[P_MENVP])                             /* ... and bend the pitch */
        pit += (v->env * p[P_MENVP]) >> 8;
    if (v->vslide || v->tb_vsl) {               /* VSL: the volume slides over the row (a table's: on) */
        v->volq = clampi(v->volq + (v->vslide + v->tb_vsl) * (int32_t)vsl_k, 0, 15 << 16);
        v->vol = (uint8_t)(v->volq >> 16);
    }
    if (e <= E_WAVE && active) {
        uint32_t inc, inc2 = 0, duty = 0x80000000u, ph = v->ph, ph2 = v->ph2;
        uint32_t cinc[CHORD_MAX - 1u], cph[CHORD_MAX - 1u], cn = 0, j;   /* (Gabare) ACC's other notes */
        int32_t cg = 32767;
        int32_t m = crush_mask(v->crush), lp = v->lp, det = p[P_DETUNE];
        int uni = (lv_mask >> t & 1u) ? lv_uni : 0;
        inc = pitch_inc(pit);
        if (uni || det)
            inc2 = uni == 2 ? inc >> 1 : inc + (inc >> 7) + (uint32_t)(((uint64_t)inc * (uint32_t)det) >> 11);
        if (v->ch_n > 1u && cpu_shed < 2u) {   /* (the governor: the note alone) */
            for (cn = 0; cn + 1u < v->ch_n; cn++) {
                cinc[cn] = pitch_inc(pit + v->ch_off[cn + 1u] * 256);
                cph[cn] = v->cph[cn];
            }
            cg = CHORD_GAIN[cn];
        }
        if (e == E_PULSE) {
            static const uint32_t DUTY[4] = {0x20000000u, 0x40000000u, 0x80000000u, 0xC0000000u};
            if (v->shape < 4u && dest != LFD_SHAPE)
                duty = DUTY[v->shape];
            else {
                v->pwm += 1400000u;             /* ~1 Hz */
                duty = 0x80000000u + (uint32_t)((dest == LFD_SHAPE ? v->lfo : sine(v->pwm)) * 40000);
            }
        }
        for (i = 0; i < CHUNK; i++) {
            int32_t s;
            if (e == E_PULSE) {
                s = (ph < duty ? 32767 : -32767) + blep(ph, inc) - blep(ph - duty, inc);
                ph += inc;
                if (inc2) {
                    s += (ph2 < duty ? 32767 : -32767) + blep(ph2, inc2) - blep(ph2 - duty, inc2);
                    s = (int32_t)(((int64_t)s * 23170) >> 15);
                    ph2 += inc2;
                }
                for (j = 0; j < cn; j++) {      /* (Gabare) ACC */
                    s += (cph[j] < duty ? 32767 : -32767) + blep(cph[j], cinc[j]) - blep(cph[j] - duty, cinc[j]);
                    cph[j] += cinc[j];
                }
            } else {
                const uint8_t *w = WAVES[v->shape & 7u];
                s = (w[ph >> 27] * 2 - 15) * 2184;
                ph += inc;
                if (inc2) {
                    s = ((s + (w[ph2 >> 27] * 2 - 15) * 2184) * 23170) >> 15;
                    ph2 += inc2;
                }
                for (j = 0; j < cn; j++) {      /* (Gabare) ACC */
                    s += (w[cph[j] >> 27] * 2 - 15) * 2184;
                    cph[j] += cinc[j];
                }
            }
            if (cn)
                s = (int32_t)(((int64_t)s * cg) >> 15);
            s &= m;
            lp += (int32_t)(((int64_t)(s - lp) * ton) >> 15);
            v->dc += (lp * 256 - v->dc) >> 11;  /* ~3 Hz high-pass, as the chips' output capacitors */
            raw[i] = (lp - (v->dc >> 8)) >> 1;
        }
        v->ph = ph;
        v->ph2 = ph2;
        v->lp = lp;
        for (j = 0; j < cn; j++)
            v->cph[j] = cph[j];
    } else if (e == E_FM) {
        const uint8_t *fp = fm_patch;
        if (t != fm_trk) {                      /* (a second FM track cannot happen: the editor refuses it) */
            active = 0;
        } else {
            if (fm_seen != fm_gen) {            /* a new patch: its LFO */
                fm_seen = fm_gen;
                fm6_lfo_reset(&fm_lfo, fp);
            }
            if (fm_pos == 0u) {                 /* a new block of 32 */
                int32_t lv, ld, lf, k;
                uint32_t j;
                int32_t bri = (p[P_FMBRI] + (dest == LFD_SHAPE ? (v->lfo * 10) >> 15 : 0)) * 48;
                for (j = 0; j < (uint32_t)CTL; j++)
                    fm_buf[j] = 0;
                lv = fm6_lfo_sample(&fm_lfo);
                ld = fm6_lfo_delay(&fm_lfo);
                for (k = 0; k < 2; k++) {
                    fm6_note_t *n = &fmn[k];
                    int32_t bus[CTL], vp = k == (int32_t)fm_cur ? pit : pit - v->pitch + fm_vpit[k];
                    lf = fm6_note_logfreq((vp >> 4) + (fp[FP_TRNSP] - 24) * 16, 0);   /* (the release: its own note) */
                    if (cpu_shed && k != fm_cur)
                        n->live = 0;            /* (the governor: no release voice) */
                    if (!n->live)
                        continue;
                    if (fm6_note_compute(n, fp, bus, lv, ld, lf, fm_alg, fm_fb, fm_dt, bri << 16)) {
                        for (j = 0; j < (uint32_t)CTL; j++) {
                            int32_t x = bus[j] - n->dc;
                            n->dc += x >> 10;   /* FM puts sidebands at 0 Hz */
                            fm_buf[j] += x >> 10;
                        }
                    }
                    if (!n->down && fm6_note_done(n, fp, fm_alg))
                        n->live = 0;
                }
                for (k = 0; k < (int32_t)(CHORD_MAX - 1u); k++) {   /* (Gabare) ACC: the chord's other notes */
                    fm6_note_t *n = &fmc[k];
                    int32_t bus[CTL], vp = fmc_rel[k] ? pit - v->pitch + fmc_rpit[k] : pit + fmc_off[k] * 256;
                    if (cpu_shed >= 2u || (cpu_shed && fmc_rel[k]))
                        n->live = 0;            /* (the governor: the chord goes, its releases first) */
                    if (!n->live)
                        continue;
                    lf = fm6_note_logfreq((vp >> 4) + (fp[FP_TRNSP] - 24) * 16, 0);
                    if (fm6_note_compute(n, fp, bus, lv, ld, lf, fm_alg, fm_fb, fm_dt, bri << 16)) {
                        for (j = 0; j < (uint32_t)CTL; j++) {
                            int32_t x = bus[j] - n->dc;
                            n->dc += x >> 10;
                            fm_buf[j] += x >> 10;
                        }
                    }
                    if (!n->down && fm6_note_done(n, fp, fm_alg))
                        n->live = 0;
                }
                if (v->ch_n > 1u)               /* (Gabare) a chord's sum, brought down */
                    for (j = 0; j < (uint32_t)CTL; j++)
                        fm_buf[j] = (int32_t)(((int64_t)fm_buf[j] * CHORD_GAIN[v->ch_n - 1u]) >> 15);
            }
            for (i = 0; i < CHUNK; i++)
                raw[i] = fm_buf[fm_pos + i];
            fm_pos = (uint8_t)(fm_pos ^ CHUNK);
            if (!fmn[0].live && !fmn[1].live && !fmc[0].live && !fmc[1].live && !fmc[2].live && !v->stage)
                active = 0;
        }
    } else if (is_sk(e)) {                      /* (Fm1Studio) Skiff's engine (kawa_sk.c) */
        if (active)
            sk_chunk(t, raw, pit, v->env, cpu_shed < 2u ? v->ch_n : 0u, v->ch_off);   /* ((Gabare) ACC) */
    } else if (e == E_KIT) {
        if (cpu_shed >= 3u) drum[t][drum_next[t]].on = 0;   /* (the governor: the older voice goes) */
        drum_chunk(&drum[t][0], raw);
        drum_chunk(&drum[t][1], raw);
        if (ton < 32767)
            for (i = 0; i < CHUNK; i++) { v->lp += (int32_t)(((int64_t)(raw[i] - v->lp) * ton) >> 15); raw[i] = v->lp; }
        active = 1;
    }
    if (!active && !v->amp)
        return;
    /* the envelope and the note's volume, ramped over the chunk */
    gain = (int32_t)(((uint32_t)v->env * (uint32_t)v->env) >> 17);
    tgt = (int32_t)(((int64_t)gain * ((v->volq * 18) >> 16)) >> 8);   /* vol 15 -> ~1.05 x gain */
    tgt = (int32_t)(((int64_t)tgt * ((p[P_VOL] & 15) * 5461)) >> 16);   /* MIX VOLUME: 12 = x 1 (no division) */
    if (dest == LFD_VOL)
        tgt = (int32_t)(((int64_t)tgt * (32767 - ((32767 - v->lfo) >> 1) * 1)) >> 15);
    if (lv_mute >> t & 1u)
        tgt = 0;
    amp = v->amp;
    da = (tgt - amp) / (int32_t)CHUNK;
    for (i = 0; i < CHUNK; i++) {
        out[i] = (int32_t)(((int64_t)raw[i] * amp) >> 15);
        amp += da;
    }
    v->amp = tgt;
    if (cpu_shed < 2u)                          /* (the load governor: kawa_seq.c) */
        fx_insert(&v->fx, p[P_FXT], fxa, p[P_FXB], p[P_FXM], out, CHUNK);
}
