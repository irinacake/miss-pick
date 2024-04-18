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


<- Done
Todo ->


- refractor update classes, heritage (est-ce vraiment nécessaire ?)

- mettre des asserts pour essayer de détecter des erreurs

- faire usage de propriétés pour noter les boucles et les esquiver lors de la working list par ensemble

- traiter les duplicas de paires (BB,cachestate) dans la liste TODO 

- traiter les duplicas de States* 

- analyse initiale supplémentaire : "projection du graphe par ensemble"

- mise en forme à rendre otawa-like

- récupérer des informations sur les échecs de timer (progression ?)

- mettre un timer dans le cpp et pas dans le python 

- nombre total d'état : il faut exclure les duplicats mais vérifier tous les saved[]

- gérer le cas de l'explosion de la complexité avec LRU pour petrinets

- compter les miss

- 