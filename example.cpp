/*
 *    MIT License
 *
 *    Copyright (c) 2016 Błażej Szczygieł
 *
 *    Permission is hereby granted, free of charge, to any person obtaining a copy
 *    of this software and associated documentation files (the "Software"), to deal
 *    in the Software without restriction, including without limitation the rights
 *    to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 *    copies of the Software, and to permit persons to whom the Software is
 *    furnished to do so, subject to the following conditions:
 *
 *    The above copyright notice and this permission notice shall be included in all
 *    copies or substantial portions of the Software.
 *
 *    THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 *    IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 *    FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 *    AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 *    LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 *    OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 *    SOFTWARE.
 */

#include "OpusVorbisDecoder.hpp"
#include "VPXDecoder.hpp"

#include <mkvparser/mkvparser.h>

#include <stdio.h>
#include <iostream>
#include <cstring>
#include <vector>

/*
class MkvReader : public mkvparser::IMkvReader
{
public:
	MkvReader(const char *filePath) :
		m_file(fopen(filePath, "rb"))
	{}
	~MkvReader()
	{
		if (m_file)
			fclose(m_file);
	}

	int Read(long long pos, long len, unsigned char *buf)
	{
		if (!m_file)
			return -1;
		fseek(m_file, pos, SEEK_SET);
		const size_t size = fread(buf, 1, len, m_file);
		if (size < size_t(len))
			return -1;
		return 0;
	}
	int Length(long long *total, long long *available)
	{
		if (!m_file)
			return -1;
		const off_t pos = ftell(m_file);
		fseek(m_file, 0, SEEK_END);
		if (total)
			*total = ftell(m_file);
		if (available)
			*available = ftell(m_file);
		fseek(m_file, pos, SEEK_SET);
		return 0;
	}

private:
	FILE *m_file;
};
*/
class MkvReader : public mkvparser::IMkvReader
{
public:
	MkvReader(const char *buff, long long buff_size):
    m_size(buff_size), m_pos(0)
	{
        m_buff = new char[buff_size];
        if(m_buff == NULL){
            std::cout<<"buffer init error \n ";
        }
        //m_buff = buff;
        memcpy(m_buff, buff, buff_size);
        {
///            FILE * file;
///            file = fopen("test.webm","wb");
///            if(file == NULL){
///                std::cout<<"file open error \n ";
///            }
///            fwrite(m_buff, sizeof(char), buff_size, file);
///            fclose(file);
        }
    }
	~MkvReader()
	{
		delete[] m_buff;
	}

	int Read(long long pos, long len, unsigned char *buf)
	{
		if (m_buff == NULL){
            std::cout<<"exceed the buffer size \n ";
			return -1;
        }
		//fseek(m_file, pos, SEEK_SET);
        if(pos > m_size){
            std::cout<<"exceed the buffer size \n ";
            return -1;
        }
        m_pos = pos;
        //char * temp;
        if(m_size - pos-1 < len){
            std::cout<<"m_size :"<<m_size<<" pos : "<<pos<<" len "<<len<<std::endl;
            len = m_size - pos;
        }
        //temp = &m_buff[pos];
        //memcpy(buf, temp, len);
        memcpy(buf, &m_buff[pos], len);
		return 0;
	}
	int Length(long long *total, long long *available)
	{
		if (m_buff == NULL)
			return -1;
        *total = m_size;
        *available = m_size;
		return 0;
	}

private:
	char *m_buff;
    long long  m_size;
    long long m_pos;
};

