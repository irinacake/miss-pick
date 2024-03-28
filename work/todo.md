- récupérer pour chaque block tous les états possibles // ou plutôt pour chaque edge
- qui vire qui ?
- pour chaque BB, il faut récupérer tous les retours de la fonction cache->block : le block de cache peut varier au sein d'une même BB



- get the first instruction of a cache block : calling updateLRU for the first inst of a BB may erronously assume that this first instruction of the BB is the first instruction of a cache block