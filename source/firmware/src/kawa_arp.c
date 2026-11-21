/* SPDX-License-Identifier: GPL-3.0-only */
/* (Gabare) The arpeggiator: the keys held on a track that plays notes sound one after the other, on the tempo (the
 * ARPEGGIO page, SCL after SCALE). Its MODE (UP, DOWN, UP-DOWN, RANDOM, AS PLAYED), its RATE (1/32 .. 1/4, with the
 * triplets), its OCTAVES (1..4) and its GATE (how long each note lasts in its step; 16: legato); ARP on its page holds
 * the notes after the keys are let go (LATCH: the next chord replaces them).
 * (Gabare 0.9) SLOOP's fifteen modes (2.7, after Felucca 1.4 / 1.5.1): with C E G held, REPEAT the last key again and
 * again; DOWN+UP G E C E; UP+8VA C E G C' (the first note again an octave over the top); CONVERGE C G E (outside in);
 * DIVERGE E G C (inside out); PINKY C G E G (the top note between the others); THUMB C E C G (the bottom one); WALK a
 * random step up or down the notes (bouncing off their ends); CHORD all the held notes together (the first four, an
 * ACC chord), an octave higher each step through OCTAVES. ARPEGGIO 2: its SWING (every other step later, 0..100: 100
 * is 75 / 25, as SLOOP's) and its CHANCE (a step plays or rests).
 *
 * The main loop sends the keys (EV_ARP_ADD, EV_ARP_DEL, EV_ARP_CLR, by the event queue); the audio interrupt keeps the
 * notes, the clock and the voice. Playing, the arpeggio runs on the sequencer's rows (its swing and groove too: a step
 * is a share of the row as it plays); stopped, on its own clock at the tempo, from the first key. While it plays the
 * sequencer leaves that track's notes unplayed. REC while the song plays: each note is written into the row playing
 * (a note between two rows with a DELAY, 1..23 24ths; a row holds one note: at 1/32 and 1/16T, the first of the row),
 * by the main loop (arp_wq, arp_drain). Included by kawa_seq.c,
 * after seq_play_cell. */
enum { AM_OFF, AM_UP, AM_DOWN, AM_UPDN, AM_RAND, AM_PLAYED, AM_REPEAT, AM_DNUP, AM_UP8, AM_CONV, AM_DIVG, AM_PINKY,
       AM_THUMB, AM_WALK, AM_CHORD, AM_N };
enum { AR_32, AR_16T, AR_16, AR_8T, AR_8, AR_4, AR_N };
static const uint32_t ARP_RATE_Q16[AR_N] = {32768u, 43691u, 65536u, 87381u, 131072u, 262144u};   /* a step, in rows (16ths), Q16 */
#define ARP_MAXN 8u                             /* notes held at once */
#define ARP_GATE_N 16u                          /* GATE: sixteenths of a step (16: legato, the note lasts to the next) */

/* main -> ISR: the setting; REC on a page that writes */
static volatile uint8_t arp_mode, arp_rate = AR_16, arp_oct = 1, arp_gate = 8, arp_rec;
static volatile uint8_t arp_swing, arp_chance = 100;   /* (Gabare 0.9) ARPEGGIO 2: 0..100 each */
/* ISR -> main: what the page shows */
static volatile uint8_t arp_now, arp_count, arp_list[ARP_MAXN], arp_on_trk = 0xFF;

/* ISR */
static uint8_t a_n, a_trk = 0xFF, a_vol = 12, a_on, a_open;
static uint8_t a_note[ARP_MAXN];                /* as played */
static uint8_t a_walk;                          /* (0.9) WALK: where it stands in the notes */
static uint32_t a_idx, a_tick = 0xFFFFFFFFu, a_free, a_wstep = 0xFFFFFFFFu;   /* a_wstep: the row REC wrote last (s_step) */

/* ISR -> main: the notes REC writes */
typedef struct { uint8_t pat, row, delay, trk, note, vol; } arp_w_t;
#define ARP_WQ 16u
static arp_w_t arp_wq[ARP_WQ];
static volatile uint32_t arp_wq_w, arp_wq_r;

static uint32_t arp_pos(void)                   /* where the arpeggio's clock is, in rows (Q16) */
{
    return sq_play ? s_step * 65536u + (s_phase >> 16) : a_free;
}

