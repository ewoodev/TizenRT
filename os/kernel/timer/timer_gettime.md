# `timer_gettime`

## Summary

`timer_gettime()` snapshots the timer's remaining watchdog delay and the last watchdog delay recorded by the module.

## Behavior

- Rejects invalid timer handles and a NULL output pointer with `EINVAL`.
- Reads the watchdog's remaining ticks with `wd_gettime()` and converts them into `value->it_value`.
- Converts `pt_last` into `value->it_interval`.
- Returns `OK` without additional locking or normalization.

## Inputs and Outputs

- `timerid`: timer handle from `timer_create()`.
- `value`: output structure for the remaining time and recorded interval value.
- Return value: `OK` on success, or `ERROR` with `errno` set to `EINVAL`.

## Dependencies

- `timer_settime.md` updates `pt_last` and arms the watchdog that this function reads.
- Uses `wd_gettime()` and `clock_ticks2time()` for the snapshot conversion.

## Notes

- `it_interval` reflects `pt_last`, not the caller's original `it_interval` field. For a one-shot timer it can therefore report the last armed delay instead of zero.
- Before the first expiration of a periodic timer, `it_interval` still reports the initial arm delay. After the first restart, it begins reporting the periodic delay.
- The watchdog may continue counting while the snapshot is taken, so the reported remaining time is approximate.
