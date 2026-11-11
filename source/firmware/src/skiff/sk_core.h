/* SPDX-License-Identifier: GPL-3.0-only
 * (Fm1Studio) What Skiff's synth engines need of its core.h: the parameter list, voice_t, vmod_t, preset_t, engine_t.
 * Extracted unchanged from Skiff 2.5 (illuminus5, commit 881307d; Felucca by Leo Kuroshita, Hugelton Instruments),
 * firmware/src/core.h. Only kawa_sk.c includes it, inside the block that renames what KAWA also defines. */
#pragma once
#ifndef NELEM
#define NELEM(a) (sizeof(a) / sizeof((a)[0]))
#endif

/* ------------------------------------------------------- parameters --- */
enum {
    F_INT, F_PCT, F_BIPCT, F_TIME, F_LFOHZ, F_CUTOFF, F_DB, F_SEMI, F_ENUM, F_BPM, F_NOTE,
    F_ONOFF, F_OCT, F_STEPS, F_ST16,  /* (Skiff U1) F_ST16: 0..127, 64 = 0, 3/16 semitone a step (DRUM's TUNE) */
    F_SHAPE                           /* (2.5) the FILTR insert's SHAPE: LP (0) .. BP (64) .. HP (127), "30 BP" between */
};

/* (Skiff 2.0) 16 bytes, its fields in this order (the initializers' too): a unit is one of the few a descriptor names
 * (PD_UNIT_OF; every other unit comes from the format, params.c param_format) */
typedef struct {
    const char *label;
    const char *const *names;   /* F_ENUM */
    int16_t min, max, def;
    uint8_t fmt;
    uint8_t unit;               /* F_INT / F_ENUM optional unit: U_CT, U_ST, U_HZ, 0 none */
} param_desc_t;
enum { U_NONE, U_CT, U_ST, U_HZ, U_MS };     /* ((2.5) U_MS: STRUM's; 0 reads OFF) */
#define PD_UNIT_OF(d) ((d)->unit == U_CT ? "ct" : (d)->unit == U_ST ? "st" : (d)->unit == U_HZ ? "Hz" : \
                       (d)->unit == U_MS ? "ms" : (const char *)0)

