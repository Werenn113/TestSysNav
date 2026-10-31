# Test électronique #2 — Alimentation régulée 230 V → 3,3 V pour GPS u-blox LEA-5H

Livrables demandés par le sujet :

| Livrable | Fichier |
|---|---|
| Schéma de câblage (composants, références, valeurs) | `SCHEMA_Alimentation.svg` |
| Note explicative des choix | `NOTE_EXPLICATIVE_Elec2.md` |

Le fichier `.svg` s'ouvre dans n'importe quel navigateur (clic double, ou
glisser-déposer dans un onglet).

## Architecture

Chaîne **linéaire discrète** (les modules intégrés type EMS 7218 sont interdits par
le sujet) : fusible → varistance → transformateur de sécurité classe II 230/6 V
→ pont de diodes → condensateur réservoir → LDO LD1117-3.3 → module LEA-5H.
Le dimensionnement (pire cas à secteur −10 %, ondulation, pointes de courant
d'acquisition, exigences u-blox < 50 mVpp, sécurité) est détaillé dans
`NOTE_EXPLICATIVE_Elec2.md`.
