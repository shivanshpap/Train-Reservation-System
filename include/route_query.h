#ifndef ROUTE_QUERY_H
#define ROUTE_QUERY_H

#include "graph.h"

/* Prints the shortest path and total distance between two named stations.
 * Returns 0 on success, -1 if either station is unknown, -2 if unreachable. */
int route_query_shortest_path(const Graph *g, const char *source, const char *destination);

/* Prints shortest distances from `source` to every other station in the network. */
int route_query_all_distances(const Graph *g, const char *source);

#endif /* ROUTE_QUERY_H */
