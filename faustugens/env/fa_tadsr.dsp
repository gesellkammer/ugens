// import("stdfaust.lib");
en = library("envelopes.lib");

trig = nentry("[1] trigger",  0, -9999, 9999, 0.0001);
att  = nentry("[2] attack",   0.1, 0, 9999, 0.0001);
dec  = nentry("[3] decay",    0.0, 0, 9999, 0.0001);
sust = nentry("[4] sustain",  1, 0, 1, 0.0001);
rel  = nentry("[5] release",  0.1, 0, 9999, 0.0001);


process = en.adsr(att,dec,sust*100,rel,trig) : _;