static void arp_publish(void)
{
    uint32_t i;
    for (i = 0; i < ARP_MAXN; i++)
        arp_list[i] = i < a_n ? a_note[i] : 0u;
    arp_count = a_n;
    arp_on_trk = a_n ? a_trk : 0xFF;
}

static void arp_silence(void)                   /* (arp_now stays: the page keeps the last note lit to the next) */
{
    if (a_on && a_trk < NTRK && tk[a_trk].note == a_on && note_mode(a_trk) == NM_NOTE)
        voice_off(a_trk);
    a_on = 0;
    a_open = 0;
}

static void arp_event(uint32_t type, uint32_t t, uint32_t note, uint32_t vol)
{
    uint32_t i, j;
    if (type == EV_ARP_CLR) {
        a_n = 0;
    } else if (type == EV_ARP_ADD && t < NTRK) {
        if (t != a_trk) {                       /* another track: the arpeggio moves to it */
            arp_silence();
            a_n = 0;
            a_trk = (uint8_t)t;
        }
        for (i = 0; i < a_n; i++)
            if (a_note[i] == note) break;
        if (i == a_n && a_n < ARP_MAXN) {
            if (!a_n) {                         /* the first key: from the start (stopped: at once; playing: on the */
                a_idx = 0;                      /* next step) */
                a_free = 0;
                a_tick = sq_play ? arp_pos() / ARP_RATE_Q16[arp_rate % AR_N] : 0xFFFFFFFFu;
            }
            a_note[a_n++] = (uint8_t)note;
        }
        a_vol = (uint8_t)(vol & 15u);
    } else if (type == EV_ARP_DEL) {
        for (i = j = 0; i < a_n; i++)
            if (a_note[i] != note) a_note[j++] = a_note[i];
        a_n = (uint8_t)j;
    }
    arp_publish();
}

/* the notes of one round, in order: held (sorted, or as played), then an octave up, ... */
static uint32_t arp_round(uint8_t *out)
{
    uint8_t s[ARP_MAXN];
    uint32_t i, j, o, n = 0, oct = arp_oct < 1u ? 1u : arp_oct > 4u ? 4u : arp_oct;
    for (i = 0; i < a_n; i++)
        s[i] = a_note[i];
    if (arp_mode != AM_PLAYED)
        for (i = 1; i < a_n; i++)
            for (j = i; j && s[j - 1u] > s[j]; j--) {
                uint8_t x = s[j];
                s[j] = s[j - 1u];
                s[j - 1u] = x;
            }
    for (o = 0; o < oct; o++)
        for (i = 0; i < a_n; i++)
            if (s[i] + 12u * o <= 119u)
                out[n++] = (uint8_t)(s[i] + 12u * o);
    if (arp_mode == AM_UP8 && n && s[0] + 12u * oct <= 119u)
        out[n++] = (uint8_t)(s[0] + 12u * oct);
    return n;
}

static uint32_t arp_next(void)
{
    uint8_t q[ARP_MAXN * 4u + 1u];
    uint32_t n = arp_round(q), k, p, i;
    if (!n)
        return 0;
    switch (arp_mode) {
    case AM_DOWN: k = n - 1u - a_idx % n; break;
    case AM_UPDN:                               /* up, then down without playing the ends twice */
        p = n > 1u ? 2u * n - 2u : 1u;
        k = a_idx % p;
        if (k >= n) k = p - k;
        break;
    case AM_RAND: k = (xr(&rnd_isr) >> 8) % n; break;
    case AM_REPEAT: a_idx++; return a_note[a_n - 1u];   /* (0.9) the last key, again */
    case AM_DNUP:                               /* down, then up, the ends once */
        p = n > 1u ? 2u * n - 2u : 1u;
        i = a_idx % p;
        k = i < n ? n - 1u - i : i - n + 1u;
        break;
    case AM_CONV:                               /* outside in: low, high, the next low, the next high... */
    case AM_DIVG:                               /* inside out: the same, backwards */
        i = a_idx % n;
        if (arp_mode == AM_DIVG) i = n - 1u - i;
        k = i & 1u ? n - 1u - i / 2u : i / 2u;
        break;
    case AM_PINKY:                              /* the top note between the others */
    case AM_THUMB:                              /* the bottom note between the others */
        if (n < 2u) { k = 0; break; }
        i = a_idx % (2u * (n - 1u));
        if (arp_mode == AM_PINKY) k = i & 1u ? n - 1u : i / 2u;
        else k = i & 1u ? 1u + i / 2u : 0u;
        break;
    case AM_WALK:                               /* a step up or down, off the ends */
        if (a_idx == 0u || a_walk >= n) a_walk = 0;
        else if (n > 1u) {
            int32_t w = (int32_t)a_walk + ((xr(&rnd_isr) >> 9) & 1u ? 1 : -1);
            a_walk = (uint8_t)(w < 0 ? 1 : w >= (int32_t)n ? (int32_t)n - 2 : w);
        }
        k = a_walk;
        break;
    case AM_CHORD:                              /* the chord's root (arp_chunk plays the rest), an octave a step */
        p = n / a_n ? n / a_n : 1u;
        k = (a_idx % p) * a_n;
        break;
    default: k = a_idx % n; break;              /* UP, AS PLAYED */
    }
    a_idx++;
    return q[k];
}

