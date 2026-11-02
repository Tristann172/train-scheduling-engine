# Train Scheduling Engine

## Data structures and complexity

- `LinkedList<T>` is the common manually allocated singly linked container used
  for stations, trains, tickets, carriages, and stop schedules. Owning domain
  lists release their entity pointers before clearing their link nodes.
- `HashMap<K, V, HashFunc>` uses separate chaining and resizes its manually
  allocated bucket array. Station IDs, train codes, and ticket IDs are indexed
  for expected `O(1)` lookup; the built-in hash functions cover `int` and
  `std::string`.
- `MinHeap<T, Comparator>` is a manually allocated, doubling binary heap. It orders
  schedule events in `O(log E)` per insertion/removal and exposes the next event
  in `O(1)`.
- `SegmentTree<T>` uses a `4N` array and lazy propagation. Range minimum queries
  and range updates are `O(log N)`.
- Each carriage keeps one segment tree per seat. A segment value of `1` means
  that seat is free on that route segment; `0` means it is booked. The first
  availability query rebuilds occupancy from the ticket list in one pass.
  Further bookings and cancellations update the cached trees directly.
- The scheduler queues station-track and single-track-block occupancy intervals,
  processes them chronologically, and uses a custom open-addressed resource
  table. Schedule changes increment a train revision and enqueue only that
  train's updated events. Older heap entries and resource reservations are
  skipped using lazy revision checks; the event heap is not rebuilt.

The project does not use STL container classes for these structures. Dynamic
arrays and linked nodes are owned and released by their respective classes.