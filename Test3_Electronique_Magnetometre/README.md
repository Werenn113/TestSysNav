# Test électronique #1 — Mesure du champ magnétique [−2 G, +2 G] à 100 Hz ±10 Hz

Livrables demandés par le sujet :

| Livrable | Fichier |
|---|---|
| Schéma de câblage (composants, références, valeurs) | `SCHEMA_Magnetometre.svg` |
| Flowchart de récupération des mesures | `FLOWCHART_Magnetometre.svg` |
| Note explicative des choix | `NOTE_EXPLICATIVE_Elec1.md` |

Les fichiers `.svg` s'ouvrent dans n'importe quel navigateur (clic double, ou
glisser-déposer dans un onglet).

## Chaîne de mesure

HMC1001 (capteur AMR en pont, ±2 G → ±32 mV) → AD623 (amplificateur
d'instrumentation, G ≈ 40) → CS5509 (CAN sigma-delta 16 bits, interface SPI) → µC.
La cadence de 100 Hz est pilotée par le timer du µC ; l'offset du pont est annulé
par la technique set/reset (demi-différence SET/RESET, note d'application
Honeywell AN-212). Le dimensionnement complet (gain, tensions, compatibilité
5 V / 3,3 V, découplage) est détaillé dans `NOTE_EXPLICATIVE_Elec1.md`.
