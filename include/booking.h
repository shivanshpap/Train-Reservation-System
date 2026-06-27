#ifndef BOOKING_H
#define BOOKING_H

#include "graph.h"

#define MAX_SEATS 50
#define MAX_PASSENGER_NAME 50
#define MAX_TICKETS 200
#define FARE_PER_UNIT_DISTANCE 2 /* simple fare model: cost = distance * this rate */
#define MAX_TRAIN_ROUTES 10

/* ---------- Train: has a fixed seat capacity and runs the network's routes ---------- */
typedef struct {
    int train_id;
    char train_name[MAX_NAME_LEN];
    int total_seats;
    int seat_occupied[MAX_SEATS]; /* 0 = free, 1 = booked */
    char routes[MAX_TRAIN_ROUTES][MAX_NAME_LEN]; /* stations this train services */
    int num_routes;
} Train;

/* ---------- Ticket: produced once a booking succeeds ---------- */
typedef struct {
    int ticket_id;
    int train_id;
    char passenger_name[MAX_PASSENGER_NAME];
    char source[MAX_NAME_LEN];
    char destination[MAX_NAME_LEN];
    int seat_number;
    int distance;
    int fare;
    int path[MAX_STATIONS];
    int path_len;
} Ticket;

/* ---------- Simple in-memory reservation system tying trains + tickets together ---------- */
typedef struct {
    Train trains[MAX_SEATS];   /* reuse MAX_SEATS as a cap on number of trains for simplicity */
    int num_trains;
    Ticket tickets[MAX_TICKETS];
    int num_tickets;
    int next_ticket_id;
} ReservationSystem;

/* ---------- Lifecycle ---------- */
void reservation_init(ReservationSystem *rs);

/* ---------- Train management ---------- */
int  reservation_add_train(ReservationSystem *rs, const char *train_name, int total_seats);
int  reservation_find_train(const ReservationSystem *rs, int train_id);
int  reservation_first_free_seat(const ReservationSystem *rs, int train_idx);
int  reservation_first_free_seat_for_route(const ReservationSystem *rs, int train_id,
                                            const char *source, const char *destination);

/* ---------- Core booking flow ----------
 * Looks up source/destination in the graph, runs Dijkstra, allocates a seat,
 * computes fare from distance, and issues a ticket. Returns ticket_id on
 * success, or a negative error code on failure:
 *   -1 station not found, -2 no path exists, -3 train not found, -4 train full
 */
int reservation_book_ticket(ReservationSystem *rs, const Graph *g, int train_id,
                             const char *passenger_name, const char *source,
                             const char *destination);

/* ---------- Cancellation ---------- */
int reservation_cancel_ticket(ReservationSystem *rs, int ticket_id); /* 0 = ok, -1 = not found */

/* ---------- Lookup / display ---------- */
const Ticket *reservation_find_ticket(const ReservationSystem *rs, int ticket_id);
void reservation_print_ticket(const Ticket *t, const Graph *g);
void reservation_print_train_status(const Train *t);
void reservation_list_tickets(const ReservationSystem *rs);

/* ---------- Train routes ---------- */
void reservation_add_train_route(ReservationSystem *rs, int train_id, const char *station);
int reservation_train_services_route(const Train *t, const char *source, const char *destination);
int reservation_find_trains_for_route(const ReservationSystem *rs, const char *source,
                                       const char *destination, int *train_ids, int max_trains);
int reservation_count_bookings_for_route(const ReservationSystem *rs, int train_id,
                                          const char *source, const char *destination);

#endif /* BOOKING_H */