enum {                          /* per-track parameters */
    P_LEVEL,
    P_ATK, P_DEC, P_SUS, P_REL,
    P_ED_FLT, P_ED_PIT, P_ED_SHP, P_ED_FX,     /* P_ED_FX: unused, kept for the formats / protocol */
    P_LRATE, P_LWAVE, P_LPHASE, P_LFADE,
    P_LD_PIT, P_LD_FLT, P_LD_SHP, P_LD_AMP,
    P_AMODE, P_ARATE, P_AOCT, P_AGATE,
    P_ASWING, P_APROB, P_AHOLD, P_AORDER,
    P_ROOT, P_SCALE, P_QUANT, P_TRANS,
    P_SLEN, P_SDIV, P_SSWING, P_SGATE,
    P_DIST, P_CHOR, P_DLY, P_REV,
    P_VOICE, P_GLIDE, P_PAN, P_MUTE,
    P_GLMODE, P_PRIO, P_ALLOC, P_DETUNE,
    P_SLCR, P_SLPAT, P_SLRATE, P_SLDEPTH,      /* SLICER insert (slicer.c) */
    P_M1SRC, P_M1DST, P_M1AMT,                 /* modulation matrix (mod.c): 4 slots of SRC, DST, AMT; */
    P_M2SRC, P_M2DST, P_M2AMT,                 /* new common parameters go just before P_E0 (user presets */
    P_M3SRC, P_M3DST, P_M3AMT,                 /* and projects map by count) */
    P_M4SRC, P_M4DST, P_M4AMT,
    P_FM1_ATK, P_FM1_DEC, P_FM1_SUS, P_FM1_REL, P_FM1_LEVEL,
    P_FM2_ATK, P_FM2_DEC, P_FM2_SUS, P_FM2_REL, P_FM2_LEVEL,
    P_FM3_ATK, P_FM3_DEC, P_FM3_SUS, P_FM3_REL, P_FM3_LEVEL,
    P_FM4_ATK, P_FM4_DEC, P_FM4_SUS, P_FM4_REL, P_FM4_LEVEL,
    P_CHRD, P_VOIC,                            /* chord keys (chord.c): one key plays a chord; its voicing */
    P_LN0, P_LN1, P_LN2, P_LN3, P_LN4, P_LN5, P_LN6, P_LN7,   /* DRUM lane levels (eng_drum.c, #97): KICK .. BELL */
    P_DIR,                                     /* (Skiff) play direction (seq.c seq_next): DIR_FWD REV PING RAND */
    P_DKSRC, P_DKAMT, P_DKREL,                 /* (Skiff) ducking (duck.c): the track it ducks under, DEPTH, RELEASE */
    P_DTYPE, P_DTONE,                          /* (Skiff phase 9) FX > DRIVE (fx.c track_dist): DIST's kind, its tone */
    P_EG2_ATK, P_EG2_DEC, P_EG2_SUS, P_EG2_REL,   /* (Skiff phase 9) ENV 2 (voice.c env2_tick): a MOD source */
    P_LTUNE0, P_LTUNE1, P_LTUNE2, P_LTUNE3, P_LTUNE4, P_LTUNE5, P_LTUNE6, P_LTUNE7,   /* (Skiff phase 9) the DRUM lanes' */
    P_LDECAY0, P_LDECAY1, P_LDECAY2, P_LDECAY3, P_LDECAY4, P_LDECAY5, P_LDECAY6, P_LDECAY7,   /* TUNE and DECAY */
    P_LSYNC, P_LTRIG, P_LPOL,                  /* (Skiff 1.3, #16, from Felucca 1.2) LFO SYNC (OFF, a note value), TRIG (NOTE,
                                                * FREE), POL (BI, UNI) */
    P_SPRD,                                    /* (1.3, from Felucca) SPREAD: a POLY / UNISON track's voices around PAN */
    P_ITYPE, P_IA, P_IB, P_IC, P_IMIX,         /* (1.3, from Felucca 1.5) the INSERT, its TYPE, three values and MIX (fx.c) */
    P_ESYNC,                                   /* (1.3, from Felucca 1.5) ENV SYNC: ATK DEC REL as note values of the tempo */
    P_LPAN0, P_LPAN1, P_LPAN2, P_LPAN3, P_LPAN4, P_LPAN5, P_LPAN6, P_LPAN7,   /* (2.0, #36) the drum lanes' PAN (voice.c
                                                * lpan_render): stored ids 135..142, after the engine values' (sid_p) */
    P_FOLLOW, P_FMODE, P_FNOTE,                /* (2.1) chord follow (follow.c): the track whose chords it follows, how,
                                                * by which of their notes; stored ids 143..145 */
    P_STRUM, P_SDIR,                           /* (2.5, the user's 7B) SCL > CHORD's STRUM (ms) and DIR (DOWN UP ALT,
                                                * voice.c strum_*); stored ids 146, 147 */
    P_E0, P_E1, P_E2, P_E3, P_E4, P_E5, P_E6, P_E7,
    P_COUNT
};

enum { V_POLY, V_MONO, V_LEGATO, V_UNISON };   /* P_VOICE */
typedef struct {
    uint8_t note, vel, gate, active;
    uint8_t stage;               /* env: 0 off, 1 attack, 2 decay/sustain, 3 release */
    uint8_t stage2;              /* (Skiff phase 9) ENV 2's (voice.c env2_tick): the same, its release at the gate's end */
    int32_t env;                 /* Q24 */
    int32_t env_out;             /* last control-rate amplitude, Q15 */
    int32_t pitch16, pitch_cur;  /* 1/16 semitone, with glide */
    int32_t gstep;               /* glide TIME mode: 1/16 st per control tick, 0 = RATE mode */
    int32_t fine;                /* unison detune: phase increment * (1 + fine / 4096) */
    uint32_t ph[3];
    int32_t s[8];                /* engine state (filters, envs) */
    uint32_t age;
    uint8_t mvel;                /* mod.c: the note-on velocity (before UNISON scaling) and RAND of the note */
    uint8_t side;                /* (1.3) SPREAD: 0 the left of PAN, 1 the right (voice.c voice_start, fx.c mix_spread) */
    int16_t mrnd;
    int32_t env2;                /* (Skiff phase 9) ENV 2, Q24: a MOD source only (mod.c MS_ENV2) */
} voice_t;

typedef struct {                 /* per-voice control-rate modulation, computed in voice.c */
    uint32_t inc;                /* phase increment of the base pitch */
    int32_t pitch16;
    int32_t amp0, amp1;          /* Q15 ramp over the block */
    int32_t cutoff;              /* 0..127 << 8 */
    int32_t shape;               /* 0..127 << 8 */
    int32_t envq15;              /* env value (for engines that use it as a mod source) */
    int32_t fine;                /* the residual below 1/16 semitone in inc: unison detune, TUNE, bend, (Skiff 1.3.2) the LFO's vibrato (1/4096) */
} vmod_t;

