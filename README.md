## swSSO, qu'est-ce que c'est ?
swSSO est un logiciel de E-SSO Open Source (ou un gestionnaire de mots de passe, si vous préférez) : il mémorise dans un fichier sécurisé tous vos identifiants et mots de passe et remplit automatiquement les formulaires de connexion à votre place !
Il est compatible avec Windows XP, Vista, 7, 8, 10 et 11 et supporte les navigateurs Chrome, Firefox, Internet Explorer et Edge ainsi que les applications de type client lourd.
Il est distribué sous licence GNU GPL v3.

## En route vers la fin du cauchemar des mots de passe !
Téléchargez la [version portable de swSSO](https://github.com/swSSO/swsso/releases/latest)
et enregistrez l'exécutable n’importe où sur votre disque (ou sur une clé USB pour emporter vos mots de passe partout avec vous !).
Double-cliquez sur swSSO.exe et choisissez votre mot de passe "maître" :

![1er lancement](https://github.com/swSSO/swsso/assets/11473235/95666105-fc22-4cfa-bca0-ab40c0043409)

> **Comprenez bien que ce mot de passe constitue l’unique clé d’accès à l’ensemble de vos autres mots de passe : ne le perdez surtout pas, il n’y a aucun moyen de le récupérer !**

Ce mot de passe vous sera demandé à chaque lancement de swSSO, sauf si vous cochez la case « Se souvenir du mot de passe…. ».  Dans ce cas, tant que vous utilisez swSSO sur le même ordinateur, le mot de passe ne vous est plus jamais demandé.

Validez en cliquant sur OK, une icône SSO apparaît dans la barre des tâches. Un clic-droit sur cette icône donne accès à l’ensemble des fonctionnalités de swSSO. 

> **Remarque importante pour les utilisateurs de navigateurs basés sur Chromium (Chrome, Opera, Edge...)** : vous devez changer la ligne de commande de lancement de votre navigateur pour ajouter --force-renderer-accessibility=form-controls

Ouvrez votre navigateur et rendez-vous sur la page de connexion du site web ou de l'application que vous souhaitez automatiser. Laissez bien votre navigateur ouvert en premier plan, faîtes clic-droit sur l’icône swSSO et choisissez le menu « Ajouter cette application… ». Cette fenêtre s'ouvre :

![Configuration](https://github.com/swSSO/swsso/assets/11473235/f6eaa411-4abc-49b8-b7f8-8c8f40eb15d2)

Saisissez votre identifiant dans le champ identifiant et le mot de passe dans le champ mot de passe, puis validez en cliquant sur le bouton OK. Ne modifiez pas les autres champs de cette fenêtre pour le moment.

Vous avez vu ? L’identifiant et le mot de passe que vous venez de taper sont saisis par swSSO dans le formulaire de connexion : votre première configuration est terminée :tada:

Si vous n’avez pas vu swSSO remplir le formulaire, refermez votre navigateur complètement (pas seulement l’onglet concerné) et ouvrez-le à nouveau sur la page de connexion : swSSO saisit votre identifiant et votre mot de passe à votre place !

## Pour aller plus loin 

Vous en savez maintenant assez pour commencer à configurer vos Web favoris : il vous suffit de répéter à chaque fois la procédure présentée précédemment ! Les champs pré-renseignés conviendront dans 90% des cas. Pour les sites Web plus complexes ou pour les applications Windows, veuillez vous référer au manuel utilisateur (disponible [dans la section Release](https://github.com/swSSO/swsso/releases/latest). 
Voici quelques premières explications très simples sur les paramètres de l’onglet configuration :
- Champ identifiant : cette valeur représente la position relative du champ identifiant par rapport au champ mot de passe. Sur la page de démonstration et sur la grande majorité des sites Web, le champ identifiant est positionné juste avant le champ mot de passe. La valeur à saisir est donc -1. Si un autre champ de saisie est intercalé entre l’identifiant et le mot de passe, mettez cette valeur à -2. C’est aussi simple que ça !
- Champ mot de passe : cette valeur indique le rang du champ mot de passe à remplir s’il y a plusieurs champs mot de passe dans la page. Dans 99% des cas, il n’y a qu’un seul champ de type mot de passe et vous pourrez laisser la valeur 1, qui signifie que le champ à renseigner est le 1er champ mot de passe de la page. Si c’est le 2nd champ mot de passe qui vous intéresse, mettez cette valeur à 2.
- Validation : la valeur [ENTER] indique que swSSO doit frapper la touche ENTER après avoir saisi l’identifiant et le mot de passe. Si vous constatez que pour valider la saisie du formulaire il faut d’abord taper sur la touche TAB pour sélectionner le bouton de validation puis taper ENTER, saisissez [TAB][ENTER].

## Et sur mon smartphone, je fais comment ?

Grace à swSSO Mobile, vous accédez également à vos mots de passe depuis votre smartphone !
Reportez-vous à la [documentation](https://swsso.000webhostapp.com/doc/swSSOMobile.pdf) et suivez la procédure !

> **Les URLs indiquées dans le document ne sont plus bonnees :** remplacez https://www.swsso.fr par https://swsso.000webhostapp.com et https://www.swsso.fr/mobile par https://swsso.000webhostapp.com/mobile

## Version entreprise

La version Entreprise de swSSO apporte de nombreux compléments indispensables pour l’utilisation de swSSO en entreprise :
- Un serveur de configuration, à héberger par vos soins (technologie PHP/MySQL) ;
- La possibilité de coupler swSSO à l’authentification Windows ;
- Un outil de déblocage en cas de perte du mot de passe maître ou de désynchronisation du mot de passe Windows ;
- La possibilité de définir une politique de sécurité pour le mot de passe maître : longueur minimale, composition (nombre minimal de minuscules, majuscules, chiffres et caractères spéciaux), fréquence de renouvellement...
- La possibilité de brider des interfaces de configuration du client, afin de limiter les droits des utilisateurs ;

Tous les modules ainsi que le guide d’administration sont disponibles [dans la section Release](https://github.com/swSSO/swsso/releases/latest). 

Et tout comme la version grand public, la version Entreprise est gratuite et distribuée sous licence GPL. Si vous avez un projet et que vous avez besoin d'aide, n'hésitez pas à me contacter !

## About swSSO
swSSO is an E-SSO (Enterprise Single Sign-On) / password manager software: it stores all your user names and passwords in a secure file and automatically fills in login forms.
It runs on Windows XP, Vista, 7, 8, 10 and 11 and is compatible with both web sites (Google Chrome, Firefox, Internet Explorer and Edge) and client-side applications.
swSSO is a free software: you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, either version 3 of the License, or (at your option) any later version. swSSO is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for more details.

> **Note for Chromium based browsers (Chrome, Opera, Edge...):** please add --force-renderer-accessibility=form-controls command line parameter.
