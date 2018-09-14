import("stdfaust.lib");

up = nentry("[1] up", 0.1, 0, 9999, 0.0001);
down = nentry("[2] down", 0.1, 0, 9999, 0.0001);

process = _ : si.lag_ud(up, down);
