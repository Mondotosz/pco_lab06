#import "@preview/ilm:1.4.0": * #import "@preview/gentle-clues:1.0.0": * #import "@preview/codly:1.0.0": *

#show: ilm.with( title: [Rapport Labo 6], author: "Ali Zoubir & Kenan Augsburger", date: datetime.today(), )

#codly( languages: ( cpp: ( name: "cpp", icon: text(font: "JetBrainsMono NFP", " "), color: rgb("#ef4444") ) ) )

#show: codly-init.with()

#show link: underline #show raw: set text(font: "JetBrainsMono NFP")

= Informations sur le projet

== Répertoire Git (sur GitHub)

#link("https://github.com/Mondotosz/pco_lab06")[mondotosz/pco_lab06]

== Membres

#list[ #link("https://github.com/Ali-Z0")[Ali Zoubir] ][ #link("https://github.com/mondotosz")[Kénan Augsburger] ]

== Introduction du problème

Dans ce laboratoire, l'objectif était de concevoir et implémenter un pool de threads capable de gérer efficacement l'exécution de tâches concurrentes tout en respectant les contraintes définies : limitation des threads actifs, gestion de la surcharge via une file d'attente limitée, et recyclage des threads inactifs après un délai donné.

= Implémentation et choix

== Architecture des classes

La classe principale ThreadPool repose sur l'utilisation d'un moniteur de Hoare pour gérer la synchronisation et la communication entre les threads. Les threads sont encapsulés dans une structure worker_t comprenant des informations comme l'état d'attente, les conditions de synchronisation, et le temps d'expiration.

#codly(languages: cpp)
```cpp
#include <thread> 
#include <queue> ... 

class ThreadPool : public PcoHoareMonitor { public: ThreadPool(int maxThreadCount, int maxNbWaiting, std::chrono::milliseconds idleTimeout); bool start(std::unique_ptr<Runnable> runnable); size_t currentNbThreads(); ... }; ]

```

 == Méthodes clés

Méthode start
Cette méthode gère l'ajout des tâches. Si un thread est disponible, il traite la tâche immédiatement. Sinon, un nouveau thread est créé jusqu'à atteindre la limite maximale.

Timer et gestion des threads inactifs
Un thread dédié surveille les threads inactifs et les termine après un délai d'inactivité spécifié.

= Tests

Le fichier de test, tst_threadpool.cpp, fournit cinq scénarios couvrant différents cas d'utilisation, notamment la gestion des tâches simultanées, des files d'attente pleines, et l'expiration des threads inactifs.

= Remarques et conclusion

== Remarques

La gestion des threads inactifs pourrait être optimisée pour réduire la consommation de ressources.
Ajouter des scénarios de tests plus complexes pour des charges irrégulières.

== Conclusion

Ce projet a permis de comprendre les concepts de base des pools de threads et leur implémentation efficace à l'aide des moniteurs de Hoare. L'implémentation a satisfait aux exigences fonctionnelles, avec une gestion robuste des tâches et des ressources.

