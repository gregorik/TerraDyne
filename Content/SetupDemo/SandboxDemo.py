import unreal

def build_salvage_demo():
    world = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_editor_world()
    actor_sys = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)

    # 1. Clean Up Old Actors (Landscape, Projectiles)
    for actor in actor_sys.get_all_level_actors():
        if "Landscape" in actor.get_name() or "TerraDyne" in actor.get_name():
            unreal.log(f"Deleting {actor.get_name()}")
            actor_sys.destroy_actor(actor)

    # 2. Spawn Manager at 0,0,0
    manager_asset = unreal.load_asset("/Game/TerraDyne/Blueprints/BP_TerraDyneManager.BP_TerraDyneManager_C")
    manager_actor = actor_sys.spawn_actor_from_class(manager_asset, unreal.Vector(0,0,0))
    
    # 3. Spawn Orchestrator (Adjusted for 100m chunk)
    # 100m = 10000 units. Center is 0,0. 
    # Spawning at 0, 0, 3000 ensures visuals fit.
    orch_class = unreal.load_class(None, "/Script/TerraDyne.TerraDyneOrchestrator")
    actor_sys.spawn_actor_from_class(orch_class, unreal.Vector(0,0,3000))

    # 4. Verify Materials are Bound
    # We can programmatically check properties here if we want, 
    # but the Manager BP defaults should handle it.

    unreal.log("Salvage Scene Ready. Play.")

if __name__ == "__main__":
    build_salvage_demo()