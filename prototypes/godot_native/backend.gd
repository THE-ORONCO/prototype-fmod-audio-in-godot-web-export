## Godot's built-in audio only (desktop and web, no FMOD).
## Music: AudioStreamSynchronized plays calm + action sample-aligned; Tension crossfades their volumes.
## SFX: AudioStreamRandomizer picks a variation and randomizes pitch.
extends AudioBackend

const SEEK_SPEED := 2.0  # Tension units per second, mirrors FMOD parameter seek speed

var _music := AudioStreamPlayer.new()
var _sync := AudioStreamSynchronized.new()
var _sfx := {}
var _tension := 0.0
var _target := 0.0


func _ready() -> void:
	_sync.stream_count = 2
	_sync.set_sync_stream(0, load("res://audio/singularity_calm.wav"))
	_sync.set_sync_stream(1, load("res://audio/singularity_action.wav"))
	_music.stream = _sync
	add_child(_music)
	_sfx["click"] = _make_sfx("click_00%d.ogg", 5)
	_sfx["footstep"] = _make_sfx("back_00%d.ogg", 4)
	_apply_tension()


func _make_sfx(pattern: String, count: int) -> AudioStreamPlayer:
	var randomizer := AudioStreamRandomizer.new()
	randomizer.random_pitch = 1.06  # about ±1 semitone
	for i in range(1, count + 1):
		randomizer.add_stream(-1, load("res://audio/" + pattern % i))
	var player := AudioStreamPlayer.new()
	player.stream = randomizer
	player.max_polyphony = 4
	add_child(player)
	return player


func start_music() -> void:
	if not _music.playing:
		_music.play()


func set_tension(value: float) -> void:
	_target = value


func play_sfx(sfx: String) -> void:
	(_sfx[sfx] as AudioStreamPlayer).play()


func _process(delta: float) -> void:
	if _tension != _target:
		_tension = move_toward(_tension, _target, SEEK_SPEED * delta)
		_apply_tension()


func describe() -> String:
	return "playing=%s calm_db=%.1f action_db=%.1f loop=%s" % [_music.playing,
		_sync.get_sync_stream_volume(0), _sync.get_sync_stream_volume(1),
		(_sync.get_sync_stream(0) as AudioStreamWAV).loop_mode]


func _apply_tension() -> void:
	_sync.set_sync_stream_volume(0, linear_to_db(1.0 - _tension))
	_sync.set_sync_stream_volume(1, linear_to_db(_tension))
