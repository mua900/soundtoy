// evaluator type
const BYTECODE_INTERP = 0;
const TREE_INTERP = 1;

// soundtoy functions
let st_initialize;
let st_sampler_create;
let st_sampler_destroy;
let st_check_expression_string;
let st_sampler_set_expression;
let st_sampler_evaluate;
let st_sampler_step_time;
let st_sampler_set_sample_rate;

let soundtoy_ready;  // promise
let resolve_soundtoy_ready;

soundtoy_ready = new Promise((resolve) => {resolve_soundtoy_ready = resolve;})

Module.onRuntimeInitialized = function load_soundtoy() {
	st_initialize = Module.cwrap("st_initialize", "boolean", []);
	st_sampler_create = Module.cwrap("st_sampler_create", "number", ["number", "number"]);
	st_sampler_destroy = Module.cwrap("st_sampler_destroy", null, ["number"]);
	st_check_expression_string = Module.cwrap("st_check_expression_string", "boolean", ["number", "number", "number"]);
	st_sampler_set_expression = Module.cwrap("st_sampler_set_expression", "boolean", ["number", "number", "number"]);
	st_sampler_evaluate = Module.cwrap("st_sampler_evaluate", "number", ["number"]);
	st_sampler_step_time = Module.cwrap("st_sampler_step_time", null, ["number", "number"]);
	st_sampler_set_sample_rate = Module.cwrap("st_sampler_set_sample_rate", null , ["number", "number"]);

	resolve_soundtoy_ready();
}

let sampler_left;
let sampler_right;

async function create_samplers () {
	await soundtoy_ready;

	console.log("Loaded soundtoy library");

	if (!st_initialize()) {
		console.error("Failed to initialize soundtoy library");
		return;
	}

	const sample_rate_default = 48000;

	sampler_left = st_sampler_create(TREE_INTERP, sample_rate_default);
	if (!sampler_left) {
		console.error("Failed to create a sampler");
	}

	sampler_right = st_sampler_create(TREE_INTERP, sample_rate_default);
}

// ui elements
let button;
let expression_input;

function setup_ui() {
	button = document.getElementById("HelloButton");
	expression_input = document.getElementById("ExpressionInput");

	button.addEventListener("click", () => { console.log("Hello"); });
}

function main() {
	create_samplers();
	setup_ui();
}

main();
