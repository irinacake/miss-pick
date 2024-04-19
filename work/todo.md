- récupérer pour chaque block tous les états possibles // ou plutôt pour chaque edge
- qui vire qui ?
- pour chaque BB, il faut récupérer tous les retours de la fonction cache->block : le block de cache peut varier au sein d'une même BB


- get the first instruction of a cache block : calling updateLRU for the first inst of a BB may erronously assume that this first instruction of the BB is the first instruction of a cache block
- don't use instructions for the property, use BB

- in order to use BBs, every time a new BB is reached, check every instruction and update if a new tag is seen
- stat: nombre d'états min/max/moy par ensemble selon les BB
- stat: nombre d'états min/max/moy par ensemble global
- ajouter des politiques
- supprimer les ensembles non utilisés dans les stats
- mettre en place un algo en working list
- refractorisation du code, séparation en plusieurs fichiers ?

- reprendre avec le par ensemble

- tester sur plusieurs bench :
 - bench dans kernel
 - bench dans sequencial (utiliser cuttlebench de M1?)


- timeout exception needs to be handled
- refractor update classes, heritage (est-ce vraiment nécessaire ?)

- mise en forme à rendre otawa-like




<- Done
Todo ->

- benchall : traiter le résultat de l'exécution de chaque bench dans le python pour pouvoir réutiliser le même fichier à chaque fois

- mettre un timer dans le cpp et pas dans le python 

- mettre des asserts pour essayer de détecter des erreurs

- faire usage de propriétés pour noter les boucles et les esquiver lors de la working list par ensemble

- traiter les duplicas de paires (BB,cachestate) dans la liste TODO 

- traiter les duplicas de States* 

- analyse initiale supplémentaire : "projection du graphe par ensemble"

- récupérer des informations sur les échecs de timer (progression ?)

- nombre total d'état : il faut exclure les duplicats mais vérifier tous les saved[]

- gérer le cas de l'explosion de la complexité avec LRU pour petrinets

- compter les miss

- définir un ordre pour l'usage d'arbre à la place de liste : doit prendre en compte les tags mais aussi les index (ex: en fifo)

- ajouter un 3e élément dans le tuple : `LockPtr` qui sert à simuler une pile d'appel / sous-appel. En cas d'arrivée sur SynthBlock : add avec un push, en cas d'arrivée sur un exit() : récupérer avec un pop() 

- --cfg-virtualize sur tous 