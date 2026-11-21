/* SPDX-License-Identifier: GPL-3.0-only */
/* KAWA 0.3's new features, played on this computer through the audio interrupt (tests/run_features_test.sh):
 * HOP and END, the second trick column, the groove, the tables, the sample edit, the MIDI clock in. Each
 * check says what it expected and what came. */
#define main kawa_host_main
#include "../firmware/src/kawa.c"
#undef main

#define BLK 256u
static int32_t out[2u * BLK];
static int fails;
#define CHECK(c, ...) do { if (!(c)) { printf("FAIL  " __VA_ARGS__); printf("\n"); fails++; } } while (0)

static void block(void)
{
    uint32_t i;
    for (i = 0; i < 2u * BLK; i++) out[i] = 0;
    app_audio(out, BLK);
}

static uint32_t rows_seen[64], nrows;
static void play_rows(uint32_t n)               /* plays until n rows have started, noting each */
{
    uint32_t last = sq_rowtick, guard = 0;
    nrows = 0;
    while (nrows < n && guard++ < 20000u) {
        block();
        if (sq_rowtick != last) {
            last = sq_rowtick;
            if (nrows < 64u) rows_seen[nrows++] = sq_row;
        }
    }
}

static void fresh(void)
{
    sq_req = 2;
    block();
    prj_new();
    prj.bpm = 160;                              /* (these timings are for 160; a new song starts at 120) */
    prj.mode = 0;
    sq_next = 0xFF;
    ev_push(EV_KILL, NTRK, 0, 0, 0, 0);
    block();
}

static void start(void)
{
    sq_start_pat = 0;
    sq_start_pos = 0;
    sq_req = 1;
}

static uint32_t last_reply(uint8_t *d, uint32_t max)   /* (Gabare 0.9) the last SysEx reply, unpacked */
{
    uint32_t w = 5;
    while (w < sizeof kw_wire && kw_wire[w] != 0xF7u) w++;
    return kw_unpack7(kw_wire + 5, w - 5u, d, max);
}

