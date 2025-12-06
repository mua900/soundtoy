async function load_soundtoy() {
	st_initialize = Module.cwrap("st_initialize", "boolean", []);
	st_sampler_create = Module.cwrap("st_sampler_create", "number", ["number", "number"]);
	st_sampler_destroy = Module.cwrap("st_sampler_destroy", null, ["number"]);
	st_check_expression_string = Module.cwrap("st_check_expression_string", "boolean", ["number", "number", "number"]);
	st_sampler_set_expression = Module.cwrap("st_sampler_set_expression", "boolean", ["number", "number", "number"]);
	st_sampler_evaluate = Module.cwrap("st_sampler_evaluate", "number", ["number"]);
	st_sampler_step_time = Module.cwrap("st_sampler_step_time", null, ["number", "number"]);
	st_sampler_set_sample_rate = Module.cwrap("st_sampler_set_sample_rate", null , ["number", "number"]);
}

load_soundtoy();

function say_hello() {
	console.log("Hello");
}
