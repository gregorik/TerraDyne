import unreal

def setup_world_eater():
    # 1. Get Modern Editor Subsystems
    unreal_editor = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem)
    actor_sys = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
    
    world = unreal_editor.get_editor_world()
    if not world:
        unreal.log_error("No active level found! Create a new Level first.")
        return

    unreal.log("--- INITIALIZING TERRA-DYNE WORLD EATER DEMO ---")

    # 2. Cleanup Legacy Actors
    # We remove old managers, pavers, and debris to ensure a clean run
    all_actors = actor_sys.get_all_level_actors()
    for actor in all_actors:
        name = actor.get_name()
        # Clean up previous runs
        if "TerraDyne" in name or "Manager" in name or "Paver" in name:
            actor_sys.destroy_actor(actor)
            
    # 3. Spawn Lighting (Directional Light) if missing
    # This ensures you aren't digging in the dark
    has_light = False
    for actor in all_actors:
        if isinstance(actor, unreal.DirectionalLight):
            has_light = True
            break
            
    if not has_light:
        light_class = unreal.load_class(None, "/Script/Engine.DirectionalLight")
        light_actor = actor_sys.spawn_actor_from_class(light_class, unreal.Vector(0,0,2000))
        # Angle it nicely
        light_actor.set_actor_rotation(unreal.Rotator(-45, 45, 0), False)
        unreal.log("Spawned Directional Light.")

    # 4. Spawn & Configure Manager
    # We load the generated Blueprint class
    manager_path = "/Game/TerraDyne/Blueprints/BP_TerraDyneManager.BP_TerraDyneManager_C"
    manager_class = unreal.load_class(None, manager_path)
    
    if not manager_class:
        unreal.log_error("CRITICAL: BP_TerraDyneManager not found! Please create it in Content/TerraDyne/Blueprints/")
        return

    manager = actor_sys.spawn_actor_from_class(manager_class, unreal.Vector(0,0,0))
    
    # --- CRITICAL: HARD LINK MATERIALS ---
    # We inject the assets directly into the instance to bypass any Blueprint default issues
    h_brush = unreal.load_asset("/Game/TerraDyne/Materials/Tools/M_HeightBrush")
    w_brush = unreal.load_asset("/Game/TerraDyne/Materials/Tools/M_WeightBrush")
    master_mat = unreal.load_asset("/Game/TerraDyne/Materials/VHFM/M_TerraDyne_Master")
    
    if h_brush: 
        manager.set_editor_property("HeightBrushMaterial", h_brush)
    else: 
        unreal.log_error("M_HeightBrush not found.")
        
    if w_brush: 
        manager.set_editor_property("WeightBrushMaterial", w_brush)
    else: 
        unreal.log_error("M_WeightBrush not found.")
        
    if master_mat: 
        manager.set_editor_property("MasterMaterial", master_mat)
    else: 
        unreal.log_error("M_TerraDyne_Master not found.")

    # Enable Auto Import (Triggers Sandbox Mode since no Landscape exists)
    manager.set_editor_property("bAutoImportAtRuntime", True)
    
    unreal.log("Manager configured successfully.")

    # 5. Spawn the Harvester (Paver)
    paver_class = unreal.load_class(None, "/Script/TerraDyne.TerraDynePaver")
    
    if paver_class:
        # Spawn high (Z=3000) to ensure it clears the Perlin Noise mountains
        # Start at X=-4000 to give it road to travel
        start_pos = unreal.Vector(-4000, 0, 3000) 
        paver = actor_sys.spawn_actor_from_class(paver_class, start_pos)
        
        # Orient flat
        paver.set_actor_rotation(unreal.Rotator(0, 0, 0), False)
        unreal.log("Harvester Spawned.")
    else:
        unreal.log_error("TerraDynePaver C++ Class not found! Compile Code.")

    unreal.log("--- SETUP COMPLETE. PRESS PLAY. ---")

if __name__ == "__main__":
    setup_world_eater()