This is the first version of this camera and had a few small design issues, these will be fixed in v2.

# openlux

openlux is an open source design for an astrophotography camera designed around the KAI-11002 CCD sensor.

### Hardware specifications

* 4-layer PCB

* KAI-11002 CCD sensor
  * 4008x2672 resolution
  * 43.3mm diagonal size
  * 9um pixels
  * Interline transfer
  * Electronic shutter

* AD9826 ADC
  * Correlated double sampling
  * 16-bit

* ATmega328P + FT2232H
  * High speed USB 2.0 (480Mb/s)


### Sample photo

* 32x 5min lights (2hrs 40mins total), 16x darks, 16x flats, 16x dark flats
* AT65EDQ + HEQ5
* ST80 & ASI174MM for guiding

![M31 sample photo](img/sample-m31.png)

### PCB render

![PCB render](img/render.png)

### Photos

![PCB photo](img/pcb.jpg)

![Camera photo](img/cam.jpg)
