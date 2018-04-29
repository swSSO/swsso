Exécuter les 3 scripts dans cet ordre :

1) creation_table_configs_domains.sql
--> Crée la nouvelle table configs_domains (vide)

2) initialisation_table_configs_domains.sql
--> Initialise la table configs_domains en récupérant l'ensemble des configurations dans la table config et en les liant à l'identifiant de domaine indiqué dans la colonne domainId de la table config

3) modification_table_config.sql 
--> Supprime la colonne domainId de la table config et ajoute la nouvelle colonne autoPublish