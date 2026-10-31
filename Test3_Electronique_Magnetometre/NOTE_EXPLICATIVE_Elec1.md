# Test Électronique #1 — Note explicative : mesure du champ magnétique

**Candidat : Baptiste Marchand — HMC1001 + CS5509, pilotés par un circuit de logique évolué**

Objectif : mesurer un champ magnétique dans une plage **[−2 G, +2 G]** avec un
**échantillonnage de 100 Hz ±10 Hz**. Livrables : schéma de câblage (`SCHEMA_Magnetometre.svg`),
flowchart (`FLOWCHART_Magnetometre.svg`), présente note. Une maquette n'est pas demandée.

---

## 1. Principe de la chaîne de mesure

```
HMC1001 (pont AMR, ±2 G → ±32 mV) → AD623 (ampli. d'instrumentation, G ≈ 40) → CS5509 (CAN ΣΔ 16 bits) → µC (SPI)
```

Le HMC1001 est un capteur **magnétorésistif (AMR) à pont de Wheatstone** : sa sortie
différentielle vaut **3,2 mV/V par gauss**, soit **16 mV/G sous 5 V**. Sur ±2 G, le signal
de pont ne fait donc que **±32 mV** : il est impossible de le numériser directement avec
une exploitation correcte, d'où un **amplificateur d'instrumentation** avant le CAN.

## 2. Étage set/reset du HMC1001 : pourquoi et comment

Les capteurs AMR se saturent et leur pont présente un offset qui dérive (température,
historique magnétique). Le HMC1001 possède une **strap de set/reset (≈ 4,5 Ω)** dans
laquelle on injecte une impulsion de courant intense (≈ 1 A pendant 1–2 µs) pour
ré-aimanter la permalloy dans un sens (SET) puis dans l'autre (RESET). Le champ
commun aux deux états s'annule par **demi-différence** : `B = (V_SET − V_RESET) / 2`.
C'est la technique recommandée par Honeywell (note d'application AN-212) : elle supprime
l'offset de pont et les dérives sans réglage analogique.

Mise en œuvre : un pont de MOSFET logiques (2 P / 2 N, pilotés par deux GPIO du µC à
travers des résistances de grille de 47 Ω) applique le 5 V dans un sens ou dans l'autre
de la strap ; un condensateur de réserve de 10 µF assure le courant de pointe sans tirer
sur l'alim. Les impulsions sont brèves et rares (2 × 1–2 µs à 100 Hz → quelques µA moyens).

## 3. Choix du gain et des tensions

| Point | Calcul | Conséquence |
|---|---|---|
| Sensibilité pont (5 V) | 16 mV/G | ±2 G → ±32 mV |
| Gain AD623 (RG = 2,55 kΩ) | G = 1 + 100 kΩ/RG ≈ 40 | sortie 640 mV/G |
| Excursion pour ±2 G | 2,5 V ± 1,28 V (référence 2,5 V) | reste dans la plage de sortie RRIO de l'AD623 sous 5 V |
| Marge offset | offset de pont possible du même ordre que le signal (cf. datasheet) → gain volontairement limité à 40 pour conserver de la marge avant saturation de l'AD623 | l'offset résiduel est annulé numériquement par set/reset |

La référence de 2,5 V (U4 : REF192, découplée 100 nF + 10 µF) polarise la broche REF de
l'AD623 **et** l'entrée AIN− du CS5509 : l'entrée différentielle du CAN mesure
`V(AD623) − 2,5 V`, qui est exactement le signal centré.

## 4. Le CS5509 et la cadence de 100 Hz

Le CS5509 est un **CAN sigma-delta 16 bits, entrée différentielle, alimentation simple
4,5–5,5 V, interface série, auto-calibration interne, 200 échantillons/s maximum**. Trois
conséquences directes :

1. **100 Hz est atteignable, mais juste** : le flowchart fait **deux** conversions par
   échantillon (une après SET, une après RESET) → il faut un taux de mot de **200 Hz**
   (2 × 5 ms = 10 ms = une mesure par tick de 10 ms). Le CAN est donc utilisé à son
   maximum : c'est un choix assumé, documenté, et conforme au besoin (mesure simple) ;
   en variante basse consommation, on peut alterner SET/RESET à chaque tick (une
   conversion par tick, 100 Hz) et garder la correction par moyenne glissante.
2. Sa **résolution** (16 bits sur une pleine échelle de plusieurs volts) correspond à
   des µG près : **très au-delà** du besoin ±2 G — aucun étage supplémentaire n'est
   nécessaire, la dynamique est mise sur la marge d'offset, pas sur la résolution.
3. L'**horloge de conversion** vient de son quartz (X1) : la fréquence d'échantillonnage
   de la *manip* est en revanche pilotée par le **timer du µC à 100 Hz ±10 Hz** (le CS5509
   produit ses conversions en continu ; le µC date et prélève). La tolérance ±10 % est
   très large devant la précision d'un quartz : sans risque.

## 5. Compatibilité logique 5 V / 3,3 V

Le µC (type dsPIC33, 3,3 V) dialogue avec un CAN 5 V :

- **µC → CS5509** (SCLK, MOSI, CS) : via des tampons **74HCT125** alimentés en 5 V — la
  famille HCT a un seuil d'entrée TTL (VIH = 2 V) : les niveaux 3,3 V sont reconnus
  sans ambiguïté, et la sortie est remise à niveau 5 V ;
- **CS5509 → µC** (SDATA) : broche µC tolérante 5 V si disponible, sinon pont diviseur
  1 kΩ / 2 kΩ.

C'est le respect des « caractéristiques des chips » demandé par le sujet : les niveaux
ne sont jamais forcés hors spécifications.

## 6. Alimentations et découplage

Le sujet fournit des **alimentations de bureau** : deux rails suffisent — **+5 V**
(pont du capteur, strap set/reset, AD623, CS5509, tampons HCT) et **+3,3 V** (µC), masse
commune. Découplage : 100 nF au plus près de chaque boîtier + 10 µF par rail ; le
condensateur de réserve de la strap set/reset (10 µF) est physiquement proche du HMC1001.

## 7. Ce qui n'est pas traité (hors périmètre du sujet)

- pas de maquette ni de validation sur cible (non demandés) ;
- les valeurs de calibration exactes (facteur µG/LSB) : elles se mesurent sur banc
  (bobine de champ connue), le flowchart prévoit le point d'entrée de ce facteur ;
- le traitement logiciel des données (stockage, filtrage) : sortie de la chaîne = un
  tableau daté de mesures en µG à 100 Hz.