typedef struct {
    const char *name;
    int8_t e[8];                 /* P_E0..P_E7 (signed: an interval below the note; every value fits) */
    uint8_t env[4];              /* ATK DEC SUS REL */
    int8_t fenv;                 /* ENV -> FILTER amount (-64..63) */
    uint8_t mono;                /* 1 = MONO (bass / lead), 0 = POLY */
    /* the rest of the patch; each value is stored + 1, 0 = the default */
    uint8_t fx[4];               /* DIST, CHORUS, DELAY, REVERB sends (a sound load never touches the steps; the
                                  * arp is the track's too: a preset does not set it) */
    struct { uint8_t id; int8_t v; } more[4];   /* (Skiff) up to four more of the sound's values, {P_* id, value}: the
                                  * LFO, GLIDE, ENV -> PITCH (sound parameters only, never param_kept's: editor_test); id 0 ends */
    uint8_t tag;                 /* (Skiff phase 9) its type, PT_*: PRESETS' LIST shows one (ui.c preset_in_list) */
} preset_t;
#define FX(d, c, dl, r) .fx = {(d) + 1, (c) + 1, (dl) + 1, (r) + 1}
#define MORE(...) .more = {__VA_ARGS__}
#define TAG(t) .tag = (t)
enum { PT_NONE, PT_BASS, PT_LEAD, PT_PAD, PT_KEYS, PT_PLUCK, PT_DRUM, PT_FX, PT_N };

struct track;
typedef struct {                 /* an engine (engines.c ENGINES[]; the eng_*.c files) */
    const char *name;            /* "ANALOG" (PRESETS, the editor) */
    const char *page_title[2];   /* EDIT 1 and EDIT 2 */
    param_desc_t edit[8];        /* P_E0..P_E7 */
    const preset_t *presets;
    uint8_t npresets;
    uint8_t knob[4];             /* HOME: the four parameters on KNOB 1..4 */
    uint8_t poly;                /* voice cap for POLY and UNISON, 0 = NVOICE */
    uint8_t sampled;             /* 1 = plays recorded material (a position, not a phase): voice.c keeps
                                  * no phases over a retrigger, spreads none for UNISON, renders at SUS 0 */
    uint8_t keep;                /* bit k: s[k] is kept when a sounding voice is retriggered (filters) */
    uint8_t oneshot;             /* 1 = hits (DRUM): always POLY, no glide; amp ends its voices (eng_drum.c) */
    void (*note_on)(struct track *t, voice_t *v);
    void (*render)(struct track *t, voice_t *v, int32_t *out, uint32_t n, const vmod_t *m);
    /* optional (0 = none): the voice amplitude instead of the ADSR curve, once per control tick;
     * gets the ADSR value (Q15, env_tick already ran: it still gates the voice), returns Q15 */
    int32_t (*amp)(struct track *t, voice_t *v, int32_t adsr);
    /* optional: a mode-dependent descriptor of EDIT k (the same range and default as edit[k],
     * another label / value names), 0 = edit[k] */
    const param_desc_t *(*desc)(const struct track *t, uint32_t k);
    /* optional: once per block and part, before its voices (also with no voice sounding) */
    void (*block)(struct track *t);
    /* optional: the note of key k (0 = the lowest) when the engine maps the keys itself, else -1
     * (then the scale keyboard of seq.c kb_map) */
    int32_t (*keys)(const struct track *t, uint32_t k);
    /* 1 = the engine's own envelopes are the voice's amplitude (FM6): voice.c renders it at 1.0 (no ADSR, no
     * velocity; LFO -> AMP, the matrix's AMP and the fades still apply; ENV is 0) and the voice ends when
     * done() says so (once per control tick, before the render), not at the end of the ADSR's release */
    uint8_t ownenv;
    int (*done)(struct track *t, voice_t *v);
} engine_t;

/* (Fm1Studio) a Skiff track as its engines see it: its parameters (P_E0..P_E7 the engine's), its voices */
typedef struct track {
    int16_t p[P_COUNT];
    voice_t v[NVOICE];           /* (KAWA plays one note a track: NVOICE is 1 in kawa_sk.c) */
} track_t;
