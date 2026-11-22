# Gabare — le tracker de KAWA, les moteurs de Skiff et de JIANT

![Gabare : l'écran de démarrage](docs/img/gabare/boot.png)

**[Installer Gabare](index.html)** · [Envoyer une banque DX7](dx7.html) · [Manuel de KAWA](manual/fr.html)

**Français** · [English](README.md) · [Español](README.es.md)

Gabare est un firmware libre pour le **M-VAVE FM-1**. C'est [KAWA](https://github.com/Guizmoh/Kawa-Tracker-Fm1)
d'Omnipotard (le tracker, ses six pistes, ses effets, ses outils, son installeur), auquel Gabare ajoute onze moteurs de
synthèse pris à trois firmwares de la famille Felucca, un arpégiateur au clavier, des effets, le MIDI par canaux et les
banques DX7 sur la piste FM. Depuis la 0.9, Gabare n'a plus le moteur SAMPLE de KAWA ni ses breaks.
Tout ce que dit le manuel de KAWA vaut pour Gabare. Cette première partie dit ce que Gabare change, [en bref](#gabare-par-rapport-à-kawa) puis en
détail ; la suite est le texte de KAWA.

> **État : 0.10, bêta.** La 0.1 à la 0.8 ont été essayées sur un vrai FM-1 (le son ANALOG mesuré contre le
> simulateur, la charge de PHYS mesurée). CORDE, ADD, l'arpégiateur et les banques DX7 (0.3) passent les tests de KAWA
> et le simulateur, pas encore tous l'essai sur le FM-1. La 0.4 relit la banque DX7 par USB, la 0.5 ajoute les 128
> voix d'origine du FM-1 sur la piste FM, la 0.6 l'effet ACC (des accords sur une piste ; sa charge sur le FM-1 reste
> à mesurer), la 0.7 la relecture du morceau par USB, la 0.8 son envoi depuis un ordinateur. La 0.9 retire SAMPLE, remplace BRUIT
> par NOISE et ajoute FM4 (de SLOOP), l'arpégiateur à 15 modes, les effets INSERT et le MIDI par canaux ; la 0.10
> ajoute le MODE PC (le FM-1 en clavier pour Fm1Studio). Ni la 0.9 ni la 0.10 ne sont encore essayées sur un FM-1. Un firmware non officiel s'installe à tes risques ; le firmware de M-VAVE se remet
> avec M-UPGRADE (voir [Revenir au firmware officiel](#revenir-au-firmware-officiel)).

| | | |
| --- | --- | --- |
| ![ANALOG](docs/img/gabare/son-analog.png) | ![PHYS](docs/img/gabare/son-phys.png) | ![CORDE](docs/img/gabare/son-corde.png) |
| ![L'arpégiateur](docs/img/gabare/arpege.png) | ![Une voix DX7 sur la piste FM](docs/img/gabare/fm-dx7.png) | ![Le démarrage](docs/img/gabare/boot.png) |

## Gabare par rapport à KAWA

Gabare part de KAWA 0.10. Ce qui n'est pas dans cette liste est KAWA tel quel ; dans le code, chaque changement
porte la marque « (Gabare) », et [LICENSING.md](LICENSING.md) dit qui a écrit quoi. Le détail de chaque ajout suit,
dans [Ce que Gabare ajoute](#ce-que-gabare-ajoute).

### Sur le FM-1

| | KAWA 0.10 | Gabare | Depuis |
| --- | --- | --- | --- |
| Moteurs | 6 : CARRÉ, ONDE, BRUIT, FM, KIT, SAMPLE | 15 : CARRÉ, ONDE, FM, KIT de KAWA ; ANALOG, PHASE, LOFI, VOIX, TRIO, ORGUE, PHYS (Skiff) ; CORDE, ADD (JIANT) ; **NOISE** (« BRUIT » à l'écran) et **FM4** (SLOOP), avec 149 sons d'usine ; la page SON dessine le moteur et ses 8 réglages | 0.1 (CORDE, ADD : 0.3 ; NOISE, FM4 : 0.9) |
| SAMPLE | le moteur SAMPLE, ses 6 breaks, 4 emplacements de samples à soi, l'édition du sample et ses tranches | **retiré** (0.9) : ~200 Ko de moins ; un ancien morceau ouvre ses pistes SAMPLE sur le kit PERCUS et son BRUIT sur NOISE | 0.9 |
| Charge | la FM sur une piste | la FM sur une piste, **PHYS sur quatre au plus** | 0.2 |
| Arpège | l'effet de case ARP | en plus, un **arpégiateur au clavier** : la page ARPÈGE (**SCL** : GAMME → ARPÈGE → ARPÈGE 2 → MAGIE), qui s'écrit en REC ; ses **15 modes** (ceux de SLOOP, 0.9), son swing, sa chance, la tenue du clavier | 0.3 |
| Effet d'une piste | AUCUN, FILT BAS, FILT HAU, PASS BND, DISTO, ECRASE, DECIME, ANNEAU, TREMOLO, WAH | en plus les **INSERT** de SLOOP : DOUCE, DURE, PLIE, FUZZ, PHASER, FLANGER, CHORUS (sur l'effet master aussi) | 0.9 |
| MIDI en entrée | l'horloge ; les notes sur la piste du curseur, par USB | en plus la **prise MIDI IN** du FM-1, les **canaux 1 à 6 sur les pistes 1 à 6** (OPTIONS), les CC (volume, pano, effet, enveloppe, envois, glissé, sustain), Program Change, pitch bend | 0.9 |
| MODE PC | — | le FM-1 en **clavier MIDI** pour Fm1Studio : PROJETS → **MODE PC** ; ses touches, boutons et encodeurs partent par USB, Gabare se tait ; **HOME tenu 2 s** pour revenir | 0.10 |
| Accords | une note par piste | l'effet de case **ACC** : jusqu'à 4 notes ensemble, sur CARRÉ, ONDE, FM, ANALOG, PHASE, LOFI, TRIO | 0.6 |
| Piste FM (**SON FM**) | les 12 sons FM de KAWA | en plus les **128 voix d'origine du FM-1** (0.5) et les 32 voix d'une **banque DX7** envoyée (0.3), gardée dans la flash | 0.3 |
| Morceau par USB | — | relu par l'ordinateur (0.7), reçu de l'ordinateur à la place de celui en mémoire (0.8) | 0.7 |
| Démarrage | KAWA | GABARE, puis KAWA by Omnipotard et les moteurs de Skiff et de JIANT | 0.1 |
| Paquet | `kawa-X.Y.fwsc`, identité `FM-1_957` | `gabare-X.Y.fwsc`, identité `FM-1_947` | 0.1 |
| Puce flash | les deux du FM-1 (depuis KAWA 0.8) : la Puya P25Q80 des premiers, l'UC25HQ80 de ceux vendus depuis fin 2026 ; une autre : rien n'est écrit, ni sauvegarde ni mise à jour | de même, et sur une puce inconnue l'envoi de banque DX7 est refusé aussi | 0.9 |

### Par USB : les commandes SysEx en plus

Même trame que l'envoi des samples de KAWA (`F0 7D 4B 57 cmd pack7(args) F7`, ses commandes 1 à 6). Elles sont
décrites en tête de `source/firmware/src/kawa_sysex.c`.

| Commande | Ce qu'elle fait | Depuis |
| --- | --- | --- |
| 1 INFO | répond aussi `GABARE` et le nombre de moteurs, si une banque DX7 est là (0.3), la taille et le format du morceau (0.7), s'il accepte un morceau (0.8), si sa puce flash est connue (0.9), s'il connaît le MODE PC (0.10) ; 0 emplacement de sample (0.9) | 0.1 |
| 2 à 6 (samples de KAWA) | répondent « mauvais arguments » : SAMPLE est retiré | 0.9 |
| 16, 17, 18 DX BEGIN, WRITE, END | écrire une banque DX7 de 32 voix dans la flash (séquenceur arrêté ; contrôle CRC) | 0.3 |
| 19 DX READ | relire la banque, 64 octets à la fois | 0.4 |
| 20 PRJ READ | relire le morceau en mémoire, sauvé ou non, 64 octets à la fois (même pendant la lecture) | 0.7 |
| 21, 22, 23 PRJ BEGIN, WRITE, END | recevoir un morceau (séquenceur arrêté) : le sien est d'abord gardé dans la sauvegarde automatique et revient si le nouveau n'arrive pas entier ; **SAVE** range le nouveau | 0.8 |
| 24 PC MODE | allume (1) ou éteint (0) le MODE PC ; répond s'il est allumé | 0.10 |

### Sur l'ordinateur

- [index.html](index.html) : la page d'installation, en français. C'est l'installeur de KAWA, qui prend le
  `gabare-*.fwsc` le plus récent que nomme `install/SHA256SUMS` ; les lanceurs `INSTALL-*` aussi. Les paquets de
  KAWA ne sont plus dans `install/`.
- [dx7.html](dx7.html) : envoyer une banque DX7 (0.3). L'outil de samples de KAWA (`samples.html`) ne sert plus : Gabare
  n'a plus de samples (0.9).
- [aide.html](aide.html) : l'aide-mémoire des gestes de Gabare, à imprimer (0.6).
- Fm1Studio joue les sons de Gabare sur le PC (avec le code de Gabare), envoie la banque DX7, importe le morceau du
  FM-1 (0.7) et lui envoie le sien (0.8) ; en MODE PC (0.10), il se joue depuis le FM-1 (« Continuer sur le PC »).

### Ce qui reste de KAWA

- Le tracker, ses six pistes, ses effets, ses outils, ses menus et ses trois langues : rien n'est enlevé.
- Les projets gardent le format de KAWA (format 5) : les projets, samples et réglages de KAWA se lisent sous Gabare.
  Un morceau qui se sert d'un moteur de Gabare, de l'effet ACC ou des voix d'origine ne sonne pas pareil sous KAWA,
  qui ne les a pas.
- L'[émulateur web](emulator/index.html) fait tourner Gabare ; son panneau de samples ne fait plus rien (0.9). Le
  [manuel](manual/fr.html) est celui de KAWA : il ne montre pas ce que Gabare ajoute, et parle encore de SAMPLE.
  [README.es.md](README.es.md) est encore le texte de KAWA seul.
- Le site (GitHub Pages) se publie depuis la branche `gabare`, et une étiquette de version (`v0.8`) fait construire et
  signer `gabare-0.8.fwsc` par la procédure de The Firmware Hub.

## Ce que Gabare ajoute

### Quinze moteurs

Chaque piste choisit son moteur sur la page SON (EDIT) : **ARP tenu + PRESETS** passe d'un moteur à l'autre (sur le
premier son d'usine du moteur), **PRESETS** seul parcourt ses sons d'usine. La bande des moteurs en montre sept à la
fois, autour du moteur choisi.

| Moteur | D'où | Le son | Sons d'usine |
| --- | --- | --- | --- |
| CARRÉ, ONDE, FM, KIT | KAWA | ceux de KAWA (voir son manuel) | ceux de KAWA |
| ANALOG | Skiff | soustractif : deux oscillateurs, un filtre résonant | 25 |
| PHASE | Skiff | distorsion de phase, à la Casio CZ | 9 |
| LOFI | Skiff | ondes 8 bits, très léger | 8 |
| VOIX | Skiff | formants : voyelles, chœurs | 8 |
| TRIO | Skiff | trois oscillateurs | 9 |
| ORGUE | Skiff | orgue à roues phoniques, ses tirettes | 8 |
| PHYS | Skiff | modèles physiques : modal, corde, membrane, cordes sympathiques | 11 |
| CORDE | JIANT | corde pincée, frappée ou frottée (Karplus-Strong), larsen de guitare | 16 |
| ADD | JIANT | additif : neuf partiels (harmoniques, impairs, cloche, barre, membrane…) | 16 |
| BRUIT (NOISE) | SLOOP | bruit analogique, crépitements, registre à décalage façon console, mode métallique accordé ; sa couleur (du blanc au brun), un filtre résonant, une dérive, un écrasement | 17 : les 6 du BRUIT de KAWA (CHARLEY, CAISSE, METAL…) refaits, et WIND, RAIN, OCEAN, VINYL, HISS, RISER… |
| FM4 (DIGITAL) | SLOOP | FM à 4 opérateurs : 8 algorithmes, les rapports des opérateurs 2 à 4, l'indice, son déclin, la réinjection | 22 : RHODES, WURLI, CLAV, MARIMBA, GLOCKEN, FM BASS… |

- Les **8 boutons** d'un moteur de Skiff, de JIANT ou de SLOOP sont ses réglages à lui, sur SON 1 et SON 2 (K1 à K4), sous
  leur nom d'origine. La page SON dessine le moteur : son nom et ses 8 réglages en barres.
- Tout le reste de KAWA s'applique autour d'eux : la note, le glissé, l'arpège, la table et le LFO font la hauteur ;
  l'enveloppe, le volume, les effets et les envois suivent. **ENV>EFF** ouvre aussi le filtre du moteur.
- Une note à la fois par piste, comme dans KAWA.
- **FM** se joue sur une piste (la règle de KAWA) et **PHYS sur quatre au plus** : sur le FM-1, trois pistes PHYS
  prennent 68 % du temps de calcul. Au-delà, PHYS est grisé dans la bande et l'écran dit « PHYS : 4 PISTES MAX ».
  Les autres moteurs vont sur les six pistes (CORDE et ADD coûtent comme ANALOG, BRUIT et FM4 moins).
- **Un morceau d'avant la 0.9** s'ouvre sans surprise : une piste SAMPLE devient le kit PERCUS (ses tranches 1 à 16
  jouent ses 16 coups), une piste du BRUIT de KAWA devient NOISE (CHARLEY, ou METAL pour le bruit métallique), son
  enveloppe gardée. L'outil magique HACHOIR mélange maintenant les temps d'un motif, sur les pistes de batterie.
- **CORDE**, sons FEEDBACK, SHOEGAZE, SQUEAL et EBOW : leur excitation FDBK change le rôle des boutons 4 à 8 (MOVE,
  GAIN, DIST, SPEAKR, BLOOM), mais l'écran garde les noms habituels. Une note qui s'arrête (OFF) étouffe la corde
  selon le RELEASE de la piste.

### L'arpégiateur

**SCL** fait maintenant le tour **GAMME → ARPÈGE → ARPÈGE 2 → MAGIE**. Sur la page ARPÈGE :

| Bouton | Réglage |
| --- | --- |
| K1 | le mode : OFF, MONTE, DESCEND, MONTE+DESC, HASARD, COMME JOUÉ, et depuis la 0.9 ceux de SLOOP (ci-dessous) |
| K2 | la vitesse : 1/32, 1/16T, 1/16, 1/8T, 1/8, 1/4 |
| K3 | les octaves : 1 à 4 |
| K4 | la durée de chaque note : de 6 % à 94 %, puis LEGATO |
| ARP | tenir les notes : elles restent une fois les touches lâchées, le prochain accord les remplace |

- Tiens des touches (jusqu'à 8) : elles s'arpègent sur la piste choisie, depuis n'importe quelle page qui joue des
  notes. Les pistes KIT ne s'arpègent pas.
- À l'arrêt, la première note part tout de suite, au tempo du morceau. Pendant la lecture, l'arpège suit les lignes
  du séquenceur, swing et groove compris, et la piste arpégée ne joue pas ses propres notes pendant ce temps.
- **REC pendant la lecture** : chaque note de l'arpège s'écrit dans la ligne qui joue, avec un RETARD si elle tombe
  entre deux lignes. Une ligne garde une note : aux vitesses plus rapides que la ligne (1/32, 1/16T), seule la
  première de chaque ligne s'écrit. À l'arrêt avec REC, une touche s'écrit au curseur, comme d'habitude.
- La vitesse, les octaves et la durée sont gardées à l'extinction ; le mode revient à OFF au démarrage.
- **Les modes de SLOOP** (0.9 ; avec do mi sol tenus) : REPETE (la dernière touche, encore et encore), DESC+MONTE
  (sol mi do mi), MONTE+8VE (do mi sol do'), CONVERGE (do sol mi, de l'extérieur vers le centre), DIVERGE (mi sol do),
  HAUT ALTERNE (do sol mi sol : la note du haut entre les autres), BAS ALTERNE (do mi do sol), BALADE (un pas au
  hasard vers le haut ou le bas), ACCORD (les notes tenues ensemble, jusqu'à 4, une octave plus haut à chaque pas
  selon OCTAVES ; REC n'en écrit que la fondamentale).
- **ARPÈGE 2** (SCL une fois de plus) : K1 le **SWING** de l'arpège (un pas sur deux plus tard, 0 à 100 : 100 fait
  75 / 25), aussi à l'arrêt ; K2 sa **CHANCE** (chaque pas joue ou se tait, 0 à 100 %) ; K3 la **TENUE** du clavier :
  sans arpège, une note jouée reste quand la touche est lâchée, la suivante la remplace. Le swing et la chance sont
  gardés à l'extinction.

### Les effets INSERT (0.9)

L'effet de chaque piste (page EFFET, et l'effet master) a sept types de plus, ceux de SLOOP (de Felucca 1.4) :

| Type | A | B |
| --- | --- | --- |
| DOUCE, DURE, PLIE, FUZZ | le gain (de x1 à x11) | le ton |
| PHASER, FLANGER, CHORUS | la vitesse (de 0,02 à 5 Hz) | la profondeur |

DOSAGE mêle l'effet au son. Un morceau qui s'en sert sonne sans cet effet sous une version plus ancienne.

### Le MIDI (0.9)

- **La prise MIDI IN** du FM-1 marche, comme l'USB (le lecteur de SLOOP).
- **OPTIONS, K3** fait le tour de l'horloge MIDI (NON, SORTIE, ENTRÉE) puis des mêmes avec **CANAUX** : les canaux
  1 à 6 jouent les pistes 1 à 6, les autres la piste du curseur. Sans CANAUX (CURSEUR), comme KAWA : toutes les
  notes sur la piste du curseur.
- **Sur la piste de la note** : CC 7 le volume, 10 le pano, 74 et 71 les réglages A et B de l'effet (la coupure et la
  résonance d'un filtre), 73 / 75 / 72 l'attaque, le déclin et le relâché, 5 le glissé (CARRÉ, ONDE), 91 et 94 les
  envois de réverb et d'écho, 64 la pédale de sustain ; **Program Change** : le son d'usine n° N du moteur de la
  piste (Bank Select, CC 0, pour aller au-delà de 128) ; **pitch bend** : ± 2 demi-tons.

### Le MODE PC (0.10)

Pour continuer sur l'ordinateur, dans Fm1Studio, un morceau commencé sur le FM-1 : le FM-1 devient le clavier de
Fm1Studio (plus de pistes, des sons de plusieurs firmwares), et c'est l'écran du PC qui montre où l'on en est.

- **Entrer** : SAVE (PROJETS), **MODE PC**, OK. Ou depuis Fm1Studio, « Continuer sur le PC » (panneau Banques
  FM-1) : il relit d'abord le morceau du FM-1, puis allume le MODE PC (commande SysEx 24).
- **Pendant** : le séquenceur s'arrête et Gabare se tait ; l'écran dit « MODE PC ». Les touches, les boutons et les
  encodeurs partent par USB au format que lit Fm1Studio : touches en notes (canal 1, 53 + n° de touche), boutons
  en notes (canal 2, 0 OCT− … 13 REC), encodeurs en CC 16 à 22 relatifs (64 ± pas). Fm1Studio en fait sa façade :
  la piste, les sons de ses firmwares (ALGORITHM le groupe, PRESETS le son), leurs réglages (HOME, K1 à K4).
- **Sortir** : **HOME tenu 2 s** sur le FM-1 (l'aide ne s'ouvre pas en MODE PC), ou « Revenir à Gabare » dans
  Fm1Studio. Gabare reprend là où il était, son morceau intact. Envoyer au FM-1 le morceau fait sur le PC reste
  une action de Fm1Studio (commandes 21 à 23).

### Les accords : l'effet ACC

Un tracker joue une note par piste. Avec l'effet **ACC** (accord) dans une case, la note joue un accord : ses notes
partent ensemble et partagent l'enveloppe, le volume et les effets de la piste (des accords « paraphoniques »).

- Sur la page PISTES, K3 choisit l'effet de la case (ACC, juste avant `---`), K4 l'accord : les 14 de l'effet ARP (MAJ,
  MIN, OCT, 5TE, SUS, 7, MA7, MI7, DIM, AUG, 2OC, AD9, TRI, BAS), jusqu'à 4 notes. Dans la case, ACC a l'icône ∞ ;
  ARP garde la sienne et joue les mêmes accords note après note.
- L'accord tient jusqu'à la note suivante de la piste, ou un OFF. ACC et ARP vont ensemble : l'arpège déplace l'accord.
- **Les moteurs qui le jouent** : CARRÉ, ONDE, FM (et les voix d'origine du FM-1, les banques DX7), FM4, ANALOG, PHASE,
  LOFI, TRIO. Les autres jouent la note seule : BRUIT, KIT, VOIX et PHYS (trop lourds), ORGUE, CORDE et ADD (leur
  mémoire est celle d'une seule voix).
- **La charge** : chaque note de plus coûte presque une piste de ce moteur (estimé : ~2 % pour CARRÉ ou LOFI, ~5 % pour
  ANALOG, ~6 % pour FM, par note, à mesurer sur le FM-1 dans OPTIONS). Quand le FM-1 sature, le régulateur de KAWA
  enlève d'abord les notes ajoutées par les accords.
- Les projets ne changent pas de format ; un firmware d'avant la 0.6 ignore l'effet.

### Les voix d'origine du FM-1 et les banques DX7

Le moteur FM de KAWA est msfa, le cœur de Dexed : six opérateurs, les 32 algorithmes du DX7 et son format de voix.
Sur la piste FM, page SON, **K1 (SON FM)** passe, sous leur nom :

1. les 12 sons FM de KAWA ;
2. **les 128 voix d'origine du FM-1** (celles du firmware M-VAVE V15 : PIANO 1, ORGAN 1, SYN LEAD 1…), toujours là ;
3. les 32 voix de **ta banque DX7**, si tu en as envoyé une.

Les réglages FM de KAWA (algorithme, rapport, feedback, désaccord, pages OPÉRATEUR et ENV OP) agissent sur toutes. Les
voix d'origine sont jouées par le moteur de Gabare : proches de celles du firmware M-VAVE, pas identiques (son moteur
n'est pas public, et les effets de chaque preset ne suivent pas). M-VAVE les publie sous licence Apache-2.0 dans
[fm1_open](https://github.com/m-vave/fm1_open).

La banque DX7 de 32 voix s'envoie depuis la page [dx7.html](dx7.html) (Chrome, Edge ou Opera, par USB) :

- le fichier est l'envoi en bloc d'un DX7 (un `.syx` de 4104 octets) ; la page montre ses 32 voix avant l'envoi ;
- une banque à la fois, gardée dans la flash (le dernier secteur de 4 Ko de la réserve, après les samples). Une
  nouvelle remplace l'ancienne ; un projet garde la voix qu'il a prise ;
- **Gabare ne contient aucun son de Yamaha** : tu apportes tes banques (les cartouches d'origine, celles de Dexed,
  les collections en ligne, les tiennes).

Pour un programme : les commandes SysEx 16, 17 et 18 l'envoient, la 19 la relit, 64 octets à la fois (0.4 ; `F0 7D 4B
57 cmd pack7(args) F7`, comme les samples de KAWA). La 20 relit le morceau en cours, tel qu'il est en mémoire, sauvé ou
non (0.7 : ses motifs, sa chaîne, ses sons, son tempo ; une lecture, qui marche pendant la lecture). Elles sont décrites
en tête de `source/firmware/src/kawa_sysex.c`. INFO répond aussi `GABARE`, le nombre de moteurs, si une banque est là,
et la taille et le format du morceau. Fm1Studio s'en sert pour importer le morceau du FM-1, effets des cases compris.

Les 21, 22 et 23 envoient un morceau à la place de celui en mémoire (0.8 ; séquenceur arrêté). Gabare garde d'abord le
sien dans la sauvegarde automatique ; un morceau incomplet, d'un autre format ou au CRC faux est refusé et le sien
revient (« MORCEAU NON RECU »), de même après 10 secondes sans trame. Le morceau reçu (« MORCEAU RECU ! ») n'est rangé
dans aucun emplacement : **SAVE** le range. Fm1Studio s'en sert pour « Envoyer le morceau » : patterns, chaîne, tempo,
notes et effets des pistes 1 à 6, sons de Gabare ; les tables, l'effet master et le groove du morceau du FM-1 restent.

### Le reste

- L'écran de démarrage dit GABARE, puis KAWA by Omnipotard et les moteurs de Skiff et de JIANT.
- L'identité du paquet est `FM-1_947` (celle de KAWA : `FM-1_957`).
- Les projets, les samples et les réglages de KAWA se lisent tels quels.

### Crédits de Gabare

Les moteurs ANALOG, PHASE, LOFI, VOIX, TRIO, ORGUE et PHYS sont ceux de [Skiff](https://github.com/illuminus5/skiff)
(illuminus5), copiés sans changement dans `source/firmware/src/skiff/`. CORDE et ADD sont ceux de
[JIANT](https://github.com/juanjiant-bit/JIANT) (Juan Ignacio Antonucci), adaptés à une voix par piste. Tous viennent
de [Felucca](https://github.com/hugelton/Felucca) (Leo Kuroshita, Hügelton Instruments), sous GPL-3.0. PHYS contient
des modèles portés de DaisySP (Electrosmith, Emilie Gillet) et de Rings (Emilie Gillet), sous licence MIT. Le lien
entre ces moteurs et KAWA est `source/firmware/src/kawa_sk.c` ; l'arpégiateur est `kawa_arp.c`. Les détails sont dans
[LICENSING.md](LICENSING.md).

---

## KAWA

*La suite est le texte de KAWA ; elle vaut pour Gabare.*

KAWA est un firmware tracker pour le **M-VAVE FM-1**, fait avant tout pour la **chiptune et l'IDM** : ondes
4 bits, carré et bruit comme les vieilles consoles, FM, breakbeats découpés en tranches, roulements et ratchets,
effets live de stutter, scratch, retrig et montée, tables, groove, outils Euclide et hasard, une saturation sur
chaque piste. (Il sait faire d'autres styles, mais c'est pour ça qu'il est construit.) Il a six pistes, chacune
avec son moteur : carré, onde 4 bits, bruit, FM à 6 opérateurs, kit de batterie ou sample. Tu écris des motifs,
tu les enchaînes en morceau, et tu joues 32 effets en direct façon Pocket Operator. Des outils « magiques »
écrivent des beats, des mélodies et des découpes de break à ta place. L'écran parle français, anglais ou
espagnol.

| | | |
| --- | --- | --- |
| ![Les pistes](manual/img/fr/02-tracks.png) | ![La FM et ses opérateurs](manual/img/fr/53-fm-operator.png) | ![Les effets live](manual/img/fr/60-fx.png) |
| ![La chaîne : chaque motif joué N fois](manual/img/fr/34-song-play.png) | ![L'enveloppe](manual/img/fr/50-envelope.png) | ![Le menu des projets](manual/img/fr/10-projects-menu.png) |

> **État : 0.5, bêta.** KAWA compile pour le FM-1 et passe toutes les vérifications de sa plateforme :
> taille, mémoire, accès au matériel, chemin d'installation testé avec des coupures de courant simulées.
> Il tourne dans le simulateur et dans l'émulateur web, et **il a été testé sur un vrai FM-1**.
> Installer un firmware non officiel se fait à tes risques : lis [Installer](#installer) et
> [Revenir au firmware officiel](#revenir-au-firmware-officiel) avant de commencer.

**Le manuel**, avec des tutoriels illustrés (un nouveau morceau, écrire un motif, enchaîner les motifs,
sauvegarder) : [manual/fr.html](manual/fr.html) ([EN](manual/index.html),
[ES](manual/es.html)).

## Composer avec un tracker

Un tracker est une façon de composer très différente d'un logiciel de musique habituel (un DAW) :

- **Le temps descend.** Chaque ligne est un pas (une double croche), chaque colonne une piste, et chaque case
  dit quoi jouer à ce pas précis : une note, son volume et un effet. On lit le morceau comme une partition qui
  défile.
- **On écrit au clavier,** note par note, à l'endroit exact, sans souris ni piano roll : la grille est
  toujours la même.
- **Les effets s'écrivent dans la case,** au pas près (arpège, glissé, roulement, volume…), au lieu de
  courbes d'automation.
- **Le morceau est une liste de motifs** courts, chacun joué N fois, au lieu de clips posés sur une timeline.

Le manuel l'explique avec l'écran sous les yeux, puis montre tout le chemin, d'un morceau vide à un morceau
complet.

## L'essayer dans le navigateur

Le firmware lui-même, compilé en WebAssembly, tourne dans [`emulator/index.html`](emulator/index.html),
avec le panneau du FM-1 autour. Appuie sur **ALLUMER**, puis sur une touche (l'écran de démarrage t'attend) :
KAWA démarre sur un morceau vide, avec une petite aide à l'écran jusqu'à la première note. Pour entendre la
démo « KAWA QUEST », appuie sur **SAVE**, choisis DEMO dans le menu PROJETS (**OCT+** valide), puis appuie sur
**PLAY**. Tu peux y glisser tes propres sons et les envoyer à ton FM-1 par USB.

## Ce qu'il y a dedans

- **Un vrai tracker.** 32 motifs de 1 à 32 lignes, deux effets dans chaque case, parmi 17 (arpège, glissé,
  vibrato, roulé, bend, chance, retard, coupe, envers, hauteur, forme, écrase, glissé de volume, pano, départ
  dans le sample, saut à une ligne, fin du motif), un saut de lignes réglable (REC tenu + SELECT),
  l'insertion et la suppression de lignes, le solo, et 10 pas d'annuler / refaire qui couvrent aussi les sons
  (SAVE tenu + OCT− / OCT+).
- **Des tables et un groove, comme sur le M8.** Chaque piste a une table : 16 pas joués dans chaque note
  (hauteur, volume, un effet chacun : FRAPPE, COUPE, VIBRATO…), pour des arpèges chiptune, des roulements,
  des chutes de hauteur. Le groove règle les ticks de chaque ligne tour à tour (7 5, 8 4, 4 4 4…), à la
  place du swing.
- **Une chaîne qui fait un morceau.** Chaque case de la chaîne joue un motif de 1 à 16 fois ; l'outil
  CLONER fait une variation en une touche et l'ajoute à la chaîne.
- **Un vrai menu de projets.** Nouveau morceau, sauver, sauver sous (emplacement et nom), charger,
  renommer, démo. Il y a quatre emplacements, dont un de sauvegarde automatique.
- **Six moteurs au choix par piste,** sur la page SON : PRESETS fait défiler les sons d'usine, rangés par
  moteur, et ARP tenu + PRESETS saute au moteur suivant. La piste FM (une seule, pour garder de la marge au
  FM-1) règle ses 6 opérateurs un par un.
- **Du sound design.** Chaque son a une ADSR dessinée dans les couleurs des potards, un effet rien qu'à
  lui (filtres, disto, écrase, décime, anneau, trémolo, wah), un LFO calé sur le tempo et une page
  MODULATION (tous deux sur le bouton LFO). Le master a son effet, une réverb, un écho et un limiteur. Un éditeur de sample règle où un
  sample commence et finit, place ses tranches à la main, le normalise et le joue à l'envers, sans toucher
  au son.
- **8 kits de batterie** synthétisés : acoustique, hip hop, techno, dub-reggae, lo-fi, Game Boy, SNES et
  percussions. S'y ajoutent **six breaks** originaux à découper en 16 tranches (SOLEIL, GRELOT, TENDU,
  ABYSSE, MAMBO, GRENIER : chacun son groove, sa batterie et son disque), et **tes propres samples**
  (4 emplacements).
- **Un potard, une couleur.** KNOB 1 rouge, 2 jaune, 3 vert, 4 bleu, sur toutes les pages. La ligne du bas
  dit ce que fait chacun. HOME tenu ouvre l'aide de la page où tu es.
- **Trois thèmes** (KAWA, GAME BOY, BONBON), **quatre polices d'écran** (DOUCE, BASE, CLAIRE, 8-BIT), l'**horloge MIDI** en sortie ou en entrée (KAWA suit le tempo
  et le START / STOP d'une autre machine), et un indicateur CPU.
- **Un chien** à casque qui t'accueille au démarrage et vit dans le coin de l'écran.

### Nouveautés de la 0.5

- **Elle démarre sur un morceau vide** à 100 BPM. L'écran de démarrage attend une touche, un bouton ou un
  potard (ce premier appui ne fait que le fermer), puis un cadre sur PISTES dit quoi faire (REC, joue les
  touches, PLAY ; HOME tenu : l'aide) jusqu'à la première note. La sauvegarde automatique te rend ton dernier
  travail après une coupure, et la démo est à une touche : SAVE, PROJETS, DEMO.
- **Une page ENVOIS** (bouton FX) : pour chaque piste, **SATURE** (une saturation douce), **ECHO** et
  **REVERB**, de 0 à 15. FX encore : les 32 effets LIVE ; FX tenu : LIVE tant que tu le tiens.
- **SAVE tenu ouvre les outils :** OPTIONS, ANNULER, REFAIRE, EFFACER, COPIER, COLLER et HASARD (une mélodie,
  une basse, un beat ou une découpe de break qui va à la piste), sur la piste en cours ou sur tout le motif.
  SELECT en choisit un, il se lance quand tu relâches. Un appui court ouvre toujours PROJETS.
- **Annuler marche aussi sur les sons.** Chaque tour de potard sur une page de son (de SON à TABLE, ENVELOPPE,
  MIX, LFO, MODULATION), les envois et chaque son d'usine chargé s'annulent ; une rafale de tours fait un seul
  pas. Motifs et sons partagent 10 pas d'historique. L'écran dit « ANNULE: SON 3 » et va sur cette piste.
- **SAVE clignote** tant qu'il y a des changements pas sauvés.
- **HOME + PLAY : panique.** Le morceau s'arrête, chaque voix et chaque effet live est coupé.
- **Un métronome** avec son volume (GLO, troisième page, OPTIONS) : non, oui, ou seulement pendant
  l'enregistrement. Le clic sort aussi dans l'audio USB : coupe-le quand tu enregistres le FM-1 sur un
  ordinateur.
- **16, 32 ou 64 pas** par motif (OCT− / OCT+ sur la grille). 64 pas relient un motif au suivant, et l'en-tête
  jaune dit où tu es (« 49-64/64 »).
- **Une barre jaune, avec son nom,** se remplit en bas de l'écran quand tenir un bouton va ouvrir quelque chose
  (HOME : AIDE, FX : LIVE, SAVE : OUTILS). Un appui court n'affiche rien. Tiens FX et appuie tout de suite sur une
  touche : cet effet live joue sans attendre la barre.
- **Les pages par bouton.** Le bouton LFO a les pages LFO et MODULATION, **MAGIE passe sur SCL** (GAMME, MAGIE,
  PISTES), et SELECT tourne les pages de EDIT, LFO, ENV, FX et GLO.
- **Pas de son changé par erreur.** Un son modifié porte une `*` après son nom. PRESETS ne le remplace pas : il
  montre seulement le son d'usine suivant en doré, avec « ? » et « OCT+ ». Continue de tourner pour parcourir,
  **OCT+** le charge, **OCT−** garde le tien (un potard, une autre piste ou page, ou 8 secondes sans toucher le
  gardent aussi). Un son pas touché change tout de suite.

| | | |
| --- | --- | --- |
| ![L'aide sur un morceau vide](manual/img/fr/08-first-start.png) | ![Un son d'usine montré en doré : OCT+ le charge](manual/img/fr/75-preset-browse.png) | ![ANNULER rend un son](manual/img/fr/77-undo-sound.png) |

## Les commandes d'un coup d'œil

| Bouton | Ce qu'il fait |
| --- | --- |
| **HOME** | Les pistes. Tenu : l'aide de la page où tu es. **HOME + PLAY** : panique (le morceau s'arrête, tout son est coupé) |
| **SEQ** | La grille de pas, 16 pas par page (encore : retour aux pistes). **OCT− / OCT+** y règlent la taille du motif : 16, 32 ou 64 pas |
| **EDIT** | SON, SON 2, puis OPÉRATEUR et ENV OPÉRAT (piste FM) ou SAMPLE (piste sample), EFFET, TABLE. Appuie encore : page suivante |
| **LFO** | LFO, puis MODULATION |
| **ENV** | ENVELOPPE, puis MIX |
| **FX** | ENVOIS (saturation, écho, réverb par piste), puis LIVE (les 32 effets en direct), puis retour. Tenu : LIVE tant que tu le tiens |
| **SCL** | GAMME, puis MAGIE (beats, mélodies, découpes de break), puis retour aux pistes |
| **GLO** | CHAÎNE, puis GROOVE, puis OPTIONS (thème, langue, horloge MIDI, police, métronome) |
| **SAVE** | Le menu PROJETS (il clignote tant qu'il y a des changements pas sauvés). Tenu (la barre jaune se remplit) : les outils. **OCT− / OCT+** pendant qu'il est tenu : annuler / refaire, pour les motifs et les sons |
| **OCT− / OCT+** | L'octave des touches. Sur LIVE : la banque. Sur la chaîne : ajouter / retirer une case. Dans PROJETS : OK / retour. Sur SON, quand un son d'usine est montré en doré : OCT+ le charge, OCT− garde le tien |
| **PLAY**, **REC** | Jouer / arrêter ; enregistrer (les touches écrivent au curseur). REC tenu + SELECT : de combien de lignes saute une note |
| **ARP** | Pas un arpégiateur : le bouton **effacer / Maj** de KAWA. Un appui sur PISTES efface la case. Tenu : + SELECT insère ou supprime une ligne (+ ALGORITHM : dans tout le motif), + KNOB 3 / 4 règle le deuxième effet de la case, + une touche noire sur MIX met une piste en solo, + PRESETS sur SON choisit le moteur suivant. Il efface aussi un pas de la grille, un pas de table ou la voie LIVE, égalise les tranches dans SAMPLE et bascule BOUCLE / CHAÎNE sur la chaîne |

| Encodeur | Ce qu'il fait |
| --- | --- |
| **SELECT** | Sur EDIT, LFO, ENV / MIX, FX et GLO : tourne les pages du groupe du bouton. Ailleurs : la ligne (PISTES), la page de 16 pas (grille), le menu (PROJETS) |
| **ALGORITHM** | La piste (sur MIX, jusqu'au master). Sur CHAÎNE la case de la chaîne, sur GROOVE le pas, sur OPTIONS le mode du métronome |
| **PRESETS** | Sur la première page SON : le son d'usine (sur un son modifié, il montre seulement le suivant en doré : OCT+ le charge). Sur les autres pages EDIT : le curseur de la page (opérateur, ligne de table, tranche). Ailleurs : le motif ; sur OPTIONS, le volume du métronome |
| **KNOB 1 à 4** | Rouge, jaune, vert, bleu, sur toutes les pages. La ligne du bas dit ce que fait chacun |

Les 27 touches (16 blanches, 11 noires) jouent des notes sur la piste en cours.

## Installer

Le firmware à installer est le fichier [`kawa-0.5.fwsc`](kawa-0.5.fwsc) tout en haut de cette page (le même fichier est dans `install/`).

La [page d'accueil](index.html) du projet a aussi un bouton **Install** : il installe KAWA directement depuis
Chrome ou Edge (Web MIDI). À côté, l'[importateur de samples](samples.html) envoie tes propres sons à KAWA de
la même manière (voir [Tes samples sur le FM-1](#tes-samples-sur-le-fm-1)). L'autre façon, ce sont les
lanceurs ci-dessous, qui demandent un câble USB de
données et Python 3 ([python.org](https://www.python.org/downloads/)). Allume le FM-1, branche-le, ferme tous
les autres programmes MIDI, puis lance l'installateur de ton ordinateur :

- **Windows :** double-clic sur `INSTALL-Windows.bat`
- **Mac :** double-clic sur `INSTALL-Mac.command` (la première fois : clic droit, puis Ouvrir)
- **Linux :** `./INSTALL-Linux.sh` dans un terminal

La première fois, il récupère ce qu'il lui faut (rtmidi2, dans `install/.venv`). Il trouve le FM-1, demande
avant d'écrire `install/kawa-0.5.fwsc`, puis redémarre le FM-1 sur KAWA. À la main :

```sh
pip3 install rtmidi2                                  # (Python 3.9 et avant : pip3 install mido python-rtmidi)
python3 install/fm1_install.py --info                 # ce que tourne le FM-1 branché
python3 install/fm1_install.py install/kawa-0.5.fwsc  # demande avant d'écrire, puis redémarre sur KAWA
```

Ce qui protège ton FM-1 :

- Le paquet est vérifié en entier avant d'effacer quoi que ce soit. Un paquet abîmé est refusé, et
  l'ancien firmware reste.
- Le bootloader de la puce n'est jamais écrit.
- Une installation coupée reprend au rallumage.
- Tiens **OCT−** en allumant pour le secours USB. Si KAWA plante deux fois au démarrage, le FM-1 y va
  tout seul.

Le détail est dans [BUILDING.md](source/BUILDING.md) (en anglais) et dans le manuel.

## Revenir au firmware officiel

1. Les projets KAWA seront perdus : le firmware officiel ne sait pas les lire.
2. Branche le FM-1 sur une alimentation stable, avec un câble USB de données.
3. Installe le firmware officiel **V15** de M-VAVE avec **M-UPGRADE**. Si KAWA se comporte mal, ouvre
   d'abord le secours USB (OCT− tenu en allumant).
4. Avant que l'outil officiel n'écrive quoi que ce soit, KAWA vérifie que c'est bien le programme de mise
   à jour de M-VAVE, et que la zone de démarrage du paquet est exactement celle de ton FM-1. Sinon, rien
   n'est écrit et KAWA continue de tourner.
5. **Ne débranche rien pendant le transfert.** C'est l'étape la plus délicate, car le firmware officiel
   réécrit aussi le bootloader. S'il s'arrête, rebranche et relance.
6. `python3 install/fm1_install.py --info` doit répondre `FM-1_015`.

## Tes samples sur le FM-1

Dans Chrome ou Edge, l'[importateur de samples](samples.html) (en anglais) fait tout dans le navigateur : dépose
des fichiers WAV, MP3, OGG ou FLAC, il les convertit et en envoie jusqu'à quatre dans les emplacements MON 1 à
MON 4 de KAWA (MY 1 à MY 4 sur la page) par USB (KAWA à l'arrêt). Depuis un terminal :

```sh
python3 install/kawa_samples.py put 1 mon-son.wav     # dans MON 1 (KAWA à l'arrêt)
python3 install/kawa_samples.py list
```

Le son est mis en mono, en 22 kHz et normalisé, jusqu'à 1,66 s par emplacement. Sur une piste SAMPLE,
choisis SOURCE = MON 1 à 4.

## Le compiler

Depuis le dossier `source/` :

- `tests/run_host.sh [script]` fait tourner KAWA sur ton ordinateur : écrans en PNG, son en WAV.
- `web/make_emulator.sh` construit l'émulateur web. Il faut clang avec la cible wasm32, et wasm-ld.
- `./build.sh` construit le paquet FM-1. Il faut le compilateur JieLi (`tools/get_toolchain.sh`) et trois
  fichiers du SDK JieLi AC79. Voir [BUILDING.md](source/BUILDING.md).
- `tests/run_loader_tests.sh` teste le chemin d'installation : paquets abîmés, coupures de courant, puce
  inconnue.
- `tests/run_kits_test.sh` et `tests/breaks_check.py` mesurent les kits de batterie et les breaks.
- `tests/run_features_test.sh` joue le saut, la fin de motif, deux effets dans une case, les tables, le groove,
  l'édition de sample et l'horloge MIDI en entrée dans l'interruption audio ; `tests/run_format_test.sh`
  charge des projets sauvés par la 0.2.
- `tests/run_ui_test.sh` pilote le panneau comme le feraient les boutons et les encodeurs : l'écran de démarrage
  et le départ à vide, les pages de chaque bouton et ce que fait SELECT dessus, les sons protégés et l'annulation
  des sons, ENVOIS, les appuis tenus et les outils, la panique, le métronome et les 16 / 32 / 64 pas. `node tests/webinstall_test.js` teste l'installateur web
  derrière le bouton Install contre un FM-1 simulé (Node tout simple, sans matériel).
- `tests/run_stress_test.sh` joue chaque son, effet, effet live, table et groove aux bouts de leurs potards
  (environ 18 minutes de son) sous UBSan, et s'arrête au premier nombre qui déborde (un clic sur le FM-1).

## Soutenir KAWA

KAWA est gratuit et libre. Si tu aimes ce que je fais, pense à me soutenir ! Chaque petit geste compte
énormément. **[Paye-moi une bière sur Ko-fi](https://ko-fi.com/omnipotard)**.

## Crédits et licence

KAWA par Omnipotard. GPL-3.0-only : tu peux l'utiliser, le modifier et le partager, et qui reçoit un
firmware construit à partir de lui reçoit aussi son code source. Il repose sur
[Dinghy](https://github.com/zvenson/dinghy) (zvenson), [Felucca](https://github.com/hugelton/Felucca)
(Leo Kuroshita, Hügelton Instruments) et [SLOOP](https://github.com/isod89/sloop-fm1) (isod89). Le moteur
FM est msfa (Dexed, Apache-2.0), dans le portage de Felucca. Les détails sont dans
[LICENSING.md](LICENSING.md). « M-VAVE » et « FM-1 » appartiennent à leurs propriétaires ; KAWA n'est ni fait
ni approuvé par eux.
