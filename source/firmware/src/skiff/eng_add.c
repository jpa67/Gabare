/* SPDX-License-Identifier: GPL-3.0-only
 * Copyright (C) 2026 Leo Kuroshita (@kurogedelic), Hügelton Instruments
 * Portions Copyright (C) 2026 Juan Ignacio Antonucci (Juan Jiant)
 * (Gabare, modified 2026-11-11) from JIANT FM by juanjiant-bit, https://github.com/juanjiant-bit/JIANT (GPL-3.0-only),
 * its eng_add.c: its voice state in the track's engine memory (kawa_sk.c sk_mem, WHEEL's), the age counted in blocks
 * of 32 samples (JIANT's are 64: the decays, BLOOM and SHIMMER keep their times), Skiff's param_desc_t order; its
 * factory sounds are in kawa_data.c. */
/* ADD (JIANT 0.9.9-snd5): additive, nine sine partials a voice whose ratios, levels and decays are the sound.
 * MODE picks the partials' ratios: HARM the first nine harmonics, ODD the odd ones (a clarinet's, a square's), WIDE
 * 1 2 3 4 6 8 11 13 16 (bright, airy), BELL a church bell's (hum, prime, tierce, quint, nominal ..), BAR a free bar's
 * (a vibraphone's, a xylophone's), DRUM a membrane's (Bessel's zeros). (gen6d, the owner: the old knobs moved too
 * little) MORPH fades the ratios to the next MODE's (a saw into a bell), BRIGHT tilts the levels (dark .. flat at 96 ..
 * the highs over the fundamental), PARTIAL how many sound (1 .. 9, the last by its fraction: an additive filter),
 * SHIFT down stretches the ratios, up adds the same Hz to every partial (inharmonic, a ring's, a radio's), COMB notches
 * sweeping over the partials (|cos k phi|: a phaser of partials; LFO / ENV -> SHAPE sweep it), DECAY each partial's
 * ring (127: held), the higher ones sooner, MOTION down a BLOOM (the higher partials come in later), up a SHIMMER
 * (each partial its own slow wander). A
 * partial past ~19 kHz is silent (nothing aliases).
 *
 * Memory: none of its own. WHEEL's per voice state (eng_wheel.c drw_v: nine phases and gains a voice, by part) is a
 * part's either way (a part is WHEEL or ADD); the voice's age in blocks in its perc. Cost: WHEEL's (a table lookup and a
 * multiply-add a partial a sample); the levels and the decays once a block. A note from silence starts its partials in
 * phase (a bell's, a pluck's attack the same every time). */
static const char *const N_ADD_MODE[] = {"HARM", "ODD", "WIDE", "BELL", "BAR", "DRUM", "PRIME", "FIFTH", "SUB", "PLATE"};
enum { AD_HARM, AD_ODD, AD_WIDE, AD_BELL, AD_BAR, AD_DRUM, AD_PRIME, AD_FIFTH, AD_SUB, AD_PLATE, AD_N };
#define AD_R(x) ((uint16_t)((x) * 1024.0 + 0.5))         /* a ratio, Q10 */
static const uint16_t ADD_RATIO[AD_N][DRW_NP] = {
    {AD_R(1), AD_R(2), AD_R(3), AD_R(4), AD_R(5), AD_R(6), AD_R(7), AD_R(8), AD_R(9)},
    {AD_R(1), AD_R(3), AD_R(5), AD_R(7), AD_R(9), AD_R(11), AD_R(13), AD_R(15), AD_R(17)},
    {AD_R(1), AD_R(2), AD_R(3), AD_R(4), AD_R(6), AD_R(8), AD_R(11), AD_R(13), AD_R(16)},
    {AD_R(0.5), AD_R(1), AD_R(1.183), AD_R(1.506), AD_R(2), AD_R(2.514), AD_R(2.662), AD_R(3.011), AD_R(4.166)},
    {AD_R(1), AD_R(2.756), AD_R(5.404), AD_R(8.933), AD_R(13.344), AD_R(17.4), AD_R(9.85), AD_R(3.93), AD_R(15.1)},
    {AD_R(1), AD_R(1.594), AD_R(2.136), AD_R(2.296), AD_R(2.653), AD_R(2.918), AD_R(3.156), AD_R(3.501), AD_R(3.6)},
    /* (p5, the owner: what is additive's own) PRIME the prime harmonics only (a hollow, glassy organ: no octaves past the
     * 2nd), FIFTH an organ's mixture of fifths and octaves (a Quint stop), SUB under the note (the 1/4 and 1/2 a sub, a
     * deep drone over its own octaves), PLATE a plate's square-root partials (a gong, a thunder sheet). MORPH between
     * any two neighbours: a choir of spectra no other engine reaches */
    {AD_R(1), AD_R(2), AD_R(3), AD_R(5), AD_R(7), AD_R(11), AD_R(13), AD_R(17), AD_R(19)},
    {AD_R(1), AD_R(1.5), AD_R(2), AD_R(3), AD_R(4), AD_R(4.5), AD_R(6), AD_R(8), AD_R(9)},
    {AD_R(0.25), AD_R(0.5), AD_R(1), AD_R(1.5), AD_R(2), AD_R(3), AD_R(4), AD_R(5), AD_R(6)},
    {AD_R(1), AD_R(1.414), AD_R(1.732), AD_R(2), AD_R(2.236), AD_R(2.449), AD_R(2.646), AD_R(2.828), AD_R(3)},
};
/* log2(k + 1), Q8: the tilt over the partials' ranks */
static const uint16_t ADD_LOG2[DRW_NP] = {0, 256, 406, 512, 594, 662, 719, 768, 811};
#define AD_TOP 1932735283u                               /* ~19.8 kHz as a phase increment: above it a partial is silent */

