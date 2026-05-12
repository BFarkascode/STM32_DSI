# STM32_DSI
We are going to implement a DSI driver on an STM32U5 device.

## General description
I am sure that anyone who is tinkering with vision systems have come across the name “MIPI alliance” without necessarily understanding, what it means. I certainly was one of those people, looking at “MIPI” thinking that it is some kind of advanced transfer protocol for screens and cameras which just wouldn’t need to bother me much when playing around with microcontrollers.

To be fair, my assessment has been correct: most mcu-s will never have the capacity to driver or use MIPI neither for screens nor for cameras.

Why bother then?

Well, here is the thing: if someone wants to get a hold of a screens that are not capped at 10 fps using SPI, the available alternatives are very limited and rapidly disappearing from the market (i.e. serial screens using LTDC or FMC). Thus, if one wants to have flexibility regarding the screens they are planning to use for a project – and churn out a visually pleasant refresh rate of more than 30 fps – soon enough the only alternative will be using MIPI screens. So, let’s take a look into them, shall we?

Of note, below I am sharing a bare-bones HAL project that drives the DSI screen on the U5G9 Disco board as well (technically, the same as one of the DSI example projects provided by ST, minus everything we don’t need for a simple drive project), followed by a custom driver of my own making.

To showcase the custom driver, we will close with a small game using the DSI screen and touch capabilities. Yes, it will be the classic game: Snake.

### MIPI
First and foremost, what is MIPI? MIPI (abbreviation for Mobile Industry Processor Interface) is an interface specification standard developed by the MIPI Alliance that is widely used for smart phones and tablets to drive screen and extract information from cameras. It can have up to 4 data lines allowing parallel transfer and can run up to a few hundred MHz. It is an LVDS signal, using 1V1 instead of 3V3. It is stable, reliable and scalable.

Due to this standardisation, it is slowly pushing any alternatives regarding camera driving (called Camera Serial Interface or CSI) and screen driving (called Display Serial Interface or DSI for short) out of the market.
Now, most devices already come with a MIPI stack – and MIPI PHY – installed on them, making the standard very easy to use (say, on a Raspberry Pi). These “most devices” don’t cover microcontrollers though, simply because they mostly have none of the physical capabilities regarding speed and voltage levels that would be necessary to generate the appropriate signals for the DSI bus.

Luckily, there are some microcontrollers which are beefy enough to interface with at least a MIPI DSI, such as the STM32U5 we will be looking at here. This U5 device can clock up to 160 MHz, has a designated SMPS switching power supply to generate the 1V1 for the DSI on the chip and has the DSI PHY included in it as well. It has 3 MB of RAM, allowing dynamic image buffering of incoming image data, something we struggled with within the F429 digicam project. It is also available as a discovery board; thus, we will be able to grab one and develop on that one directly, no need for custom hardware.

(Of note, the newer ESP32 chips have both CSI and DSI, making them capable to run a screen and a 5 MP camera at the same time, as it is beautiful demonstrated by the Adafruit Memento…but, as usual, I don’t want to use existing solutions in favour of building my own. Less fun.)

Anyway, what we should try to do here is build a screen driving solution that will be able to refresh with, at least, 30 fps.

### Devil in the details?
Well, now that we have that out of the bag, what is so specific about MIPI DSI that it demands a separate repo? After all, we have already used multiple screen-driving solutions – FMC and LTDC to be precise – why would this be any different?

Well, because of the details. MIPI is a standard and an advanced one. It is significantly more complex than the ones we were using before. For example, in STM32-s, it is directly using the LTDC to manage the data flow and have the DSI HOST driver put on top of it to communicate with the screen…which, of course, will have its own driver structure to be complied to. In other words, it won’t be just a driver: it will be a driver stack.
To make things even worse, the documentation is dense and complicated to follow (in comparison, LTDC documentation is 30 pages in the refman, DSI is 130 pages), there aren’t a lot of working STM32 DSI project out there to look for hints and explanations and even the ones we do have rely on HAL to do the heavy lifting instead of doing a deep dive into the registers using bare metal.

I ended up using one of the official ST examples for the disco board we have for the U5 as a guideline eventually (I have the STM32U5G9-DK1, but the DK2 should work fine as well). As the very first exercise, we will do the same thing we did when we explored the BLE capabilities of the STM32WB5: we will strip down the existing example to the bare minimum to just publish an image and nothing else. Once we have that, we will be able to use it as a template and replace each HAL function with our own custom version. (This stripped HAL project is shared.)