/* a chunk: the next step's note, the end of the one sounding (its GATE) */
static void arp_chunk(void)
{
    uint32_t R = ARP_RATE_Q16[arp_rate % AR_N], P, tick, ph;
    if (!sq_play)
        a_free += (s_inc * CHUNK) >> 16;
    if (arp_mode == AM_OFF || !a_n || a_trk >= NTRK || note_mode(a_trk) != NM_NOTE) {
        if (a_on) arp_silence();
        arp_now = 0;
        if (arp_mode == AM_OFF && a_n) { a_n = 0; arp_publish(); }
        return;
    }
    P = arp_pos();
    tick = P / R;
    ph = P - tick * R;
    if (tick & 1u) {                            /* (0.9) SWING: every other step later, up to half a step */
        uint32_t late = (uint32_t)(((uint64_t)R * arp_swing) / 200u);
        if (ph < late) { tick--; ph += R - late; } else ph -= late;
    }
    if (tick != a_tick) {
        uint32_t note = arp_next();
        tricks_t k = NO_TRICKS;
        a_tick = tick;
        if (!note) return;
        if (arp_chance < 100u && (xr(&rnd_isr) >> 8) % 100u >= arp_chance) {   /* (0.9) CHANCE: this step rests */
            if (a_on) arp_silence();
            return;
        }
        if (arp_mode == AM_CHORD && a_n > 1u) {   /* (0.9) CHORD: the held notes together, from the lowest */
            uint8_t s[ARP_MAXN];
            uint32_t i, j, c = a_n < CHORD_MAX ? a_n : CHORD_MAX;
            for (i = 0; i < a_n; i++) s[i] = a_note[i];
            for (i = 1; i < a_n; i++)
                for (j = i; j && s[j - 1u] > s[j]; j--) { uint8_t x = s[j]; s[j] = s[j - 1u]; s[j - 1u] = x; }
            for (i = 0; i < CHORD_MAX; i++) chord_custom[a_trk][i] = (int8_t)(i < c ? s[i] - s[0] : 0);
            chord_custom_n[a_trk] = (uint8_t)c;
            k.fx = FX_CHORD;
            k.v = CHORD_CUSTOM;
        }
        seq_play_cell(a_trk, note, a_vol, &k);
        a_on = (uint8_t)note;
        a_open = 1;
        arp_now = (uint8_t)note;
        if (arp_rec && sq_play && s_step != a_wstep && arp_wq_w - arp_wq_r < ARP_WQ) {   /* REC: into the row playing (a row
                                                                                      * holds one note: faster than the
                                                                                      * rows, its first) */
            arp_w_t *w = &arp_wq[arp_wq_w % ARP_WQ];
            w->pat = s_pat;
            w->row = s_row;
            w->delay = (uint8_t)(((s_phase >> 16) * 24u) >> 16);   /* (0: on the row; 1..23: a DELAY) */
            w->trk = a_trk;
            w->note = (uint8_t)note;
            w->vol = a_vol;
            a_wstep = s_step;
            RING_PUBLISH();
            arp_wq_w++;
        }
    } else if (a_open && arp_gate < ARP_GATE_N && ph >= (R >> 4) * arp_gate) {
        arp_silence();
    }
}

/* the sequencer leaves the arpeggio's track alone while it plays */
static int arp_owns(uint32_t t)
{
    return arp_mode != AM_OFF && a_n && a_trk == t;
}
