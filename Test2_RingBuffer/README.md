# Test code #2 — Buffer circulaire sans écrasement (RingBuffer)

Dossier **autonome** : tout ce qu'il faut pour compiler et exécuter le test sur un PC.

## Contenu

| Fichier | Rôle |
|---|---|
| `TEST_RingBuffer.c` | Harnais de test exécutable sur PC (init, FIFO, plein sans écrasement, écrasement interdit, données aléatoires, multi-instances) |
| `LibC_reduite/Os/Common/Utils/RingBuffer.c`, `RingBuffer.h` | Module livré (identique à celui intégré dans l'API à la racine du dépôt) |
| `LibC_reduite/Os/Common/Utils/GenericTypeDefs.h` | Types de l'API (inchangé par rapport au livrable Test #1) |
| `EXEMPLE_UTILISATION_Test2.c` | Exemple d'utilisation demandé par le sujet : trames NMEA 4 Hz ↔ IT UART, tampon d'écriture Sdcard |
| `NOTE_EXPLICATIVE_Test2.md` | Note explicative demandée par le sujet |
| `VALIDATION_Test2.md` | Processus de validation demandé par le sujet |

## Compilation et exécution du test

```bash
gcc -std=c99 -Wall -Wextra -I LibC_reduite -o test_rb \
    TEST_RingBuffer.c LibC_reduite/Os/Common/Utils/RingBuffer.c
./test_rb
```

**Succès : message `=== TOUS LES TESTS ONT PASSE ===`.**

## À propos de l'exemple d'utilisation

`EXEMPLE_UTILISATION_Test2.c` est un **extrait de firmware** (il inclut `xc.h` et
`Os/Os.h`) : il montre comment brancher le buffer sur l'UART en interruption et sur
l'écriture Sdcard par blocs de 512 octets. Il n'est pas destiné à être compilé sur
PC — le sujet ne le demande pas ; le harnais `TEST_RingBuffer.c` couvre le comportement.