### Stripped, but not naked
I don’t want to bore anyone with the details, but the STM32U5 DSI examples have waaaay more in them than what we need. I took the “DSI_VideoMode_SingleBuffer” example and then extracted from it everything we need to build our own HAL-based DSI solution locally (so not reaching back towards the official drivers and BSPs).

So, what files will we need to build our own version of the project?

####	.ioc
The most apparent element we will be setting up is the “.ioc” file (i.e, CubeMx). Now, the ideal solution would be to just take the existing “.ioc” from the example project and then clone it for our new project. Unfortunately, this will not work either because there are some integration elements within the example project that is not transferred this way, or because the example project is written for an U5A9 and not for the U5G9 (mind, the A9 is the previous version of the G9 that is not available anymore).

Long story short, we will need to set up our own “.ioc” by hand from scratch by:
o	enabling HSE
o	set RCC parameter settings
o	enable LTDC and configure it for 24 bits RGB888 DSI mode
o	enable DMA2D and configure it
o	enable DSIHost at and configure it for video mode
o	set PWR, especially SMPS
o	set up a GPIO pull-up on the screen reset pin on PD5

We will also need to set the clock config to:
o	run using PLLCLK at 160 MHz
o	PLL2 on HSE running PLL2R at 20.8333 MHz (can be omitted if we aren’t using PLL2 as LTDC source)
o	PLL3 on HSE running PLL3R at 20.8333 MHz ay PLL3P at 62.5 MHz
o	set the LTDC clock as PLL3R (can be PLL2R as alternative)
o	clock the DSI lane byte clock at 62.5 MHz using either the HSE or the PLL3P (this could be switched to the local DSI PLL, but isn’t necessary just to make the code work)
o	clock the DSI txclkesc clock at 15.625 MHz by prescaling the lane byte clock

Mind, this is just a clumsy recreation of something that works. Also, the “.ioc” we will generate this way will be transferrable, unlike the original one.

####	lcd driver
We need to carry over the general stm32 LCD driver header file (lcd.h). This is the general lcd driver that will be in the “BSP” section of the project repository (something like Drivers/BSP/Components/Common).

####	stm32_lcd
This is the second driver we will need to implement. It will not be stored in components but in “utilities”, usually within the device repository root folder. This is a generic driver for the lcd screen. Mind, the “stm32_lcd” driver uses the fonts for certain functions, so we will need to include the “font.h” header and different font source files somewhere as well, unfortunately.

####	main
We can clone the “main.c” and its header from the original project, but we will need to make some small modifications: we will have to include the “lcd.h” header somewhere and remove the “stm32ux9j_discovery.h” header. We can also cut many support functions, just as the red LED control and the text publishing from the screen since we won’t need them and doing so makes the code a lot easier to follow afetrwards.

####	image header files
We also have two image header files which store the two stock ST image in RGB888 format. It is best to carry those over as well and save us the trouble of generating our own example images.

### HAL project analysis
We now have our own working version of the original project with all the unnecessary bits removed. Let’s go through step-by-step, what is going on in this project, shall we?
-	We define the LCD driver handler (LCD_UTILS_drv_t). Mind, this handler is a function pointer (!) struct, so we will have to fill it up with functions (see below).
-	We define the image array as static constants (in FLASH, not RAM).
-	We have the stock HAL drivers for clocking, GPIO, DMA2D, LTDC and DSIHOST (this latest one having a user-added element where the clocking is defined, see below). We will replace these in out custom solution.
-	We have a handful of supporting functions:
o	Copybuffer: engages the DMA2D and transfers the image to the LCD buffer
o	LCD_Set_Default_clock: sets DSIPHY clocking and resets the LCD
o	SetPanelConfig: this is the LCD configuration command set
o	LCD_FillRect: support function for the LCD_UTILS handler. We will call this through the driver. (On the original example, we have a lot more, but we don’t need them so, they are cut.)
-	We have the main loop then calling all the HAL config followed by the LCD configuration function.
-	Next, we load the function pointers into the LCD driver handler.
-	Then we glue the STM32_LCD driver(stm32_lcd.h) to the LCD driver (lcd.h).
-	We wipe the LCD to black using the UTIL_LCD_Clear function (which calls UTIL_LCD_FillRect function, which calls the LCD_FillRect support function from above).
-	We finish by calling the Copybuffer function.

