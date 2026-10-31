# Linux Rond

Interface Linux pour écrans circulaires tactiles, basée sur **KDE Plasma 6 / KWin**.
Projet autonome, indépendant de toute collection personnelle de dotfiles.

## Disponible et validé visuellement

- Barres en arc : icônes orientées, espacement régulier et indicateurs d’état courbes.
- Fenêtres maintenues dans le carré visible de l’écran rond (script KWin).
- Maillage carré → disque équilibré et lissé, validé sur des simulations :
  [données JSON](projection/approved-mesh.json), [OBJ avec UV](projection/approved-mesh.obj).

**La projection n’est pas encore un effet KWin utilisable.** Son rendu et la transformation
inverse des entrées doivent être intégrés ensemble. La configuration actuelle conserve
les applications dans la zone carrée sûre. Les captures personnelles ayant servi aux essais
ne sont pas distribuées.

## Installation utilisateur

Prérequis : session Plasma 6, `kpackagetool6`, `kwriteconfig6`, `qdbus6`, Python 3,
`kscreen-doctor`, et service utilisateur `plasma-plasmashell.service`.
Aucun paquet système n’est installé par ces scripts.

```sh
git clone https://github.com/softyoda/linux-rond.git
cd linux-rond
./scripts/install.sh
# Facultatif : remplacer les panneaux de l’unique sortie carrée par les barres en arc.
./scripts/configure-panels.sh
```

La première commande installe le conteneur et la zone sûre dans le compte courant,
puis active le script KWin. La seconde **remplace les panneaux de la sortie carrée**,
sauvegarde la disposition et relance Plasma. Les autres sorties ne sont pas modifiées.
En cas de plusieurs sorties carrées, elle refuse de choisir arbitrairement.

L’échelle de l’écran se règle dans Configuration du système → Affichage.
Le projet ne force pas une échelle ni un nom de sortie propre à une machine.

Aperçu d’une capture locale (Pillow requis) : `python3 tools/preview.py capture.png aperçu.png`.

Vérification : `./scripts/check.sh`. Pour restaurer les panneaux, passer le chemin
indiqué lors de la configuration à `./scripts/restore-panels.sh /chemin/sauvegarde`.
Cela restaure la disposition Plasma complète sauvegardée et désactive la zone sûre.

## Suite du projet

Voir [la feuille de route](docs/ROADMAP.md) pour le mode rond par application,
les commandes dans la barre, les entrées corrigées, l’aperçu en bulles et le partage
horizontal de l’écran. Ce sont des objectifs de développement, pas des fonctions déjà livrées.

Le socle provient du module `round-screen` de `softyoda/dotfiles` (révision
`84db998`, contenant la barre validée initialement dans `34c32c5`). Les dotfiles
référencent désormais ce dépôt à une révision immuable.

## Contribuer

Les essais doivent préciser la distribution, les versions de Plasma/KWin,
le modèle/résolution de l’écran et les entrées testées. Le socle est spécifique à Plasma ;
la compatibilité avec GNOME ou d’autres compositeurs n’est pas implémentée.

```sh
./scripts/check-source.sh
```

Licence : GPL-2.0-or-later. Les dépendances de KDE conservent leurs licences respectives.
