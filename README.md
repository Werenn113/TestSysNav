# Test recrutement SYSNAV — Livrables

**Ingénieur Électronique, Firmware & Logiciel Embarqué — Baptiste Marchand**

Réponse complète aux 4 tests du document `Test_IngenieurElecFirmware&LogEmb_1.6.pdf`.
Les tests sur cible n'étant pas demandés, chaque livrable est accompagné d'un
processus de validation (document + harnais exécutable sur PC pour le code).

## Organisation du dépôt

| Dossier | Contenu |
|---|---|
| `LibC_reduite/` | API multi-systèmes fournie, **complétée des modules livrés** : `Os/Pic/Time/` (Test code #1), `Os/Common/Utils/RingBuffer.c/.h` (Test code #2) et `GenericTypeDefs.h` corrigé (type `Time` → 64 bits) |
| `Test1_getTime/` | Test code #1 — dossier autonome, recompilable et exécutable sur PC |
| `Test2_RingBuffer/` | Test code #2 — dossier autonome, recompilable et exécutable sur PC |
| `Test3_Electronique_Magnetometre/` | Test élec #1 — schéma de câblage + flowchart + note explicative |
| `Test4_Electronique_Alimentation/` | Test élec #2 — schéma de câblage + note explicative |

## Compilation et exécution des tests sur PC hôte

**Test #1 — `getTime()`** (succès = 7 tests sur 7) :

```bash
cd Test1_getTime
gcc -std=c99 -Wall -Wextra -I test_host -I LibC_reduite -I LibC_reduite/Os/Pic/Time \
    -o test_time TEST_Time.c LibC_reduite/Os/Pic/Time/Time.c
./test_time
```

**Test #2 — buffer circulaire** (succès = message `=== TOUS LES TESTS ONT PASSE ===`) :

```bash
cd Test2_RingBuffer
gcc -std=c99 -Wall -Wextra -I LibC_reduite -o test_rb \
    TEST_RingBuffer.c LibC_reduite/Os/Common/Utils/RingBuffer.c
./test_rb
```

## Résumé des livrables

### Test code #1 — `getTime()` (dossier `Test1_getTime/`)

| Livrable demandé | Fichier |
|---|---|
| API modifiée (C) | `Time.c`, `Time.h`, `GenericTypeDefs.h` (typé `Time` → 64 bits, justifié dans la note §1) |
| Processus de validation | `VALIDATION_Test1.md` + harnais exécutable sur PC : `TEST_Time.c` + `test_host/xc.h` (mock dsPIC33) |
| Note explicative | `NOTE_EXPLICATIVE_Test1.md` |

Points-clés : Timer2/3 mode 32 bits @ Fcy = 30 MHz, période 1 s exacte (PR = 29 999 999),
extension logicielle 64 bits par interruption, capture atomique (priorité CPU → 7) avec
correction du débordement en attente (T3IF) et relecture comparative du compteur —
erreur de datation ≤ ~3 µs (spec 100 µs), exactitude vérifiée sur 10 h simulées.

### Test code #2 — buffer circulaire sans écrasement (dossier `Test2_RingBuffer/`)

| Livrable demandé | Fichier |
|---|---|
| API modifiée avec la fonctionnalité | `RingBuffer.c`, `RingBuffer.h` (dans `Os/Common/Utils/`) |
| Processus de validation | `VALIDATION_Test2.md` + `TEST_RingBuffer.c` (harnais exécutable sur PC) |
| Code exemple d'utilisation | `EXEMPLE_UTILISATION_Test2.c` (trames NMEA 4 Hz ↔ IT UART ; tampon Sdcard) |
| Note explicative | `NOTE_EXPLICATIVE_Test2.md` |

Points-clés : multi-instances, écriture/lecture transactionnelles (jamais d'écrasement),
contrat producteur/consommateur sans coupure d'interruptions (publication d'index après
copie via pointeurs volatile), index libres « free-running » (pas d'ambiguïté plein/vide).

### Test élec #1 — mesure du champ magnétique (dossier `Test3_Electronique_Magnetometre/`)

| Livrable demandé | Fichier |
|---|---|
| Schéma de câblage (réfs + valeurs) | `SCHEMA_Magnetometre.svg` |
| Flowchart de récupération (readSPI/writeSPI) | `FLOWCHART_Magnetometre.svg` |
| Note explicative des choix | `NOTE_EXPLICATIVE_Elec1.md` |

Chaîne : HMC1001 (pont AMR ±2 G) → AD623 (G ≈ 40) → CS5509 (ΣΔ 16 bits, 200 sps,
2 conversions/échantillon) → 100 Hz. Technique set/reset pour annuler l'offset de pont.

### Test élec #2 — alimentation LEA-5H depuis le 230 V (dossier `Test4_Electronique_Alimentation/`)

| Livrable demandé | Fichier |
|---|---|
| Schéma de câblage (réfs + valeurs) | `SCHEMA_Alimentation.svg` |
| Note explicative des choix | `NOTE_EXPLICATIVE_Elec2.md` |

Chaîne linéaire discrète : fusible + varistance + transfo de sécurité classe II 230/6 V
+ pont + réservoir + LD1117-3.3, conformité aux exigences u-blox (ondulation, pointes).
