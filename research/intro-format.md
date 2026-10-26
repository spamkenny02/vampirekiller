# Demarrage et entree du chateau — jalon 4

## Routines retrouvees

| Routine | Role dans la reconstruction |
| --- | --- |
| 4B59 | Initialisation video, mode 5 et pages |
| 6202, puis 624C | Logo Konami et copie progressive des lignes |
| 4D47 | Ecran titre international |
| 44C6 | Initialisation des variables d'une partie |
| 63D3, avec C41A=1 | Scene exterieure devant la grille |
| 4E93 | Mise a jour des entites et de leurs motifs dans cette scene |
| 62D0 puis 422C | Chargement de la premiere zone de jeu et du HUD |

Le logo et le titre sont reconstruits depuis les donnees du jeu, sans images
redessinees. Le titre international utilise la valeur BIOS 002B=1. Le texte
PUSH SPACE KEY est remplace par PRESS START dans le programme MD.

## Reconstruction bornee

`intro_harness.py` execute les instructions Z80 des routines selectionnees,
avec ROM, RAM et selecteurs Konami. Les ecritures directes de VRAM passent
par les ports adaptes 98/99. Les appels BIOS et certaines operations VDP sont
remplaces explicitement par des operations synchrones sur une VRAM logicielle :

- WRTVDP et CHGMOD pour la configuration video;
- 4814 pour les changements de palette;
- HMMV, HMMM, LMMM et HMMC pour les transferts graphiques;
- LINE pour les contours horizontaux et verticaux;
- attente VDP terminee immediatement et appels sonores ignores.

Ce n'est pas une machine MSX2 complete. Les delais BIOS, le temps d'execution
des commandes VDP, les interruptions et le son ne sont pas reproduits.
La verification des pixels MD porte sur ces references reconstruites, pas
sur une capture independante d'un MSX2 executant le jeu entier.

## Animation devant la grille

La scene est initialisee par 63D3, puis mise a jour pendant 320 ticks par
4E93. Ces appels produisent les positions et motifs du nuage, des deux
chauves-souris et de Simon qui avance vers le centre de l'ecran.

Les attributs a F600, couleurs de lignes a F400 et motifs a F800 permettent
de regrouper les couches superposees par position. Les couches CC sont
composees par combinaison des indices de couleur. Les huit groupes obtenus
deviennent huit sprites MD de 16 x 16 par image. Les 320 positions sont
conservees; seules 14 images distinctes de sprites sont necessaires.

Simon commence a x=231 dans les attributs affiches, puis s'arrete a x=119.
Les positions sont extraites de l'execution originale, sans trajectoire
dessinee a la main. La duree d'un tick suit la cadence video de la MD.
Le logo utilise un fondu MD adapte sur 120 images au lieu de la copie ligne
par ligne du demarrage MSX.

## La vraie premiere zone : stage 0

Le jalon 3 commencait directement dans le hall, stage 1, salle 0. L'execution
de l'initialisation de partie montre que D000 vaut d'abord 0. Le depart est
x=16,y=176 et les trois ecrans exterieurs precedent le hall.

Leurs connexions sont obtenues via les routines de chargement :

| Ecran de la cour | Gauche | Droite |
| --- | --- | --- |
| 0 | FF, aucune sortie | 1 |
| 1 | 0 | 2 |
| 2 | 1 | FF, aucune sortie ordinaire |

Le chargement de jeu modifie la carte apres son expansion initiale. Pour
ces trois ecrans, l'extracteur conserve donc les fonds et cartes de collision
apres l'execution de 422C et de ses sous-routines. Une simple expansion de
4F83 ne suffit pas a restituer toutes ces modifications.

L'exterieur est affichable et parcourable dans la version MD. L'activation
de la porte avec HAUT, dans le dernier ecran et a x>=192, est une adaptation
du prototype. Elle charge le hall avec le HUD du stage 1. Les limites de
salles et changements d'etat ne constituent pas encore une traduction
complete du gestionnaire original.

Les flammes visibles dans les fonds sont pour le moment figees et sans
collision offensive. Les objets, ennemis, attaques et effets sonores restent
a porter.

## Verification

`verify_intro.py` controle les etats en RAM de la ROM MD et pilote tout le
parcours depuis le titre jusqu'au hall. Il compare les pixels du logo, du
titre (hors invite adaptee), de trois images animees devant la grille, de la
cour de depart, de la porte et du hall avec leurs references reconstruites.

Les tests precedents restent distincts : 152 fonds de l'explorateur,
153 cartes developpees par la routine originale et 53 872 comparaisons de
primitives C/Z80. Ce dernier nombre inclut maintenant les cartes modifiees
des trois ecrans exterieurs.