As we can see, there is a whole lot of generalisation and drivers calling drivers that we can further simplify.

We can replace the “UTIL_LCD_Clear” function with its support function counterpart (instance will be Null or just 0). Mind, “FillRect” manipulates the LTDC, not the DSI. It technically just change whatever is loaded to the LTDC by adjusting the memory addresses directly (the layer_0 start address is “hltdc.LayerCfg[0].FBStartAdress”, which should be “0x200d0000” if we properly copied the “.ioc” from the example project).

By the way, the “CopyBuffer” function also just manipulates the memory of the LTDC by using DMA2D.

### We can rebuild it, we have the technology…
So then, from the analysis above, what needs to be done to make our own, non-HAL version of this project?

Well, as I see, we need to do the following:
-	Do clocks in the U5 (PLLs included)
-	Calibrate DMA2D and rewrite Copybuffer function
-	Port LTDC to U5 from a previous project and rewrite RectFill function
-	Set DSIHOST
-	Configure the screen
	
Let’s get to it then!

#### Clocking
We will need to set the clocking up from scratch. Mind, we “could” just let CubeMx do it (I haven’t had any problems with doing it beforehand), but who could say no to a bit of appetizer before the main dishes?

Anyway, the setting up is rather straight forwards, except for the fact that we have 3 (!) PLLs in the U5 and we will need to select, which ones we will be using. My selection goes with PLL3 since it can drive the LTDC and the DSI as well, meaning we won’t be using up resources where not necessary.

Mind, the DSI also has a separate PLL that will drive the DSI PHY physical interface. This we will have to set up as well, albeit within the DSI configuration function.

Of note, the original test code that we will be modifying/bare-metalling forces the DSI to run on its own PHY PLL in the “Set_LCD_Default_Clock()” function, despite the fact that the CubeMX setup defines it as PLL3. I am not sure, why this is so, though playing around with the calibration indicates that the DSI may need to run on external PLL before it could transition to its own PLL. This is also supported by the recommended DSI setup from page 1792 of the refman.

#### DMA2D
The DMA2D will do the data loading into our frame buffer, thus making it the first element in the DSI stack.

As the name suggests, DMA2D is a DMA that is dedicated to 2D elements, i.e. images. It is (primarily) a memory-memory device running on the AHB clock. It is technically a DMA that can be programmed to move around in a 2D array of data points in a specific manner (unlike normal DMA, which runs straight from a specific memory point until stopped). This allows the DMA2D to jump around in an image, updating only a section of it compared to the entire image (as we would do with normal DMA). Obviously, this makes it significantly more efficient compared to normal DMA.

It can also do blending operation from two different layers, allowing a smooth transition from one frame buffer to the other (LTDC, the screen driver solution can also have two different layers by the way, so is the DSI Host).

In order to do a one-to-one replacement for the existing HAL driver, checking the initiation function that HAL has generated, we need to initiate the DMA2D as memory-to-memory, set colour mode output as ARGB888, no byte swap and output offset expressed in pixels (we will be define the output offset as 0, but will overwrite that value in the CopyBuffer to adjust to the image we want to put on the screen). We will need to initiate the input layer as well, which will be set up the input and the output as ARGB888, no input offset, normal alpha mode, regular alpha and regular colour swap and an alpha of 0xFF. Here we won’t need to change the input layer parameters once set.

The Copybuffer function then will do a DMA2D start function, taking in the source pointer, the destination pointer (pointing to the layer frame buffer but offset with the position where we want to place the image on the screen) and the size of the image. Mind, the DMA2D will have to be configured to cater to the image size in the “x” direction, the output offset defining, how much the DMA2D will have to “skip” after a line of the image has been transferred. Once the DMA2D is running, we can either poll for the TC flag to be set or use an interrupt (the example uses polling).

Anyway, configuring the DMA2D is rather straight-forwards in bare metal. We will be using only the front layer (registers with “FG” in the name) as input, give the layer a constant 0xFF alpha (FGPFCCR), point the layer to the images stored in FLASH (FGMAR) and then set the output with the necessary offset (OOR - here, 160 since both images are 320 pixels…note, we define the offset in pixels by leaving the appropriate bit as “0” in the CR) and define the size we wish to update within the frame (NLR) and define the destination (OMAR, which will be the destination frame buffer, plus the start offset). We launch the DMA2D by setting the appropriate bit in CR and then poll for the ISR flag (or wait for the ISR to trigger…here we won’t set it). In the end, we can remove the HAL init function for the DMA2D and simplify the “Copybuffer” function to a mere 8 lines of bare metal code.

