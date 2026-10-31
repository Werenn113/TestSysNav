# Test Code Embarqué #1 — Note explicative : fonctionnalité temps `getTime`

**Candidat : Baptiste Marchand — cible : dsPIC33EP512MU814, quartz 16 MHz**

---

## 1. Analyse du besoin

| Contrainte du sujet | Conséquence sur la conception |
|---|---|
| Résolution 1 µs par LSB | Compter en µs, granularité ≤ 1 µs |
| Durée max 10 h = 36 000 s = **3,6 × 10¹⁰ µs** | Le type `Time` d'origine (`uint8`, max 255 µs) est **impossible à utiliser** ; `uint32` (max 4,29 × 10⁹ µs ≈ 71 min) est insuffisant aussi → **`Time` passe en `uint64`** (modif. documentée dans `GenericTypeDefs.h`) |
| Erreur de datation ≤ 100 µs | La lecture de l'heure doit être rapide ET cohérente (cf. §4) |
| Appels très nombreux (code projet + périphériques) | Fonction légère, reentrante, sans blocage, sans pile/déplacement de pointeur |
| t = 0 = mise sous tension | Convention : t = 0 = démarrage du timer dans `Time_Init()` (cf. §6) |

## 2. Ressource matérielle choisie

L'horloge système est configurée par `systemPowerUp()` : quartz 16 MHz, `PLLFBD = 13`,
`PLLPRE = 0`, `PLLPOST = 0` → **Fcy = 16 MHz × (13+2) / ((0+2) × 2 × (0+1)) / 2 = 30 MHz**.

J'utilise le couple **Timer2 + Timer3 en mode 32 bits** (`T2CONbits.T32 = 1`), horloge Fcy,
prescaler 1:1 : 1 tick = 33,3 ns, compteur `TMR3:TMR2` sur 32 bits.

