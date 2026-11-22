/* SPDX-License-Identifier: GPL-3.0-only */
/* KAWA: the user's samples over USB-MIDI, from the emulator's page (Web MIDI) or install/kawa_samples.py.
 * Felucca's way (its editor's SMP_* commands, hardware-checked): one request, one reply; the slot's header is
 * erased first and written last, after the CRC of what reached the flash is checked, so a cut upload leaves
 * an empty slot, never a broken one. The audio interrupt reads the slots where they are (memory-mapped flash).
 *
 * Frames: F0 7D 4B 57 cmd pack7(args) F7 ("KW", 7D: the non-commercial id; pack7: the updater's bit stream,
 * ota.c: 7 bits at a time, the low ones first). Every reply: the same cmd, pack7(rc, ...). Commands:
 *   1 INFO                           -> rc, "KAWA", version (4 chars), slots, capacity (3 B LE), (Gabare) "GABARE", engines
 *   2 BEGIN slot                     -> rc, slot         (stopped only: the sound holds while the flash is written)
 *   3 WRITE slot off(3 B LE) data    -> rc, slot, off    (off from 0, a multiple of 256, in order; data <= 256 B)
 *   4 END slot header(32 B)          -> rc, slot         (usr_hdr_t: magic, len, rate, name, crc)
 *   5 ERASE slot                     -> rc, slot
 *   6 LIST                           -> rc, then per slot: used, rate (2 B), len (3 B), name (12 B)
 * (Gabare 0.9) 2 to 6 answer 1 (bad arguments): SAMPLE and the user's sample slots are gone; INFO says 0 slots.
 * (Gabare) a DX7 bank (kawa_edit.c DX_AT: the 4096 bytes of a 32-voice bulk dump), the same way, bank 0 only:
 *  16 DX BEGIN 0                     -> rc, 0            (stopped only; the sector erased: no bank until END)
 *  17 DX WRITE 0 off(3 B LE) data    -> rc, 0, off       (off a multiple of 256, in order, to 4096)
 *  18 DX END 0 crc(4 B LE)           -> rc, 0            (the CRC-32 of the 4096 bytes; 7-bit bytes, a name)
 *  19 DX READ 0 off(3 B LE)          -> rc, 0, off, 64 B (off a multiple of 64, below 4096; rc 5 and no data: no bank)
 * (Gabare 0.7) the song being played or edited, read back (the RAM's project_t, saved or not; a read: never busy):
 *  20 PRJ READ 0 off(3 B LE)         -> rc, 0, off, up to 64 B (off a multiple of 64, below its size; the last one shorter)
 * INFO (Gabare 0.7) also says the project's size (3 B LE) and its format (PRJ_VERSION), after the bank's byte.
 * (Gabare 0.8) a song from the computer, in place of the one in RAM (INFO's byte 25: 1, it takes one):
 *  21 PRJ BEGIN 0                    -> rc, 0            (stopped only; the song in RAM saved first as the autosave)
 *  22 PRJ WRITE 0 off(3 B LE) data   -> rc, 0, off       (off a multiple of 256, in order; data <= 256 B, into RAM)
 *  23 PRJ END 0                      -> rc, 0            (whole, its magic, format and CRC right: it plays; else, and
 *                                                         after 10 s without a frame, the autosave comes back)
 * (Gabare 0.9) MODE PC, the FM-1 a MIDI keyboard for the computer (kawa_ui.c pc_input; INFO's byte 27: 1, it has it):
 *  24 PC MODE on (1) / off (0)        -> rc, on
 * The song received is not saved into a slot: SAVE on the FM-1 does (the autosave keeps it, with its '*').
 * INFO's last byte (Gabare): 1 a DX7 bank is there.
 * (Gabare 0.9) INFO's byte 26: 1, the flash chip is one the FM-1 ships with (FL_CHIP_OK: the Puya 0x856014 or the
 * UC25HQ80 0xB36014); 0, an unknown chip: BEGIN, ERASE and DX BEGIN answer 3 (flash) and nothing is written, as the
 * saves, the USB rescue and the update loader already do (and SLOOP 2.8.5: an unknown chip is left alone).
 * Codes: 0 ok, 1 bad arguments, 2 busy (playing), 3 flash, 4 CRC, 5 no session / out of order. */
