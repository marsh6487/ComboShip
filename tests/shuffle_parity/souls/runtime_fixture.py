from pathlib import Path
ROOT=Path(__file__).resolve().parents[3]
def prepare_runtime(t):
    actors=(ROOT/'soh/src/code/z_actor.c').read_text()
    a=actors.index('Actor* Actor_Spawn(');b=actors.index('    ActorDBEntry* dbEntry',a)
    (Path(t)/'spawn.inc').write_text(actors[a:b]+'    ++spawnReachesAllocation;return nullptr;\n}\n')
    collision=(ROOT/'soh/src/code/z_collision_check.c').read_text()
    start=collision.index('void CollisionCheck_AC(')
    end=collision.index('\n/**',start)
    (Path(t)/'collision.inc').write_text(collision[start:end])
    a=actors.index("Actor* Actor_RemoveFromCategory(");b=actors.index("\n}",a)+2
    (Path(t)/"remove.inc").write_text(actors[a:b])