int main(void)
{
    uint32_t i, k;
    char got[160];
    /* 1. HOP: row 4 (index 3) goes on to row 8 (index 7) */
    fresh();
    prj.pat[0].len = 16;
    prj.pat[0].c[3][2].fx2 = FX_HOP;
    prj.pat[0].c[3][2].fxv2 = 7;
    start();
    play_rows(1);                               /* (the start's row 0 is already on) */
    play_rows(14);
    got[0] = 0;
    for (i = 0; i < nrows; i++) fmt_u(got + str_len(got), rows_seen[i], 0), str_cat(got, " ");
    {
        static const uint8_t WANT[14] = {1, 2, 3, 7, 8, 9, 10, 11, 12, 13, 14, 15, 0, 1};
        for (i = 0; i < 14u; i++) CHECK(rows_seen[i] == WANT[i], "1. HOP: rows %s(want 1 2 3 7 8 ... 15 0 1)", got);
    }
    printf("1. HOP to row 8 on row 4: %s\n", got);
    /* 2. END: the pattern ends after row 6; in the song, the next entry plays */
    fresh();
    prj.pat[0].len = 16;
    prj.pat[0].c[5][0].fx = FX_END;
    prj.pat[1].len = 16;
    prj.song[0].pat = 0; prj.song[0].rep = 1;
    prj.song[1].pat = 1; prj.song[1].rep = 1;
    prj.song_len = 2;
    prj.mode = 1;
    start();
    play_rows(1);
    play_rows(8);
    got[0] = 0;
    for (i = 0; i < nrows; i++) fmt_u(got + str_len(got), rows_seen[i], 0), str_cat(got, " ");
    CHECK(rows_seen[4] == 5u && rows_seen[5] == 0u && sq_pat == 1u, "2. END: rows %s, pattern %u (want ... 5 0, pattern 2)",
          got, sq_pat + 1u);
    printf("2. END on row 6: rows %s, then pattern %u\n", got, sq_pat + 1u);
    /* 3. two tricks in one cell: both play (VIB in the first column, PAN in the second) */
    fresh();
    prj.pat[0].c[0][0].note = 69;
    prj.pat[0].c[0][0].vol = 12;
    prj.pat[0].c[0][0].fx = FX_VIB;
    prj.pat[0].c[0][0].fxv = 9;
    prj.pat[0].c[0][0].fx2 = FX_PAN;
    prj.pat[0].c[0][0].fxv2 = (uint8_t)(int8_t)-7;
    start();
    block();
    CHECK(tk[0].vib == 9u && tk[0].npan == -7, "3. two tricks: vibrato %u pan %d (want 9, -7)", tk[0].vib, tk[0].npan);
    printf("3. VIB 9 + PAN -7 in one cell: vibrato %u, pan %d\n", tk[0].vib, tk[0].npan);
    /* 4. DELAY in one column, PITCH in the other: the late note keeps its pitch */
    fresh();
    prj.pat[0].c[0][0].note = 60;
    prj.pat[0].c[0][0].vol = 12;
    prj.pat[0].c[0][0].fx = FX_PITCH;
    prj.pat[0].c[0][0].fxv = 7;
    prj.pat[0].c[0][0].fx2 = FX_DELAY;
    prj.pat[0].c[0][0].fxv2 = 12;
    start();
    block();
    k = (uint32_t)tk[0].gate;                   /* (half a row at 160 BPM: 7.5 blocks; the row 15) */
    for (i = 0; i < 11u; i++) block();
    CHECK(!k && tk[0].gate && tk[0].pitch == 67 * 256, "4. DELAY + PITCH: gate %u then %u, pitch %d (want 0, 1, %d)", k,
          tk[0].gate, tk[0].pitch, 67 * 256);
    printf("4. DELAY 12 + PITCH +7: late, at %d/256 semitones\n", tk[0].pitch);
    /* 5. a table: +0 +12 +7, a row a step, looping; a HIT on a KIT track plays the drum again; the stop ends it */
    fresh();
    prj.tbl[0].on = 1;
    prj.tbl[0].speed = 6;
    prj.tbl[0].len = 3;
    prj.tbl[0].loop = 0;
    prj.tbl[0].s[1].tr = 12;
    prj.tbl[0].s[2].tr = 7;
    prj.tbl[0].s[2].vol = 5;
    ev_push(EV_ON, 0, 60, 12, 0, 0);
    {
        int32_t seen[8];
        uint32_t f = 0, n = 0, rowf = g_rowlen;
        while (n < 7u) {                        /* the table's pitch in the middle of each row */
            uint32_t j;
            for (j = 0; j < 16u; j++) out[j] = 0;
            app_audio(out, 16);
            f += 16u;
            if (f >= rowf / 2u + n * rowf) seen[n++] = tk[0].tb_tr;
        }
        got[0] = 0;
        for (i = 0; i < 7u; i++) fmt_s(got + str_len(got), seen[i]), str_cat(got, " ");
        CHECK(seen[0] == 0 && seen[1] == 12 && seen[2] == 7 && seen[3] == 0 && seen[4] == 12 && seen[5] == 7 &&
              tk[0].vol == 5u, "5. table: steps %s(want 0 12 7 0 12 7), volume %u (want 5: '--' keeps it)", got, tk[0].vol);
        printf("5. a table +0 +12 +7, looping, a row a step: %s\n", got);
    }
    fresh();
    sound_preset(4, preset_named(E_KIT, 0));
    prj.tbl[4].on = 1;
    prj.tbl[4].speed = 3;
    prj.tbl[4].len = 2;
    prj.tbl[4].loop = 0;
    prj.tbl[4].s[0].fx = TB_HIT;
    prj.tbl[4].s[1].fx = TB_HIT;
    prj.tbl[4].s[1].tr = 5;
    ev_push(EV_ON, 4, 1, 12, 0, 0);
    {
        uint32_t hits = 0, j, last = 0;
        for (j = 0; j < 60u; j++) {             /* ~350 ms: a hit every half row (3 ticks) */
            block();
            if (drum[4][drum_next[4] ^ 1u].t < last) hits++;
            last = drum[4][drum_next[4] ^ 1u].t;
        }
        CHECK(hits >= 5u && drum[4][drum_next[4] ^ 1u].tune == prj.snd[4].p[P_TUNE] + 5 * ((tk[4].tb_i & 1u)),
              "5. table HIT on a KIT: %u hits in 350 ms (want ~6)", hits);
        printf("5. a table of HITs on a KIT track: %u hits in 350 ms\n", hits);
        sq_req = 2;
        block();
        CHECK(!tk[4].tb_run, "5. the stop did not end the table");
        for (j = 0; j < 100u; j++) block();
        CHECK(!drum[4][0].on && !drum[4][1].on, "5. a drum still plays after the stop");
    }
    /* 6. the groove: 9 then 3 ticks; the rows last that long, the MIDI clock sends as many ticks */
    fresh();
    prj.grv.on = 1;
    prj.grv.len = 2;
    prj.grv.tick[0] = 9;
    prj.grv.tick[1] = 3;
    start();
    {
        uint32_t f = 0, last = sq_rowtick, n = 0, at[9], t0 = clk_ticks, j;
        while (n < 9u) {
            for (j = 0; j < 32u; j++) out[j] = 0;
            app_audio(out, 16);
            f += 16u;
            if (sq_rowtick != last) { last = sq_rowtick; at[n++] = f; }
        }
        {
            uint32_t a = at[1] - at[0], b = at[2] - at[1], want_a = g_rowlen * 3u / 2u, want_b = g_rowlen / 2u;
            uint32_t ticks = clk_ticks - t0;
            CHECK((a > want_a ? a - want_a : want_a - a) <= 16u && (b > want_b ? b - want_b : want_b - b) <= 16u,
                  "6. groove 9/3: rows of %u and %u frames (want %u, %u)", a, b, want_a, want_b);
            CHECK(ticks >= 48u && ticks <= 49u, "6. groove: %u clock ticks over 8 rows (want 48)", ticks);
            printf("6. groove 9 3: rows of %u and %u frames (a straight row: %u), %u MIDI ticks in 8 rows\n", a, b,
                   g_rowlen, ticks);
        }
    }
    /* 7. (Gabare 0.9) the arpeggio's modes (SLOOP's), C E G held (as played: E C G), one octave; then two for CHORD */
    {
        static const struct { uint8_t mode, want[8]; } M[] = {
            {AM_UP, {60, 64, 67, 60, 64, 67, 60, 64}},   {AM_DOWN, {67, 64, 60, 67, 64, 60, 67, 64}},
            {AM_UPDN, {60, 64, 67, 64, 60, 64, 67, 64}}, {AM_PLAYED, {64, 60, 67, 64, 60, 67, 64, 60}},
            {AM_REPEAT, {67, 67, 67, 67, 67, 67, 67, 67}}, {AM_DNUP, {67, 64, 60, 64, 67, 64, 60, 64}},
            {AM_UP8, {60, 64, 67, 72, 60, 64, 67, 72}},  {AM_CONV, {60, 67, 64, 60, 67, 64, 60, 67}},
            {AM_DIVG, {64, 67, 60, 64, 67, 60, 64, 67}}, {AM_PINKY, {60, 67, 64, 67, 60, 67, 64, 67}},
            {AM_THUMB, {60, 64, 60, 67, 60, 64, 60, 67}},
        };
        uint32_t m, j, ok = 1;
        for (m = 0; m < sizeof M / sizeof M[0]; m++) {
            arp_mode = M[m].mode; arp_oct = 1;
            a_n = 3; a_note[0] = 64; a_note[1] = 60; a_note[2] = 67; a_idx = 0;
            for (j = 0; j < 8u; j++) {
                uint32_t n = arp_next();
                if (n != M[m].want[j]) { CHECK(0, "7. arpeggio mode %u, step %u: %u (want %u)", M[m].mode, j, n, M[m].want[j]); ok = 0; break; }
            }
        }
        arp_mode = AM_WALK; a_idx = 0;           /* WALK: each step next to the last */
        {
            uint32_t last = arp_next(), n;
            for (j = 0; j < 30u; j++) {
                n = arp_next();
                if (!((last == 60u && n == 64u) || (last == 64u && (n == 60u || n == 67u)) || (last == 67u && n == 64u))) {
                    CHECK(0, "7. WALK jumped from %u to %u", last, n); ok = 0; break;
                }
                last = n;
            }
        }
        arp_mode = AM_CHORD; arp_oct = 2; a_idx = 0;   /* CHORD: its root, an octave a step */
        CHECK(arp_next() == 60u && arp_next() == 72u && arp_next() == 60u, "7. CHORD: roots 60, 72, 60");
        arp_mode = AM_OFF; arp_oct = 1; a_n = 0;
        if (ok) printf("7. the arpeggio's 15 modes: UP .. THUMB as SLOOP's for C E G, WALK by steps, CHORD by octaves\n");
    }
    /* 7b. (Gabare 0.9) MIDI by channel: channel 3 plays track 3, its CC 7 its volume, a program its sound, the pedal */
    fresh();
    {
        uint32_t pr = prj.snd[2].preset;
        set.mchan = 1;
        host_midi_in[hmi_w++ % 64u] = 0x92u | 60u << 8 | 100u << 16;   /* note on, channel 3 */
        host_midi_in[hmi_w++ % 64u] = 0xB2u | 7u << 8 | 0u << 16;      /* CC 7: 0 */
        host_midi_in[hmi_w++ % 64u] = 0xC2u | 1u << 8;                 /* program 2 of the engine */
        host_midi_in[hmi_w++ % 64u] = 0xB2u | 64u << 8 | 127u << 16;   /* the pedal down */
        host_midi_in[hmi_w++ % 64u] = 0x82u | 60u << 8;                /* note off: held */
        midi_in();
        block();
        CHECK(tk[2].note == 60u && tk[2].gate, "7b. channel 3: track 3 plays note %u (gate %u)", tk[2].note, tk[2].gate);
        CHECK(prj.snd[2].p[P_VOL] == 0, "7b. CC 7 on channel 3: volume %d", prj.snd[2].p[P_VOL]);
        CHECK(prj.snd[2].preset != pr && PRESETS[prj.snd[2].preset].engine == ENG(2), "7b. program change: sound %u", prj.snd[2].preset);
        host_midi_in[hmi_w++ % 64u] = 0xB2u | 64u << 8;                /* the pedal up: the note goes */
        midi_in();
        block();
        CHECK(!tk[2].gate, "7b. the pedal up: the held note ends");
        host_midi_in[hmi_w++ % 64u] = 0xE2u | 0x7Fu << 8 | 0x7Fu << 16;   /* bend all the way up */
        midi_in();
        CHECK(midi_bend[2] > 500 && midi_bend[2] <= 512, "7b. pitch bend up: %d (want ~512: 2 semitones)", midi_bend[2]);
        midi_bend[2] = 0;
        set.mchan = 0;
        printf("7b. MIDI channels: a note, CC 7, a program, the pedal, the bend on track 3\n");
    }
    /* 8. the MIDI clock in: START, then 120 BPM; the rows follow the ticks, the tempo is measured; 150 BPM; STOP */
    fresh();
    set.clock = 2;
    clk_in = 1;
    clock_in_reset();
    {
        uint32_t f = 0, nt = 0, last = sq_rowtick, rows = 0, worst = 0, worst_any = 0, j, bpm_seen = 0;
        double tf = 44100.0 * 60.0 / 120.0 / 24.0, next = 0.0;
        clock_in(0xFA);
        while (nt < 24u * 16u) {                /* 16 beats at 120, then 150 */
            if (nt == 24u * 8u) tf = 44100.0 * 60.0 / 150.0 / 24.0;
            for (j = 0; j < 32u; j++) out[j] = 0;
            app_audio(out, 16);
            f += 16u;
            fm1_ms = (uint32_t)((uint64_t)f * 1000u / 44100u);
            if ((double)f >= next) {            /* a tick comes (as the USB would bring it: on a block's edge) */
                clock_in(0xF8);
                nt++;
                next += tf;
            }
            if (sq_rowtick != last) {           /* a row starts: how far from its tick (6 a row) */
                uint32_t want = 1u + rows * 6u, have = clk_in_ticks, d = have > want ? have - want : want - have;
                uint32_t beat = rows / 4u;
                last = sq_rowtick;
                if (d > worst_any) worst_any = d;
                if (((beat >= 2u && beat < 8u) || beat >= 10u) && d > worst) worst = d;   /* (the tempo known) */
                rows++;
            }
            if (nt == 24u * 8u - 1u) bpm_seen = bpm_now();
        }
        CHECK(rows >= 63u && rows <= 65u, "8. clock in: %u rows for 64 (16 beats)", rows);
        CHECK(worst <= 1u && worst_any <= 2u, "8. clock in: a row started %u (%u while the tempo changed) ticks away",
              worst, worst_any);
        CHECK(bpm_seen >= 119u && bpm_seen <= 121u && bpm_now() >= 149u && bpm_now() <= 151u,
              "8. clock in: tempo %u then %u (want 120, 150)", bpm_seen, bpm_now());
        printf("8. MIDI clock in at 120 then 150 BPM: %u rows in 16 beats, each within %u tick of its own (%u while the "
               "tempo changed); tempo %u, %u\n", rows, worst, worst_any, bpm_seen, bpm_now());
        clock_in(0xFC);
        block();
        CHECK(!sq_play, "8. STOP: still playing");
    }
    {                                           /* as on the FM-1: blocks of 256 frames, each tick 0..3 ms late (the USB,
                                                 * the main loop); where each row starts against its tick */
        uint32_t f = 0, nt = 0, last, rows = 0, j, seed = 12345u, pend = 0, pend_at = 0;
        int32_t err_min = 1 << 30, err_max = -(1 << 30), sum = 0, nerr = 0;
        double tf = 44100.0 * 60.0 / 133.0 / 24.0, next = 0.0;
        fresh();
        set.clock = 2;
        clk_in = 1;
        clock_in_reset();
        last = sq_rowtick;
        clock_in(0xFA);
        while (nt < 24u * 32u) {                /* 32 beats at 133 BPM */
            block();
            f += BLK;
            fm1_ms = (uint32_t)((uint64_t)f * 1000u / 44100u);
            if (sq_rowtick != last) {           /* the row started in this block: where in it */
                uint32_t into = (uint32_t)(((uint64_t)(sq_frac >> 16) * s_len) >> 16);
                int32_t start = (int32_t)(f - into), want = (int32_t)(rows * 6u * tf), e = start - want;
                last = sq_rowtick;
                if (rows >= 16u) {
                    if (e < err_min) err_min = e;
                    if (e > err_max) err_max = e;
                    sum += e;
                    nerr++;
                }
                rows++;
            }
            for (j = 0; j < 3u; j++) {          /* the ticks of this block, each seen 0..3 ms after its time */
                if (!pend && (double)f >= next) {
                    seed = seed * 1103515245u + 12345u;
                    pend = 1;
                    pend_at = (uint32_t)next + ((seed >> 16) % 133u);
                    next += tf;
                }
                if (pend && f >= pend_at) { clock_in(0xF8); nt++; pend = 0; }
            }
        }
        CHECK(nerr > 0 && err_max - err_min < 441 && sum / nerr < 441 && sum / nerr > -100,
              "8. clock in, blocks of 256, late ticks: rows %d..%d frames off (mean %d)", err_min, err_max, nerr ? sum / nerr : 0);
        printf("8. as on the FM-1 (blocks of 256, ticks 0..3 ms late): rows %.1f..%.1f ms after their tick (mean %.1f ms)\n",
               err_min / 44.1, err_max / 44.1, nerr ? sum / nerr / 44.1 : 0.0);
        clock_in(0xFC);
        block();
    }
    {                                           /* PLAY on the clock in, and no clock: it waits on the first row */
        uint32_t j, r0;
        clock_in_reset();
        sq_tick0 = clk_in_ticks;
        start();
        block();
        r0 = sq_rowtick;
        for (j = 0; j < 400u; j++) block();
        CHECK(sq_play && sq_rowtick == r0, "8. no clock: %u rows went by (want none)", sq_rowtick - r0);
        printf("8. PLAY on the clock in without a clock: it waits (%u rows in 2.3 s)\n", sq_rowtick - r0);
        sq_req = 2;
        block();
        clk_in = 0;
        set.clock = 0;
    }
    /* 9. the settings: those of 0.2 (four bytes) keep the language and the theme, and take the smooth font; a new
     * FM-1 starts in English with it; a font chosen comes back */
    {
        static const uint8_t OLD[4] = {LANG_FR, 1, 0, 0};
        memset(host_settings, 0, sizeof host_settings);
        settings_save(OLD, 4);
        memset(&set, 0, sizeof set);
        lang = LANG_EN;
        app_init();
        CHECK(lang == LANG_FR && theme == 1u && g_font == FN_DOUCE, "9. settings of 0.2: language %u theme %u font %u (want FR, 1, "
              "DOUCE)", lang, theme, g_font);
        memset(host_settings, 0, sizeof host_settings);
        memset(&set, 0, sizeof set);
        lang = LANG_EN;
        app_init();
        CHECK(lang == LANG_EN && theme == 0u && g_font == FN_DOUCE, "9. a new FM-1: language %u theme %u font %u", lang, theme,
              g_font);
        g_font_set(FN_8BIT);
        set_changed();
        settings_save(&set, sizeof set);
        g_font_set(FN_BASE);
        app_init();
        CHECK(g_font == FN_8BIT, "9. the font chosen did not come back (%u)", g_font);
        printf("9. settings: 0.2's keep FR and the theme, font DOUCE; new: EN, DOUCE; a choice is kept\n");
    }
    {   /* 10. (Gabare 0.9) the flash chip: INFO says if it is known; on an unknown one, samples and the DX7 bank
         * are refused and nothing is written (as the saves, the USB rescue and the update loader) */
        static const uint8_t SLOT0[1] = {0};
        uint8_t d[64];
        uint32_t nd;
        sq_req = 2;
        block();
        kw_handle(KW_INFO, 0, 0);
        nd = last_reply(d, sizeof d);
        CHECK(nd >= 27u && d[26] == 1u, "10. INFO on a known chip: %u bytes, byte 26 = %u (want 27, 1)", nd, nd > 26u ? d[26] : 0u);
        kw_flash_known = 0;
        memset(kw_flash + (DX_AT - USR_AT), 0x5A, 0x1000u);
        kw_handle(KW_INFO, 0, 0);
        nd = last_reply(d, sizeof d);
        CHECK(nd >= 27u && d[26] == 0u, "10. INFO on an unknown chip: byte 26 = %u (want 0)", nd > 26u ? d[26] : 9u);
        kw_handle(KW_BEGIN, SLOT0, 1);
        last_reply(d, sizeof d);
        CHECK(d[0] == KW_EARGS, "10. sample BEGIN (no sample slot since 0.9): rc %u (want 1)", d[0]);
        kw_handle(KW_DXBEGIN, SLOT0, 1);
        last_reply(d, sizeof d);
        CHECK(d[0] == KW_EFLASH, "10. DX BEGIN on an unknown chip: rc %u (want 3)", d[0]);
        CHECK(kw_flash[DX_AT - USR_AT] == 0x5Au && kw_flash[DX_AT - USR_AT + 0xFFFu] == 0x5Au,
              "10. an unknown chip was written to");
        CHECK(kw_slot == 0xFFu && !audio_hold, "10. a session was left open (slot %u, hold %u)", kw_slot, audio_hold);
        kw_flash_known = 1;
        printf("10. flash chip: INFO says known / unknown; on an unknown one DX BEGIN answers 3, nothing written\n");
    }
    if (fails) { printf("features_test: %d failures\n", fails); return 1; }
    printf("features_test: passed\n");
    return 0;
}
