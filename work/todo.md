- récupérer pour chaque block tous les états possibles // ou plutôt pour chaque edge
- qui vire qui ?
- pour chaque BB, il faut récupérer tous les retours de la fonction cache->block : le block de cache peut varier au sein d'une même BB



- get the first instruction of a cache block : calling updateLRU for the first inst of a BB may erronously assume that this first instruction of the BB is the first instruction of a cache block



- don't use instructions for the property, use BB
- in order to use BBs, every time a new BB is reached, check every instruction and update if a new tag is seen



- stat: nombre d'états min/max/moy par ensemble selon les BB
- stat: nombre d'états min/max/moy par ensemble global
- ajouter des politiques