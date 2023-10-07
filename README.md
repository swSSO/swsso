## swSSO, qu'est-ce que c'est ?
swSSO est un logiciel de E-SSO Open Source (ou un gestionnaire de mots de passe, si vous préférez) : il mémorise dans un fichier sécurisé tous vos identifiants et mots de passe et remplit automatiquement les formulaires de connexion à votre place !
Il est compatible avec Windows XP, Vista, 7, 8, 10 et 11 et supporte les navigateurs Chrome, Firefox, Internet Explorer et Edge ainsi que les applications de type client lourd.
Il est distribué sous licence GNU GPL v3.

## Première utilisation
Téléchargez la version portable de swSSO et enregistrez l'exécutable n’importe où sur votre disque (ou sur une clé USB pour emporter vos mots de passe partout avec vous !).
Double-cliquez sur swSSO.exe :

![](https://github.com/swSSO/swsso/assets/11473235/95666105-fc22-4cfa-bca0-ab40c0043409)

Comprenez bien que le mot de passe que vous allez choisir constitue l’unique clé d’accès à l’ensemble de vos autres mots de passe : **ne le perdez surtout pas, il n’y a aucun moyen de le retrouver !**
Ce mot de passe vous sera demandé à chaque lancement de swSSO, sauf si vous cochez la case « Se souvenir du mot de passe…. ».  Dans ce cas, tant que vous utilisez swSSO sur le même ordinateur, le mot de passe ne vous est plus jamais demandé.

Validez en cliquant sur OK, une icône SSO apparaît dans la barre des tâches. Un clic-droit sur cette icône donne accès à l’ensemble des fonctionnalités de swSSO. 

Ouvrez votre navigateur et rendez-vous sur la page de connexion du site web ou de l'application que vous souhaitez automatiser. Laissez bien votre navigateur ouvert en premier plan, faîtes clic-droit sur l’icône swSSO et choisissez le menu « Ajouter cette application… ».

La fenêtre s'ouvre :

Saisissez ce que vous voulez dans le champ identifiant et dans le champ mot de passe, puis validez en cliquant sur le bouton OK. Ne modifiez pas les autres champs de cette fenêtre pour le moment.

Vous avez vu ? L’identifiant et le mot de passe que vous venez de taper sont saisis par swSSO dans le formulaire de connexion : votre première configuration est terminée ! Si vous n’avez pas vu swSSO remplir le formulaire, refermez votre navigateur complètement (pas seulement l’onglet concerné) et ouvrez-le à nouveau sur la page de connexion : swSSO saisit votre identifiant et votre mot de passe à votre place !

Remarque importante pour les utilisateurs de navigateurs basés sur Chromium (Chrome, Opera, Edge...) : vous devez changer la ligne de commande de lancement de votre navigateur pour ajouter --force-renderer-accessibility=form-controls

##


## swSSO
swSSO is an E-SSO (Enterprise Single Sign-On) software: it stores all your user names and passwords in a secure file and automatically fills in login forms.
It runs on Windows XP, Vista, 7, 8, 10 and 11 and is compatible with both web sites (Google Chrome, Firefox, Internet Explorer and Edge) and client-side applications.
swSSO is a free software: you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, either version 3 of the License, or (at your option) any later version. swSSO is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for more details.

Note for Chromium based browsers (Chrome, Opera, Edge...): please add --force-renderer-accessibility=form-controls command line parameter.
