/* SPDX-License-Identifier: GPL-3.0-only */
/* (Fm1Studio) Skiff's synth engines on KAWA's tracks.
 *
 * Skiff (illuminus5, 2.5, on Felucca by Leo Kuroshita, Hugelton Instruments) has engines KAWA has not: ANALOG, PHASE,
 * LOFI, VOICE, TRIO, WHEEL, PHYS; (Gabare) and JIANT's STRING and ADD (skiff/eng_string.c, eng_add.c). Their code is copied unchanged in skiff/ (with what they need of Skiff's core.h and
 * its generated tables) and built here, inside a block that renames or puts aside what KAWA also defines
 * (kawa_sk_names.h, tools/gen_sk_names.py). GRAIN is left out: it plays Skiff's samples, which this firmware does not
 * carry.
 *
 * A KAWA track playing one of them keeps all of KAWA around it: the note, the glide, the arpeggio, the table and the
 * LFO make the pitch; the envelope, the volume, the effect and the sends follow the engine. The engine only makes the
 * raw sound, as KAWA's own oscillators do (track_chunk). Its 8 knobs are the sound's SOUND 1-2 (P_S0..P_S7 -> Skiff's
 * P_E0..P_E7, the same 0..127). ENV>EFF (P_MENVA) also opens the engine's own filter, as Skiff's ENV -> FILTER does.
 * One voice a track: KAWA's tracks play one note at a time, and a new note restarts the engine's voice; (Gabare 0.6) the
 * light engines (ANALOG, PHASE, LOFI, TRIO) play an ACC chord's notes on voices 1..3 too, the others v[0] only. */

/* ---- Skiff's code, its names kept apart from KAWA's */
#pragma push_macro("FS")                       /* (Skiff's tables define their own FS and CTL: KAWA's come back after) */
#pragma push_macro("CTL")
#undef FS
#undef CTL
#define clamp sk_clamp
#define blep sk_blep
#define pitch_inc sk_pitch_inc
#define preset_t sk_preset_t
#define SINE SK_SINE
#define NVOICE 4u                              /* (Gabare) CHORD_MAX: an ACC chord's notes (the light engines only) */
#include "kawa_sk_names.h"                      /* Skiff's names KAWA also has */
#include "skiff/sk_core.h"
#include "skiff/skiff_tables.h"
static void *eng_part(const track_t *t);        /* a track's engine memory (below), as Skiff's engines.c */
static track_t sk_trk[NTRK];                    /* what each track's engine reads: its 8 knobs, its voice */
#define trk sk_trk                              /* (Skiff's tracks, as its engines name them: these) */
#define NPART NTRK
#define NHEAVY NTRK                             /* (every track has its engine memory: PHYS plays on all six) */
static uint32_t sk_rng_state = 0x1234567u;      /* Skiff's libc.c rng(), its own state */
static uint32_t sk_rng(void)
{
    sk_rng_state ^= sk_rng_state << 13;
    sk_rng_state ^= sk_rng_state >> 17;
    sk_rng_state ^= sk_rng_state << 5;
    return sk_rng_state;
}
#define rng sk_rng
#include "skiff/dsp.c"
#include "skiff/eng_analog.c"
#include "skiff/eng_phase.c"
#include "skiff/eng_lofi.c"
#include "skiff/eng_formant.c"
#include "skiff/eng_trio.c"
#include "skiff/eng_wheel.c"
#include "skiff/eng_phys.c"
#include "skiff/eng_string.c"                  /* (Gabare) JIANT's STRING and ADD (ADD after WHEEL and STRING: their code) */
#include "skiff/eng_add.c"
#include "skiff/eng_noise.c"                   /* (Gabare 0.9) SLOOP's NOISE and DIGITAL (Felucca's) */
#include "skiff/eng_digital.c"
/* a track's engine memory: the biggest engine's (PHYS: one model slot, the only voice's). The engines with memory of
 * their own play v[0] only (no chord): one voice's */
typedef union {
    phys_slot_t phys[1];
    drw_vc_t wheel[1];                          /* (WHEEL's, ADD's) */
    int16_t string[ST_SL];                      /* (STRING's line) */
} sk_mem_t;
#undef clamp
#undef blep
#undef pitch_inc
#undef preset_t
#undef SINE
#undef NVOICE
#undef trk
#undef NPART
#undef NHEAVY
#undef rng
#define SK_UNDO_NAMES
#include "kawa_sk_names.h"
#undef SK_UNDO_NAMES
#undef FS
#undef CTL
#pragma pop_macro("FS")
#pragma pop_macro("CTL")