static void add_note_on(track_t *t, voice_t *v)
{
    uint32_t vi = (uint32_t)(v - t->v) % NVOICE, k;
    drw_vc_t *V = &((drw_vc_t *)eng_part(t))[vi];   /* (Gabare) */
    if (!v->env) {                                       /* (from silence: in phase, its partials from nothing) */
        for (k = 0; k < DRW_NP; k++) {
            V->ph[k] = 0;
            V->gp[k] = 0;
        }
    }
    V->perc = 0;                                         /* (its age in blocks: the partials' decays from the note) */
    V->init = 1;
}

static void add_render(track_t *t, voice_t *v, int32_t *out, uint32_t n, const vmod_t *m)
{
    const int16_t *p = t->p;
    drw_vc_t *V = &((drw_vc_t *)eng_part(t))[(uint32_t)(v - t->v) % NVOICE];   /* (Gabare) */
    uint32_t mo = (uint32_t)p[P_E0] % AD_N;
    const uint16_t *R = ADD_RATIO[mo], *R2 = ADD_RATIO[(mo + 1u) % AD_N];
    int32_t acc[CTL], lv[DRW_NP], sum = 0, norm, mp = clamp(p[P_E1], 0, 127), slope = (96 - clamp(p[P_E2], 0, 127)) * 5;
    int32_t np = 256 + clamp(p[P_E3], 0, 127) * 2048 / 127, sh = clamp(p[P_E4], -64, 63), dec = clamp(p[P_E6], 0, 127);
    int32_t mot = clamp(p[P_E7], -64, 63), cmb = clamp(p[P_E5] + ((m->shape - (64 << 8)) >> 8), 0, 127) * 2;
    uint32_t i, k, age = (uint32_t)V->perc, rate = dec >= 127 ? 0u : str_exp2n((uint32_t)dec * 2201u / 127u) * 19u / 65u;
    uint32_t shz = sh > 0 ? (uint32_t)(sh * sh) * 19478u : 0u;   /* (SHIFT up: + sh^2 / 5 Hz to every partial) */
    if (n > CTL)
        n = CTL;
    for (k = 0; k < DRW_NP; k++) {                       /* the levels: BRIGHT's tilt, COMB's notches, PARTIALS */
        uint32_t lt = slope >= 0 ? (uint32_t)(slope * ADD_LOG2[k]) : (uint32_t)(-slope * (ADD_LOG2[DRW_NP - 1u] - ADD_LOG2[k]));
        int32_t l = (int32_t)(str_exp2n(lt >> 8) >> 1), kq = (int32_t)k << 8;
        if (cmb) {                                       /* |cos((k + 1) phi)|, phi 0 .. pi/2: notches sweeping */
            int32_t c = SINE[((k + 1u) * (uint32_t)cmb + 256u) & 1023u];
            c = c < 0 ? -c : c;
            l = (l * (c + 1024)) >> 15;              /* (l < 2^15, c < 2^15: in 32 bits) */
        }
        if (kq >= np) l = 0;
        else if (np - kq < 256) l = l * (np - kq) >> 8;  /* (the last one in by its fraction: a smooth sweep) */
        lv[k] = l;
        sum += l;
    }
    norm = (int32_t)((32768u * 32768u) / (uint32_t)(sum + 16384));
    for (i = 0; i < n; i++)
        acc[i] = 0;
    for (k = 0; k < DRW_NP; k++) {
        int32_t r = (int32_t)R[k] + ((((int32_t)R2[k] - (int32_t)R[k]) * mp * 258) >> 15), g0 = V->gp[k], g1;
        uint32_t inc, ph = V->ph[k];
        if (sh < 0)                                      /* SHIFT down: the ratios stretched (a stiff string, a bell bent) */
            r = clamp(r + ((r * ((-sh * (r - 1024)) >> 10)) >> 10), 256, 40000);
        inc = (m->inc >> 10) * (uint32_t)r + shz;
        if (inc >= AD_TOP || !lv[k])
            g1 = 0;
        else {
            uint32_t rk = rate + rate * k / 2u, x;       /* (its decay: Q16 octaves a block; the higher ones sooner) */
            x = !rk ? 0u : age >= 0xFFFFFFFFu / rk ? 0xFFFFu : (age * rk) >> 9;   /* (Gabare: >> 9, blocks of 32) */
            g1 = mulq15(mulq15(lv[k], norm), (int32_t)(str_exp2n(x) >> 1));
            if (mot < 0 && k) {                          /* BLOOM: partial k full after k * |MOTION| * 2 blocks */
                uint32_t span = k * (uint32_t)-mot * 4u;   /* (Gabare: blocks of 32) */
                if (age < span) g1 = g1 * (int32_t)age / (int32_t)span;
            } else if (mot > 0) {                        /* SHIMMER: each partial its own slow wander */
                int32_t w = SINE[((age * (2u + 3u * k)) / 4u + k * 397u) & 1023u];   /* (Gabare: blocks of 32) */
                g1 = (g1 * (32768 - ((mot * 300 * ((w + 32767) >> 1)) >> 15))) >> 15;
            }
        }
        V->gp[k] = g1;
        if (!g0 && !g1) {
            V->ph[k] = ph + inc * n;
            continue;
        }
        if (g0 == g1) {
            for (i = 0; i < n; i++) {
                ph += inc;
                acc[i] += (SINE[ph >> 22] * g1) >> 15;
            }
        } else {                                         /* the gain ramped over the block */
            int32_t gq = g0 << CTL_LOG2, dg = g1 - g0;
            for (i = 0; i < n; i++) {
                ph += inc;
                gq += dg;
                acc[i] += (SINE[ph >> 22] * (gq >> CTL_LOG2)) >> 15;
            }
        }
        V->ph[k] = ph;
    }
    V->perc = (int32_t)(age < 0x7FFFFFu ? age + 1u : age);
    for (i = 0; i < n; i++)
        out[i] += voice_amp(acc[i] >> 1, m, i) << 1;
}


static const engine_t ENG_ADD = {
    .name = "ADD",
    .page_title = {"SPECTRUM", "MOTION"},
    .edit = {
        {"MODE", N_ADD_MODE, 0, AD_N - 1, AD_HARM, F_ENUM, 0},
        {"MORPH", 0, 0, 127, 0, F_PCT, 0},
        {"BRIGHT", 0, 0, 127, 70, F_PCT, 0},
        {"PARTIAL", 0, 0, 127, 127, F_PCT, 0},
        {"SHIFT", 0, -64, 63, 0, F_INT, 0},
        {"COMB", 0, 0, 127, 0, F_PCT, 0},
        {"DECAY", 0, 0, 127, 127, F_PCT, 0},
        {"MOTION", 0, -64, 63, 0, F_INT, 0},
    },
    .note_on = add_note_on,
    .render = add_render,
    .knob = {P_E0, P_E3, P_E5, P_E6},
};