Mind, we aren’t doing any pixel transformation, both the input and the output of the DMS2D are 32-bit ARGB888 pixels. Nevertheless, this DMA can do the transformation for us, should we set it up as such. We can also use the AMTCR register to add some dead time between DMA2D AHB accesses or put automated colouring to the layer using in-built colours.

Also, one last things: the DMA2D can indeed place data anywhere in a matrix, but as we clone it from the HAL solution, it will not extract data from anywhere in a matrix, meaning that we while the output pointer can be moved around at our leisure, the input will not. Should we wish to set it as such, we will have to manipulate the FGOR register of the DMA2D which adds a line offset on the input side. This means that any time we wish to replace a section of an image, the input side of the DMA2D will have to be serialised.

#### LTDC
Suffice to say, the LTDC is the next element in the stack, one that will take the frame buffer(s), serialise them and then send it to the screen…or in this case, the DSI PHY. (Of note, the U5 I have only can do LTDC to DSI and has no direct LTDC outputs, but I do know that some STM32 chips can have both.) We already had an LTDC project, so I won’t go into deep detail over what we need to do with it. The drive of the LTDC is completely identical to what was done using the F429 anyway (except the clocking).

Just to give a recap, the LTDC is a serialising peripheral that has its own DMA (which is likely the DMA2D by the way, just accessed through the LTDC peripheral) and clocking. It uses AHB for data transfer to the LTDC layer, APB to clock the registers AND it has a designated PLL that generates the pixel clock. It can handle two layers and blend them. It will ALLWAYS read along the Y axis of the frame first, meaning that it will read a 320x240 resolution image in 240-pixel chunks. This we must keep in mind and calibrate for when using a screen, otherwise we will not read out the frame buffer the same way we are loading it. (Luckily, the U5 disco board has a 480x480 resolution screen, so here we won’t be bothered by this particularity).

Anyway, we will need to enable the clocking to the LTDC interface, then set the PLL clocking (can have a designated one in the form of PLL2, or we can leave it on PLL3 like the DSIHOST will be), followed by setting the screen parameters and the layer positions/parameters. In general, we can simply port the existing LTDC solution we had, except for setting the GPIOs and the clock.

Of note, the input format can be different for the LTDC layers (here we will use ARGB888), but the output is 16, 18 or24-bit RGB (565,666 and 888, respectively). The alpha is dropped which isn’t an issue since the DSI Host input is RGB888 only anyway.

We engage the LTDC only in the HAL init and the LCDFillRect functions, though the later two are just directly manipulating the same frame buffer we were manipulating using the DMA2D: LTDC frame buffer is the same as the DMA2D output frame buffer. Technically, we could just use the DMA2D to fill the frame with colours/data instead. Anyway, we only need to replace the LTDC init function with a custom version.

Now, we will carry over paddings and synchronisation parameters from the project since they must be the same as the one HAL sets for us to comply with the signal demands of the screen. Mind, this applies even though we will put the DSIHOST on top of the data flow afterwards. Since we don’t have a datasheet for the screen we are driving, carrying over the parameters from the HAL version of the project is our only way to drive the screen.

#### DSI Host
Okay, we got the crown of the stack: the DSI!

But, what is it really? Well, it is a physical engine that turns incoming – LTDC-supplied – serial RGB data into a specific standard (MIPI DSI). This standard is differential, has strict – and adjustable – timing and provide a screen driving that is reminiscent on what we had for LTDC. Technically, the DSI will be just a very fast and low voltage coms bus driven by the data that is the same as we use for any other screen drive (porches, polarities, synch signals and so on). We will feed it parallelized RGB data from the LTDC and will set it up to comply to the demands of the screen we have.

Regarding the physical construction of the DSI, it has the DSI Wrapper – controlled by the APB2 clock registers – providing control to the DSI PLL, the DSI Bias and LTDC interface (RGB data plus the LTDC control signals) to the DSI Host. The DSI Host takes then this RGB data and transform it into appropriate DSI packets through the packet handler that will be then formed and placed on the DSI bus using the DSI PHY.  DSI PHY control is managed by the Host, not us externally using the APB clock. Both the Host and the Wrapper are enabled by the same bit in the APB2 register.

