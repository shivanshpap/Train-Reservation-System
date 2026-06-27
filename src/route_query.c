#include <stdio.h>
#include "route_query.h"

int route_query_shortest_path(const Graph *g, const char *source, const char *destination) {
    int src_idx = graph_find_station(g, source);
    int dest_idx = graph_find_station(g, destination);

    if (src_idx == -1 || dest_idx == -1) {
        printf("Route query failed: unknown station ('%s' or '%s').\n", source, destination);
        return -1;
    }

    PathResult res = graph_dijkstra(g, src_idx);
    if (res.dist[dest_idx] == INF) {
        printf("No route exists from %s to %s.\n", source, destination);
        return -2;
    }

    int path[MAX_STATIONS];
    int len = graph_reconstruct_path(&res, src_idx, dest_idx, path, MAX_STATIONS);

    printf("Shortest route from %s to %s (distance %d):\n  ",
           source, destination, res.dist[dest_idx]);
    for (int i = 0; i < len; i++) {
        printf("%s", g->stations[path[i]].name);
        if (i != len - 1) printf(" -> ");
    }
    printf("\n");
    return 0;
}

int route_query_all_distances(const Graph *g, const char *source) {
    int src_idx = graph_find_station(g, source);
    if (src_idx == -1) {
        printf("Route query failed: unknown station '%s'.\n", source);
        return -1;
    }

    PathResult res = graph_dijkstra(g, src_idx);
    printf("Shortest distances from %s:\n", source);
    for (int i = 0; i < g->num_stations; i++) {
        if (i == src_idx) continue;
        if (res.dist[i] == INF) {
            printf("  %-20s unreachable\n", g->stations[i].name);
        } else {
            printf("  %-20s %d\n", g->stations[i].name, res.dist[i]);
        }
    }
    return 0;
}