- **Période = 1 seconde exacte** : `PR3:PR2 = 29 999 999`. Détail important : le compteur
  va de 0 à PR **inclus**, soit (PR+1) cycles par période → charger 30 000 000 dans PR
  donnerait une seconde de 30 000 001 cycles (dérive +33 ns/s ≈ +1,2 ms sur 10 h, négligeable
  mais évitable — je l'évite).
- Un compteur 32 bits à 30 MHz boucle en 143 s : trop court pour 10 h → il faut **étendre
  le compteur en logiciel**.

## 3. Extension logicielle 64 bits

À chaque débordement (match de période), une **interruption Timer3** incrémente un compteur
de secondes `timeSeconds` (volatile). Le temps final :

```
time_us = timeSeconds × 1 000 000 + ticks / 30        (ticks = valeur TMR3:TMR2, 0..29 999 999)
```

- `ticks / 30` : 30 ticks Fcy = 1 µs → résolution de 1 µs par LSB ✔
- Priorité de l'interruption : **T3IP = 5**. Sur dsPIC33, une priorité 0 laisse
  l'interruption désactivée (piège classique) — elle est positionnée explicitement.
- C'est un compteur lent (1 Hz) : priorité volontairement moyenne ; on verra en §4 que la
  conception ne dépend **pas** de la latence de cette interruption.

## 4. Lecture cohérente : le vrai point difficile

`getTime()` peut être appelée pendant que l'interruption Timer3 modifie `timeSeconds`, et
pendant que le timer matériel boucle (reset à 0 au match de période). Trois lectures
séparées (secondes / timer / drapeau) peuvent être **incohérentes entre elles** :

- si l'ISR s'exécute *entre* la lecture des secondes et celle du timer, la simple boucle
  « relire tant que les secondes ont changé » suffit (pattern seqlock) ;
- **mais** si le timer boucle *entre* la lecture du timer et celle du drapeau, on obtient
  `secondes = N` et `ticks ≈ 0` alors que la vraie heure est `(N+1) s + ε` : erreur d'une
  **seconde entière**, non détectable par relecture.

La solution retenue : **section critique matérielle** — le niveau de priorité du CPU est
monté à 7 (`SR |= 0x00E0`) pendant les 5 lectures (~15 cycles = 0,5 µs), puis restauré.
La capture est alors **atomique** :

- si le drapeau `T3IF` est à 1 au moment de la capture, le timer vient de boucler et
  l'ISR n'a pas encore incrémenté → `getTime` corrige elle-même (`sec++`). La valeur
  retournée reste exacte **même si l'ISR est retardée de près d'une seconde** (charge
  d'interruptions, priorités, sections critiques ailleurs) ;
- dans la section critique, lire `TMR2` **fige le poids fort** dans `TMR3HLD` (mode 32 bits) :
  c'est la séquence TMR2 puis TMR3HLD qui garantit un mot de 32 bits cohérent.

Il reste pourtant une ambiguïté : le drapeau peut aussi s'être levé **pendant** la capture —
le timer matériel ne s'arrête pas sous IPL=7 ! Si `T3IF = 1`, `getTime` **relit donc le
compteur** : si la nouvelle valeur est *inférieure* à la précédente, le compteur a bouclé
entre les deux lectures, donc le débordement date de notre propre capture et les ticks
capturés sont pré-débordement : il ne faut **pas** compter cette seconde. Cette relecture
comparative ferme la dernière fenêtre de course (quelques cycles) : sans elle, environ un
appel sur dix millions daterait une seconde trop tôt — un défaut quasi impossible à
reproduire sur cible, mais que le harnais de test (`TEST_Time.c`, sonde T3b) rend
déterministe.

Pourquoi pas `disi` ? Je préfère la sauvegarde/restauration de `SR` : elle masque **tous**
les niveaux utilisateur (1 à 7) et restaure exactement l'état antérieur, alors que `disi`
se comporte de façon différente selon les niveaux de priorité et ne s'annule pas proprement.

## 5. Analyse d'erreur (spec : ≤ 100 µs)

| Source d'erreur | Ordre de grandeur |
|---|---|
| Durée d'exécution de `getTime` (~30–60 cycles dont la division 32 bits) | 1–2 µs |
| Désactivation des interruptions pendant la capture | ~0,5 µs |
| Débordement en attente (T3IF) | **Corrigé** par `sec++` → 0 µs |
| Débordement pendant la capture | **Corrigé** par relecture comparative du compteur → 0 µs |
| Latence ISR (si ISR en retard) | **Sans effet** grâce à la correction T3IF |
| Résolution ticks (truncation ticks/30) | ≤ 1 µs |

→ Erreur de datation ≤ ~3 µs dans tous les cas, marge ×30 par rapport aux 100 µs demandés.

Le temps retourné correspond à l'instant de la lecture de `TMR2` (premier registre lu dans
la section critique) : la sémantique « temps à l'appel » est respectée.

**Limite documentée** : si une autre partie du système coupe les interruptions pendant
**plus d'une seconde** (à proscrire !), un deuxième débordement survient avant que le
premier soit compté : un seul incrément est effectué, la base temps dérive d'1 s. Le
drapeau T3IF ne pouvant compter les débordements multiples, toute section critique doit
rester ≪ 1 s (contrainte raisonnable ; une surveillance « wrapPending à 1 pendant > 100 ms »
peut être ajoutée comme garde-fou en phase d'intégration).

**Précision long terme** : la dérive dépend de la tolérance du quartz (typ. ±20–50 ppm,
soit ±1,8 s sur 10 h à 50 ppm). C'est hors périmètre de la spec (erreur de *datation* d'un
appel), mais si une exactitude absolue est requise, l'horloge RTCC ou une calibration
logicielle peuvent affiner le facteur de conversion.

## 6. Convention t = 0

Il est impossible de dater l'instant physique de la mise sous tension sans RTC externe.
Convention retenue, cohérente avec l'architecture existante : **t = 0 = démarrage du timer
dans `Time_Init()`**, appelé par `systemInit()` juste après `systemPowerUp()` — soit quelques
centaines de µs après le reset. Les usages visés (datation *relative* des opérations) ne
sont pas impactés.

## 7. Revue de la version initiale du fichier `Time.c` (repo)

Une première implémentation était présente dans l'archive ; les défauts suivants ont été
corrigés — ils illustrent les points traités ci-dessus :

1. **Cast `(Time)sec` avec `Time = uint8`** : la valeur retournée était tronquée à 8 bits
   → retour faux dès 255 µs. (Le changement de typedef est indispensable, pas optionnel.)
2. **Priorité d'interruption non configurée** (`T3IP` reste 0) : sur dsPIC33, priorité 0 =
   interruption inactive → `system_seconds` ne s'incrémentait jamais.
3. **`PR = 30 000 000`** au lieu de 29 999 999 : seconde de 30 000 001 cycles.
4. **Race condition non couverte** : la boucle seqlock ne détecte pas le cas
   « le timer a bouclé entre la lecture des ticks et la comparaison des secondes »
   (erreur potentielle d'1 s) → remplacée par la capture atomique §4.
5. **Style** : types `stdint.h` (`uint32_t`…) au lieu des types de l'API (`uint32`…) ;
   commentaires doxygen d'en-tête manquants ; nommage des variables hors conventions.

## 8. Forme du code

- Types, bannières de fichiers, en-têtes doxygen `///`, nommage `Time_Init`/`getTime`,
  `ReturnStatus` : conformes au reste de la LibC.
- Types stdint utilisés nulle part : uniquement `uint8/16/32/64` de `GenericTypeDefs.h`.
- Aucune allocation dynamique, aucune dépendance autre que l'API et `xc.h`.
