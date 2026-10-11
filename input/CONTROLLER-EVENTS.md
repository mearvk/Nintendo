# Normalized Controller Events (Draft)

Status: schema proposal.

A normalized event should record `schema_version`, platform/controller identifier, event sequence, timestamp with explicit units, and button states using named keys. Record whether an event is captured from physical hardware, an emulator, or a synthetic test.

Adapters must document debounce/polling semantics, bit ordering, disconnect handling, and any lost information. Tests should include no input, each individual button, simultaneous button combinations, and malformed or out-of-order records.
