# Steam achievement icons

One pair per API name, 256x256:

- `ACH_*_on.jpg` / `ACH_*_off.jpg` — exactly what is uploaded in Steamworks
  (exported from the Stats & Achievements page on 11.09.2026, 32 achievements
  = v1.0.7). If an icon is ever lost or changed in Steamworks, these are the
  copy of record.
- `ACH_*_on.png` / `ACH_*_off.png` — authored sources where we have them; the
  locked `_off.png` is produced by `Tools/make_achievement_off_icons.py`.

Missing on purpose: `ACH_ENDING_THE_EXIT` (1.1, not in Steamworks yet).
