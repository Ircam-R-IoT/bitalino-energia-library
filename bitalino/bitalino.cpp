/**
 * \file
 * \copyright  Copyright 2014 PLUX - Wireless Biosignals, S.A.
 * \author     Filipe Silva
 * \version    1.1
 * \date       July 2014
 * 
 * \section LICENSE
 
 This program is free software: you can redistribute it and/or modify
 it under the terms of the GNU Lesser General Public License as published by
 the Free Software Foundation, either version 3 of the License, or
 (at your option) any later version.
 
 This program is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU Lesser General Public License for more details.
 
 You should have received a copy of the GNU Lesser General Public License
 along with this program.  If not, see <http://www.gnu.org/licenses/>.
 
 */


 /**
  \mainpage
  
  The BITalino Arduino API (available at http://github.com/BITalinoWorld/arduino-api)
  is a library which enables Arduino sketches to communicate with a BITalino device through a simple interface.
  The API is composed of a single file (bitalino.ino).
  A sample test sketch ([test.ino](http://github.com/BITalinoWorld/arduino-api/tree/master/test.ino)) is also provided.

  The BITalino device is connected to an Arduino through the UART TX and RX pins of both devices.
  The TX pin of one device connects to the RX pin of the other device.
  The BITalino Bluetooth module must be detached from the MCU module (as in BITalino Free Style configuration),
  since the Bluetooth module also uses the UART pins.
  The BITalino AGND pin must be connected to the Arduino GND pin.
  In a typical configuration, the BITalino device draws power from the Arduino,
  so the BITalino AVCC pin must be connected to the Arduino 5V pin.
  
  Since the Arduino UART is used to communicate with the BITalino device, it cannot be used to any other purpose at the same time.
  If you upload Arduino firmware using the Arduino bootloader,
  you must first disconnect the Arduino RX pin from the BITalino device.
  If you upload Arduino firmware using an external programmer, there is no need to disconnect any UART pin from the BITalino device.
  
  The API exposes a single object (\ref BITalinoClass "BITalino") representing a connection to a BITalino device.
  The connection is established in the \ref BITalinoClass::begin "begin()" method and released in the \ref BITalinoClass::end "end()" method,
  as in Arduino Serial library.
  
  The API was tested in an Arduino Diecimila.

  \section sampleapp About the sample sketch

  The sample sketch ([test.ino](http://github.com/BITalinoWorld/arduino-api/tree/master/test.ino))
  open a connection to a BITalino device.
  Then it starts acquiring all channels on the device at 1000 Hz and, inside loop() routine,
  it prints out one frame out of 100 and toggles the device green LED indefinitely.
  
  Since the Arduino UART is used to communicate with the BITalino device, this sketch uses a
  software-implemented serial port to output textual information from Arduino.
  The serial port TX is attached to Arduino pin 9, and the RX is not used.
  If you want to see the output text, you must connect Arduino pin 9 to the RX of an external serial port
  and set its baudrate to 115200 bps, no parity, 8 data bits, 1 stop bit, no flow control.
  The ground pins from Arduino and from external serial port must be connected.
  The serial port voltage levels must be compatible with Arduino (TTL 5V).
  
  \section compiling Uploading the sample sketch

  To upload the library and the sample sketch:
  - in the Arduino IDE, select the appropriate board model;
  - if uploading using Arduino bootloader, select the appropriate serial port
  (the Arduino RX pin must be disconnected from BITalino during upload as described before);
  - copy [bitalino.ino](http://github.com/BITalinoWorld/arduino-api/tree/master/bitalino.ino) and
  [test.ino](http://github.com/BITalinoWorld/arduino-api/tree/master/test.ino) to a new directory named `bitalino`;
  - open %bitalino.ino in Arduino IDE (test.ino is automatically opened in a new tab);
  - upload to Arduino;
  - if Arduino RX pin was disconnected from BITalino, reconnect it and reset Arduino.

  */
  
  // Mods by Emmanuel FLETY - IRCAM
  // Added simplified (but synchronized with CRC check) single frame grabbing for use with the RIOT module
  // added a 5ms delay in the version string grabbing member function as the RIOT CPU was too fast while expecting the bitalino to answer
  // with the actual string.

  
