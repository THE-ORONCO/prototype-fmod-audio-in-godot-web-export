// Approach 2: FMOD Studio HTML5 SDK runs beside Godot. GDScript talks to window.fmodBridge via JavaScriptBridge.
// Calls made before FMOD finished loading are queued.
// Latency A/B via URL: ?fmod_buffer=<length>x<count> (default 512x4), ?fmod_update=<ms>|frame (default frame).
// Defaults match the desktop gdextension (~43 ms buffer); 1024x4 or 2048x2 trade latency for dropout safety.
var FMOD = {};
(function () {
  var system = null;
  var queue = [];
  var bridge = { ready: false, lastError: "", settings: "" };
  window.fmodBridge = bridge;

  function check(result, what) {
    if (result !== FMOD.OK) {
      bridge.lastError = what + ": " + FMOD.ErrorString(result);
      console.error("[fmodBridge] " + bridge.lastError);
      return false;
    }
    return true;
  }

  function whenReady(fn) {
    if (bridge.ready) fn(); else queue.push(fn);
  }

  // "512x4" -> [512, 4]; anything else -> default. FMOD validates the values itself in setDSPBufferSize.
  function bufferSetting(value) {
    var match = /^(\d+)x(\d+)$/.exec(value || "");
    return match ? [+match[1], +match[2]] : [512, 4];
  }

  function startUpdates(value) {
    if (!value || value === "frame") {
      (function tick() { system.update(); window.requestAnimationFrame(tick); })();
      return "every frame";
    }
    var ms = parseInt(value, 10);
    if (!(ms > 0)) { console.warn("[fmodBridge] ignoring fmod_update=" + value); return startUpdates("frame"); }
    window.setInterval(function () { system.update(); }, ms);
    return "every " + ms + " ms";
  }

  FMOD.onRuntimeInitialized = function () {
    var params = new URLSearchParams(window.location.search);
    var buffer = bufferSetting(params.get("fmod_buffer"));
    var out = {};
    if (!check(FMOD.Studio_System_Create(out), "Studio_System_Create")) return;
    system = out.val;
    var core = {};
    check(system.getCoreSystem(core), "getCoreSystem");
    check(core.val.setDSPBufferSize(buffer[0], buffer[1]), "setDSPBufferSize");
    var rate = {};
    check(core.val.getDriverInfo(0, null, null, rate, null, null), "getDriverInfo");
    check(core.val.setSoftwareFormat(rate.val, FMOD.SPEAKERMODE_DEFAULT, 0), "setSoftwareFormat");
    if (!check(system.initialize(256, FMOD.STUDIO_INIT_NORMAL, FMOD.INIT_NORMAL, null), "initialize")) return;
    var updates = startUpdates(params.get("fmod_update"));
    // Report what FMOD actually uses, not what was requested.
    var length = {}, count = {}, mixRate = {};
    core.val.getDSPBufferSize(length, count);
    core.val.getSoftwareFormat(mixRate, {}, {});
    bridge.settings = "dsp=" + length.val + "x" + count.val + " (" +
      Math.round(1000 * length.val * count.val / mixRate.val) + " ms) rate=" + mixRate.val + " update=" + updates;
    bridge.ready = true;
    queue.splice(0).forEach(function (fn) { fn(); });
    console.log("[fmodBridge] ready, " + bridge.settings);
  };
  FMOD.INITIAL_MEMORY = 64 * 1024 * 1024;
  FMODModule(FMOD);

  // Bank bytes come from Godot's .pck as base64, so no extra files need hosting.
  bridge.loadBank = function (name, base64) {
    whenReady(function () {
      var binary = atob(base64);
      var bytes = new Uint8Array(binary.length);
      for (var i = 0; i < binary.length; i++) bytes[i] = binary.charCodeAt(i);
      FMOD.FS_createDataFile("/", name, bytes, true, false);
      if (check(system.loadBankFile("/" + name, FMOD.STUDIO_LOAD_BANK_NORMAL, {}), "loadBankFile " + name)) {
        console.log("[fmodBridge] loaded " + name);
      }
    });
  };

  function instance(path) {
    var description = {};
    var created = {};
    if (!check(system.getEvent(path, description), "getEvent " + path)) return null;
    if (!check(description.val.createInstance(created), "createInstance " + path)) return null;
    return { description: description.val, instance: created.val };
  }

  var music = null;
  var tension = 0;

  bridge.startMusic = function (path) {
    whenReady(function () {
      if (music) return;
      music = instance(path);
      if (!music) return;
      var desc = {};
      check(music.description.getParameterDescriptionByName("Tension", desc), "getParameterDescriptionByName");
      music.tensionId = desc.id;
      check(music.instance.setParameterByID(music.tensionId, tension, false), "setParameterByID");
      check(music.instance.start(), "start music");
    });
  };

  bridge.setTension = function (value) {
    tension = value;
    if (music) check(music.instance.setParameterByID(music.tensionId, value, false), "setParameterByID");
  };

  bridge.playOneShot = function (path) {
    whenReady(function () {
      var shot = instance(path);
      if (!shot) return;
      check(shot.instance.start(), "start " + path);
      check(shot.instance.release(), "release " + path);
    });
  };
})();
