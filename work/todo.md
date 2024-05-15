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

- benchall : traiter le résultat de l'exécution de chaque bench dans le python pour pouvoir réutiliser le même fichier à chaque fois

- nombre total d'état : il faut exclure les duplicats mais vérifier tous les saved[]

- --cfg-virtualize sur tous 



<- Done
Todo ->


- travail sur les EXP :
    - mettre un timer dans le cpp et pas dans le python 
    - teamviewer la tour pour lancer les EXP ?
    - insérer les méthodes de travail dans la classe


- traitement des duplicas : il faut passer des listes aux AVL tree :
    - définir un ordre pour l'usage d'arbre à la place de liste : doit prendre en compte les tags mais aussi les index (ex: en fifo)
    - traiter les duplicas de paires (BB,cachestate) dans la liste TODO 
    - traiter les duplicas de States* 
    - utiliser `#include <elm/avl/Set.h>`





- WL par ensemble : ne pas ajouter d'état aux BB non concernés par l'ensemble :
    - faire usage de propriétés pour noter les boucles et les esquiver lors de la working list par ensemble  # obsolète ?
    - stocker en 3e argument le dernier BB qui a touché à l'ensemble concerné. Il faut vérifier si l'ajout du BB suivant passe par un arc retour, auquel cas on n'ajoute que si le dernier block qui a touché l'ensemble appartient à la boucle courante (= la boucle dont la tête est le BB pointé par l'arc retour)
    - `extended_loop_feature`




- autres :
    - mettre des asserts pour essayer de détecter des erreurs
    - analyse initiale supplémentaire : "projection du graphe par ensemble"
    - récupérer des informations sur les échecs de timer (progression ?)
    - gérer le cas de l'explosion de la complexité avec LRU pour petrinets
    - compter les miss



- Remplace `--cfg-virtualize` :
    - ajouter un 3e élément dans le tuple : `LockPtr` qui sert à simuler une pile d'appel / sous-appel. En cas d'arrivée sur SynthBlock : add avec un push, en cas d'arrivée sur un exit() : récupérer avec un pop() 



- il faudra free les savestates à la fin de l'analyse pour ne conserver que les résultats (pas sûr... à voir)




Todo, ordre de priorité :
- Mettre en place la simulation de la pile  (DONE)

- récupérer le nombre de miss "par l-block" :
 - v1: un bitset pour chaque l-block : quand un l-block A vire un l-block B, le l-block A doit aller mettre à 1 le bit lui correspondant dans le bitset du l-block B (utiliser des bitvectors)
 - v2: par interprétation abstraite
- support de Top "T" : indique l'indéfini. Au départ l'ensemble du cache est rempli avec "T", ce qui signifie que les premières updates génèrent plus de 1 ensemble

- StateSaver : stockage des SetState par AVL Tree
- Mêmoïsation : chaque CacheState est unique et référencé dans une SDD globale, afin d'empêcher la création de duplicas
- Projection de graphe : un graphe par ensemble à traiter

- usage du bottom (⊥)