#include <Energia.h>
#include <bitalino.h>

BITalinoClass BITalino;

/**
 * The BITalino device class.
 */
BITalinoClass::BITalinoClass(void) 
{
	nChannels = -1;
	SerialIndex = 0;
}

  /**
   * Opens a connection with the BITalino device.
   * \note This method must be called before any other method.
   */
void BITalinoClass::begin(void)
{
	if (nChannels != -1) return;

    Serial.begin(115200);
	Serial.setTimeout(5000); // 5 s
    nChannels = 0;
    // send "stop" command to device
	Serial.write((uint8_t)0);
	delay(50);
}
  
  /**
   * Closes the connection with the BITalino device. If an aquisition is running, it is stopped.
   * \note After closing the connection, begin() must be called again before calling any other method.
   */
void BITalinoClass::end(void)
{
    if (nChannels == -1)  return;
    
    if (nChannels != 0)  stop();
    Serial.end();
    nChannels = -1;
}
  
  /**
   * Retrieves the device firmware version string.
   * \param[out] buffer Character array to receive the version string.
   * \param[in] len Size of the character array in bytes.
   * \returns Number of bytes stored in buffer, excluding the terminating null character, or zero if an error occurred.
   * \note This method cannot be called during an acquisition.
   */
byte BITalinoClass::version(char buffer[], byte len)
{
	if (nChannels != 0)
    {
      buffer[0] = 0;
      return 0;
    }
    
    const char *header = "BITalino";
    const int headerLen = strlen(header);

    // send "get version string" command to device
    Serial.write((uint8_t)7);
	
    
    byte n = 0;    
    while(1)
    {
	  delay(5);	// E.FLET - MODS
      if (n == len)
      {
        buffer[0] = 0;
        return 0;
      }
      
      int chr = Serial.read();
      if (chr == -1)
      {
        buffer[0] = 0;
        return 0;
      }

      if (n >= headerLen)
      {
        if (chr == '\n')
        {
          buffer[n] = 0;
          return n;
        }

        buffer[n++] = chr;
      }
      else
        if (chr == header[n])
          buffer[n++] = chr;
        else
        {
          n = 0;   // discard all data before version header
          if (chr == header[0])   buffer[n++] = chr;
        }
    }
}
  
// Send RAW serial commands / bytes to the bitalino MCU

boolean BITalinoClass::sendRaw(byte CommandByte)
{
	// We do a bit of command analysis to retrieve the number of channel used
	if(CommandByte & 1 || CommandByte & 2) // Live or simulated
	{
		byte chBmp = (CommandByte >> 2) & 0B00111111;
		nChannels = 0;
		for(byte i = 0; i < 6; i++)
		{
			if (chBmp & 1)  nChannels++;      
			chBmp >>= 1;
		}
	}
}
  
  
  /**
   * Starts a signal acquisition from the device.
   * \param[in] samplingRate Sampling rate in Hz. Accepted values are 1, 10, 100 or 1000 Hz. Default value is 1000 Hz.
   * \param[in] channelsBitmap Bitmap of channels to acquire. Channel 0 is 0x01 and channel 5 is 0x20.
   * Default value is 0x3F (all 6 analog channels).
   * \param[in] simulated If true, start in simulated mode. Otherwise start in live mode. Default is to start in live mode.
   * \returns true if operation succeeded, or false if an error occurred.
   * \note This method cannot be called during an acquisition.
   */
boolean BITalinoClass::start(int samplingRate = 1000, byte channelsBitmap = 0x3F, bool simulated = false)
{
    if (nChannels != 0)  return false;
    
    byte cmd;

    switch (samplingRate)
    {
    case 1:
      cmd = 0;
      break;
    case 10:
      cmd = 1;
      break;
    case 100:
      cmd = 2;
      break;
    case 1000:
      cmd = 3;
      break;
    default:
      return false;
    }
    
    byte chBmp = channelsBitmap & 0x3F;
    for(byte i = 0; i < 6; i++)
    {
      if (channelsBitmap & 1)  nChannels++;      
      channelsBitmap >>= 1;
    }
    
    if (nChannels == 0)  return false;
    
    // send "set samplerate" command to device
    Serial.write((uint8_t)((cmd << 6) | 0x03));

    delay(50);

    // send "start" command to device
    Serial.write((uint8_t)(chBmp << 2) | (simulated ? 0x02 : 0x01));

    return true;
}
  
  /**
   * Stops a signal acquisition.
   * \returns true if operation succeeded, or false if an error occurred.
   * \note This method must be called only during an acquisition.
   */
