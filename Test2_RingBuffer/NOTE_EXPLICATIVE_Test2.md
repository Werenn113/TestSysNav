# Test Code Embarqué #2 — Note explicative : buffer circulaire sans écrasement

**Candidat : Baptiste Marchand — API multi-systèmes (LibC réduite)**

---

## 1. Analyse du besoin

| Exigence du sujet | Réponse |
|---|---|
| Fonctionnalité **buffer circulaire** générique dans l'API | Module `RingBuffer` dans `Os/Common/Utils/` : couche commune, aucun accès matériel, portable sur toute cible de l'API (PIC24F, dsPIC33E/F) |
| **Sans écrasement** | `RingBuffer_Write` est *transactionnel* : si la place libre est insuffisante, **rien n'est écrit** et `RETURN_BUSY` est retourné. Les données les plus anciennes sont toujours intactes |
| **Multi-instances** | Structure `RingBuffer` autonome ; la mémoire est fournie par l'appelant (`RingBuffer_Init(&rb, mem, size)`) : pas d'allocation dynamique (interdite en embarqué), plusieurs buffers indépendants (NMEA, Sdcard, …) |
| Exemple : NMEA 4 Hz ↔ périphérique en interruption | Producteur = boucle principale, consommateur = ISR : voir `EXEMPLE_UTILISATION_Test2.c` |
| Exemple : tampon d'écriture Sdcard binaire | Idem avec flush par blocs de 512 octets |

## 2. Choix de conception

### 2.1 Index libres (« free-running ») plutôt qu'index modulo

`write` et `read` sont des compteurs **uint16 qui ne reviennent jamais à zéro** ; la position
mémoire est obtenue par masque (`index & (size-1)`). Avantages :

- **pas d'ambiguïté plein/vide** : avec des index « enroulés » sur la taille, `write == read`
  signifie aussi bien vide que plein (il faut sacrifier une case). Avec des index libres :
  `count = write - read` (modulo 65536) → 0 = vide, `size` = plein, **capacité pleinement
  utilisable** ;
- pas de division/modulo : un simple masque (le masque exige une **taille puissance de 2**,
  contrôlée dans `Init` — 512, 4096, … sont des tailles naturelles pour ce genre de tampons) ;
- contrepartie : `size ≤ 32768` pour que `write - read` reste non ambigu (compté, documenté).

### 2.2 Cohérence producteur / consommateur **sans coupure d'interruptions**

Contrat : **1 producteur + 1 consommateur** (cas des deux exemples du sujet). Ce contrat
permet une implémentation *lock-free* :

1. le producteur copie les données, **puis publie** `write` ;
2. le consommateur copie, **puis publie** `read`.

Sur un cœur unique (dsPIC), une interruption ne démarre qu'entre deux instructions : si
l'ISR consommatrice voit l'index avancé, les données sont déjà en mémoire. Deux précautions
du côté compilateur :

- les index sont `volatile` (jamais mis en cache, jamais fusionnés) ;
- la copie se fait via un **pointeur volatile** : sans lui, le compilateur pourrait
  réordonner/copier en bloc après la publication de l'index — c'est exactement le genre de
  bug invisible en revue. (`memcpy` est écarté pour cette raison : pas de garantie
  d'ordre vis-à-vis des accès volatile.)

Les tests `GetCount`/`GetFreeSpace` sont des lectures isolées : toute valeur lue est un
état cohérent (peut être légèrement périmée, jamais incohérente). La vérification
d'espace libre puis l'écriture ne sont pas atomiques mais **sûres** : dans le contrat
SPSC, seul le consommateur fait croître la place libre — un `Write` ne peut donc jamais
sous-estimer la place et écraser (le contrôle est fait à chaque octet... non : à chaque
transaction, et la place ne peut qu'augmenter entre le contrôle et la copie).

### 2.3 Échecs propres plutôt qu'écrasement ou attente

- `RETURN_BUSY` (code existant de `ReturnStatus` !) quand ça ne passe pas : l'appelant
  décide (retenter, compter une perte, signaler) — l'API ne bloque jamais, ne consomme
  pas de temps système en attente ;
- arguments invalides → `RETURN_ARGn_ERR` : codes déjà définis par l'API, réutilisés à bon
  escient (`n` = numéro de l'argument).

### 2.4 Simplicité d'utilisation

Cinq fonctions, une ligne d'instanciation, aucune donnée à cacher :

```c
RingBuffer_Init(&rb, mem, 512);         // instance
RingBuffer_Write(&rb, frame, len);      // producteur
RingBuffer_Read(&rb, &byte, 1);         // consommateur (ISR)
RingBuffer_GetCount(&rb);               // place occupée
RingBuffer_GetFreeSpace(&rb);          // place libre
```

## 3. Pourquoi ce placement dans l'API

`Os/Common/Utils/` accueille `GenericTypeDefs.h` : c'est le dossier des briques communes
sans dépendance matérielle. Le buffer n'utilise ni `xc.h`, ni registres : il compilera
sans modification sur toutes les architectures couvertes par `Peripheral.h`. Aucune
couche d'abstraction supplémentaire n'est nécessaire — le sujet demande une fonctionnalité
**utilisée** comme interface, pas un pilote.

## 4. Limites documentées

- **1 producteur + 1 consommateur** maximum (au-delà : encapsuler les appels dans une
  section critique côté appelant, l'API ne force pas son coût à tous les utilisateurs) ;
- taille : puissance de 2, entre 2 et 32768 octets ;
- granularité : l'octet (les deux exemples du sujet sont byte-oriented ; une variante
  « items » de taille fixe se dériverait en changeant deux lignes — non livrée pour
  rester minimal) ;
- si un `Write` de `length > size` est demandé, il échouera toujours (`RETURN_BUSY`) :
  cohérent avec la sémantique « sans écrasement ».
