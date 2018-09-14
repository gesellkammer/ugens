# Miscelaneous ugens for csound

## bpf

Breakpoint function (linear interpolation). Interpolates between the points (kxn, kyn).



    ky  bpf kx, kx0, ky0, kx1, ky1, kx2, ky2, ...



## linlin

Linear to linear conversion, similar to Supercollider's `linlin`.

Converts a value `x` defined within a range `kxlow - kxhigh` to the range
`kylow - kyhigh`


    ky  linlin kx, kxlow, kxhigh, kylow, kyhigh


## ftom, mtof

Frequency <--> Midi conversion with optional value for A4
(default=442)


    kfreq = mtof(69, 443)  ; the reference freq. is optional


## ntom, mton

Notename to midi conversion. Format used: `4C[+15]`

* Octave + notename. `4C`, `5Db`, `3A#`, etc
* `4C` is the central C on the piano
* Cents can be indicated as `4C+15` (15 cents higher), `4C-31` (31
cents lower)
* `4C+` is also accepted, and is the same as `4C+50` (the same is
valid for `4C-`
* Only uppercase is accepted


## pchtom

Convert octave.pitch to midi


    kmidi = pchtom(8.07)


## xyscale

2D interpolation between 4 presets, like a xy widget



    kout  xyscale kx, ky, k00, k10, k01, k11
 


Given 4 values corresponding to points at


    k10          k11



    k00          k10


interpolate the values according to kx, ky, 
where kx and ky are between 0-1


This is the same as:


    ky0 = scale(kx, k01, k00)
    ky1 = scale(ky, k11, k10)
	kout = scale(ky, ky1, ky0)