boolean BITalinoClass::stop(void)
{
    if (nChannels <= 0)  return false;
    
    // send "stop" command to device
    Serial.write((uint8_t)0);

    nChannels = 0;
    return true;    
}

static const unsigned char CRC4tab[16] = {0, 3, 6, 5, 12, 15, 10, 9, 11, 8, 13, 14, 7, 4, 1, 2};
  
// check CRC on a buffer of bytes 
unsigned char BITalinoClass::checkCRC4(char *data, int len)
{
	unsigned char crc = 0;

	for (int i = 0; i < len-1; i++)
	{
      const unsigned char b = data[i];
      crc = CRC4tab[crc] ^ (b >> 4);
      crc = CRC4tab[crc] ^ (b & 0x0F);
	}

	// CRC for last byte
	crc = CRC4tab[crc] ^ (data[len-1] >> 4);
	crc = CRC4tab[crc];

	return (crc == (data[len-1] & 0x0F));
}
  
  
  
  /**
   * Reads acquisition frames from the device.
   * This method does not return until all requested frames are received from the device.
   * \param[in] nFrames Number of frames to be receive.
   * \param[out] frames Array of frames to be filled. This array must hold at least the number of frames requested.
   * \returns Number of frames stored in the given frames array. If an error occurred, this value is less than nFrames.
   * \note This method must be called only during an acquisition.
   */   
word BITalinoClass::read(word nFrames, BITalinoFrame frames[])
{
    if (nChannels <= 0)  return 0;
    
    byte buffer[8]; // frame maximum size is 8 bytes

    byte nBytes = nChannels + 2;
    if (nChannels >= 3 && nChannels <= 5)  nBytes++;

    for(word fCnt = 0; fCnt < nFrames; fCnt++)
    {
      if (Serial.readBytes((char *) buffer, nBytes) < nBytes)  return fCnt;  // return previous frames if timeout occurred
      
      // check CRC
      byte crc = buffer[nBytes-1] & 0x0F;
      buffer[nBytes-1] &= 0xF0;  // clear CRC bits in frame

      byte x = 0;
      for(byte i = 0; i < nBytes; i++)
         for(signed char bit = 7; bit >= 0; bit--)
         {
            x <<= 1;
            if (x & 0x10)  x ^= 0x03;
            x ^= ((buffer[i] >> bit) & 0x01);
         }

      if (crc != (x & 0x0F))  return fCnt;  // return previous frames if CRC mismatch occurred

      BITalinoFrame &f = frames[fCnt];
      f.seq = buffer[nBytes-1] >> 4;
      for(byte i = 0; i < 4; i++)
         f.digital[i] = ((buffer[nBytes-2] & (0x80 >> i)) != 0);

      f.analog[0] = ((unsigned short)(buffer[nBytes-2] & 0x0F) << 6) | (buffer[nBytes-3] >> 2);
      if (nChannels > 1)
         f.analog[1] = ((unsigned short)(buffer[nBytes-3] & 0x03) << 8) | buffer[nBytes-4];
      if (nChannels > 2)
         f.analog[2] = ((unsigned short)(buffer[nBytes-5]) << 2) | (buffer[nBytes-6] >> 6);
      if (nChannels > 3)
         f.analog[3] = ((unsigned short)(buffer[nBytes-6] & 0x3F) << 4) | (buffer[nBytes-7] >> 4);
      if (nChannels > 4)
         f.analog[4] = ((unsigned short)(buffer[nBytes-7] & 0x0F) << 2) | (buffer[nBytes-8] >> 6);
      if (nChannels > 5)
         f.analog[5] = buffer[nBytes-8] & 0x3F;   
    }
    
    return nFrames;
}


 /**
   * Reads a single frame from the device.
   */
