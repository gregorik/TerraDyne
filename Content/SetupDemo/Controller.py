import unreal

def setup_gui_inputs():
    # 1. Add Input Mapping
    input_settings = unreal.get_default_object(unreal.InputSettings)
    
    # Check if action exists
    action_names = unreal.InputSettings.get_action_names(input_settings)
    if "TerraDyneClick" not in action_names:
        action = unreal.InputActionKeyMapping()
        action.action_name = "TerraDyneClick"
        action.key = unreal.InputLibrary.get_key("LeftMouseButton")
        unreal.InputSettings.add_action_mapping(input_settings, action, False)
        unreal.InputSettings.save_key_mappings(input_settings)
        unreal.log("Added Input Mapping: TerraDyneClick")

    # 2. Create Game Mode to enforce controller
    gm_path = "/Game/TerraDyne/Blueprints/GM_TerraDyneEdit"
    
    # Create GameModeBase if not exists
    if not unreal.EditorAssetLibrary.does_asset_exist(gm_path):
        factory = unreal.BlueprintFactory()
        factory.set_editor_property("parent_class", unreal.GameModeBase)
        
        gm_asset = unreal.AssetToolsHelpers.get_asset_tools().create_asset(
            "GM_TerraDyneEdit", "/Game/TerraDyne/Blueprints", None, factory
        )
        
        # We can't easily compile-edit the DefaultController class via Python directly 
        # without K2 nodes, so we log instruction.
        unreal.log_warning(">>> ACTION REQUIRED: Open GM_TerraDyneEdit and set PlayerControllerClass to ATerraDyneEditController.")

if __name__ == "__main__":
    setup_gui_inputs()