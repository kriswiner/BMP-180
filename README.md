BMP-180
=======

Bosch's newest low-power, small-footprint, high-precision pressure sensor/altimeter

Basic program to read temperature and pressure from the BMP-180 data registers, properly
scale them to get temperature in degrees Centigrade of Fahrenheit, pressure in Pa or mPa, and altitude in meters
or feet. One program is the main (BMP180.ino) and one is a function library (HT16K33.ino for the HT16K33 led display driver.

This I2C sensor requires only four connections to the microcontroller. The use of 4 four-digit bubble displays
for data output is an ideal use for this ![display array](https://github.com/kriswiner/HT16K33_Display_Driver). It also requires only four connections to the microcontroller while showing pressure, temperature, and altitude in meters and feet.

![](https://d3s5r33r268y59.cloudfront.net/44691/products/thumbs/2014-07-15T00:56:27.193Z-Image2.png.855x570_q85_pad_rcrop.png)

The main code averages over the pressure about sixteen times to get the best accuracy; this is quoted as 0.17 m
in the data sheet. In practice, this is a little excessive. The averaging takes place while waiting for the
2 Hz display duty cycle to begin. If less averaging is desired, the display duty cycle can be increased to 5 Hz for
six data averages, or 10 Hz for the recommended three data averages. Or averaging can be eliminated. All of these changes can be managed in a straightforward way in this heavily commented and concise program.

Lastly, I include a version for the mBed compiler running on the STM32F401 ARM processor.
