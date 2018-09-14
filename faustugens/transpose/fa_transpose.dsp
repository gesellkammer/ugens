import("stdfaust.lib");
sr = ma.SR;
winlength_samples = nentry("[1] winlength_secs", 0.040, 0.001, 0.250, 0.0001) * sr;
xfade_samples = int(nentry("[2] xfade_secs", 0.010, 0.0001, 0.250, 0.0001) * sr);
shift = nentry("[3] shift", 0, -12, +12, 0.01);

process = _ : ef.transpose(winlength_samples, xfade_samples, shift) : _;
