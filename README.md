## PrettyEQ

prettyeq is a system-wide paramateric equalizer for pulseaudio. This software
is in alpha. Use at your own discretion.

![prettyeq demo](https://i.fluffy.cc/gH3TjhjzV1rCH0Pdhwjxh6MxRqB39j05.gif)

### Usage

When the program is executed all pulseaudio streams will be routed through the
equalizer. With no filters activated prettyeq acts as a passthrough sink.

Right now prettyeq only supports two-channel sound.

##### Filter types

prettyeq has three filter types:
* one **low shelf** filter mounted at 20Hz
* one **high shelf** filter mounted at 20kHz
* five **peakingEQ** filters that can move freely

##### Controls

Click and drag points to boost and cut frequency bands. The dB gain range is
±12dB. Filter bandwidth and slope can be changed with the mousewheel.

##### Quitting

If your desktop has a system tray, the close button will hide the GUI but the
equalizer will still be in effect. There are context menus in the application
and tray that have a "exit" option to quit the application.
