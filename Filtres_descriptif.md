LES FILTRES:

MONO/STEREO:

Pour permettre au programme client de lire un fichier audio en mono, c'est celui
ci qui envoie cette requête au serveur en même temps que le nom du
fichier demandé
Le serveur va alors envoyer au programme client les paquets correspondant a sa
requete. Pour ce faire, il va proceder de la manière suivante :
Sur un paquet le programme serveur va trier les samples et selectionner uniquement
ceux correspondant au channel gauche. Ces  512 premiers samples vont ensuite
être placés dans un buffer. Lors de la lecture du prochain paquet par le serveur, le
buffer sera alors complété par 512 autres samples du channel gauche. Le buffer sera
alors envoyé par le serveur au client pour être lu. Donc pour un paquet envoyé en
mono, deux paquets sont lus en stéréo.

VOLUME SONORE:

Ce filtre est executé par le programme client, au moment de la lecture des paquets.
A chaque envoi de paquet, celui ci est décomposé pour qu'il lui soit appliqué une
formule. L'utilisateur peut, via les touches "up" et "down" (quand le curseur est sur
la barre de volume), choisir de modifier la valeur (en decibel) du volume de la piste
écoutée. Lorsque celui ci décide d'augmenter le volume, la formule, appliquée aux 
bytes du paquet, les multiplie linéairement. Le même raisonnement est semblable 
quand il s'agit de baisser le volume. Si le volume sonore est trop élevé, une limite a
été introduite pour ne pas la franchir (donc le son clip).

EGALISEUR/VISUALISEUR:

Le filtre égaliseur permet de modifier le volume sonore de certains intervalles de
fréquence du paquet écouté. L'égaliseur est composé de 10 filtres qui traitent chacun
une certaine partie du spectre audio. Tous les filtres sont appliqués en parallèle.
Ils sont appliqués individuellement au signal original puis sommés entre eux, le
résultat est divisé par le nombre de filtres. Les formules des filtres ont été trouvé
sur Internet. Pour les détails concernant celles ci, voir REFERENCES.
Des tableaux pour enregistrer les dernières valeurs modifiées par chaque filtre
sont necessaires à leur application. Lors du passage d'un paquet à un autre, les
dernières valeurs originales et modifiées du paquet précedent doivent être sauvegardées.
Le type integer signed sur 16 bits sert pour l'écriture des paquets pour être joué.
Le type float est necessaire pour l'application des filtres. Il a fallu gerer les
conversions de l'un à l'autre pour réaliser cet egaliseur.

Le visualiseur et l'interface ont été ralisé avec les librairies FFTW3 et NCURSES
respectivement. Les touches suivantes permettent de realiser les actions suivantes:
-pour naviguer : 'Right' et 'Left'
-pour modifier le volume ou le gain de la bande de l'égaliseur sur laquelle se trouve le
 curseur : 'Up' et 'Down'
-pour masquer/afficher l'interface de l'égaliseur: 'E'
-pour masquer/afficher le volume: 'V'
-pour activer/desactiver l'egaliseur: 'B'
-pour modifier le volume (peu importe où est le curseur) : '+' et '-'
-pour quitter: 'Q'

REFERENCES

[Niveau (audio) - Wikipédia](https://fr.wikipedia.org/wiki/Niveau_(audio))
[All About Audio Equalization](www.mdpi.com/2076-3417/6/5/129/pdf)
[Audio EQ CookBook C Exemple](https://ethanwiner.com/eq-dsp.htm)
[Audio Filter - Wikipédia](https://en.wikipedia.org/wiki/Audio_filter)
[Audio EQ CookBook](http://shepazu.github.io/Audio-EQ-Cookbook/audio-eq-cookbook.html)