word BITalinoClass::readSingle(BITalinoFrame *frame)
{
    if (nChannels <= 0)  return 0;
    
    byte nBytes = nChannels + 2;
    if (nChannels >= 3 && nChannels <= 5)  nBytes++;
    
	if(Serial.readBytes(SerialBuffer, nBytes) < nBytes)
        return 0;
    else
    {
        SerialIndex += nBytes;
        // We should do something here too or use a circular buffer but let's start with a buffer large enough
        // to not have this happening (30 = 3 frames)
        if(SerialIndex > MAX_SERIAL_BUFFER)
          SerialIndex = 0;
		
		int Attempt = nBytes-1;
		// CRC failed, we try to re-synch on the next frame
        if(!BITalino.checkCRC4(SerialBuffer, nBytes))
        {
          int Attempt = nBytes-1;
          while (!BITalino.checkCRC4(SerialBuffer, nBytes) && Attempt)
          {   // if CRC check failed, try to resynchronize with the next valid frame
             // checking with one new byte at a time
             memmove(SerialBuffer, SerialBuffer+1, nBytes-1);
             if(Serial.available() > 0)
             {
                Serial.readBytes(SerialBuffer, 1);  // reads one byte with timeout
             }
             Attempt--;
          }
        }

		if(Attempt) // sanity check
        {
			// Bitalino data grab
			frame->seq = SerialBuffer[nBytes-1] >> 4;
			for(byte i = 0; i < 4; i++)
				frame->digital[i] = ((SerialBuffer[nBytes-2] & (0x80 >> i)) != 0);

			frame->analog[0] = ((unsigned short)(SerialBuffer[nBytes-2] & 0x0F) << 6) | (SerialBuffer[nBytes-3] >> 2);
			if (nChannels > 1)
				frame->analog[1] = ((unsigned short)(SerialBuffer[nBytes-3] & 0x03) << 8) | SerialBuffer[nBytes-4];
			if (nChannels > 2)
				frame->analog[2] = ((unsigned short)(SerialBuffer[nBytes-5]) << 2) | (SerialBuffer[nBytes-6] >> 6);
			if (nChannels > 3)
				frame->analog[3] = ((unsigned short)(SerialBuffer[nBytes-6] & 0x3F) << 4) | (SerialBuffer[nBytes-7] >> 4);
			if (nChannels > 4)
				frame->analog[4] = ((unsigned short)(SerialBuffer[nBytes-7] & 0x0F) << 2) | (SerialBuffer[nBytes-8] >> 6);
			if (nChannels > 5)
				frame->analog[5] = SerialBuffer[nBytes-8] & 0x3F;  
			
			return 1;
        }
     }
	 
    return 0;
}


  
  /**
   * Sets the battery voltage threshold for the low-battery LED.
   * \param[in] value Battery voltage threshold. Default value is 0.
   * Value | Voltage Threshold
   * ----- | -----------------
   *     0 |   3.4 V
   *  ...  |   ...
   *    63 |   3.8 V
   * \returns true if operation succeeded, or false if an error occurred.
   * \note This method cannot be called during an acquisition.
   */
boolean BITalinoClass::battery(byte value = 0)
{
	if (nChannels != 0)  return false;

    if (value > 63)   return false;

    // send "set battery" command to device
    Serial.write((uint8_t)(value << 2));
     
    return true;
}
  
  /**
   * Sets the digital outputs states.
   * \param[in] outputsBitmap Bitmap to assign to digital outputs. Output 0 is 0x01 and output 3 is 0x08.
   * Default value is 0 (all digital outputs are set to low state).
   * \returns true if operation succeeded, or false if an error occurred.
   * \note This method must be called only during an acquisition.
   */
boolean BITalinoClass::trigger(byte outputsBitmap = 0)
{
    if (nChannels <= 0)  return false;

    // send "set digital output" command to device
    Serial.write((uint8_t)(((outputsBitmap & 0x0F) << 2) | 0x03));
     
    return true;
}

