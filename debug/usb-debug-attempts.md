# USB Debug Attempts

## UI state

- Developer options contained only USB debugging
- USB debugging enabled

## Results

- No adb device enumerated via tested USB path
- Hypothesis: tested port was USB host, not OTG/device

## Follow-up

- Prefer serial debug via DEBUG header
- Revisit OTG only if a dedicated device-mode port is later identified
