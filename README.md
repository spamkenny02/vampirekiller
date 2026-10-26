# Vampire Killer MSX2 → Mega Drive / SGDK

Jalon 3, 26 octobre 2026 : **Simon peut marcher et sauter dans la premiere salle
sur Mega Drive.** La ROM conserve aussi l'explorateur de 152 fonds sur 18 stages.
Les sondes de collision et l'increment de saut portes en C ont ete compares
a l'execution du code Z80 original dans 53 080 cas.

Le mode personnage est un prototype limite a la salle 1/0 : graphismes
d'origine, primitives traduites et controleur encore simplifie. Attaques,
escaliers, sorties, objets, ennemis et son restent a implementer.

## Essayer

Ouvrir `build/vampire-killer-md-viewer.bin` dans un emulateur Mega Drive.
La ROM demarre directement dans le decor du stage 1, salle 0.

| Commande | Action |
| --- | --- |
| Gauche / droite | Salle precedente / suivante, avec retour en boucle |
| Haut / bas | Stage suivant / precedent, avec retour en boucle |
| A | Basculer entre la salle et les 192 emplacements de tuiles |
| START | Revenir au stage 1, salle 0 |
| B | Entrer dans le mode personnage de la premiere salle / quitter ce mode |

Dans le mode personnage : gauche/droite deplace Simon, **C** declenche un saut,
START reinitialise sa position et B revient a l'explorateur.

Les numeros de salles commencent a zero. Le texte de diagnostic remplace le HUD
original. Le programme n'execute pas le jeu MSX complet.

## Resultats verifies

- 153 entrees de salles decodees dans les tables des stages 1 a 18.
- Pour chacune, execution de la routine Z80 originale `4F83/4FAF` dans un harness
  CPU + mapper : les **768 octets de carte correspondent exactement** a l'extraction.
- 152 fonds convertis en ressources SGDK; 7 jeux de tuiles distincts.
- Pour les 152 fonds, comparaison pixel par pixel dans Genesis Plus GX avec la
  reconstruction attendue : **45 056 pixels identiques par fond**, soit 6 848 512 au total.
- Navigation entre salles et stages, bascule vers l'atlas et remise a zero testees.
- Compilation SGDK 2.11 / GCC m68k-elf 13.1.0 sans avertissement; ROM de 393216 octets.
- Signature SEGA, alignement et checksum au format SGDK controles.
- 53 080 comparaisons C/Z80 : classification, adressage de carte, sondes
  au sol et laterales, calcul du saut.
- Huit poses de Simon extraites des masques RLE et des tables de placement.
- Test MD : 60 pixels en 30 images a droite, 20 pixels en 10 images a gauche,
  saut et retour au sol. Cinq captures correspondent pixel par pixel aux
  masques originaux composes sur le decor.

La derniere entree, stage 18 / salle 9, utilise des identifiants de tuiles
192 a 245 qui ne sont pas couverts par le chargement standard identifie.
Son image de diagnostic comporte du magenta; elle est exclue de l'explorateur.
Ce cas demande une analyse specifique. Les autres images restent des fonds
statiques sans les modifications du jeu en cours d'execution.

## Parcourir les fichiers

| Chemin | Contenu |
| --- | --- |
| `build/vampire-killer-md-viewer.bin` | ROM Mega Drive prete a essayer |
| `validation/md-stage-01-room-00.png` | Capture reelle de la ROM dans Genesis Plus GX |
| `validation/md-player-jump.png` | Simon en plein saut dans la premiere salle |
| `research/player-format.md` | Sprite, collisions, saut et limites du prototype |
| `assets/stage-overview.png` | Apercu reconstruit des 18 premiers fonds de stages |
| `assets/rooms/` | 153 images PNG et cartes completes de 32 x 24 tuiles |
| `assets/manifest.json` | Provenance ROM, palettes, statut et limites de chaque fond |
| `research/room-format.md` | Formats retrouves et correspondances d'adresses |
| `research/loading-and-states.asm` | 801 instructions de regions selectionnees |
| `research/known.asm` | 111 annotations initiales, avec octets attendus |
| `sgdk/src/` | Explorateur et ressources C generees |
| `tools/` | Analyse, extraction, desassemblage et verification |
| `validation/` | Rapports et captures de test |

