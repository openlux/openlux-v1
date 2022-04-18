using MadWizard.WinUSBNet;
using System;
using System.Collections.Concurrent;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading;
using System.Threading.Tasks;
using System.Windows.Forms;

namespace ASCOM.openlux.k11
{
    class Openlux
    {
        const byte SCLK = 0x01;
        const byte MOSI = 0x02;
        const byte MISO = 0x04;
        const byte RST = 0x08;
        const byte CS = 0x10;
        const byte L1 = 0x20;
        const byte L2 = 0x40;
        const byte L3 = 0x80;

        const byte PINDIRS = SCLK | MOSI | RST | CS;
        const byte PINSIDLE = RST | CS;

        private USBDevice device;// = USBDevice.GetSingleDevice(new Guid("{dee824ef-729b-4a0e-9c14-b7117d33a817}"));

        private USBPipe pipe, pipeSpi;

        private RingBuffer rb = new RingBuffer(4096 * 2 * 2720 * 2);

        private readonly BlockingCollection<int[,]> frameQueue = new BlockingCollection<int[,]>(new ConcurrentQueue<int[,]>(), 1);

        private Camera camera;

        public Openlux(Camera camera)
        {
            this.camera = camera;
        }

        public void connect()
        {
            device = USBDevice.GetSingleDevice(new Guid("{dee824ef-729b-4a0e-9c14-b7117d33a817}"));

            pipe = device.Interfaces[0].InPipe;
            pipe.Policy.RawIO = true;

            pipeSpi = device.Interfaces[1].OutPipe;
            pipeSpi.Policy.RawIO = true;

            device.ControlOut(0x40, 0x00, 0x0000, 0); //usb_reset
            device.ControlOut(0x40, 0x0B, 0x0000, 2); //clear bitmode
            device.ControlOut(0x40, 0x0B, 0x0200, 2); //set bitmode
            device.ControlOut(0x40, 0x00, 0x0001, 2); //purge rx buffer
            device.ControlOut(0x40, 0x00, 0x0002, 2); //purge tx buffer

            Thread.Sleep(50);

            int n = 0;
            byte[] buf = new byte[256];
            buf[n++] = 0x86; //tck_divisor
            buf[n++] = 0x05; //divider for 1mhz
            buf[n++] = 0x00;
            buf[n++] = 0x97; //dis_adaptive
            buf[n++] = 0x8d; //dis_3_phase
            buf[n++] = 0x80; //set_bits_low
            buf[n++] = PINSIDLE; //TODO initial states
            buf[n++] = PINDIRS; //TODO pin directions

            // TODO send buf
            pipeSpi.Write(buf, 0, n);
        }

        public int[,] takeFrame()
        {
            return frameQueue.Take();
        }

        public void writeSpi(int value)
        {
            int n = 0;
            byte[] buf = new byte[256];
            buf[n++] = 0x80; //set_bits_low
            buf[n++] = PINSIDLE & ~CS;
            buf[n++] = PINDIRS;

            buf[n++] = 0x10 | 0x01; //do_write and write_neg //TODO read?
            buf[n++] = 0x00; //len
            buf[n++] = 0x00;
            buf[n++] = (byte) value;// 0xaa; //data

            buf[n++] = 0x80; //set_bits_low
            buf[n++] = PINSIDLE;
            buf[n++] = PINDIRS;

            // TODO send buf
            pipeSpi.Write(buf, 0, n);

            Thread.Sleep(12);
        }

        /*public void ReadThread()
        {
            var buffer = new byte[512];

            try
            {

                while (true) {
                    var len = pipe.Read(buffer);
                    rb.Write(buffer, 2, len - 2);
                }
            }
            catch
            {
                // TODO
            }
        }*/

        public void startExposure(double duration)
        {
            //Thread.Sleep(25);

            Thread.CurrentThread.Priority = ThreadPriority.Highest;

            int seconds = (int) duration;
            double ovf = duration - seconds;
            int millis = (int)(ovf * 1000d);

            if (seconds == 0 && millis == 0) millis = 1;
            if (millis > 999) millis = 999;

            //MessageBox.Show("Start " + seconds + "s " + millis + "m");

            writeSpi(0xAA);

            // seconds
            writeSpi(seconds >> 8);
            writeSpi(seconds);

            // millis
            writeSpi(millis >> 8);
            writeSpi(millis);

            pipe.Policy.PipeTransferTimeout = 1000 * 10;
            //pipe.Flush();
            if (seconds > 5) Thread.Sleep(1000 * (seconds - 5));

            try
            {
                var buffer = new byte[512];

                int toread = 4096 * 2 * 2720;// 2720;
                while (toread > 0)
                {
                    int len = 510;
                    if (toread < 510) len = toread;

                    //toread -= len;

                    len += 2;

                    //pipe.Policy.PipeTransferTimeout
                    var rd = pipe.Read(buffer, 0, 512);// port.Read(buffer, 0, buffer.Length);
                    rd -= 2;
                    rb.Write(buffer, 2, rd);

                    toread -= rd;
                }
            } catch (Exception ex)
            {
                // TODO read timed out, lost data HANDLE THIS PROPERLY?
                //MessageBox.Show(ex.Message);
            }

            //MessageBox.Show("Read all data");

            rb.Consume((7 * 2) + 1);
            //rb.Consume((20 + 12) * 2);
            rb.Consume(4096 * 2 * (18 + 8));

            int[,] data = new int[4008, 2672];

            for (int y = 0; y < 2672; y++)
            {
                rb.Consume((20 + 12) * 2);

                byte[] row = rb.Read(4008 * 2);
                for (int x = 0; x < 4008; x++)
                {
                    data[x, y] = (row[x << 1] << 8) | row[(x << 1) | 1];
                }

                rb.Consume(56 * 2);
            }

            //rb.Consume(4096 * 2 * 22);
            //rb.Consume(((4096 - 7) * 2)/* - 1*/);
            //rb.Consume(49 - 38 - 26); // TODO where is this coming from
            rb.Clear();
            //rb.Consume(38); // TODO where is this coming from
            //MessageBox.Show("" + rb.Available());
            //rb.Clear();

            //MessageBox.Show("Consumed all rb data");

            frameQueue.Add(data);

            //MessageBox.Show("Added frame to queue");

            camera.cameraImageReady = true;

            //MessageBox.Show("Flagged image ready");
        }
    }
}
