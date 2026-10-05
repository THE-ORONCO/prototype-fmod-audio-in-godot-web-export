#pragma once

#include "core/object/object.h"
#include "core/object/ref_counted.h"

#include <fmod_studio.hpp>

// One event instance. Stopped (with fadeout) and released when the last reference goes away.
class FmodStudioEvent : public RefCounted {
	GDCLASS(FmodStudioEvent, RefCounted);

	FMOD::Studio::EventInstance *instance = nullptr;

protected:
	static void _bind_methods();

public:
	void start();
	void stop(bool p_allow_fadeout);
	void set_parameter(const String &p_name, float p_value);
	float get_parameter(const String &p_name) const;
	// FMOD_STUDIO_PLAYBACK_STATE: 0 playing, 1 sustaining, 2 stopped, 3 starting, 4 stopping.
	int get_playback_state() const;

	void init(FMOD::Studio::EventInstance *p_instance) { instance = p_instance; }
	~FmodStudioEvent();
};

// Singleton "FmodStudio": owns the Studio system. Web only: the module is linked against the FMOD HTML5 SDK.
class FmodStudio : public Object {
	GDCLASS(FmodStudio, Object);

	FMOD::Studio::System *system = nullptr;
	String last_error;
	long update_interval = 0;

	bool check(int p_result, const String &p_what);
	FMOD::Studio::EventInstance *new_instance(const String &p_path);
	static void update(void *p_self);

protected:
	static void _bind_methods();

public:
	bool is_initialized() const { return system != nullptr; }
	bool load_bank(const String &p_path); // Idempotent; banks stay loaded until the system is released.
	bool has_event(const String &p_path) const;
	int get_instance_count(const String &p_path) const; // Live instances, e.g. one-shots still playing.
	Ref<FmodStudioEvent> create_instance(const String &p_path);
	void play_one_shot(const String &p_path);
	String get_last_error() const { return last_error; }

	FmodStudio();
	~FmodStudio();
};
