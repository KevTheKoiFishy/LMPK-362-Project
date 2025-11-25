After over 20 hours of this I finally made… basically a very crude iPod! In case anyone else needs help getting this to work, I want to leave some notes about what worked and what didn’t. The actual implementation and design choices are up to you!

## Setup
### Audio Output Choice

For starters, my approach is using PWM to play audio. This is because if I drive one pin when the wave is positive and another when the wave is negative, with the duty cycles proportional to abs(wave), then I can input those two PWMs to an H-bridge and double the RMS voltage… four times the audio power!

![Back To The Future (Opening Guitar Amp Scene) - COMPLETE SOUND REPLACEMENT on Make a GIF](https://i.makeagif.com/media/3-04-2016/ldboDZ.gif)

In hindsight, it would have made much more sense to use a DAC… but whatever, I’ve already sent the PCB in. 🤷🏻‍♀️

### File Format Choice

Now I had to choose the file format: **WAV or MP3?** The trade off is WAVs need higher memory bandwidth for their lossless playback, but MP3s need more compute for decoding. As long as `memory-copy time + decoding time < sampling period`, **it really don’t matter.**

HOWEVER. After looking into how to decode MP3s (`Huffman decoding -> inverse cosine transform -> multiply-accumulate`) I thought I should start with WAV for debuggability and sanity reasons. I ended up getting a **memcpy time : playback time** of only 1/8 at 32 KSPS, and this is pretty good.

I chose to use WAV formats with 16 bit-depth, mono channel, and PCM encoding. This just means that after the file header, the audio samples are stored sequentially as little-endian signed `int16_t`s. For each sample, I

1.  get the sample rate from the file header,
2.  set the pwm period to as close as `1 / sample_rate` as possible,
3.  and then update the pwm period to the next sample on each wrap (Lab 5 ts).

For a 44.1kHz audio, the standard sample rate, this means the prescaler is 1 and the top is `(3401 - 1)`. This also means I scale my `[-32768, 32767]` audio sample by `3401.0 / 32767.0` (0 - 100% duty) for each side of the H-bridge. While it’s an 8x reduction in actual precision, it’s only 3 of the 16 bits, which means the precision loss you hear only reduces by 20%.

> _In general_, for any rate exceeding `floor(F_CLK / (1 << bit_depth))`, increasing sample rate reduces bit depth. (This value sits at `2,288` samp/sec for `int16_t`) **This is the main disadvantage of the PWM approach VS a DAC + Class D amp, which achieves the same result with some noise.**

![image.png](/redirect/s3?bucket=uploads&prefix=paste%2Flj4sq92apfjja%2F92a4491258127c74013774b2fbbe30a25dac93d381035c3b4bb292d2d6483887%2Fimage.png)

You can use \[online converters\] (https://www.freeconvert.com/mp3-to-wav) to change the bitrate, bit depth, etc. to fit your hardware constraints. I found that my setup works well at **32kHz, 16bit, mono**. After that, it tends to trip over itself - ie. there’s a small chance the memcpy doesn’t complete before the PWM ISR needs it, aka a buffer overrun.

### SPI clock speed

This one really tripped me up. You’d think that with a sample rate of 44.1 Khz, you’d need on the order of `8 bits / byte * 2 bytes / samp * 44.1e3 samp / sec`, so 1Mhz would cut it. However, this kept leading to buffer overruns, and I had to get it to 20Mhz to work. If anyone knows why, please enlighten me!

## Problems and Solutions
Development was not linear, I am but writing it so for presentability.

0. **CLOCK SPEED |** At first I could not get the sd card above 400kHz. I fixed this by writing 32 empty bytes with MOSI == HIGH after `sd_io_high_speed()` is called in `diskio.c` / `disk_initialize`. I just called `sdcard_init_clock();` 16 times. I’m also not sure why this works, but after adding it, I was able to get F\_sck all the way to 30Mhz.

<table><tbody><tr><td><img src="/redirect/s3?bucket=uploads&prefix=paste%2Flj4sq92apfjja%2Fdac26449fb0132d7fbd2ca1e817c9bfeea0688cb4af4dd68fa0a38b53c6a451d%2Fimage.png"></td><td><img src="/redirect/s3?bucket=uploads&prefix=paste%2Flj4sq92apfjja%2Fe3f2eabd2ea10341b9becbc3cd4904e2517536d8b67c494c4ea1184255644dc5%2Fimage.png"></td></tr></tbody></table>  
Yuhhh.
<br><br>

1. **AUDIO BUFFER |** The first thing I learned was that copying one sample at a time was slow; there’s overhead from going through calls stacks, loops, and accessing the file. When reading like this, the audio playback hardly kept up. I may not have perfect pitch, but it sounded worse than the piano next to WALC on a rainy day.  
On the other hand, reading chunks of 512-1024 bytes takes much less time per byte. So, instead of reading one sample from the file on each pwm-update-isr call, I created a circular FIFO buffer, tracking a read and write index. I raised a flag  in the ISR every time my buffer was 75% empty, and a function running in the main loop cleared the flag before filling it up. 

2. **COPY SIZE |** _HOWEVER_ beyond 1024 bytes at a time, I got hardware errors from FatFS, after which the SD card became inaccessible. If I wanted to consume more than 1024 bytes, I’d `f_read` multiple times, passing in an offset of the buffer each time.

3. **MULTICORE |** This is great, but now the SD read operation takes a good fraction of the buffer-playback time. So what? Well, that means the pwm-updating ISR will often interrupt the memcpy operation!  
SD cards are spoiled and become total princesses when it comes to timing, so this isn’t really ideal. (I’d often get glitches in audio when the memcpy operation gets thrown off).  
Besides, I wanted the main function to be busy doing other things, like – I don’t know – _rendering an entire display?!_ So I moved the poll-buffer-and-memcpy function to core 1, which got rid of most of the glitching.

4. **BUFFER SIZE |** I just tuned this one experimentally; 4096 and 8192 worked well. If you expect delays in reading from the SD, ie. from long wiring, give it more breathing space.

5. **DOUBLE BUFFERING |** Having the circular FIFO wasn't working really well, since the interrupt has to do a bunch of pointer math to figure out where to start and end copying, when to wrap around, etc. Which is DELAY. BAD. BOOO 👻.  
Therefore, I made a read (playback) buffer and write (memcpy) buffer, each 8192 in length (What was that? Oh, don't worry, the RP2350 has 512KB of space!) Now the memcpy function just has to fill up the write buffer from start to end, no wrap-arounds needed.
    - **The setup:**
        - Still have the PWM ISR read samples from the buffer and wrap around to 0 when it reaches the end.
        - Change the memcpy function to `0 -> buff_size` instead of `write index -> read index`.
        - Initialize a Buffer A, a Buffer B, a pointer to the read buffer, and a pointer to the write buffer. Assign one pointer to buffer A and the other to buffer B.
        - Initialize the memcpy flag to true.
        - Have the PWM ISR set the memcpy flag when the read index is at 0 rather than 75% of the way. Also, when read index is 0, swap the read and write buffer pointers.
    - **The process:**
        1. The memcpy flag is initialized to `true`, so as soon as the audio file opens and the `audio_file_open` flag is set, core1 fills the write buffer.
        2. When the read index reaches zero, the read and write buffers are swapped and the memcpy flag is set. Core0 plays what was initially written by Core1, while Core1 prepares the next buffer for Core0.
        3. The process repeats until the song is over, ie. when the file-offset pointer `&audio_file -> fptr` exceeds the filesize, which can be found in the WAV file header or by running `f_size(&audio_file)`.
<br><br>
6. **HARDWARE IMPROVEMENTS |** Even after all that, I was getting periodic glitching. I wasn't able to pinpoint what wasn't being copied using the debugger, and it would only happen sporadically. I thought the long wires between Pi and Display module might be the culprit, so I soldered the pins of a TF-to-SD adapter to short jumper wires and used that instead of the display SD slot. No problems so far.

## Results
Here's a before and after.
<table>
<tr>
<td> Before <b>(VOLUME WARNING)</b> </td> <td> During </td> <td> After </td>
</tr>
<tr>
<td>
<video height="300" controls>
  <source src="/redirect/s3?bucket=uploads&amp;prefix=paste%2Flj4sq92apfjja%2Fbab594b9123cea66a9ada61378599e2250a0d67be978048ed859affb52cb81f0%2FIMG_3299.mp4" type="video/mp4">
</video>
</td>
<td>
<video height="300" controls>
  <source src="/redirect/s3?bucket=uploads&amp;prefix=paste%2Flj4sq92apfjja%2Fdf9a8fb0222346a54379998bff99a8366d6ca67caed4ef2266eb1e69799ea6d2%2FIMG_3327.mp4" type="video/mp4">
</video>
</td>
<td>
<video height="300" controls>
  <source src="/redirect/s3?bucket=uploads&amp;prefix=paste%2Flj4sq92apfjja%2Fa9b92adaf7f5c627dd0be54747a4194046cf4a748687be5e299638a4c2cb165d%2FIMG_3387.mp4" type="video/mp4">
</video>
</td>
</tr>
</table>
<br>
I hope this helps.