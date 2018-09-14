import("stdfaust.lib");

N = 48000 * 4;  // max delay

it = nentry("[1] fadetime", 0.01, 0, 0.9, 0.0001) * ma.SR;
dt = nentry("[2] delaytime", 0.1, 0, 1,   0.0001) * ma.SR;

process = _ : de.sdelay(N, it, dt) : _;