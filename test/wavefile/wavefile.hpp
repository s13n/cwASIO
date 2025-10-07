#pragma once

#include <cstdint>
#include <filesystem>
#include <string>

class WaveFile {
public:
    struct WAVFILE_HEADER {
        char        main_chunk[4];  /* 'RIFF' or 'RF64'	*/
        uint32_t    main_length;    /* Laenge der Datei	*/
        char        chunk_type[4];  /* 'WAVE'	*/
    };

    // ds64 chunk of a WavFile
    struct WAVFILE_DS64CHUNK {
        char        ds64_chunk[4];  /* 'ds64'                       */
        uint32_t    ds64_length;    /* length of ds64 chunk         */
        uint64_t    riffSize;       /* 64 bit length of RIFF file   */
        uint64_t    dataSize;       /* 64 bit length of data chunk  */
        uint64_t    sampleCount;    /* sample count in 64 bit       */
        uint32_t    table_length;   /* length of ds64 chunk table   */
    };

    // format chunk of a WavFile
    struct WAVFILE_FMTCHUNK {
        char        fmt_chunk[4];       /* 'fmt '                                           */
        uint32_t    fmt_length;         /* length of format chunk, always 16 bytes          */
        uint16_t    formatTag;          /* 1 for PCM, 0x50 for MPEG                         */
        int16_t     nChannels;          /* 1 for mono, 2 for stereo                         */
        int32_t     nSamplesPerSec;     /* 11.025kHz (0x2b11), 22.050(0x5622), 44.1($ac44)  */
        int32_t     nAvgBytesPerSec;    /* samples/sec * channels * bytes/sample            */
        int16_t     nBlockAlign;        /* offset = bytes/sample * channels                 */
        int16_t     nBitsPerSample;     /* 8, 12, 16, 24 or 32 bits                         */
    };

    // GUID
    struct GUID {
        uint32_t    Data1;
        uint16_t    Data2;
        uint16_t    Data3;
        uint8_t     Data4[8];
    };

    // wave format extensible chunk of a WavFile
    struct WAVFILEEXT_FMTCHUNKEXT {
        uint16_t    wValidBitsPerSample;    /* bits of precision  */
        uint32_t    dwChannelMask;          /* which channels are selected */
        GUID        SubFormat;
    };

    // data chunk header of a WavFile
    struct WAVFILE_CHUNK {
        char        chunk[4];   /* fourcc          */
        uint32_t    length;     /* length of chunk */
    };

    WaveFile ( );
    virtual ~WaveFile ( );

    std::string open ( std::filesystem::path filename,  unsigned long samplerate, unsigned long bitsPerSample, unsigned long channels );
    std::string open ( std::filesystem::path filename );
    std::string close ( );
    std::string write ( void const *data, unsigned long samples );
    unsigned long read ( unsigned long samples, void *data );
    unsigned long long getPosition ( ) const { return samples_; }
    bool setPosition ( long long samples, int origin );

    std::string getFilename ( ) const { return filename_; }
    unsigned long getSamplerate ( ) const { return samplerate_; }
    unsigned long getBitsPerSample ( ) const { return bitsPerSample_; }
    unsigned long getBytesPerSample ( ) const { return bitsPerSample_ / 8 + ((bitsPerSample_ % 8) ? 1 : 0); }
    unsigned long getChannels ( ) const { return channels_; }
    unsigned long long getTotalSamples ( ) const { return dataLength_ / getBytesPerSample ( ) / channels_; }

private:
    std::string writeHeaders ( FILE* fp, unsigned long bitsPerSample, unsigned long samplerate, unsigned long channels, unsigned long long samplesWritten );
    std::string readHeaders  ( FILE* fp );
    std::string searchChunk  ( FILE* fp, char const *identifier );
    bool setPositionRelative ( long long samples );
    bool setPositionAbsoluteForward ( unsigned long long samples );
    bool setPositionAbsoluteBackward ( unsigned long long samples );

    FILE* fp_;
    std::string filename_;
    unsigned long samplerate_;
    unsigned long bitsPerSample_;
    unsigned long channels_;
    unsigned long long samples_;
    bool readNotWrite_;
    unsigned long long dataLength_;
    unsigned long startOfData_;
};
