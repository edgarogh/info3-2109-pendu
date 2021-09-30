# INFO3 Polytech – Pendu (sept. 2021)

Edgar Onghena &lt;dev@edgar.bzh&gt;

## Compilation

Le projet utilise CMake pour la compilation. Vu la simplicité du projet, il est tout à fait possible de compiler sans. Il utilise la macro `NDEBUG` standard pour compiler conditionnellement une variante "debug" ou "release", ce dernier activant le seedage du générateur de nombre aléatoires comme suggéré.
Les instructions ci-dessous permettent de compiler en "release", la version définitive présentable.

Sans CMake:
```bash
gcc -DNDEBUG main.c -o pendu
./pendu
```

Avec CMake (sur Unix):
```bash
mkdir build
cd build
cmake -DCMAKE_BUILD_TYPE=Release ..
make
./pendu
```
(CMake s'intègre surtout bien à certains IDEs et est beaucoup plus pratique ainsi)

## Configuration

Le nombre d'erreurs autorisées est contraint par la constante `MAX_ERRORS`. Comme j'utilise un nombre d'erreur maximales au lieu d'un nombre d'essai maximal, il est impossible que la victoire soit impossible, donc aucune vérification n'a besoin d'être faite.
