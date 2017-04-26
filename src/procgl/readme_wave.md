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
You can sample a basic wave like this:
```
    float sample = pg_wave_sample(my_wave_ptr, 3, (vec4){ x, y, z });
    float sample = pg_wave_sample(&PG_WAVE_PERLIN(), 2, (vec4){ x, y });
    float sample = pg_wave_sample(&PG_WAVE_PERLIN(.frequency = { 2, 2 }),
                                  2, (vec4){ x, y });
```

Built-in wave defs allow you to specify non-default wave parameters.
The available parameters are: `scale` and `add`, which are applied to the final
sample (scale, then add); `phase[4]`, which are added to each of the sample
point's coordinates; and `frequency[4]`. `frequency` and `scale` always default
to 1, while `add` and `phase` default to 0.

## More complex wave generators
To get more complex behavior, you will need to combine multiple wave
objects, using wave arrays.  Construct a wave array like (this is 3 octaves
of sine wave, with faint perlin noise):
```
    struct pg_wave w[] = {
        PG_WAVE_PERLIN(.scale = 0.1),
        { PG_WAVE_MODIFIER, .mod = PG_WAVE_MOD_OCTAVES,
            .octaves = 3, .ratio = 2, .decay = 0.5 },
        PG_WAVE_SINE() };
```

Then sample the array like:
```
    float sample = pg_wave_sample(PG_WAVE_ARRAY(w, 3), 1, (vec4){ s });
```

You can see how the `PG_WAVE_ARRAY` macro logically collapses the array of
wave objects into a single conceptual wave function. Unless a mixing modifier
is used, waves are simply added to each other, and the array is processed in
sequential order, with modifiers affecting the waves after them in the array.

### Using wave modifiers
Modifiers can dramatically change the behavior of a wave function. For example,
the `PG_WAVE_MOD_SEAMLESS_2D` modifier can produce a seamlessly repeating 2d
wave from any 4d noise function, and `PG_WAVE_MOD_EXPAND` can expand a naturally
1d wave function (like a sine wave) to multiple dimensions.

For example, to get a sine-wave with more than just one dimension, this
modifier allows you to specify how the coordinates should be transformed into a
single sample point. For example to get a n-d sine wave of the form
`(sin(x) + sin(y) + ...)`, construct the wave array like this:
```
    struct pg_wave w[] = {
        { PG_WAVE_MODIFIER, .mod = PG_WAVE_MOD_EXPAND,
            .op = PG_WAVE_EXPAND_ADD, .mode = PG_WAVE_EXPAND_AFTER },
        PG_WAVE_SINE() };
```

Or to instead have `(sin(x * y * ...))`, construct it like this:
```
    struct pg_wave w[] = {
        { PG_WAVE_MODIFIER, .mod = PG_WAVE_MOD_EXPAND,
            .op = PG_WAVE_EXPAND_MUL, .mode = PG_WAVE_EXPAND_BEFORE },
        PG_WAVE_SINE() };
```

Members in a wave array can themselves be wave arrays as well, allowing
whatever branching system of wave functions, mixing, distortions, etc.
that you might want to create. Even without this, though, just building
a single wave array using the available modifiers is still quite powerful.

### Creating new wave functions

To build your own wave function, just fill in a `struct pg_wave`'s
`func1`, `func2`, `func3`, and/or `func4` members with function pointers,
and `dimension_mask` with a bitmask indicating which dimensions your wave
function supports. The lowest bit is 1d, second bit is 2d, and so on. Look at
`wave_defs.h` to see how the built-in wave functions are defined.

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
    * `mix`, a function pointer with the signature float(float,float)
* `PG_WAVE_MOD_DISTORT`
    * `dist_v` a vec4 used to modulate the distortion
    * `distort` a function pointer with the signature
        void(vec4 out, vec4 in, vec4 dist_v)

TODO: add spherical, cylindrical, and cubic mappings.

