#include "fmod_studio.h"

#include "core/io/file_access.h"
#include "core/object/class_db.h"
#include "core/string/print_string.h"

#include <fmod_errors.h>

#include <emscripten/eventloop.h>

void FmodStudioEvent::_bind_methods() {
	ClassDB::bind_method(D_METHOD("start"), &FmodStudioEvent::start);
	ClassDB::bind_method(D_METHOD("stop", "allow_fadeout"), &FmodStudioEvent::stop, DEFVAL(true));
	ClassDB::bind_method(D_METHOD("set_parameter", "name", "value"), &FmodStudioEvent::set_parameter);
	ClassDB::bind_method(D_METHOD("get_parameter", "name"), &FmodStudioEvent::get_parameter);
	ClassDB::bind_method(D_METHOD("get_playback_state"), &FmodStudioEvent::get_playback_state);
}

void FmodStudioEvent::start() {
	instance->start();
}

void FmodStudioEvent::stop(bool p_allow_fadeout) {
	instance->stop(p_allow_fadeout ? FMOD_STUDIO_STOP_ALLOWFADEOUT : FMOD_STUDIO_STOP_IMMEDIATE);
}

void FmodStudioEvent::set_parameter(const String &p_name, float p_value) {
	instance->setParameterByName(p_name.utf8().get_data(), p_value);
}

float FmodStudioEvent::get_parameter(const String &p_name) const {
	float value = 0.0f;
	instance->getParameterByName(p_name.utf8().get_data(), &value);
	return value;
}

int FmodStudioEvent::get_playback_state() const {
	FMOD_STUDIO_PLAYBACK_STATE state = FMOD_STUDIO_PLAYBACK_STOPPED;
	instance->getPlaybackState(&state);
	return state;
}

FmodStudioEvent::~FmodStudioEvent() {
	if (instance) {
		instance->stop(FMOD_STUDIO_STOP_ALLOWFADEOUT);
		instance->release(); // FMOD frees it once it has stopped.
	}
}

void FmodStudio::_bind_methods() {
	ClassDB::bind_method(D_METHOD("is_initialized"), &FmodStudio::is_initialized);
	ClassDB::bind_method(D_METHOD("load_bank", "path"), &FmodStudio::load_bank);
	ClassDB::bind_method(D_METHOD("has_event", "path"), &FmodStudio::has_event);
	ClassDB::bind_method(D_METHOD("get_instance_count", "path"), &FmodStudio::get_instance_count);
	ClassDB::bind_method(D_METHOD("create_instance", "path"), &FmodStudio::create_instance);
	ClassDB::bind_method(D_METHOD("play_one_shot", "path"), &FmodStudio::play_one_shot);
	ClassDB::bind_method(D_METHOD("get_last_error"), &FmodStudio::get_last_error);
}

bool FmodStudio::check(int p_result, const String &p_what) {
	if (p_result == FMOD_OK) {
		return true;
	}
	last_error = p_what + ": " + FMOD_ErrorString((FMOD_RESULT)p_result);
	ERR_PRINT("FmodStudio: " + last_error);
	return false;
}

// The non-pthread FMOD build mixes only inside update(). A browser interval keeps it running independent of the
// SceneTree, so stop()/release() on freed nodes still take effect (same 20 ms cadence as the fmod_html5 bridge).
void FmodStudio::update(void *p_self) {
	static_cast<FmodStudio *>(p_self)->system->update();
}

bool FmodStudio::load_bank(const String &p_path) {
	ERR_FAIL_NULL_V(system, false);
	// res:// lives in the .pck, not in Emscripten's file system, so hand FMOD the bytes.
	Error err;
	Vector<uint8_t> bytes = FileAccess::get_file_as_bytes(p_path, &err);
	if (err != OK) {
		last_error = "load_bank: cannot read " + p_path;
		ERR_PRINT("FmodStudio: " + last_error);
		return false;
	}
	FMOD::Studio::Bank *bank = nullptr;
	FMOD_RESULT result = system->loadBankMemory((const char *)bytes.ptr(), bytes.size(), FMOD_STUDIO_LOAD_MEMORY,
			FMOD_STUDIO_LOAD_BANK_NORMAL, &bank);
	// Banks belong to the system, not to a caller: loading one again (e.g. a re-instanced backend) is a no-op.
	if (result == FMOD_ERR_EVENT_ALREADY_LOADED) {
		return true;
	}
	if (!check(result, "load_bank " + p_path)) {
		return false;
	}
	print_line("FmodStudio: loaded " + p_path);
	return true;
}

bool FmodStudio::has_event(const String &p_path) const {
	FMOD::Studio::EventDescription *description = nullptr;
	return system && system->getEvent(p_path.utf8().get_data(), &description) == FMOD_OK;
}

FMOD::Studio::EventInstance *FmodStudio::new_instance(const String &p_path) {
	ERR_FAIL_NULL_V(system, nullptr);
	FMOD::Studio::EventDescription *description = nullptr;
	FMOD::Studio::EventInstance *instance = nullptr;
	if (!check(system->getEvent(p_path.utf8().get_data(), &description), "getEvent " + p_path) ||
			!check(description->createInstance(&instance), "createInstance " + p_path)) {
		return nullptr;
	}
	return instance;
}

int FmodStudio::get_instance_count(const String &p_path) const {
	FMOD::Studio::EventDescription *description = nullptr;
	int count = 0;
	if (system && system->getEvent(p_path.utf8().get_data(), &description) == FMOD_OK) {
		description->getInstanceCount(&count);
	}
	return count;
}

Ref<FmodStudioEvent> FmodStudio::create_instance(const String &p_path) {
	FMOD::Studio::EventInstance *instance = new_instance(p_path);
	if (!instance) {
		return Ref<FmodStudioEvent>();
	}
	Ref<FmodStudioEvent> event;
	event.instantiate();
	event->init(instance);
	return event;
}

void FmodStudio::play_one_shot(const String &p_path) {
	// No FmodStudioEvent wrapper: its destructor stops the instance, which would cut the one-shot off immediately.
	// release() right after start() lets FMOD free the instance once it has finished playing.
	FMOD::Studio::EventInstance *instance = new_instance(p_path);
	if (instance) {
		check(instance->start(), "start " + p_path);
		instance->release();
	}
}

FmodStudio::FmodStudio() {
	FMOD::Studio::System *created = nullptr;
	if (!check(FMOD::Studio::System::create(&created), "Studio::System::create")) {
		return;
	}
	FMOD::System *core = nullptr;
	created->getCoreSystem(&core);
	// Same settings as the fmod_html5 bridge: large DSP buffer against WebAudio underruns, driver's sample rate.
	core->setDSPBufferSize(2048, 2);
	int rate = 0;
	core->getDriverInfo(0, nullptr, 0, nullptr, &rate, nullptr, nullptr);
	core->setSoftwareFormat(rate, FMOD_SPEAKERMODE_DEFAULT, 0);
	if (!check(created->initialize(256, FMOD_STUDIO_INIT_NORMAL, FMOD_INIT_NORMAL, nullptr), "initialize")) {
		created->release();
		return;
	}
	system = created;
	update_interval = emscripten_set_interval(&FmodStudio::update, 20, this);
	print_line("FmodStudio: initialized");
}

FmodStudio::~FmodStudio() {
	if (system) {
		emscripten_clear_interval(update_interval);
		system->release(); // Also unloads all banks.
	}
}