enum { KW_INFO = 1, KW_BEGIN, KW_WRITE, KW_END, KW_ERASE, KW_LIST, KW_DXBEGIN = 16, KW_DXWRITE, KW_DXEND, KW_DXREAD, KW_PRJREAD, KW_PRJBEGIN, KW_PRJWRITE, KW_PRJEND, KW_PCMODE };
#define KW_DXCHUNK 64u                          /* (Gabare) DX READ: 69 bytes a reply, under kw_reply's 100 */
#define KW_DXSLOT 0xDDu                         /* (Gabare) kw_slot while a DX7 bank comes */
#define KW_PRJSLOT 0xEEu                        /* (Gabare 0.8) kw_slot while a song comes */
typedef char dx_fits_the_store[DX_AT + 0x1000u <= STORE_HI && (DX_AT & 0xFFFu) == 0u ? 1 : -1];
enum { KW_OK, KW_EARGS, KW_EBUSY, KW_EFLASH, KW_ECRC, KW_ESTATE };

static uint32_t kw_pack7(const uint8_t *in, uint32_t n, uint8_t *out)   /* (as ota.c's) */
{
    uint32_t acc = 0, nb = 0, o = 0;
    while (n--) {
        acc |= (uint32_t)*in++ << nb;
        nb += 8u;
        while (nb >= 7u) { out[o++] = (uint8_t)(acc & 0x7Fu); acc >>= 7; nb -= 7u; }
    }
    if (nb)
        out[o++] = (uint8_t)(acc & 0x7Fu);
    return o;
}
static uint32_t kw_unpack7(const uint8_t *in, uint32_t n, uint8_t *out, uint32_t max)
{
    uint32_t acc = 0, nb = 0, o = 0;
    while (n--) {
        acc |= (uint32_t)(*in++ & 0x7Fu) << nb;
        nb += 7u;
        if (nb >= 8u) {
            if (o < max) out[o] = (uint8_t)acc;
            o++;
            acc >>= 8;
            nb -= 8u;
        }
    }
    return o;
}

/* ---- the flash, or a stand-in for it on the computer (tests/host.c "sysex": tests/samples_test.py) */
#if defined(DINGHY_HOST) || defined(KAWA_WASM)
static uint8_t kw_flash[STORE_HI - USR_AT];
static int kw_erase(uint32_t off) { uint32_t i; for (i = 0; i < 0x1000u; i++) kw_flash[off - USR_AT + i] = 0xFF; return 0; }
static int kw_prog(uint32_t off, const uint8_t *p, uint32_t n)
{
    uint32_t i;
    for (i = 0; i < n; i++) kw_flash[off - USR_AT + i] &= p[i];   /* NOR: programming only clears bits */
    return 0;
}
static int kw_read(uint32_t off, void *p, uint32_t n) { memcpy(p, kw_flash + off - USR_AT, n); return 0; }
static const uint8_t *kw_xip(uint32_t off) { return kw_flash + off - USR_AT; }
static uint8_t kw_flash_known = 1;              /* (Gabare 0.9) the stand-in is a known chip; a test sets 0 */
static void kw_hold_wait(void) {}
#if defined(DINGHY_HOST)
static void kw_send(const uint8_t *p, uint32_t n)   /* the reply, for the test script */
{
    uint32_t i;
    printf("%6u ms  sysex <", fm1_ms);
    for (i = 0; i < n; i++) printf(" %02X", p[i]);
    printf("\n");
}
#else
static void kw_send(const uint8_t *p, uint32_t n) { (void)p; (void)n; }
#endif
#else
static void kw_send(const uint8_t *p, uint32_t n) { ota_wire_send(p, n); }
static int kw_erase(uint32_t off) { uint32_t took; fm1_wdt_feed(); return fl_erase4k(off, &took); }
static int kw_prog(uint32_t off, const uint8_t *p, uint32_t n) { return fl_write(off, p, n); }
#define kw_flash_known flash_ok                 /* (Gabare 0.9) platform.c: FL_CHIP_OK at power-on */
static int kw_read(uint32_t off, void *p, uint32_t n) { return st_read(off, p, n); }
static const uint8_t *kw_xip(uint32_t off) { return (const uint8_t *)FM1_XIP(off); }
static void kw_hold_wait(void)                  /* two blocks: the interrupt is silent, the DMA plays zeros */
{
    uint32_t b = audio_blocks, t0 = fm1_ms;
    while (audio_blocks - b < 3u && fm1_ms - t0 < 60u)
        fm1_wdt_feed();
}
#endif