/* ---- the bridge: KAWA's engine numbers from E_SK0 on are these, in this order */
static const engine_t *const SK_ENGINES[] = {&ENG_ANALOG, &ENG_PHASE, &ENG_LOFI, &ENG_FORMANT, &ENG_TRIO, &ENG_WHEEL, &ENG_PHYS, &ENG_STRING, &ENG_ADD,
                                             &ENG_NOISE, &ENG_DIGITAL};   /* ((Gabare 0.9) E_NOISE, E_DIGI) */
#define SK_N ((uint32_t)NELEM(SK_ENGINES))
typedef char sk_engines_listed[E_N - E_SK0 == SK_N ? 1 : -1];

/* each engine's level (Q4): Skiff's 11/16; (Gabare) JIANT's STRING and ADD play ~12 and ~8 dB under them (measured on
 * their factory sounds), brought up to them */
static const uint8_t SK_GAIN[] = {11, 11, 11, 11, 11, 11, 11, 33, 22, 11, 11};
typedef char sk_gains_listed[NELEM(SK_GAIN) == SK_N ? 1 : -1];
/* (Gabare) the engines that play an ACC chord (one voice a note): ANALOG, PHASE, LOFI, TRIO; not VOICE (too heavy), nor
 * the ones with memory of their own (WHEEL, PHYS, STRING, ADD) */
static const uint8_t SK_CHORD[] = {1, 1, 1, 0, 1, 0, 0, 0, 0, 0, 1};   /* ((0.9) DIGITAL: yes; NOISE: no) */
typedef char sk_chords_listed[NELEM(SK_CHORD) == SK_N && CHORD_MAX <= NELEM(sk_trk[0].v) ? 1 : -1];
static uint8_t sk_nv[NTRK];                     /* the voices the last note started */

#define SK_BLOCK 32u
typedef char sk_two_chunks[SK_BLOCK == 2u * CHUNK ? 1 : -1];
static sk_mem_t sk_mem[NTRK] POOL;              /* each track's engine memory */
static uint32_t sk_age;
static uint8_t sk_eng[NTRK];                    /* the engine each track's memory belongs to (a switch zeroes it) */
static int32_t sk_buf[NTRK][SK_BLOCK];          /* a block of Skiff's (its CTL, 32) and the half to play next */
static uint8_t sk_pos[NTRK];

static void *eng_part(const track_t *t) { return &sk_mem[(uint32_t)(t - sk_trk) % NTRK]; }

static int is_sk(uint32_t e) { return e >= E_SK0 && e < E_N; }
static const engine_t *sk_engine(uint32_t e) { return SK_ENGINES[(e - E_SK0) % SK_N]; }

static void sk_knobs(uint32_t t)
{
    uint32_t i;
    for (i = 0; i < 8u; i++)
        sk_trk[t].p[P_E0 + i] = prj.snd[t].p[P_S0 + i];
    /* (Gabare) STRING's damper when the note is let go (an OFF): KAWA's RELEASE, 0..15 -> 7..127 */
    sk_trk[t].p[SK_P_REL] = (int16_t)(prj.snd[t].p[P_REL] * 8 + 7);
}

/* a new note: the engine's voice starts again (its phases, its filter: Skiff's note_on); (Gabare) an ACC chord of nch
 * notes (off: their semitones over the note, off[0] 0): one voice each on the light engines */
