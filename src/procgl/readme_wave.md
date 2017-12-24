# pg\_wave

This is a generic 1, 2, 3, and 4d wave function handler.
It allows constructing complex wave generators, with modifiers to generate
seamless 1d and 2d noise from any 4d noise source, or to expand 1d functions
to higher dimensions by several methods, apply distortion functions, and
several other features. At the same time it is very small, consisting of
just one structure (`struct pg_wave`), and one function (`pg_wave_sample`).

Each `struct pg_wave` is either a constant (the simplest), a wave function
(with function pointers for each dimension it supports), a modifier, or
an array (just a pointer to a user-supplied array of other waves).
A wave array can be sampled as a whole, in which case the modifier wave
types can be used to change the behavior of the wave functions.

Note that this is not intended for use as a real-time wave generator, so
I will not be putting much effort into making it blazing fast, as the
main focus is feature robustness. Ideally wave generators will be used
primarily at game launch to generate other assets, rather than being used
extensively in real-time (for example, if you're trying to use this to
generate audio in real-time, don't get mad if it turns out to be too slow).

## Basic usage:
You can sample a basic wave function like this, using
`pg_wave_sample(struct pg_wave* wave, int dimensions, float* p)`:
```
    /*  As a variable   */
    struct pg_wave my_wave =
        PG_WAVE_FUNC_PERLIN(.frequency = { 4, 4, 4, 4 }, .phase = { 1, 1, 1, 1 },
                       .scale = 0.5, .add = 0.5);
    float 4d_sample = pg_wave_sample(&my_wave, 4, (vec4){ x, y, z, w });
    /*  Or directly to the sample function */
    float 3d_sample = pg_wave_sample(&PG_WAVE_FUNC_PERLIN(),
                                  3, (vec3){ x, y, z });
    float 2d_sample = pg_wave_sample(&PG_WAVE_FUNC_PERLIN(.frequency = { 2, 2 }),
                                  2, (vec2){ x, y });
    float 1d_sample = pg_wave_sample(&PG_WAVE_FUNC_SINE(), 1, &x);
```

`PG_WAVE_*` macros all evaluate to a single `struct pg_wave`. With only one
`struct pg_wave`, you are limited to a single sample of a single wave function
(see built-in wave functions below). However, all waves have four attributes
which can always be set with additional *designated* arguments:
`vec4 frequency`, `vec4 phase`, `float scale`, and `float add` (listed here
in the order they are applied). `phase` and `add` default to 0, `frequency` and
`scale` to 1. Note that if you designate a `frequency` or `phase`  argument
with fewer than 4 members, the unspecified members will revert to 0. Also note
that none of these macros will result in any dynamic memory allocations so
no cleanup code will ever be required after using them.

The built-in basic wave functions are:
*   `PG_WAVE_FUNC_SINE()`
*   `PG_WAVE_FUNC_SAW()`
*   `PG_WAVE_FUNC_TRIANGLE()`
*   `PG_WAVE_FUNC_SQUARE()`
*   `PG_WAVE_FUNC_PERLIN()`
*   `PG_WAVE_FUNC_DISTANCE()`
*   `PG_WAVE_FUNC_MAX()`
*   `PG_WAVE_FUNC_MIN()`

## More complex wave generators
To get more complex behavior, you will need to combine multiple wave
functions, using wave arrays. Construct a wave array like (this is 3 octaves
of sine wave, with faint perlin noise):
```
    struct pg_wave w[] = {
        PG_WAVE_FUNC_PERLIN(.scale = 0.1),
        PG_WAVE_MOD_EXPAND(.op = PG_WAVE_EXPAND_ADD, .mode = PG_WAVE_EXPAND_AFTER),
        PG_WAVE_FUNC_SINE() };
    float sample = pg_wave_sample(&PG_WAVE_ARRAY(w, 4), 3, (vec3){ x, y, z });
```

You can see how the `PG_WAVE_ARRAY` macro logically collapses the array of
wave objects into a single conceptual wave function. Unless a mixing modifier
is used, waves are simply added to each other, and the array is processed in
sequential order, with modifiers affecting the waves after them in the array.

Also note the use of the `PG_WAVE_MOD_EXPAND` modifier. This modifier expands
one dimension up to 2, 3, or 4, in a way described by its `op` and `mode`
arguments. The available operations are `PG_WAVE_EXPAND_ADD`, `...SUB`, `...MUL`,
`...DIV`, and `...AVG`. The available modes are `PG_WAVE_EXPAND_BEFORE` and
`...AFTER`. The two combined describe the format of the transformation used
to expand the coordinate: `ADD, BEFORE` results in func2(x + y, x + y),
while `MUL, AFTER` results in func1(x) * func1(y); `ADD, AFTER` with a sine
wave (sin(x) + sin(y)) produces a field of circular blobs. This wave function
might be useful for generating an infinite grid of circular islands. Read on
for more info about the various wave modifiers and how to use them.

### Using wave modifiers
Modifiers can dramatically change the behavior of a wave function. For example,
the `PG_WAVE_SEAMLESS_2D` modifier can produce a seamlessly repeating 2d
wave from any 4d noise function, and `PG_WAVE_MOD_EXPAND` can expand a naturally
1d wave function (like a sine wave) to multiple dimensions.

## Available modifiers
The available modifiers and their parameters are:
* `PG_WAVE_MOD_EXPAND` to expand 1d waves to higher dimensions.
    * `op` can be `PG_WAVE_EXPAND_(ADD|SUB|MUL|DIV|AVG)`
    * `mode` can be `PG_WAVE_EXPAND_BEFORE` or `PG_WAVE_EXPAND_AFTER`
* `PG_WAVE_MOD_OCTAVES` samples several octaves of a wave.
    * `octaves` is the number of octaves to sample
    * `ratio` is the frequency ratio of each next octave
    * `decay` is the amplitude decay of each next octave
* `PG_WAVE_MOD_SEAMLESS_1D` and `PG_WAVE_SEAMLESS_2D`
    * These modifiers don't have any parameters. They work by transforming
        1d or 2d coordinates to map to circles in a higher-dimensional space,
        producing a subtle re-characterization of the original function which
        repeats seamlessly. For 1d, x is mapped to a circle on the XY plane of
        a 2d noise function, and for 2d, x is mapped to a circle on XY, and
        y is mapped to a circle on the ZW plane of a 4d noise function.
* `PG_WAVE_MOD_MIX_FUNC`
    * `mix`, a function pointer with the signature `float(float,float)`
* `PG_WAVE_MOD_DISTORT`
    * `dist_v` a vec4 used to modulate the distortion
    * `distort` a function pointer with the signature
        `void(vec4 out, vec4 in, vec4 dist_v)`

TODO: add spherical, cylindrical, and cubic mappings.