static uint32_t kw_crc(const uint8_t *p, uint32_t n) { return ~crc_step(0xFFFFFFFFu, p, n); }

static void kw_boot(void)                       /* at start: ((Gabare 0.9) the DX7 bank; KAWA's sample slots are gone) */
{
#if !defined(DINGHY_HOST) && !defined(KAWA_WASM)
    if (!flash_ok)
        return;
    fl_plain_window_init();                     /* the data area reads as it was written (Felucca's persist_boot) */
#endif
    dx_ok = (uint8_t)dx_valid(kw_xip(DX_AT));   /* (Gabare) the DX7 bank */
}

/* ---- the session: one slot at a time; the sound holds from BEGIN to END (10 s without a frame: given up) */
static uint8_t kw_slot = 0xFF;
static uint32_t kw_next, kw_ms;
static uint8_t kw_dec[300], kw_wire[8 + 120];
static uint8_t kw_last_rc;

static void kw_reply(uint32_t cmd, const uint8_t *b, uint32_t n)   /* n <= 100 */
{
    uint32_t w = 5;
    kw_wire[0] = 0xF0;
    kw_wire[1] = 0x7D;
    kw_wire[2] = 0x4B;
    kw_wire[3] = 0x57;
    kw_wire[4] = (uint8_t)cmd;
    w += kw_pack7(b, n, kw_wire + 5);
    kw_wire[w++] = 0xF7;
    kw_send(kw_wire, w);
}

/* (Gabare 0.8) the end of a song's session: the song received (ok), or the one saved at BEGIN back (the autosave) */
static void kw_prj_done(int ok)
{
    if (ok) {
        prj_sanitize();
        prj.home = 0;                           /* (it is no slot's yet: SAVE asks for one) */
        dirty = 1;
        auto_due = 1;
    } else if (prj_load(0))
        prj_new();
    hist_clear();
    brushes_reset();
    ev_push(EV_KILL, NTRK, 0, 0, 0, 0);         /* (every voice of the old song goes, before the sound comes back) */
    proj_cur = ok ? 0xFF : prj.home >= 1u && prj.home < NSLOT ? prj.home : 0xFF;
    prj_rx = 0;
    song_fresh(ok ? TR("MORCEAU RECU !", "SONG IN !", "CANCION RECIBIDA !")
                  : TR("MORCEAU NON RECU", "SONG NOT IN", "CANCION NO RECIBIDA"));
    g_force = 1;
}

static void kw_end_session(void)
{
    if (kw_slot == KW_PRJSLOT) {                /* (a song half received: the one before it comes back) */
        kw_slot = 0xFF;
        kw_prj_done(0);
    }
    kw_slot = 0xFF;
    audio_hold = 0;
}

