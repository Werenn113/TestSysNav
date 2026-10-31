# Test Code Embarqué #2 — Processus de validation du buffer circulaire

Le module `RingBuffer` est du C pur (aucune dépendance matérielle) : il se teste **complètement
sur hôte**, ce qui est le meilleur rapport coût/couverture pour du code embarqué. Les tests
ci-dessous sont implémentés dans `TEST_RingBuffer.c` (harnais `assert` : un échec interrompt
immédiatement l'exécution — la dernière ligne affichée garantit donc que TOUT est passé).

---

## 1. Tests sur hôte (PC)

Compilation :

```
gcc -std=c99 -Wall -Wextra -I LibC_reduite -o test_rb TEST_RingBuffer.c LibC_reduite/Os/Common/Utils/RingBuffer.c
./test_rb
```

Succès attendu : `=== TOUS LES TESTS ONT PASSE ===`

| # | Test | Vérifie |
|---|---|---|
| T1 | `Init` : pointeurs nuls, tailles non puissances de 2, taille 1, tailles > 32768 → `RETURN_ARGn_ERR` ; cas valides → `RETURN_SUCCESS`, count = 0, free = size | Contrôles d'arguments |
| T2 | FIFO simple : 16 octets écrits puis lus, ordre identique (`memcmp`) | Sémantique file |
| T3 | **Sans écrasement** : remplissage exact jusqu'à la capacité ; un `Write` supplémentaire → `RETURN_BUSY` sans rien modifier ; drain complet et vérification bit à bit du contenu | L'exigence centrale du sujet |
| T4 | **Transactionalité des échecs** : 48 stockés / 16 libres — `Write` de 24 → `RETURN_BUSY`, état inchangé ; `Read` de 49 → `RETURN_BUSY`, rien retiré ; puis lecture intégrale vérifiée en deux moitiés | « Rien n'est écrit / lu à moitié » |
| T5 (×2) | **Fuzz contre file de référence** : 2 passes de 20 000 opérations aléatoires (longueurs 1–20, seed déterministe) ; chaque octet lu est comparé à une file de référence ; **invariants vérifiés à chaque itération** (`count + free == size`, cohérence bit à bit) ; les index libres s'enroulent de nombreuses fois (modulo 65536), la position mémoire traverse le masque | Index libres, masque, état jamais corrompu |
| T7 | Producteur/consommateur **octet par octet** alternés, 20 000 itérations (modèle de l'ISR UART qui vide pendant que la boucle remplit) | Cas d'emploi NMEA |
| T8 | Buffer de 4096, échantillons de 12 octets, flush **par blocs de 512 complets uniquement**, reste partiel conservé et vérifié | Cas d'emploi Sdcard |

Résultat : **0 échec** sur l'ensemble (2 × 20 000 itérations fuzz + 20 000 octet/octet + scénarios
déterministes), intégrité bit à bit contre file de référence, rejets propres systématiquement
comptés (état inchangé après chaque refus).

## 2. Revue de concurrence (analyse)

Les scénarios de course producteur/consommateur sont vérifiés par **raisonnement + contraintes
du code** (liste à cocher) :

- publication de `write`/`read` **après** la copie — `volatile` sur les index **et** copie via
  pointeur `volatile` → le compilateur ne peut pas réordonner les écritures au-delà de la
  publication ( raison pour laquelle `memcpy` n'est pas utilisé) ;
- un seul contexte écrit chaque index (contrat SPSC documenté dans `RingBuffer.h`) ;
- `count` n'est jamais stocké : toujours recalculé par différence → pas de mise à jour perdue ;
- toutes les lectures/écritures 16 bits sont atomiques sur dsPIC33 (accès alignés).

Sur cible, ce point est confirmé par le test T7 exécuté dans un vrai contexte d'interruption
(cf. §3).

## 3. Plan de validation sur cible (dès accès matériel — non exigé par le sujet)

1. exécuter le même harnais sur le dsPIC (le module est portable tel quel) ;
2. exemple NMEA réel : trames à 4 Hz, UART à 115 200 bauds — vérifier aucune trame perdue en
   régime établi (compteur de `RETURN_BUSY` nul), aucune trame erronée (contrôle d'intégrité au
   récepteur), reprise propre après une pause du récepteur ;
3. saturations volontaires : récepteur arrêté 30 s → trames refusées et **comptées**, aucune
   trame tronquée/écrasée dans le flux au redémarrage ;
4. essai Sdcard : débit soutenu + retrait de carte → aucune écriture partielle, le tampon
   restitue tout à la remise en service ;
5. mesure de durée des primitives à l'oscilloscope (GPIO basculé en entrée/sortie) : budget
   ISR UART ~quelques dizaines de cycles, à comparer à 1 octet / 86 µs à 115 200 bauds.

## 4. Traçabilité exigence ↔ test

| Exigence | Test(s) |
|---|---|
| Buffer circulaire générique, multi-instances | T1–T8 + code exemple |
| **Sans écrasement** (énoncé) | T3, T4, T5 |
| Interface code projet / périphérique en interruption | T7, §2, cible §3.2 |
| Tampon Sdcard de données binaires | T8, cible §3.4 |
| Simplicité d'emploi | Revue de l'API (5 fonctions), code exemple |