DSI can be run in multiple different modes, though we will only look at video mode for now, and even in that one, only the burst mode. We will do so because that is what is set by HAL, indicating to me that both the screen as well as the device RAM is enough to work with greater pixel packages. (Mind, non-burst will be able to send over smaller packages and thus consume less RAM. Of course, the LTDC input pixel ratio must match then the DSI output pixel ratio, otherwise the FIFO will be overflown and we lose pixel data. Something to keep in mind for more custom solutions…)

Something important to note is that the DSI Host has its own separate error check and timeout mechanism (separate from the DSI PHY and the DSI wrapper that is) that will generate interrupts should we set it up as such. In video mode these are mostly irrelevant due to the direct publishing philosophy of that particular mode, but it is still worth keeping in mind that it exists. Similarly, in video mode, the DSI Host will place commands on the DSI bus whenever it is possible to do so, so we don’t need to concern ourselves much about commanding the screen separately.

Lastly, there is also a pattern generator that will give back a colour bar. Obviously, this is very useful for debugging screen connections. The frame requirements would need to be adjusted to the screen, obviously (see page 1777 in the refman what needs to be set). We won’t use it here.

Programming up the DSI itself – despite an overflow of options - isn’t that complicated in case we follow thee refman page 1792 strictly…with one notable exception. Despite page 1792 of the refman being clear over how we should set up the DSI, there is a mistake in it that, which, if not circumvented, would not make the DSI work: step 10 (calibration of the DSI PHY) must be done AFTER the DSI PHY clock is calibrated and enabled, i.e. after steps 11, 12 and 13. (Of note, this is why it is useful to have a working HAL code in parallel since the only way I could find this issue was by picking apart the HAL code line-by-line.)

Anyway while looking at page 1792, I also suggest looking at the HAL-generated functions (and CubeMx) to understand, what we need to set. We can see that we will have to configure the PLL clocking and the physical properties of the DSI host, such as the number of data lines or the frequency range/band control/skew rate of the signal exiting the DSI PHY (of note, we will have to do this for the two data lines and the clock line as well). Once done, we will have to set the host timeouts/errors and the LTDC interface physical timings, followed by the video configuration. Mind, these will have to comply with the screen we are using.

The input for the DSI – and the output as well – will be 24-bit RGB888 (can be less too, but we will stick with 24-bit). This aligns with what the screen asks for (might be a calibration question for some screens) and what the LTDC provides (the alpha was dropped on the exit from the LTDC peripheral, see above). The frequency range of the DSI will be set between 450-510 MHz exiting the DSI PHY on 2 data lanes, which, again, must be adjusted to the screen we are using. Anyway, what should be clear from all this is that the DSI does not have a standard specification that would work for most screens, it must be adjusted to whatever we have connected to the DSI PHY.

If my explanations here have been found overwhelming, I suggest looking at the custom code itself to read the comments over what is going on in each line of code.

#### Screen driving
This is the last element of the stack, where we will be using our DSI Host to communicate a set of commands to the screen and thus set it up for use.

HAL uses two types of commands, one called “short write”, the other “long write”. The practical difference between the two is that “short write” send only a byte over the DSI bus to command the screen, while “long write” sends a byte array cut up into 32-bit words.

Anyway, we merely need to recreate these two functions our own way and then feed them the calibration arrays we already have available from HAL. I have not managed to find the screen’s datasheet (code J025F1CN0201W according to the ST MB1835 board schematic) so this solution is not just practical but also very much necessary.

The functions themselves are simple; one merely needs to create 32-bit sized chunks and then put them on the DSI bus when the bus is free (write FIFO is empty). The other things to set is the DSI packet header, which is again just a 32-bit register that should be filled. The only thing here that might be a bit difficult to figure out is the data packet type (or DSI communication mode) within the header. I wager, these are defined within the screen’s datasheet, i.e. that will tell the format each command has to take to be accepted by the screen. Mind, the packet types can only be certain values. (What these values are and what they mean I only have managed to figure out by looking at the “stm32u5xx_hal_dsi.h” header file.)

