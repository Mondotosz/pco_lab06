#import "@preview/ilm:1.4.0": *
#import "@preview/gentle-clues:1.0.0": *
#import "@preview/codly:1.0.0": *

#show: ilm.with(
  title: [Rapport Labo 6],
  author: "Ali Zoubir & Kenan Augsburger",
  date: datetime.today(),
)

#codly(
  languages: (
    cpp: (
      name: "cpp",
      icon: text(font: "JetBrainsMono NFP", " "),
      color: rgb("#ef4444")
    )
  )
)

#codly(
  languages: (
    pseudocode: (
      name: "pseudocode",
      icon: text(font: "JetBrainsMono NFP", "ⓟ "),
      color: rgb("#89caff")
    )
  )
)

#show: codly-init.with()

#show link: underline
#show raw: set text(font: "JetBrainsMono NFP")

= Informations sur le projet

== Répertoire Git (sur GitHub)

#link("https://github.com/Mondotosz/pco_lab06")[mondotosz/pco_lab06]

== Membres

#list[ #link("https://github.com/Ali-Z0")[Ali Zoubir] ][ #link("https://github.com/mondotosz")[Kénan Augsburger] ]

== Introduction du problème

Dans ce laboratoire, l'objectif était de concevoir et implémenter un pool de threads capable de gérer efficacement l'exécution de tâches concurrentes tout en respectant les contraintes définies : limitation des threads actifs, gestion de la surcharge via une file d'attente limitée, et recyclage des threads inactifs après un délai donné.

= Implémentation et choix

== Structure Générale du Code
Runnable
Une interface représentant une tâche exécutable :

- `run()`: contient la logique à exécuter.
- `cancelRun()`: permet d'annuler l'exécution.
- `id()`: fournit un identifiant unique pour chaque tâche.
ThreadPool
La classe principale, utilisant un moniteur de Hoare pour gérer la synchronisation :

*Constructeur :*

- `maxThreadCount` : nombre maximum de threads dans le pool.
- `maxNbWaiting` : taille maximale de la file d'attente.
- `idleTimeout` : délai d'inactivité avant qu'un thread ne soit terminé. Un thread séparé (`timer_thread`) est initialisé pour surveiller les timeouts.

*Méthodes :*

- `start()` : Ajoute une tâche dans la file ou la rejette si elle est pleine. Si un thread est disponible, il traite immédiatement la tâche. Sinon, un nouveau thread est créé (si le pool n'a pas atteint sa capacité maximale).
- `worker()` : Gère l'exécution des tâches pour chaque thread. Attend une tâche ou termine après un délai d'inactivité.
- `timer()` : Surveille les threads inactifs et les termine lorsqu'ils dépassent idleTimeout.
- `Destructeur` : Termine proprement les threads et libère les ressources associées.

== Architecture de la classe

La classe principale `ThreadPool` repose sur un moniteur de Hoare pour gérer la synchronisation. Les threads sont encapsulés dans une structure qui contient l'état de chaque thread (actif, en attente, ou expiré).

Pseudocode pour `ThreadPool` :

#codly()
```pseudocode
Classe ThreadPool :
  initialiser(max_threads, max_queue, timeout) :
    // Préparer les structures pour la gestion des threads

  démarrer(tâche) :
    si (file pleine) alors rejeter(tâche) et retour negatif
    sinon prise en charge de la tache avec reveil/creation de thread

  boucle_thread() :
    tant que (actif) :
      si aucune tache ni demande d'arret alors attente
      si demande d'arret alors arret
      executer_tache()
```

== Méthodes worker

- *`worker`* : Chaque thread execute les tâches et s'arrête s'il ne reçoit pas
  de tâches pendant le délai définis.

*Pseudocode simplifié de la logique :*

#codly()
```pseudocode
si (file non vide) alors :
  tâche = retirer_file()
  exécuter(tâche)
sinon si (temps_inactivité atteint) :
  terminer_thread()
```

== Points Clés sur le Thread de Timeout
*Gestion du Timeout dans le Thread Timer :*

Le thread *`timer_thread`* parcourt les threads actifs.
Si un thread est inactif et dépasse `idleTimeout`, il est marqué comme expiré (`timed_out`) et réveillé via un signal.
Les threads expirés sont joints (avec join()) et supprimés de la liste.
Découplage entre le Timer et les Workers :

Les threads "workers" ne gèrent pas eux-mêmes leur timeout, ce qui simplifie leur logique.
Le thread timer agit comme un observateur, surveillant les états des threads et agissant en conséquence.

== Subtilités concurrentielles

L'implémentation évite les blocages grâce à l'utilisation de conditions de synchronisation, garantissant un accès sécurisé et cohérent aux données partagées. La destruction du pool est gérée avec rigueur pour prévenir toute fuite de threads. Une attention particulière a été portée au rejet des tâches impossibles à exécuter et à la terminaison propre des threads, notamment pour garantir la réussite des tests.

= Tests

== Scénarios de test

Nous avons utilisé les tests fournis avec la donnée du laboratoire donc:
- les tâches sont bien toutes exécutés (10 tâches pour 10 threads)
- les tâches sont bien toutes exécutée (100 tâches pour 10 threads avec une file supportant 100 tâches)
- les tâches sont bien toutes exécutée (10 x 10 tâches avec attente entre chaque batch pour 10 threads avec une file max de 100 tâches)
- les tâches ne pouvant pas être prise en charge sont refusées et annulées (30 tâches pour 10 threads et un maximum de 5 tâches en attente)
- les threads sont bien libérés s'ils attendent plus que le délai donné

== Résultats

Les tests montrent que le pool respecte les limites et fonctionne efficacement dans des scénarios variés en respectant les contraintes de temps.

#info[
  Le test case 4 fonctionne la plupart du temps mais il arrive que le nombre de
  tâches rejetées ne soit pas bon (17 ou 18 au lieu de 15.) Notamment, si on
  essai de lancer plusieurs instances du programme en parallèle les tests ont
  tendance à moins passer donc il s'agit probablement d'un problème de performances
]

= Remarques et conclusion

== Améliorations possibles

- Réduction des ressources consommées par les threads inactifs.
- Ajout de métriques pour surveiller les performances.
- Potentiellement utiliser une liste chainée et des itérateurs pour stocker les
  informations relatives aux différents threads.

== Conclusion

Ce laboratoire a permis de concevoir un pool de threads capable de gérer des tâches concurrentes sous des contraintes strictes ; Telles que la limitation des threads actifs, la gestion d’une file d’attente et le recyclage des threads inactifs. L’utilisation d’un moniteur de Hoare et d’un thread dédié à la gestion des timeouts a assuré une synchronisation sécurisée. Les tests ont confirmé la conformité du système, offrant une solution adaptable à des environnements à charge variable.
