One from the "because you can" bucket for you.

Here is an AM Radio covering long and medium wave running on a dsPIC33AKMC102 28 pin microcontroller: antenna in, audio out. There are no other active devices between the antenna and the audio output. (OK, there is an I2C LCD display).

Almost all the processing is done using single precision floating point, including the quadrature local oscillator, mixer, low pass filtering, decimation, demodulation, and AGC. The dsPIC33A series has a trick up its sleeve for software defined radio: sin() and cos() are performed in silicon, each in 4 cycles, and you can reduce that to 1 cycle effective if you're smart with re-ordering your instructions in the pipeline.

31.4k bytes of compiled code, half of that is for the macro-expanded unrolled assembler.

73.6% of the CPU is taken up performing the signal processing.

Current draw at 3.3V: 69mA NICE!

I last did this on ARM M4F with an LPC4370 some years ago. That was rather  compromised because it was undersampling, at 2MSa/s, and it used about 95% of the CPU even then, with some hand-rolled assembler in the pinch points. I have used the same approach here, except for the filtering aspect, and I've added the non-essentials ;-) like a tuning knob and a display.

Code is attached, needs MPLAB X 6.25 and XC-DSC 3.21. It uses -O3 optimisation for some functions, but will also work with the free -O2 albeit with a little more CPU usage (76.1% up from 73.6%).

Here's an extract from a more detailed description in the projects the main.c heading, describing how the firmware works at a high level:

https://youtu.be/MSIYZMGDxqc?si=v69JGGJIZcqtkfic

https://www.eevblog.com/forum/projects/single-chip-am-radio-on-a-microcontroller

