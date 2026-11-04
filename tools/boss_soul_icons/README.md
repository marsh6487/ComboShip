# Boss soul HD icon metadata repair

`BossSoul_HD_Tracker_POC5_Metadata_Fix.o2r` replaces POC4. Enable it as the winning
boss soul icon pack in each applicable game's mod folder. It supplies both base
and Alt resource paths, preserving all original artwork pixels. Do not leave
POC4 with higher resource priority.

SHA-256: `d94225c92ea75def3af90b6774d2bd1f90285450b9f3ab1a038ebee8e5762265`.

This pack repairs dialogue/tracker texture metadata only. The Graveyard,
Majora GI, soul flame and equipment fixes require the matching rebuilt candidate;
keep both game modules and their packaged application archives together.
Original game archives, saves and accepted scene/model packs are retained.

`repair_pack.py` reproduces the archive from the exact original POC4 input
identified in that script. The candidate is source/static verified; configured
in-game dialogue/tracker appearance under both asset modes remains untested.
