# Test Code Embarqué #1 — Processus de validation de `getTime`

Le sujet ne demande pas de test sur cible. Le processus suit donc 3 niveaux, du plus
précoce au plus coûteux : **revue statique → tests unitaires sur hôte (PC) → plan de
validation sur cible (décrit, exécutable dès l'accès au matériel)**.

---

## Niveau 1 — Revue statique (checklist)

Vérifications faites sur le code, point par point contre les contraintes :

| # | Vérification | Résultat |
|---|---|---|
| 1 | Résolution : 1 LSB = 1 µs | `ticks/30` à 30 MHz ✔ |
| 2 | 10 h représentables | 3,6e10 µs < 2⁶⁴ ; testé par assertion sur le type ✔ |
| 3 | Erreur de datation ≤ 100 µs | décompte de cycles de la section critique + division ≈ 3 µs ✔ |
| 4 | Cohérence de lecture (races ISR/main) | capture atomique IPL=7 ; scénarios de course listés et vérifiés sur modèle (§Niveau 2) ✔ |
| 5 | Reentrance / appel depuis ISR | pas de variable statique modifiée, SR restauré ✔ |
| 6 | Interruption réellement active | T3IP ≠ 0, T3IF/nettoyé, T3IE posé ✔ |
| 7 | Unités et débordements | analyse des 4 multiplications/additions (largeur des opérandes) ✔ |
| 8 | Style et conformité API | types LibC, bannières, nommage ✔ |

## Niveau 2 — Tests unitaires sur hôte (PC), sans cible

`Time.c` est compilé tel quel sur PC avec un **mock de la couche matérielle** : `test_host/xc.h`
fournit les registres `T2CONbits`, `TMR2`, `TMR3HLD`, `IFS0bits`, `IEC0bits`, `IPC2bits`,
`PR2/PR3`, `SR` en mémoire, et simule le temps : chaque accès à un SFR consomme des cycles
(« chaos », 0–7 cycles ou valeur fixe), le débordement lève `T3IF`, et l'ISR Timer3 s'exécute
avec une **latence configurable, même aléatoire** (0 à ~1 s) pour jouer la charge système.
Le harnais `TEST_Time.c` exécute 7 tests (compilation :
`gcc -std=c99 -Wall -Wextra -I test_host -I LibC_reduite -o test_time TEST_Time.c Os/Pic/Time/Time.c`) :

- **T1 — statiques** : PR3:PR2 = 29 999 999 (0x01C9C37F), mode 32 bits, T3IP ≥ 1 (piège
  dsPIC33 : priorité 0 = interruption inactive), T3IE, T3IF nettoyé.
- **T2 — exactitude** : 2 000 lectures à des instants aléatoires → erreur max mesurée **0,97 µs**.
- **T3a — sonde « wrap pendant la lecture »** : compteur placé 200 cycles avant le débordement,
  IT retardée de 400 ms → mesure l'erreur exactement au wrap ; tolérance 30 µs.
- **T3b — sonde « wrap pendant la capture »** : débordement programmé **entre** la lecture du
  compteur et celle du drapeau (le compteur capturé est pré-débordement) ; tolérance 30 µs.
- **T4 — soupe aléatoire** : 2 000 lectures, latence d'IT tirée de 0 à 999 999 cycles, un appel
  sur cinq placé au bord du débordement → erreur max mesurée **1,53 µs**.
- **T5 — monotonie** : 1 000 appels en rafale, jamais de retour en arrière.
- **T6 — dix heures** : avance rapide de 1,08e12 cycles → 36 000 débordements, 36 000 IT
  exécutées, `t = 36 000 000 000 µs` exactement.

*Résultats mesurés* — le harnais a été exécuté contre trois implémentations :

| Implémentation | T1 | T2 | T3a | T3b | T4 | T5 | T6 |
|---|---|---|---|---|---|---|---|
| Corrigé (section critique + correction T3IF + relecture) | ✔ | ✔ | ✔ | ✔ | ✔ | ✔ | ✔ |
| Corrigé **sans** relecture comparative | ✔ | ✔ | ✔ | **✘ +999 999,67 µs** | ✘ | ✘ | ✔ |
| Seqlock du fichier initial (sans correction T3IF) | ✔ | ✔ | **✘ −1 000 000 µs** | ✔ | ✔ | ✔ | ✔ |

Chaque test est **déterministe** (LCG 64 bits à graine fixe) : il échoue de la même façon
tant que le code n'est pas corrigé, puis passe à coup sûr.

## Niveau 3 — Plan de validation sur cible (à exécuter si accès matériel)

1. **Latence de `getTime`** : GPIO à 1 avant l'appel, 0 après, mesure à l'oscilloscope
   (ou capture par un autre timer). Critère : < 100 µs, attendu ~2 µs.
2. **Exactitude de la base temps** : toutes les 10 s pendant 10 h, dater un front GPIO
   et comparer à un chronomètre de référence (ou comparer 2 cartes entre elles).
   Critère : dérive conforme à la tolérance du quartz.
3. **Continuité au bord de seconde** : échantillonner `getTime()` en rafale (100 kHz)
   pendant plusieurs secondes ; vérifier la monotonicité stricte — détecterait la moindre
   erreur « d'1 s » de la course critique.
4. **Robustesse sous charge** : ajouter une charge d'interruptions (UART à 115 200 bauds,
   timer 10 kHz) et rejouer le test 3.
5. **Champ complet** : vérifier `T3IP/T3IE/T3IF` à l'analyseur logique (l'ISR doit
   s'exécuter ~1 Hz) ; vérifier `PR3:PR2` = 0x01C9C37F au débogage.

## Traçabilité exigence ↔ test

| Exigence | Test(s) |
|---|---|
| Résolution 1 µs/LSB | T2, Niveau 3.1 |
| Erreur ≤ 100 µs | T2, T3a, T3b, T4, Niveau 3.1 + 3.4 |
| Durée 10 h | T6, Niveau 3.2 |
| Appels très nombreux / ISR | T4, T5, Niveau 3.3/3.4 |
| t = 0 mise sous tension | Revue (convention documentée), Niveau 3.2 |
