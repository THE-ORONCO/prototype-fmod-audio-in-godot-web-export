#include "fmod_studio.h"

#include "core/io/file_access.h"
#include "core/math/math_funcs.h"
#include "core/object/class_db.h"
#include "core/string/print_string.h"

#include <fmod_errors.h>

#include <emscripten/emscripten.h>
#include <emscripten/eventloop.h>
#include <emscripten/html5.h>

FmodStudio *FmodStudio::singleton = nullptr;

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
	ClassDB::bind_method(D_METHOD("get_settings"), &FmodStudio::get_settings);
}

bool FmodStudio::check(int p_result, const String &p_what) {
	if (p_result == FMOD_OK) {
		return true;
	}
	last_error = p_what + ": " + FMOD_ErrorString((FMOD_RESULT)p_result);
	ERR_PRINT("FmodStudio: " + last_error);
	return false;
}

// Studio commands (start, parameters, stop) are processed in update(). A browser timer keeps it running independent
// of the SceneTree, so stop()/release() on freed nodes still take effect.
void FmodStudio::update(void *p_self) {
	static_cast<FmodStudio *>(p_self)->system->update();
}

bool FmodStudio::update_frame(double p_time, void *p_self) {
	if (singleton == nullptr) {
		return false; // Ends the requestAnimationFrame loop after shutdown.
	}
	singleton->system->update();
	return true;
}

// Reads ?<p_name>=… from the page URL, so latency settings can be A/B-tested without rebuilding.
static String url_param(const char *p_name) {
	String script = vformat("new URLSearchParams(window.location.search).get('%s') || ''", p_name);
	return String::utf8(emscripten_run_script_string(script.utf8().get_data()));
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
	// Default 512x4 (~43 ms), as the desktop gdextension. FMOD and Godot share the main thread here, so the buffer is
	// the cushion against frame stalls: ?fmod_buffer=1024x4 or 2048x2 (FMOD's HTML5 advice) trade latency for safety.
	unsigned int buffer_length = 512;
	int buffer_count = 4;
	PackedStringArray buffer = url_param("fmod_buffer").split("x");
	if (buffer.size() == 2 && buffer[0].is_valid_int() && buffer[1].is_valid_int()) {
		buffer_length = buffer[0].to_int();
		buffer_count = buffer[1].to_int();
	}
	check(core->setDSPBufferSize(buffer_length, buffer_count), "setDSPBufferSize");
	int rate = 0;
	core->getDriverInfo(0, nullptr, 0, nullptr, &rate, nullptr, nullptr);
	core->setSoftwareFormat(rate, FMOD_SPEAKERMODE_DEFAULT, 0);
	if (!check(created->initialize(256, FMOD_STUDIO_INIT_NORMAL, FMOD_INIT_NORMAL, nullptr), "initialize")) {
		created->release();
		return;
	}
	system = created;
	singleton = this;

	// Default every frame, as the desktop gdextension. Override with ?fmod_update=<ms>.
	String update = url_param("fmod_update");
	String updates;
	if (update.is_empty() || update == "frame" || !update.is_valid_int() || update.to_int() <= 0) {
		if (!update.is_empty() && update != "frame") {
			WARN_PRINT("FmodStudio: ignoring fmod_update=" + update);
		}
		emscripten_request_animation_frame_loop(&FmodStudio::update_frame, nullptr);
		updates = "every frame";
	} else {
		int ms = update.to_int();
		update_interval = emscripten_set_interval(&FmodStudio::update, ms, this);
		updates = vformat("every %d ms", ms);
	}

	// Report what FMOD actually uses, not what was requested.
	core->getDSPBufferSize(&buffer_length, &buffer_count);
	core->getSoftwareFormat(&rate, nullptr, nullptr);
	settings = vformat("dsp=%dx%d (%d ms) rate=%d update=%s", buffer_length, buffer_count,
			(int)Math::round(1000.0 * buffer_length * buffer_count / rate), rate, updates);
	print_line("FmodStudio: initialized, " + settings);
}

FmodStudio::~FmodStudio() {
	if (system) {
		singleton = nullptr;
		if (update_interval) {
			emscripten_clear_interval(update_interval);
		}
		system->release(); // Also unloads all banks.
	}
}
