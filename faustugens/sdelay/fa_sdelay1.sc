FaustFaSdelay1 : UGen
{
  *ar { | in1, fadetime(0.01), delaytime(0.1) |
      ^this.multiNew('audio', in1, fadetime, delaytime)
  }

  *kr { | in1, fadetime(0.01), delaytime(0.1) |
      ^this.multiNew('control', in1, fadetime, delaytime)
  } 

  checkInputs {
    if (rate == 'audio', {
      1.do({|i|
        if (inputs.at(i).rate != 'audio', {
          ^(" input at index " + i + "(" + inputs.at(i) + 
            ") is not audio rate");
        });
      });
    });
    ^this.checkValidInputs
  }

  name { ^"FaustFaSdelay1" }
}

