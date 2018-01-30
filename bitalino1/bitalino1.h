#ifndef BITALINO_H
#define BITALINO_H 

#define MAX_SERIAL_BUFFER  30

struct BITalinoFrame
 {
    byte    seq;        ///< Frame sequence number (0...15)
    boolean digital[4]; ///< Array of digital inputs states (false or true)
    unsigned short   analog[6];  ///< Array of analog inputs values (0...1023 or 0...63)
 };


 
class BITalinoClass
{
	private:
		signed char nChannels;
	public:
		char SerialBuffer[MAX_SERIAL_BUFFER];
		int SerialIndex;
	
		BITalinoClass(void);
		void begin(void);
		void end(void);
		byte version(char buffer[], byte len);
		boolean sendRaw(byte CommandByte);
		boolean start(int samplingRate, byte channelsBitmap, bool simulated);
		boolean stop(void);
		word read(word nFrames, BITalinoFrame frames[]);
		word readSingle(BITalinoFrame *frame);
		boolean battery(byte value);
		boolean trigger(byte outputsBitmap);
		unsigned char checkCRC4(char *data, int len);
		
};

extern BITalinoClass BITalino;


#endif