static void sk_note_on(uint32_t t, uint32_t note, uint32_t vol, uint32_t nch, const int8_t *off)
{
    track_t *k = &sk_trk[t];
    uint32_t e = prj.snd[t].engine, j;
    uint32_t nv = SK_CHORD[(e - E_SK0) % SK_N] && nch > 1u ? (nch < CHORD_MAX ? nch : CHORD_MAX) : 1u;
    if (sk_eng[t] != e) {                       /* another engine: its memory from zero, as Skiff's engine switch */
        memset(&sk_mem[t], 0, sizeof sk_mem[t]);
        memset(k->v, 0, sizeof k->v);
        sk_eng[t] = (uint8_t)e;
    }
    sk_knobs(t);
    sk_pos[t] = 0;                              /* (a new block from the note's start) */
    for (j = 0; j < NELEM(k->v); j++) {
        voice_t *v = &k->v[j];
        int32_t n = (int32_t)note + (j ? off[j] : 0);
        if (j >= nv) {                          /* (a last chord's voice: silent) */
            v->gate = 0;
            v->active = 0;
            continue;
        }
        n = n < 0 ? 0 : n > 127 ? 127 : n;
        v->note = (uint8_t)n;
        v->vel = (uint8_t)(vol * 8u + 7u);
        v->gate = 1;
        v->active = 1;
        v->stage = 1;
        v->age = ++sk_age;
        v->pitch16 = v->pitch_cur = n << 4;
        if (sk_engine(e)->note_on)
            sk_engine(e)->note_on(k, v);
    }
    sk_nv[t] = (uint8_t)nv;
}

/* the key is let go (the engines that hold their own sound to it: WHEEL's percussion, PHYS's bow) */
static void sk_note_off(uint32_t t)
{
    uint32_t j;
    for (j = 0; j < NELEM(sk_trk[t].v); j++) {
        sk_trk[t].v[j].gate = 0;
        sk_trk[t].v[j].stage = 3;
    }
}

/* a chunk of the raw sound (added to raw[], silent otherwise): pit, the pitch as KAWA made it (Q8 semitones); env,
 * KAWA's envelope (Q16), for ENV>EFF on the engine's filter. Skiff's engines run in blocks of SK_BLOCK (its CTL, 32):
 * one is rendered every other chunk, as KAWA's FM does, and played in two halves */
static void sk_chunk(uint32_t t, int32_t *raw, int32_t pit, int32_t env, uint32_t nch, const int8_t *off)
{
    const int8_t *p = prj.snd[t].p;
    track_t *k = &sk_trk[t];
    const engine_t *eng = sk_engine(prj.snd[t].engine);
    vmod_t m;
    int32_t *buf = sk_buf[t], p16 = pit >> 4, g;
    uint32_t i, j, nv = nch > 1u ? (nch < sk_nv[t] ? nch : sk_nv[t]) : 1u;   /* (the governor: nch 0, the note alone) */
    if (sk_eng[t] != prj.snd[t].engine)         /* (no note yet on this engine) */
        return;
    if (sk_pos[t]) {                            /* the second half of the block */
        for (i = 0; i < CHUNK; i++)
            raw[i] += buf[CHUNK + i];
        sk_pos[t] = 0;
        return;
    }
    sk_pos[t] = 1;
    sk_knobs(t);                                /* (the knobs move while the note plays) */
    m.amp0 = m.amp1 = 32767;                    /* KAWA's envelope and volume come after */
    m.envq15 = env >> 1;
    m.cutoff = ((env >> 1) * (p[P_MENVA] * 4)) >> 7;   /* as Skiff's ENV -> FILTER (-64..63): ENV>EFF x 4 */
    m.shape = 64 << 8;
    if (eng->block)
        eng->block(k);
    for (i = 0; i < SK_BLOCK; i++)
        buf[i] = 0;
    for (j = 0; j < nv; j++) {                  /* (Gabare) each note of an ACC chord on its voice, at its pitch */
        int32_t pj = pit + (j ? off[j] * 256 : 0);
        p16 = pj >> 4;
        m.pitch16 = p16 < 0 ? 0 : p16 > 2047 ? 2047 : p16;
        m.fine = (pj & 15) * 256;               /* the residual below 1/16 semitone, in 1/4096 */
        m.inc = sk_pitch_inc((uint32_t)m.pitch16);
        m.inc += (uint32_t)((int32_t)(m.inc >> 12) * m.fine);
        k->v[j].pitch16 = m.pitch16;
        k->v[j].env_out = 32767;
        eng->render(k, &k->v[j], buf, SK_BLOCK, &m);
    }
    g = (SK_GAIN[(prj.snd[t].engine - E_SK0) % SK_N] * CHORD_GAIN[nv - 1u]) >> 4;   /* (Q15) */
    for (i = 0; i < SK_BLOCK; i++)              /* a voice at full scale is 24000 in Skiff; KAWA's oscillators ~16000 */
        buf[i] = (int32_t)(((int64_t)buf[i] * g) >> 15);
    for (i = 0; i < CHUNK; i++)
        raw[i] += buf[i];
}
