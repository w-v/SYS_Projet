LES FILTRES:

MONO/STEREO:

Pour permettre au programme client de lire un fichier audio en mono, c'est celui
ci qui envoie cette requ�te au serveur en m�me temps que le nom du
fichier demand�
Le serveur va alors envoyer au programme client les paquets correspondant a sa
requete. Pour ce faire, il va proceder de la mani�re suivante :
Sur un paquet le programme serveur va trier les samples et selectionner uniquement
ceux correspondant au channel gauche. Ces  512 premiers samples vont ensuite
�tre plac�s dans un buffer. Lors de la lecture du prochain paquet par le serveur, le
buffer sera alors compl�t� par 512 autres samples du channel gauche. Le buffer sera
alors envoy� par le serveur au client pour �tre lu. Donc pour un paquet envoy� en
mono, deux paquets sont lus en st�r�o.

VOLUME SONORE:

Ce filtre est execut� par le programme client, au moment de la lecture des paquets.
A chaque envoi de paquet, celui ci est d�compos� pour qu'il lui soit appliqu� une
formule. L'utilisateur peut, via les touches "up" et "down" (quand le curseur est sur
la barre de volume), choisir de modifier la valeur (en decibel) du volume de la piste
�cout�e. Lorsque celui ci d�cide d'augmenter le volume, la formule, appliqu�e aux 
bytes du paquet, les multiplie lin�airement. Le m�me raisonnement est semblable 
quand il s'agit de baisser le volume. Si le volume sonore est trop �lev�, une limite a
�t� introduite pour ne pas la franchir (donc le son clip).

EGALISEUR/VISUALISEUR:

Le filtre �galiseur permet de modifier le volume sonore de certains intervalles de
fr�quence du paquet �cout�. L'�galiseur est compos� de 10 filtres qui traitent chacun
une certaine partie du spectre audio. Tous les filtres sont appliqu�s en parall�le.
Ils sont appliqu�s individuellement au signal original puis somm�s entre eux, le
r�sultat est divis� par le nombre de filtres. Les formules des filtres ont �t� trouv�
sur Internet. Pour les d�tails concernant celles ci, voir REFERENCES.
Des tableaux pour enregistrer les derni�res valeurs modifi�es par chaque filtre
sont necessaires � leur application. Lors du passage d'un paquet � un autre, les
derni�res valeurs originales et modifi�es du paquet pr�cedent doivent �tre sauvegard�es.
Le type integer signed sur 16 bits sert pour l'�criture des paquets pour �tre jou�.
Le type float est necessaire pour l'application des filtres. Il a fallu gerer les
conversions de l'un � l'autre pour r�aliser cet egaliseur.

Le visualiseur et l'interface ont �t� ralis� avec les librairies FFTW3 et NCURSES
respectivement. Les touches suivantes permettent de realiser les actions suivantes:
-pour naviguer : 'Right' et 'Left'
-pour modifier le volume ou le gain de la bande de l'�galiseur sur laquelle se trouve le
 curseur : 'Up' et 'Down'
-pour masquer/afficher l'interface de l'�galiseur: 'E'
-pour masquer/afficher le volume: 'V'
-pour activer/desactiver l'egaliseur: 'B'
-pour modifier le volume (peu importe o� est le curseur) : '+' et '-'
-pour quitter: 'Q'

REFERENCES

[Niveau (audio) - Wikip�dia](https://fr.wikipedia.org/wiki/Niveau_(audio))
[All About Audio Equalization](www.mdpi.com/2076-3417/6/5/129/pdf)
[Audio EQ CookBook C Exemple](https://ethanwiner.com/eq-dsp.htm)
[Audio Filter - Wikip�dia](https://en.wikipedia.org/wiki/Audio_filter)
[Audio EQ CookBook](http://shepazu.github.io/Audio-EQ-Cookbook/audio-eq-cookbook.html)
[Audio EQ CookBook](http://shepazu.github.io/Audio-EQ-Cookbook/audio-eq-cookbook.html)
