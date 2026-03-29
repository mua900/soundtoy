# HOWTO

This files gives on overview of what expressions you can type in to the application.

simple sine at 440 Hz:
- sin(t * tau * 440)

other waves at 440 Hz:

- triangle(t * 440)
- square(t * 440)
- saw(t * 440)

Sound is how a human brain perceives pressure waves propagating through the air.
We can represent sound as a signal with the signals amplitude representing at each point corresponding to the strength of the pressure wave at that particular point in time.
A point in the signal corresponds to a point in time so we say the the signal is in time domain.
Analog system may be able to use analog continious signals but digital systems can't.
So to represent the same signal we can sample the continious signal at discrete equidistant points in time and store the samples which gives us a digital signal.
And we can later reconstruct the original signal from the samples as long as we have sufficiently many of them which can be useful if we are trying to give that signal to an analog system for example.
So sound can be represented as a signal and a signal can be described as a mathematical expression telling what value the signal should take at a given point.

Which is what this project is about. You type a mathematical expression and the app gets it's samples from evaluating that expression which is then eventually given to sound hardware which converts the digital signal into an analog signal and then to mechanical pressure waves that you can hear.

This part gives details about the syntax and semantics of the expression language.

Expression language accepts mathematical expressions.
It supports arithmetic operations you would expect it to support.
Like addition, subtraction, multiplication, division, modulo (with or without floats). Also comparisons which produce boolean values and ternary operator.
The expression language defines several symbols that it knows how to interpret and you can use them in an expression.
The most important one is time. Written literally as "time" or abreviated as just "t". As the name suggest it represents time. It starts at 0 and increments (1 / sample_rate) between each sample.
There is a builtin variable to access the current sample rate. Written as literally in snake case as "sample_rate" or abreviated as "sr".
There is a builtin variable for the current sample index. Written as "sample_index" or "si".
And finally there is a builtin variable for an input sample. It is the audio sample taken at the same sample index position as the executing expression. Coming from a loaded file which is called an input stream. You can access it via typing "input_sample" or it's abreviation "s".

There are builtin functions that you can use. Most of them are very standard functions that shouldn't require detailed explanation.
They include:

- exp(x)           -- exponential function or e^x
- abs(x)           -- absolute value
- sign(x)          -- sign of x
- ceil(x)          -- ceil function
- floor(x)         -- floor function
- sin(x)           -- sine
- cos(x)           -- cosine
- tan(x)           -- tangent
- asin(x)          -- inverse sine
- acos(x)          -- inverse cosine
- atan(x)          -- inverse tangent
- fract(x)         -- fractional part of x
- smoothstep(x)    -- smoothstep between 0 and 1 of x (cubic same as in some shader languages)
- mix(a, b, t)     -- linear interpolation between a and b with interpolation parameter t
- saw(x)           -- sawtooth wave
- square(x)        -- square wave
- triangle(x)      -- triangle wave
- clamp(x, a, b)   -- clamp x between a and b
- pow(x, y)        -- x to the power y
- log(x)           -- natural logarithm of x
- min(x, y)        -- min
- max(x, y)        -- max

If you know a little bit of C++, it is trivial to add new ones to this existing set. Take a look at st/evaluation/builtin.h if you are interested.