int webm_parser2buf(long long buff_size, const char *src, short *dst, long long &pcm_buff_size )
//int main(int argc, char *argv[])
{
	WebMDemuxer demuxer(new MkvReader(src, buff_size));
	if (demuxer.isOpen())
	{
		VPXDecoder videoDec(demuxer, 8);
		OpusVorbisDecoder audioDec(demuxer);

		WebMFrame videoFrame, audioFrame;

		VPXDecoder::Image image;

		short *pcm = audioDec.isOpen() ? new short[audioDec.getBufferSamples() * demuxer.getChannels()] : NULL;

		fprintf(stderr, "buffersample: %d  channel %d \n", audioDec.getBufferSamples(), demuxer.getChannels());
		fprintf(stderr, "Length: %f\n", demuxer.getLength());
		fprintf(stderr, "audio open : %d audio frame valid %d \n", audioDec.isOpen(), audioFrame.isValid() );

        FILE* file;
        file = fopen("/home/haruband/mywav.wav","wb");
        if(file == NULL){
            printf("file open error");
            return 0;
        }           
        int frame_num = 0;
		while (demuxer.readFrame(&videoFrame, &audioFrame))
		{
			if (videoDec.isOpen() && videoFrame.isValid())
			{
				if (!videoDec.decode(videoFrame))
				{
					fprintf(stderr, "Video decode error\n");
					break;
				}
				while (videoDec.getImage(image) == VPXDecoder::NO_ERROR)
				{
// 					for (int p = 0; p < 3; ++p)
// 					{
// 						const int w = image.getWidth(p);
// 						const int h = image.getHeight(p);
// 						int offset = 0;
// 						for (int y = 0; y < h; ++y)
// 						{
// 							fwrite(image.planes[p] + offset, 1, w, stdout);
// 							offset += image.linesize[p];
// 						}
// 					}
				}
			}
			if (audioDec.isOpen() && audioFrame.isValid())
			{
				int numOutSamples;
				if (!audioDec.getPCMS16(audioFrame, pcm, numOutSamples))
				{
					fprintf(stderr, "Audio decode error\n");
					break;
				}
                //std::cout<<"num : "<<numOutSamples<<" buff size : "<<audioFrame.bufferSize<<"\n";
 				fwrite(pcm, 1, numOutSamples * demuxer.getChannels() * sizeof(short), file);
                memcpy(&dst[frame_num*numOutSamples], pcm, numOutSamples*2); // memcpy copy 1 byte, *2 for 2bytes copy
                frame_num++;
			}
            pcm_buff_size = 2880*frame_num*2;
		}

        std::cout<<"total frame count : "<<frame_num<<"\n";
//        fwrite(pcm, 1, numOutSamples * demuxer.getChannels() * sizeof(short), file);
        fclose(file);
		delete[] pcm;
	}
    else
        std::cout<<"demuxer creation error \n ";
	return 0;
}

int webm_parser2vec(long long buff_size, const char *src, std::vector<short> &dst_vec )
//int main(int argc, char *argv[])
{
	WebMDemuxer demuxer(new MkvReader(src, buff_size));
	if (demuxer.isOpen())
	{
		VPXDecoder videoDec(demuxer, 8);
		OpusVorbisDecoder audioDec(demuxer);

		WebMFrame videoFrame, audioFrame;

		VPXDecoder::Image image;

		short *pcm = audioDec.isOpen() ? new short[audioDec.getBufferSamples() * demuxer.getChannels()] : NULL;

		fprintf(stderr, "buffersample: %d  channel %d \n", audioDec.getBufferSamples(), demuxer.getChannels());
		fprintf(stderr, "Length: %f\n", demuxer.getLength());
		fprintf(stderr, "audio open : %d audio frame valid %d \n", audioDec.isOpen(), audioFrame.isValid() );

        int frame_num = 0;
		while (demuxer.readFrame(&videoFrame, &audioFrame))
		{
			if (videoDec.isOpen() && videoFrame.isValid())
			{
				if (!videoDec.decode(videoFrame))
				{
					fprintf(stderr, "Video decode error\n");
					break;
				}
				while (videoDec.getImage(image) == VPXDecoder::NO_ERROR)
				{
				}
			}
			if (audioDec.isOpen() && audioFrame.isValid())
			{
				int numOutSamples;
				if (!audioDec.getPCMS16(audioFrame, pcm, numOutSamples))
				{
					fprintf(stderr, "Audio decode error\n");
					break;
				}
                //std::cout<<"pcm size : "<<sizeof(pcm)<<"numout: "<<numOutSamples<<std::endl;
                dst_vec.insert(dst_vec.end(),&pcm[0], &pcm[numOutSamples]);
                //std::cout<<"num : "<<numOutSamples<<" buff size : "<<audioFrame.bufferSize<<"\n";
 				//fwrite(pcm, 1, numOutSamples * demuxer.getChannels() * sizeof(short), file);
                //copy vector to array by using memcpy
                //memcpy( tmp, &vPoint[0], sizeof(Point)*n );
                //
                //vector.insert(vector.end(), buffer, buffer + size);
                //std::copy(buffer, buffer + size, std::back_inserter(vector));
                //memcpy(&dst[frame_num*numOutSamples], pcm, numOutSamples*2); // memcpy copy 1 byte, *2 for 2bytes copy
                frame_num++;
			}
		}

        std::cout<<"total frame count : "<<frame_num<<"\n";
//        fwrite(pcm, 1, numOutSamples * demuxer.getChannels() * sizeof(short), file);
		delete[] pcm;
	}
    else
        std::cout<<"demuxer creation error \n ";
	return 0;
}