Now, what must be noted that pretty much all HAL DSI functions lock the HAL before running them, making them non-interruptible. This is likely due to the timing sensitivity of the DSI bus. Should someone wish to implement something complex, it may very much be necessary to assign all DSI actions to an RTOS thread and then protect that thread using mutexes.

### RNG
Sneaking this one in here since it will be used later in one of the examples…

So, what is RNG and why do we need it? Well, RNG is the Random Number Generator and is used to, you guessed it, generate a random number.

Now, there are C-based random number generators (like the “rand()” function) which base their results on the current state of the hardware where they are being run. This technically means that if we reset the hardware, the random number generation will be exactly the same as during the previous run, hurting random number generation uniformity.

To get around this problem, microcontrollers tend to have a designated RNG peripheral that generates a random number from entropy, i.e., analog noise. This noise is generated by internal ring oscillators, captured then with a designated ADC.

### I2C
The I2C peripheral will be used to have capacitive touch sensing on the screen. The IC is a Sitronix ST1633i.

The driver is the same as on an L0 (so not the same as it was on the F429). Mind, I am not using the interrupt-based version of this driver but the polling/blocking version since it is simpler to implement. This also means that the touch sensing is “sluggish” and will be detected with a delay.

## To read
The refman, of course.

I suggest checking the LTDC implementation I had for the F429:
STM32_SCREEN_LTDC_DCMI_OV7670

One can also read up on DSI a bit (like, for instance the AN4860 application note), though I have found the document pretty overwhelming.

Also, if someone can’t find the example projects for the DSI, they can be also found here:
STM32CubeU5/Projects/STM32U5x9J-DK/Examples/DSI at main · STMicroelectronics/STM32CubeU5 · GitHub

## Particularities
I have touched upon those above, and setting up the custom code is more busywork than anything complicated.

We could look at some questions instead though, so anyone using the custom code would now immediately, where to look for answers. 

### How fast can we go with DSI?
We know that we are in video mode and there the DSI will continuously publish stuff in a chunk, thus we will only be limited by the speed of updating the frame buffer. We can see this in action in the image roll section of the code where the two ST stock photos are rolled a user-defined pixel amount at a time on the screen one after each other with maximum speed instead of bulk publishing them and rotating between them every 2 second. (Mind, in the roll section we are extracting a certain section of a source matrix and then put it on a similarly-sized destination – the frame buffer.)

To be fair, best would be to do a direct test on this and see where the bottlenecks for the data flow are. Unfortunately, we can’t hook a camera to the U5G9 DK since it does not have DCMI capabilities. Ideally, we would be generating a steady flow of data on the screen with greater and greater resolution and then see how it changes. This would need to be a new project though.

At any rate, my guess is that we will be able to go comfortable above 30 fps even at 480x480 resolution and RGB888 output. We can most likely gain even more speed by using the “icache” system, though I have had it deactivated in this project.

### How do we adjust to a different screen?
Well, we will have to set everything up to comply to the demands of the screen, be that the DMA2D, the LTDC or the DSI Host (e.g. resolution, porches, synch signal directions and so on). Also, values will have to be kept consistent across the code flow every time.

### Can we drive multiple screens?
Yes, we can also drive multiple screens using the virtual layers or drive two layers into the same screen to make the two layers blend.

### When do we need to adjust the DSI parameters, like the skew?
This I don’t know. Probably it is also something screen specific where a certain signal profile is demanded. I have not come across a detailed DSI screen datasheet yet to wisen up on this matter. It may also be possible to use the same signal setup to drive all DSI screens with the same amount of data lines.

### What’s up with the RNG peripheral?
Not much, we only need to set one register, the CR and leave the rest at reset value.

Within our particular mcu, there are always 4 words – 128 bits – of random number generated and stored in the FIFO. Once the output register is read out 4 times, a new set of 4 numbers can be generated. Mind, we won’t need to reset the RNG peripheral once we have run out of numbers, it will automatically generate a new set by itself.

## User guide
Well, the usual thins is implemented where certain functions are behind if/def sections.

The first section will publish two stock ST images carried over the official ST DSI example.

The second section draws out the same two images but using custom-defined sections.

The third section is a simplified version of the classic game “Snake”. Eat the red bits and grow the snake until you collide with yourself. Mind, this section uses also the capacitive touch sensing of the screen of the U5G9, which is connected to the mcu through I2C5.

## Conclusion
This is a showcase to run the DSI on a U5G9 Disco board.

