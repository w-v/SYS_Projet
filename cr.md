ADILI Robin et TOMAS Pablo

Compte Rendu du Projet du module de Systèmes du 23/03/2018

Notre programme serveur est ecrit dans audioserver.c et
notre programme client est ecrit dans audioguest.c.
Ces deux programmes ont des fonctions en communs. 
recv_packet() et send_packet() permettent 
respectivement, d'envoyer un packet et de vérifier
que la fonction sendto() a bien suivie correctement
la procédure d'envoi, de recevoir un packet et de
vérifier que la fonction recvfrom() a bien suivie
correctement la procédure de reception. Ces deux
fontions ont été ecrites dans le fichier socketlib.c
(et leur prototype est dans le fichier header
du même nom). Dans le fichier socketlib.h, nous avons
également implémenté une structure dest_info. Celle ci
nous permet de transmettre des informatons utiles
(via ses attributs) aux fonctions sendto() et
recvfrom(). Cette structure permet au main() du
serveur et du client d'être plus lisible en evitant
d'appeler des fonctions contenant de longues séries
de paramètres La structure audio_packet y est
également présente. Elle est utisee pour envoyer
des donnees avec leur identifiant. Dans le fichier
audioserver.c, la fonction socket_server_init()
permet dans un premier temps d'initialiser la 
structure dest_info contenant les informations du 
client, et dans un second temps, de donner une
adresse a la socket du serveur. Dans le fichier
audioguest.c, la fonction socket_guest_init() est un
equivalent de la fonction socket_server_init().
Celle ci permet d'initialiser la structure dest_info
contenant les informations du serveur. La fonction
req_until_ack permet, via l'inclusion du module
<sys/time.h> et de la fonction select, d'instaurer
un timer pour la fonction recvfrom() (présente dans
la fonction recv_packet()). Ainsi, le programme qui
l'utilise n'est plus bloqué lorsqu celui ci est 
dans l'attente d'un paquet (qu'il risque de ne 
jamais recevoir). 

Le protocole suivi par notre programme est le suivant:
1) (Serveur) Attente d'une connexion d'un client.

2) (Client) Envoi d'une requete contenant le nom du 
fichier qu'il souhaite lire.

3) (Serveur) Verification de la présence du fichier
demandee.

Scenario 1: Absence du fichier

4.1) (Serveur) Renvoi d'un code d'erreur et attente d'une
nouvelle requete d'un client.

5.1) (Client) Reception du code d'erreur et arret en 
affichant un message d'erreur.

Scenario 2: Presence du fichier

4.2) (Serveur) Lecture de l'en-tete du fichier et envoie
des informations necessaires au client pour le lire.

5.2) (Client) Reception des informations envoyees et 
initialisation des hauts parleurs.

6) (Client) Demande l'envoi du premier paquet.

  Eventualite : Non reception de la requete par le serveur.

  6bis) (Client) Mise en place du timer. Le serveur n'ayant
  pas recu de requete, n'envoie pas le paquet demande. Le
  temps du timer est ecoule donc renvoi de la requete par
  le client.

7) (Serveur) Lecture et envoi du paquet demande.

  Eventualite : Non reception de l'envoi du paquet

  7bis) (Client) Mise en place du timer. Le serveur n'ayant
  pas recu la requete suivante, il ne se passe rien. Le
  temps du timer est ecoule donc renvoi de la requete par
  le client. Retour au 6).

8) (Client) Reception et ecriture dans les hauts parleurs.
Requete du paquet suivant au serveur. Retour au 6) pour
le paquet suivant jusqu'au dernier paquet.

9) (Serveur) Fin du fichier audio. Lecture terminee.
Envoi du code de fin. Retour en 1).

10) (Client) Reception du code de fin. Arret du programme.
