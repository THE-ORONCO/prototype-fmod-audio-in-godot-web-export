## Contract every prototype implements. The gallery only talks to this API.
## The demo: one music event with a Tension parameter (0 = calm, 1 = action) and named one-shot SFX.
class_name AudioBackend
extends Node


## Whether this backend can run on the current platform/build. Called on the script, before instancing.
static func is_available() -> bool:
	return true


## Starts the music. Called from a button press, which is also the user gesture browsers need to unlock audio.
func start_music() -> void:
	pass


## Sets the adaptive parameter, 0..1. Backends smooth it themselves (FMOD seek speed or equivalent).
func set_tension(_value: float) -> void:
	pass


## Plays a one-shot by name: "click" or "footstep".
func play_sfx(_sfx: String) -> void:
	pass


## One line of verifiable state for the headless smoke test (`-- --check`).
func describe() -> String:
	return ""