## Reproduire l'extraction

Depuis ce dossier, avec Python 3.10 ou plus recent :

```sh
python3 -m pip install -r requirements.txt
python3 tools/probe.py "/chemin/Vampire Killer (Japan, Europe).zip"
python3 tools/extract_rooms.py "/chemin/Vampire Killer (Japan, Europe).zip"
python3 tools/extract_player.py "/chemin/Vampire Killer (Japan, Europe).zip"
python3 tools/disassemble_regions.py "/chemin/Vampire Killer (Japan, Europe).zip"
python3 tools/verify_z80_maps.py "/chemin/Vampire Killer (Japan, Europe).zip"
```

Un fichier `.rom` brut est egalement accepte. Les outils refusent une empreinte
SHA256 differente du dump etudie. La ROM MSX originale n'est pas incluse dans
ce dossier. Les ressources graphiques du programme proviennent du fichier fourni.
Les PNG et cartes dans `assets/rooms/` sont regenerables et exclus de Git.
Les ressources C generees sont versionnees : compiler la ROM MD ne necessite
pas le fichier MSX dans le checkout.

## Compiler

Avec une installation SGDK 2.11 fonctionnelle et sa chaine m68k-elf :

```sh
cd sgdk
make GDK=/chemin/vers/sgdk
```

La sortie est `sgdk/out/rom.bin`. Sous Windows, on peut employer le makefile
de l'installation SGDK avec les outils fournis dans `bin/`.

L'environnement de validation utilise Marsdev v1.0.0-rc1 / GCC 13.1.0. Le format
LTO de la bibliotheque precompilee SGDK 2.11 etait incompatible avec ce GCC.
Les objets natifs deja presents dans cette archive ont ete utilises, en retirant
uniquement les sections LTO d'une copie de la bibliotheque :

```sh
m68k-elf-objcopy --remove-section='.gnu.lto_*' \
  /chemin/sgdk/lib/libmd.a /chemin/sgdk/lib/libmd-native.a
make GDK=/chemin/sgdk LIBMD=/chemin/sgdk/lib/libmd-native.a
```

La version Java disponible ici necessitait aussi que son dossier `lib/` soit
present dans `LD_LIBRARY_PATH`. Cela depend de l'installation locale de Java.
Les versions et empreintes des archives utilisees figurent dans `validation/toolchain.json`.

## Reproduire le test Mega Drive

Avec un core libretro Genesis Plus GX compile localement en sortie RGB565 :

```sh
python3 tools/verify_viewer.py /chemin/genesis_plus_gx_libretro.so \
  "/chemin/Vampire Killer (Japan, Europe).zip" build/vampire-killer-md-viewer.bin
```

La verification couvre le rendu MD par rapport a l'extraction. L'execution de
la routine Z80 valide separement les cartes. Aucune execution complete MSX2
avec BIOS, VDP et interruptions n'a encore ete comparee a ce portage.

Pour verifier les primitives du personnage avec un compilateur C natif :

```sh
cc -shared -fPIC -std=c99 -Wall -Wextra -Werror -O2 \
  sgdk/src/player.c sgdk/src/generated/player_assets.c -o /tmp/player.so
python3 tools/verify_player_primitives.py \
  "/chemin/Vampire Killer (Japan, Europe).zip" /tmp/player.so
python3 tools/verify_play_mode.py /chemin/genesis_plus_gx_libretro.so \
  "/chemin/Vampire Killer (Japan, Europe).zip" sgdk/out/rom.bin sgdk/out/symbol.txt
```

Les fichiers de demarrage SGDK conservent leur licence MIT dans
`third_party/SGDK-LICENSE.txt`. Les emulateurs et la chaine de compilation ne sont
pas inclus. Prochain jalon : fouet, escaliers et transitions de salles.
Depot : https://github.com/spamkenny02/vampirekiller.
