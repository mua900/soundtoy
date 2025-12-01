#pragma once

extern "C" {

    /**
     * @file api.h
     * @brief Public C API for the soundtoy core.
     */

    /**
     * @enum Evaluator_Type
     * @brief Selects the underlying evaluation backend for all samplers.
     */
    enum Evaluator_Type : int {
        BYTECODE_INTERP,
        TREE_INTERP,
    };

    /**
     * @struct St_Sampler
     * @brief Represents a single expression evaluator instance.
     *
     * Each sampler owns an evaluator implementation whose type is determined at the time of creation.
     */
    struct St_Sampler {
        Evaluator_Type evaluator_type;  /**< Evaluator backend used by this sampler. */
        void* evaluator;                /**< Opaque pointer to evaluator state. */
    };

    /**
     * @brief Initializes the soundtoy core library and selects an evaluator implementation to use.
     *
     * This should be called to initialize or reinitialize the library.
     *
     * @param evaluator_type  The evaluator backend that created samplers will use.
     * @return true on success, false if initialization failed.
     */
    bool st_initialize(Evaluator_Type evaluator_type);

    /**
     * @brief Returns a human-readable error message describing the last failure.
     *
     * Some API functions return only true/false and errors may occur without an explicit failure status.
     * Error logs are stored internally and retrieved here.
     *
     * @return Pointer to an internal null-terminated error string.  
     *         The caller must NOT free this memory.
     */
    const char* get_last_error();

    /**
     * @brief Creates a new sampler instance.
     *
     * The sampler starts with:
     *   - no assigned expression
     *   - sample time = 0
     *   - a default sample rate (implementation-defined)
     *   - an empty variable table
     *
     * @return Pointer to a new sampler, or NULL on failure.
     */
    St_Sampler* st_sampler_create();

    /**
     * @brief Destroys a sampler and frees all associated resources.
     *
     * @param sampler Pointer previously returned by st_sampler_create().
     */
    void st_sampler_destroy(St_Sampler* sampler);

    /**
     * @brief Validates an expression string.
     *
     * If sampler_or_null is null, only validates the syntax of the given expression.
     * Doesn't check or report errors on variable names.
     * If sampler_or_null is not null, additionally checks correct usage of registered variables.
     *
     * On failure, returns false and sets an error message.
     *
     * @param sampler_or_null  sampler; if null performs syntax-only validation.
     * @param expression_string  expression text.
     * @param length  Number of bytes in the expression string.
     * @return true if valid, false if invalid.
     */
    bool st_check_expression_string(const St_Sampler* sampler_or_null, const char* expression_string, int length);

    /**
     * @brief Compiles and sets the expression for the sampler.
     *
     * Variables used in the expression are looked for in the sampler's set of registered variables.
     * Use of unregistered variables will generate a compile error.
     *
     * @param sampler  sampler.
     * @param expression_string  expression text.
     * @param length  Number of bytes in the expression string.
     * @return true on success; false if compilation fails.
     */
    bool st_sampler_set_expression(St_Sampler* sampler, const char* expression_string, int length);

    /**
     * @brief Evaluates the expression once at the sampler's current sample time.
     *
     * This does NOT advance the sampler's internal sample time.
     *
     * @param sampler  Target sampler.
     * @return The computed floating-point sample value.
     */
    float st_sampler_evaluate(const St_Sampler* sampler);

    /**
     * @brief Sets the sampler's sample rate in Hz (samples per second).
     *
     * @param sampler  Target sampler.
     * @param sample_rate  Sample rate in Hz (samples per second).
     */
    void st_sampler_set_sample_rate(St_Sampler* sampler, float sample_rate);

    /**
     * @brief Sets the sampler's internal sample time measured in seconds.
     *
     * @param sampler  Target sampler.
     * @param sample_time  New sample time value.
     */
    void st_sampler_set_sample_time(St_Sampler* sampler, float sample_time);

    /**
     * @brief Gets the sampler's sample rate in Hz (samples per second).
     */
    float st_sampler_get_sample_rate(const St_Sampler* sampler);

    /**
     * @brief Gets the sampler's current sample time (in seconds).
     */
    float st_sampler_get_sample_time(const St_Sampler* sampler);


    /**
     * @brief Registers a new variable name for this sampler.
     * 
     * Caller must register variables before it tries to compile an expression that tries to use the variable.
     * So that the sampler will know the variable name and be able to compile the expression.
     *
     * Do not confuse this mechanism with the builtin variables built into the expression language.
     * Trying to register the following names will collide with a builtin variable name:
     *  time (or t)
     *  sample_rate (or sr)
     *  e (or E)
     *  pi (or PI)
     * Trying to register a variable with the name of a builtin variable will fail and generate an error.
     * 
     * Registered variables remain stable for the lifetime of the sampler.
     * Trying to re-register an existing name will return the id of the already registered variable but
     * will also set a warning message (retrievable via get_last_error()).
     *
     * @param sampler  Target sampler.
     * @param name     Variable name.
     * @param length   Length of the name in bytes.
     * @return A non-negative variable index on success or -1 on failure.
    */
    int st_sampler_register_variable(St_Sampler* sampler, const char* name, int length);

    /**
     * @brief Sets the value of a previously registered variable.
     *
     * @param sampler  Target sampler.
     * @param variable Index returned by st_sampler_register_variable().
     * @param value    New value.
     * @return true on success, false on invalid index.
     */
    bool st_sampler_set_variable_value(St_Sampler* sampler, int variable, float value);

    /**
     * @brief Gets the current value of a registered variable.
     *
     * @param sampler  Target sampler.
     * @param variable Variable index.
     * @return Current value. Returns NaN if index is invalid (check get_last_error()).
     */
    float st_sampler_get_variable_value(const St_Sampler* sampler, int variable);

    /**
     * @brief Returns the name of the variable at the given index.
     *
     * The returned pointer is owned internally; do NOT free it.
     *
     * @param sampler  Target sampler.
     * @param index    Variable index (0 ≤ index < count).
     * @return variable name, or null if index is invalid.
     */
    const char* st_sampler_get_variable_name_at_index(const St_Sampler* sampler, int index);

    /**
     * @brief Returns the number of variables registered for this sampler.
     */
    int st_sampler_get_variable_count(const St_Sampler* sampler);

    /**
     * @brief Fills a buffer with sequential audio samples.
     *
     * The sampler's internal time is advanced for each written sample by (1.0 / sample_rate).
     *
     * @param sampler  Target sampler.
     * @param buffer   Output buffer of at least `length` floats.
     * @param length   Number of samples to write.
     */
    void st_fill(St_Sampler* sampler, float* buffer, int length);

    /**
     * @brief Fills an interleaved stereo buffer using two samplers.
     *
     * Equivalent to:
     *   L = sampler0, R = sampler1
     * Buffer layout: [L, R, L, R, ...]
     *
     * Both samplers advance their sample time independently by (1.0 / sample_rate) using their own sample rates.
     *
     * @param sampler0  Left-channel sampler.
     * @param sampler1  Right-channel sampler.
     * @param buffer    Interleaved output buffer of length `2 * length`.
     * @param length    Number of stereo frames to write.
     */
    void st_fill_interleaved(St_Sampler* sampler0, St_Sampler* sampler1, float* buffer, int length);

}
