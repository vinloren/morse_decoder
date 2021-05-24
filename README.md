### ESP32 Morse decoder
This is a succesful attempt to build a morse decoder based on ESP32 microcontroller and a GY-MAX4466 Electret microphone directly connected to ESP32 ADC 35 pin to pick up the audio signal coming from the radio set.

In order to detect the dash/dot note we usually should rely on a FFT (Fast Fourier Transormation) algorithm or either to a Goertzel's one, here instead I chose a far more simpler approach that consists in measuring the dot/dash time as long as the ADC value is above the noise threshold. I could use this approach since the frequency of the signal tone is irrelevant.

The testMark() function show how I devised this. This function returns the time span where a dot or a dash was present (dash time span should be 3 times the dot's). Similarly the time span between dot/dash is measured by testSpace().

This arduino sketch allows the selection of 3 different speeds you can choose before pushing the "run button" by pushing the "speed set button": 15wpm, 25wpm, 40wpm. 