static void kw_handle(uint32_t cmd, const uint8_t *a, uint32_t n)
{
    uint8_t r[100];
    uint32_t k = n ? a[0] : 0xFFu, i;
    r[0] = KW_OK;
    r[1] = (uint8_t)k;
    kw_ms = fm1_ms;
    switch (cmd) {
    case KW_INFO: {
        static const char V[] = APP_VER "    ";
        r[1] = 'K'; r[2] = 'A'; r[3] = 'W'; r[4] = 'A';
        for (i = 0; i < 4u; i++) r[5 + i] = (uint8_t)V[i];
        r[9] = 0;                               /* ((Gabare 0.9) no sample slot: SAMPLE is gone) */
        r[10] = r[11] = r[12] = 0;
        /* (Gabare) after KAWA's fields, which its sample tools read (they stop there): "GABARE" and the number of
         * engines, so a computer tells Gabare from KAWA */
        r[13] = 'G'; r[14] = 'A'; r[15] = 'B'; r[16] = 'A'; r[17] = 'R'; r[18] = 'E';
        r[19] = (uint8_t)ENG_NORDER;            /* ((0.9) the engines it plays, BRUIT and SAMPLE retired: 15) */
        r[20] = dx_ok;                          /* (Gabare) a DX7 bank */
        r[21] = (uint8_t)sizeof prj; r[22] = (uint8_t)(sizeof prj >> 8); r[23] = (uint8_t)(sizeof prj >> 16);   /* (0.7) */
        r[24] = PRJ_VERSION;
        r[25] = 1;                              /* (0.8) it takes a song (PRJ WRITE) */
        r[26] = kw_flash_known ? 1u : 0u;       /* (0.9) a flash chip it knows */
        r[27] = 1;                              /* (0.9) it has MODE PC (24) */
        kw_reply(cmd, r, 28);
        return;
    }
    case KW_BEGIN: case KW_WRITE: case KW_END: case KW_ERASE: case KW_LIST:
        r[0] = KW_EARGS;                        /* ((Gabare 0.9) KAWA's user samples: gone with SAMPLE) */
        break;
    case KW_PCMODE:                             /* (Gabare 0.9) MODE PC on / off */
        if (n != 1u || k > 1u) { r[0] = KW_EARGS; break; }
        if (k) pc_enter(); else pc_exit();
        r[1] = pc_mode;
        break;
    case KW_DXBEGIN:                            /* (Gabare) the DX7 bank */
        if (n != 1u || k != 0u) { r[0] = KW_EARGS; break; }
        if (!kw_flash_known) { r[0] = KW_EFLASH; break; }   /* (0.9) an unknown chip: the bank stays as it is */
        if (sq_play) { r[0] = KW_EBUSY; break; }
        if (kw_slot != 0xFFu) kw_end_session();
        dx_ok = 0;                              /* (nothing reads it from now on: FM SOUND shows KAWA's) */
        kw_hold_wait();
        audio_hold = 1;
        kw_hold_wait();
        if (kw_erase(DX_AT)) { r[0] = KW_EFLASH; kw_end_session(); break; }
        kw_slot = KW_DXSLOT;
        kw_next = 0;
        break;
    case KW_DXWRITE: {
        uint32_t off = n >= 4u ? (uint32_t)a[1] | (uint32_t)a[2] << 8 | (uint32_t)a[3] << 16 : 0xFFFFFFFFu, len = n - 4u;
        r[2] = n >= 4u ? a[1] : 0; r[3] = n >= 4u ? a[2] : 0; r[4] = n >= 4u ? a[3] : 0;
        if (n < 5u || k != 0u || len > 256u || (off & 0xFFu) || off + len > DX_SIZE) { r[0] = KW_EARGS; kw_reply(cmd, r, 5); return; }
        if (kw_slot != KW_DXSLOT) { r[0] = KW_ESTATE; kw_reply(cmd, r, 5); return; }
        if (off + 256u == kw_next) { r[0] = kw_last_rc; kw_reply(cmd, r, 5); return; }   /* (its reply was lost) */
        if (off != kw_next) { r[0] = KW_ESTATE; kw_reply(cmd, r, 5); return; }
        if (kw_prog(DX_AT + off, a + 4, len)) r[0] = KW_EFLASH;
        if (r[0] == KW_OK) kw_next = off + 256u;
        kw_last_rc = r[0];
        kw_reply(cmd, r, 5);
        return;
    }
    case KW_DXEND: {
        uint32_t crc = n == 5u ? (uint32_t)a[1] | (uint32_t)a[2] << 8 | (uint32_t)a[3] << 16 | (uint32_t)a[4] << 24 : 0u;
        if (n != 5u || k != 0u) { r[0] = KW_EARGS; break; }
        if (kw_slot != KW_DXSLOT) { r[0] = KW_ESTATE; break; }
        if (kw_next < DX_SIZE || kw_crc(kw_xip(DX_AT), DX_SIZE) != crc || !dx_valid(kw_xip(DX_AT))) {
            kw_erase(DX_AT);                    /* (a bank that did not arrive whole: none) */
            r[0] = KW_ECRC;
            kw_end_session();
            break;
        }
        dx_ok = 1;
        kw_end_session();
        toast(TR("BANQUE DX7 RECUE !", "DX7 BANK IN !", "BANCO DX7 RECIBIDO !"));
        break;
    }
    case KW_DXREAD: {                           /* (Gabare) the bank, back to the computer (a read: never busy) */
        uint32_t off = n == 4u ? (uint32_t)a[1] | (uint32_t)a[2] << 8 | (uint32_t)a[3] << 16 : 0xFFFFFFFFu;
        r[2] = n == 4u ? a[1] : 0; r[3] = n == 4u ? a[2] : 0; r[4] = n == 4u ? a[3] : 0;
        if (n != 4u || k != 0u || off % KW_DXCHUNK || off >= DX_SIZE) { r[0] = KW_EARGS; kw_reply(cmd, r, 5); return; }
        if (!dx_ok) { r[0] = KW_ESTATE; kw_reply(cmd, r, 5); return; }
        memcpy(r + 5, kw_xip(DX_AT + off), KW_DXCHUNK);
        kw_reply(cmd, r, 5 + KW_DXCHUNK);
        return;
    }
    case KW_PRJREAD: {                          /* (Gabare 0.7) the song, back to the computer (a read: never busy) */
        uint32_t off = n == 4u ? (uint32_t)a[1] | (uint32_t)a[2] << 8 | (uint32_t)a[3] << 16 : 0xFFFFFFFFu, len;
        r[2] = n == 4u ? a[1] : 0; r[3] = n == 4u ? a[2] : 0; r[4] = n == 4u ? a[3] : 0;
        if (n != 4u || k != 0u || off % KW_DXCHUNK || off >= sizeof prj) { r[0] = KW_EARGS; kw_reply(cmd, r, 5); return; }
        len = sizeof prj - off < KW_DXCHUNK ? (uint32_t)(sizeof prj - off) : KW_DXCHUNK;
        memcpy(r + 5, (const uint8_t *)&prj + off, len);
        kw_reply(cmd, r, 5 + len);
        return;
    }
    case KW_PRJBEGIN:                           /* (Gabare 0.8) a song from the computer */
        if (n != 1u || k != 0u) { r[0] = KW_EARGS; break; }
        if (sq_play) { r[0] = KW_EBUSY; break; }
        if (kw_slot != 0xFFu) kw_end_session();
        kw_hold_wait();
        audio_hold = 1;                         /* (erasing stalls the bus; and no voice plays a half-written song) */
        kw_hold_wait();
        if (prj_save(0)) { r[0] = KW_EFLASH; audio_hold = 0; break; }   /* the song in RAM, kept: it comes back if this fails */
        prj_rx = 1;
        kw_slot = KW_PRJSLOT;
        kw_next = 0;
        break;
    case KW_PRJWRITE: {
        uint32_t off = n >= 4u ? (uint32_t)a[1] | (uint32_t)a[2] << 8 | (uint32_t)a[3] << 16 : 0xFFFFFFFFu, len = n - 4u;
        r[2] = n >= 4u ? a[1] : 0; r[3] = n >= 4u ? a[2] : 0; r[4] = n >= 4u ? a[3] : 0;
        if (n < 5u || k != 0u || len > 256u || (off & 0xFFu) || off + len > sizeof prj) { r[0] = KW_EARGS; kw_reply(cmd, r, 5); return; }
        if (kw_slot != KW_PRJSLOT) { r[0] = KW_ESTATE; kw_reply(cmd, r, 5); return; }
        if (off + 256u == kw_next) { r[0] = KW_OK; kw_reply(cmd, r, 5); return; }   /* (its reply was lost) */
        if (off != kw_next) { r[0] = KW_ESTATE; kw_reply(cmd, r, 5); return; }
        memcpy((uint8_t *)&prj + off, a + 4, len);
        kw_next = off + 256u;
        kw_reply(cmd, r, 5);
        return;
    }
    case KW_PRJEND:
        if (n != 1u || k != 0u) { r[0] = KW_EARGS; break; }
        if (kw_slot != KW_PRJSLOT) { r[0] = KW_ESTATE; break; }
        if (kw_next < sizeof prj || prj.magic != PRJ_MAGIC || prj.version != PRJ_VERSION || prj_crc(&prj) != prj.crc) {
            r[0] = KW_ECRC;
            kw_end_session();                   /* (the song before it comes back) */
            break;
        }
        kw_slot = 0xFF;
        kw_prj_done(1);
        audio_hold = 0;
        break;
    default:
        r[0] = KW_EARGS;
        break;
    }
    kw_reply(cmd, r, 2);
}

/* a frame from the computer (the bytes between F0 and F7): ours (1), or the updater's (0: ota_service takes it) */
static int app_sysex(const uint8_t *p, uint32_t n)
{
    uint32_t d;
    if (n < 4u || p[0] != 0x7Du || p[1] != 0x4Bu || p[2] != 0x57u)
        return 0;
    d = kw_unpack7(p + 4, n - 4u, kw_dec, sizeof kw_dec);
    if (d <= sizeof kw_dec)
        kw_handle(p[3], kw_dec, d);
    return 1;
}

static void kw_poll(void)                       /* main loop: a session left hanging gives the sound back */
{
    if (kw_slot != 0xFFu && fm1_ms - kw_ms > 10000u)
        kw_end_session();
}
