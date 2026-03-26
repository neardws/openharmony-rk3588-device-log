# Flashing / Recovery Strategy

## Most likely paths

1. Serial console via DEBUG header
2. BOOT button assisted loader/recovery mode
3. TF-card-based upgrade path
4. Vendor/Rockchip PC flashing path if OTG/device-mode interface can be identified

## Important caution

Do not flash generic images before:
- identifying exact board variant
- capturing current boot logs
- confirming storage layout / boot chain
- checking recoverability of current vendor image
