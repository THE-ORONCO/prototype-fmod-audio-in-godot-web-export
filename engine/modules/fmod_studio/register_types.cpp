#include "register_types.h"

#include "fmod_studio.h"

#include "core/config/engine.h"
#include "core/object/class_db.h"

static FmodStudio *fmod_studio = nullptr;

void initialize_fmod_studio_module(ModuleInitializationLevel p_level) {
	if (p_level != MODULE_INITIALIZATION_LEVEL_SCENE) {
		return;
	}
	GDREGISTER_CLASS(FmodStudio);
	GDREGISTER_CLASS(FmodStudioEvent);
	fmod_studio = memnew(FmodStudio);
	Engine::get_singleton()->add_singleton(Engine::Singleton("FmodStudio", fmod_studio));
}

void uninitialize_fmod_studio_module(ModuleInitializationLevel p_level) {
	if (p_level != MODULE_INITIALIZATION_LEVEL_SCENE) {
		return;
	}
	Engine::get_singleton()->remove_singleton("FmodStudio");
	memdelete(fmod_studio);
	fmod_studio = nullptr;
}
