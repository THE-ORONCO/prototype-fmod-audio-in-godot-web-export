## FMOD Studio HTML5 SDK running in the page next to Godot, driven through JavaScriptBridge (web only).
## web/fmod_bridge.js defines window.fmodBridge; banks are read from the .pck and handed over as base64.
extends AudioBackend

const MUSIC := "event:/Music/Adaptive"
const EVENTS := {"click": "event:/SFX/Click", "footstep": "event:/SFX/Footstep"}

var _bridge: JavaScriptObject


static func is_available() -> bool:
	return OS.has_feature("web") and JavaScriptBridge.eval("typeof window.fmodBridge") == "object"


func _ready() -> void:
	_bridge = JavaScriptBridge.get_interface("fmodBridge")
	for bank in ["Master.strings.bank", "Master.bank"]:
		var bytes := FileAccess.get_file_as_bytes("res://banks/" + bank)
		_bridge.call("loadBank", bank, Marshalls.raw_to_base64(bytes))


func start_music() -> void:
	_bridge.call("startMusic", MUSIC)


func set_tension(value: float) -> void:
	_bridge.call("setTension", value)


func play_sfx(sfx: String) -> void:
	_bridge.call("playOneShot", EVENTS[sfx])


func describe() -> String:
	return "ready=%s %s lastError=%s" % [_bridge.get("ready"), _bridge.get("settings"), _bridge.get("lastError")]
