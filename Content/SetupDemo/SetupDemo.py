import unreal

def setup_demo_level():
    # 1. Get Access to Editor Subsystems (The Modern API)
    editor_subsystem = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem)
    actor_subsystem = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)

    # 2. Get the World
    world = editor_subsystem.get_editor_world()
    if not world:
        unreal.log_error("No active Editor World found!")
        return

    # 3. Spawn Manager (BP)
    # We use Try/Except to handle potential path errors gracefully
    manager_path = "/Game/TerraDyne/Blueprints/BP_TerraDyneManager.BP_TerraDyneManager_C"
    manager_class = unreal.load_class(None, manager_path)
    
    if manager_class:
        # Spawn at 0,0,0
        actor_subsystem.spawn_actor_from_class(manager_class, unreal.Vector(0,0,0))
        unreal.log("Spawned BP_TerraDyneManager.")
    else:
        unreal.log_error(f"Could not load Blueprint: {manager_path}. Check your Content folder.")

    # 4. Spawn Orchestrator (C++)
    orch_path = "/Script/TerraDyne.TerraDyneOrchestrator"
    orch_class = unreal.load_class(None, orch_path)
    
    if orch_class:
        # Spawn high up at 0,0,3000
        actor_subsystem.spawn_actor_from_class(orch_class, unreal.Vector(0,0,3000))
        unreal.log("Spawned TerraDyneOrchestrator.")
    else:
        unreal.log_error(f"Could not load C++ Class: {orch_path}. Ensure Plugin code is compiled.")

    unreal.log("TerraDyne Demo Level Setup Complete. Press Play.")

if __name__ == "__main__":
    setup_demo_level()