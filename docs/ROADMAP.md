# Mode rond interactif — demande du 31 octobre 2026

## Comportement demandé

1. Toutes les fenêtres principales utilisent un canevas carré commun et fixe. Le rendu
   doit disposer d’assez de pixels avant déformation pour limiter le flou. Les dialogues,
   menus, sous-surfaces et applications non redimensionnables nécessitent un traitement explicite.
2. Réduire, basculer carré/rond et fermer sont accessibles dans la barre supérieure,
   à droite de la luminosité. Le bouton d’agrandissement devient le basculement de projection.
3. Le choix carré/rond est mémorisé par identifiant d’application et restauré au lancement
   et après redémarrage. Déplacer/redimensionner librement les fenêtres n’est plus le mode d’usage.
4. La même transformation géométrique et son inverse doivent être utilisées pour l’image,
   la souris et le tactile : viser un contrôle doit activer ce contrôle, y compris dans les menus.
5. Un bouton près de Démarrer et `Super+Tab` ouvrent une vue de toutes les fenêtres :
   placement de bulles sans chevauchement, fenêtres ouvertes plus grandes que les réduites,
   applications en mode carré représentées par des carrés aux coins arrondis.
6. Dans cette vue, glisser une fenêtre à gauche/droite l’installe dans un demi-disque ;
   les fenêtres restantes apparaissent dans l’autre moitié pour choisir la seconde application.
7. Le mode doit pouvoir être désactivé pour retrouver la configuration sûre précédente.

## Base retenue

Maillage M5 : 49 × 49 sommets, 48 × 48 faces, surface des cellules et étirement
régularisés avec lissage de la courbure. Les fichiers `projection/approved-mesh.*`
sont la référence. Une densité de texels homogène ne garantit pas seule la lisibilité :
il faut aussi limiter l’anisotropie et les changements brusques de direction.

## Travail technique en cours

- Extension native KWin envisagée : déformation des fenêtres et filtre d’entrées.
- Versions inspectées : KWin 6.8.1 et 6.8.2. Les extensions binaires nécessitent une
  compilation pour la version exacte de KWin installée.
- Essais dans un compositeur isolé avant toute activation sur la session principale.
- La compatibilité universelle n’est pas acquise : tester explicitement Wayland,
  XWayland, décorations côté client, popups, dialogues, glisser-déposer, souris et multitouch.

## Critères de validation

- Même transformation pour les triangles affichés et les coordonnées d’interaction ;
  tests aller-retour sur la grille et aux frontières.
- Aucune face inversée, aucun trou ; disposition des bulles sans chevauchement.
- Aucune intervention du filtre avant les protections du verrouillage de session.
- Réglages persistants, déchargement propre et restauration des fenêtres/panneaux.
- Mesures de lisibilité, de fréquence de rendu et de consommation sur l’écran cible.
- Vérification réelle des entrées tactiles quand le matériel est disponible.
