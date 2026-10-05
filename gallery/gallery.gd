## Gallery launcher: one UI (Start, Tension slider, SFX buttons) driving whichever prototype backend is selected.
## Desktop: `-- --prototype=<id>` preselects a backend, `-- --check` smoke-tests all available ones headless.
extends Control

## Registry. Order matters: the first available entry is the default (on web, no CLI args exist).
const PROTOTYPES: Array[Dictionary] = [
	{id = "fmod_gdextension", title = "FMOD (gdextension)", script = "res://prototypes/fmod_gdextension/backend.gd"},
	{id = "fmod_html5", title = "FMOD (HTML5 SDK)", script = "res://prototypes/fmod_html5/backend.gd"},
	{id = "godot_native", title = "Godot audio", script = "res://prototypes/godot_native/backend.gd"},
]

var _available: Array[Dictionary] = []
var _backend: AudioBackend
var _picker := OptionButton.new()
var _slider := HSlider.new()
var _status := Label.new()

# .NET CLI location is not specified in settings correctly and cannot be discovered automatically. Please check .NET CLI settings under File | Settings | Build, Execution, Deployment | Toolset and Build.
func _ready() -> void:
	# FmodManager pumps FmodServer.update(). Added here instead of as autoload so builds without the extension start cleanly.
	if Engine.has_singleton("FmodServer"):
		add_child(load("res://addons/fmod/FmodManager.gd").new())
	for prototype in PROTOTYPES:
		if load(prototype.script).is_available():
			_available.append(prototype)
			_picker.add_item(prototype.title)
	_picker.item_selected.connect(_use_backend)

	_slider.max_value = 1.0
	_slider.step = 0.01
	_slider.custom_minimum_size.x = 300
	_slider.value_changed.connect(func(v: float) -> void:
		_backend.set_tension(v)
		_status.text = "Tension %.2f" % v)

	var box := VBoxContainer.new()
	box.position = Vector2(24, 24)
	box.add_child(_picker)
	box.add_child(_button("Start music", func() -> void: _backend.start_music()))
	box.add_child(_labeled("Tension", _slider))
	box.add_child(_button("Click", func() -> void: _backend.play_sfx("click")))
	box.add_child(_button("Footstep", func() -> void: _backend.play_sfx("footstep")))
	box.add_child(_status)
	add_child(box)

	var args := OS.get_cmdline_user_args()
	_select(_requested_index(args))
	if "--check" in args:
		_self_check.call_deferred()


func _requested_index(args: PackedStringArray) -> int:
	for arg in args:
		if arg.begins_with("--prototype="):
			var id := arg.trim_prefix("--prototype=")
			for i in _available.size():
				if _available[i].id == id:
					return i
			push_error("Prototype '%s' is unknown or not available on this platform" % id)
	return 0


func _select(index: int) -> void:
	_picker.select(index)
	_use_backend(index)


func _use_backend(index: int) -> void:
	if _backend != null:
		_backend.queue_free()
	_backend = (load(_available[index].script) as GDScript).new()
	add_child(_backend)
	_slider.set_value_no_signal(0.0)
	_status.text = "Prototype: " + _available[index].title


func _button(text: String, on_pressed: Callable) -> Button:
	var button := Button.new()
	button.text = text
	button.pressed.connect(on_pressed)
	return button


func _labeled(text: String, control: Control) -> HBoxContainer:
	var row := HBoxContainer.new()
	var label := Label.new()
	label.text = text
	row.add_child(label)
	row.add_child(control)
	return row


## Headless smoke test: plays every available backend briefly and prints what it can verify.
func _self_check() -> void:
	for i in _available.size():
		_select(i)
		await get_tree().process_frame
		_backend.start_music()
		_backend.set_tension(0.7)
		_backend.play_sfx("click")
		_backend.play_sfx("footstep")
		await get_tree().create_timer(1.0).timeout
		print("[check] ", _available[i].id, ": ", _backend.describe())
	get_tree().quit()
