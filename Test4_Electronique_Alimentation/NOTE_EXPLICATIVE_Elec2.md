# Test Électronique #2 — Note explicative : alimentation 230 V → LEA-5H

**Candidat : Baptiste Marchand — alimentation régulée pour GPS u-blox LEA-5H depuis le secteur 230 V**

Livrables : schéma de câblage (`SCHEMA_Alimentation.svg`) + présente note. Maquette non demandée.
Contrainte du sujet : les **modules de conversion intégrés type EMS 7218 sont interdits** →
chaîne de conversion **discrète**, réalisée avec des composants du commerce.

---

## 1. Architecture retenue : abaissoir linéaire

```
230 V → F1 (fusible) → RV1 (varistance) → TR1 (transfo sécurité 230/6 V, 3,2 VA)
      → B1 (pont DB107S) → C1 (réservoir 1500 µF) → U1 (LDO LD1117-3.3) → LEA-5H
```

Pourquoi un **linéaire** et pas un découpage (buck) :

- le sujet interdit les modules intégrés ; un buck discret serait réalisable mais
  apporterait des **perturbations conduites/rayonnées** (di/dt, harmoniques) à côté d'un
  récepteur GNSS à −160 dBm — le risque d'auto-brouillage est réel, le coût de mise au
  point élevé, pour 0,1 W à convertir ;
- la consommation est minuscule (~50–70 mA) : le rendement n'est pas un critère ;
- la **qualité de l'alimentation** du récepteur prime : le Hardware Integration Manual
  u-blox exige une **ondulation < 50 mVpp** sur VCC et une alimentation capable de
  **soutenir les pointes de courant** — un LDO linéaire avec un réservoir de faible ESR
  répond à cela avec une PSRR > 60 dB.

## 2. Dimensionnement (les chiffres de la note)

**Charge** : LEA-5H ≈ 50 mA typ., pointes à l'acquisition (~67 mA). L'alim est dimensionnée
à **300 mA** (marge ×4 sur les pointes).

**Transformateur TR1** : 230 V / 6 V, **3,2 VA** (≈ 530 mA au secondaire), type **classe II**
à isolation renforcée, transformateur de sécurité pour montage PCB (ex. famille Myrra 44xxx,
fusible thermique intégré). Le 6 V est un bon compromis : assez haut pour garder la marge
de dropout dans le pire cas, assez bas pour limiter la dissipation du LDO.

**Pire cas d'entrée du LDO** (secteur −10 %, ondulation en creux) :

```
V_crete = 6 × 0,9 × 1,414       ≈ 7,6 V
V_min  = 7,6 − 1,4 (pont) − 1,2 (ondulation C1) ≈ 5,0 V
seuil : 3,3 V + dropout 1,1 V = 4,4 V   →   marge de 0,6 V ✔
```

**Pont B1** : DB107S (1 A, 1000 V) — courant dérisoire devant son calibre, tenue secteur.

**Réservoir C1** : `C ≥ I × Δt / ΔV = 0,3 A × 8 ms / 1,2 V = 2 mF` → **1500 à 2200 µF / 16 V**
(à faible ESR). Bleeder **R1 = 10 kΩ** pour décharger C1 à l'arrêt (sécurité maintenance).

**LDO U1** : **LD1117-3.3** en SOT-223 : dropout 1,1 V @ 100 mA, 800 mA max, PSRR ~60 dB.
Dissipation : `P ≈ (5 − 3,3) × 0,07 ≈ 0,12 W` → échauffement ~6 °C en SOT-223 : pas de
radiateur. Découplage : 10 µF + 100 nF à l'entrée, **22 µF + 100 nF** en sortie
(stabilité du régulateur : le datasheet du LD1117 exige une capa de sortie ≥ 10 µF).

**Au niveau du module GPS** : perle de ferrite **L1** + **47 µF** + 100 nF au plus près des
broches VCC : le 47 µF couvre les pointes d'appel du récepteur (le câblage vers le LDO a
toujours une inductance parasite), la perle isole le module du bruit résiduel — le combo
respecte l'exigence « < 50 mVpp » avec une grande marge.

**V_BCKP** : pile CR2032 via diode Schottky D1 — option retenue pour conserver la RTC/backup
RAM ( Hot/Warm Start) lors des coupures ; peut être omise (démarrage à froid).

**Antenne** : patch **passive** → pas d'alimentation d'antenne. Variante antenne active :
VCC_RF existe sur le LEA-5H (100 mA max) avec sa résistance de 10 Ω en série — mentionnée
pour mémoire, non retenue (manip simple).

## 3. Sécurité (au moins autant évaluée que la conversion)

- **Fusible F1 (T 500 mA / 250 V)** en tête, avant tout composant ;
- **RV1 : varistance 275 V** entre L et N (claquage des transitoires secteur) ;
- **TR1 = composant de sécurité classe II** (double isolation) : pas de liaison terre
  nécessaire, aucune partie basse tension reliée au secteur ;
- aucune surface du secondaire en contact avec le primaire, distances d'isolement
  respectées par le transformateur lui-même ;
- arrivée secteur **sous connecteur isolé et gaine thermorétractable** ; l'ensemble
  sera encastré pour la manip.

## 4. Bilan et points vérifiables

| Grandeur | Valeur calculée | Critère |
|---|---|---|
| Tension LDO au pire cas | ≥ 5,0 V | > 4,4 V (dropout) ✔ |
| Ondulation VCC module | < 10 mVpp attendus | < 50 mVpp (u-blox) ✔ |
| Courant fourni | 300 mA | > 67 mA pointe ✔ ✔ |
| Échauffement LDO | ~6 °C | très inférieur aux limites ✔ |
| Puissance TR1 | 3,2 VA | > 0,4 VA utile ✔ ✔ |

Tests de vérification (sur maquette éventuelle) : tension et ondulation VCC à l'oscillo
en charge réelle (pointe d'acquisition), tenue aux creux secteur (variac −10 %),
température du LDO après 1 h, et time-to-first-fix nominal du GPS.
