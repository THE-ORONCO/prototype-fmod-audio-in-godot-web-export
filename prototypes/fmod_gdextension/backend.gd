## FMOD Studio through the fmod-gdextension addon (desktop only: the official web templates cannot load GDExtensions).
## Uses Engine.get_singleton so this script also parses in builds without the extension.
extends AudioBackend

const MUSIC := "event:/Music/Adaptive"
const EVENTS := {"click": "event:/SFX/Click", "footstep": "event:/SFX/Footstep"}

var _server: Object = Engine.get_singleton("FmodServer")
var _banks: Array = []  # keep references, banks unload when freed
var _music: Object


static func is_available() -> bool:
	return Engine.has_singleton("FmodServer")


## Needs the gallery's FmodManager (pumps FmodServer.update()); it outlives this node so stop() on exit still applies.
func _ready() -> void:
	for path in ["res://banks/Master.strings.bank", "res://banks/Master.bank"]:
		_banks.append(_server.call("load_bank", path, 0))


func start_music() -> void:
	if _music == null:
		_music = _server.call("create_event_instance", MUSIC)
		_music.call("start")


func set_tension(value: float) -> void:
	if _music != null:
		_music.call("set_parameter_by_name", "Tension", value)


func play_sfx(sfx: String) -> void:
	_server.call("play_one_shot", EVENTS[sfx])


func describe() -> String:
	var paths := []
	for path: String in [MUSIC] + EVENTS.values():
		paths.append("%s=%s" % [path, _server.call("check_event_path", path)])
	return "%s valid=%s state=%s Tension=%s" % [", ".join(paths), _music.call("is_valid"),
		_music.call("get_playback_state"), _music.call("get_parameter_by_name", "Tension")]


func _exit_tree() -> void:
	if _music != null:
		_music.call("stop", 0)
		_music.call("release")
