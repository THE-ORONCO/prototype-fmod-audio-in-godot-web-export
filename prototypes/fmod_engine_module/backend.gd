## FMOD Studio compiled into the engine: our engine/modules/fmod_studio, linked into custom web templates.
## The module registers the FmodStudio singleton; it only exists in builds made with `task engine:build`.
extends AudioBackend

const MUSIC := "event:/Music/Adaptive"
const EVENTS := {"click": "event:/SFX/Click", "footstep": "event:/SFX/Footstep"}

var _fmod: Object = Engine.get_singleton("FmodStudio") if Engine.has_singleton("FmodStudio") else null
var _music: Object  # FmodStudioEvent, released by refcount when this backend is freed


static func is_available() -> bool:
	return Engine.has_singleton("FmodStudio")


func _ready() -> void:
	for path in ["res://banks/Master.strings.bank", "res://banks/Master.bank"]:
		_fmod.call("load_bank", path)


func start_music() -> void:
	if _music == null:
		_music = _fmod.call("create_instance", MUSIC)
		_music.call("start")


func set_tension(value: float) -> void:
	if _music != null:
		_music.call("set_parameter", "Tension", value)


func play_sfx(sfx: String) -> void:
	_fmod.call("play_one_shot", EVENTS[sfx])


func describe() -> String:
	var paths := []
	for path: String in [MUSIC] + EVENTS.values():
		paths.append("%s=%s" % [path, _fmod.call("has_event", path)])
	var sfx := []
	for path: String in EVENTS.values():
		sfx.append("%s=%d" % [path.get_file(), _fmod.call("get_instance_count", path)])
	return "%s state=%s Tension=%s sfx_instances(%s) lastError=%s" % [", ".join(paths),
		_music.call("get_playback_state"), _music.call("get_parameter", "Tension"), ", ".join(sfx),
		_fmod.call("get_last_error")]
