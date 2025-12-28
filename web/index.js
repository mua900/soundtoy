import { StAudio } from "./audio.js";

const BYTECODE_INTERP = 0;
const TREE_INTERP = 1;

let st_initialize;
let st_sampler_create;
let st_sampler_destroy;
let st_check_expression_string;
let st_sampler_set_expression;
let st_sampler_evaluate;
let st_sampler_step_time;
let st_sampler_set_sample_rate;
let st_sampler_set_sample_time;
let st_fill;
let st_fill_interleaved;
let st_fill_strided;
let st_fill_planar;

let soundtoy_ready;
let resolve_soundtoy_ready;

soundtoy_ready = new Promise((resolve) => {resolve_soundtoy_ready = resolve;});

Module.onRuntimeInitialized = function load_soundtoy() {
    st_initialize = Module.cwrap("st_initialize", "boolean", []);
    st_sampler_create = Module.cwrap("st_sampler_create", "number", ["number", "number"]);
    st_sampler_destroy = Module.cwrap("st_sampler_destroy", null, ["number"]);
    st_check_expression_string = Module.cwrap("st_check_expression_string", "boolean", ["number", "number", "number"]);
    st_sampler_set_expression = Module.cwrap("st_sampler_set_expression", "boolean", ["number", "number", "number"]);
    st_sampler_evaluate = Module.cwrap("st_sampler_evaluate", "number", ["number"]);
    st_sampler_step_time = Module.cwrap("st_sampler_step_time", null, ["number", "number"]);
    st_sampler_set_sample_rate = Module.cwrap("st_sampler_set_sample_rate", null , ["number", "number"]);
    st_sampler_set_sample_time = Module.cwrap("st_sampler_set_sample_time", null, ["number", "number"]);
    st_fill = Module.cwrap("st_fill", null, ["number", "number", "number"]);
    st_fill_interleaved = Module.cwrap("st_fill_interleaved", null, ["number", "number", "number", "number"]);
    st_fill_strided = Module.cwrap("st_fill_strided", null, ["number", "number", "number"]);
    st_fill_planar = Module.cwrap("st_fill_planar", null, ["number", "number", "number", "number"]);

    resolve_soundtoy_ready();
}

let sampler_left;
let sampler_right;

const sample_rate_default = 48000;
const channel_count_default = 1;  // mono
const expression_default = "sin(2*PI*t)";

async function setup_samplers () {
	await soundtoy_ready;

	console.log("Loaded soundtoy library");

	if (!st_initialize()) {
		console.error("Failed to initialize soundtoy library");
		return false;
	}

	sampler_left = st_sampler_create(TREE_INTERP, sample_rate_default);
	if (!sampler_left) {
		console.error("Failed to create sampler");
		return false;
	}

	sampler_right = st_sampler_create(TREE_INTERP, sample_rate_default);
	if (!sampler_right) {
		console.error("Failed to create sampler");
		st_sampler_destroy(sampler_left);
		return false;
	}

	st_sampler_set_sample_rate(sampler_left, sample_rate_default);
	st_sampler_set_sample_rate(sampler_right, sample_rate_default);

	// build the string
	const ptr = Module._malloc(expression_default.length + 1);
	Module.stringToUTF8(expression_default, ptr, expression_default.length + 1);

	let valid = st_check_expression_string(null, ptr, expression_default.length);
	console.log("expression", valid ? "valid" : "invalid");

	let set_success = true;

	set_success &= st_sampler_set_expression(sampler_left, ptr, expression_default.length);
	set_success &= st_sampler_set_expression(sampler_right, ptr, expression_default.length);

	if (!set_success) {
		console.error("Could not set sample expression");
		st_sampler_destroy(sampler_left);
		st_sampler_destroy(sampler_right);
		return false;
	}

	return true;
}

// ui elements
let button;
let expression_input;
let sample_rate_box;

function setup_ui() {
    button = document.getElementById("HelloButton");
    expression_input = document.getElementById("ExpressionInput");
    sample_rate_box = document.getElementById("SampleRateBox");

    button.addEventListener("click", () => { console.log("Hello"); });

    expression_input.setAttribute("placeholder", expression_default);
    sample_rate_box.setAttribute("placeholder", String(sample_rate_default));

    expression_input.addEventListener("change", event => {console.log(this.value)});
}

let audio;

function setup_audio() {
	audio = new StAudio(sample_rate_default, channel_count_default, sampler_left, sampler_right);

	audio.initialize();
}

function main() {
    setup_ui();
    setup_samplers();
    setup_audio();
}

main();
