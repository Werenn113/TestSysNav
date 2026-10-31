# Test code #1 — Fonctionnalité temps `getTime()`

Dossier **autonome** : tout ce qu'il faut pour compiler et exécuter le test sur un PC
(le dsPIC33 est « mocké » par `test_host/xc.h`).

## Contenu

| Fichier | Rôle |
|---|---|
| `TEST_Time.c` | Harnais de test exécutable sur PC (7 tests, dont les courses critiques rendues déterministes par les sondes T3a/T3b) |
| `LibC_reduite/Os/Pic/Time/Time.c`, `Time.h` | Module livré (identique à celui intégré dans l'API à la racine du dépôt) |
| `LibC_reduite/Os/Common/Utils/GenericTypeDefs.h` | Type `Time` corrigé en 64 bits (justification dans la note explicative §1) |
| `test_host/xc.h` | Mock des registres du dsPIC33 pour la compilation sur PC |
| `NOTE_EXPLICATIVE_Test1.md` | Note explicative demandée par le sujet |
| `VALIDATION_Test1.md` | Processus de validation demandé par le sujet |

## Compilation et exécution

```bash
gcc -std=c99 -Wall -Wextra -I test_host -I LibC_reduite -I LibC_reduite/Os/Pic/Time \
    -o test_time TEST_Time.c LibC_reduite/Os/Pic/Time/Time.c
./test_time
```

**Succès : les 7 tests affichent OK.**
