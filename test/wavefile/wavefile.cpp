#include <cassert>
#include <cstdio>
#include <cstring>
#include <limits>
#include "wavefile.hpp"

#ifndef WAVE_FORMAT_PCM
    #define WAVE_FORMAT_PCM 1
#endif

#ifndef WAVE_FORMAT_EXTENSIBLE
    #define WAVE_FORMAT_EXTENSIBLE 0xFFFE
#endif

const WaveFile::GUID KSDATAFORMAT_SUBTYPE_PCM { 0x00000001, 0x0000, 0x0010, { 0x80, 0x00, 0x00, 0xaa, 0x00, 0x38, 0x9b, 0x71 } };

using namespace std::string_literals;


WaveFile::WaveFile ( )
  : fp_ ( NULL ),
    samplerate_ ( 0 ),
    bitsPerSample_ ( 0 ),
    channels_ ( 0 ),
    samples_ ( 0 ),
    readNotWrite_ ( true ),
    dataLength_ ( 0 ),
    startOfData_ ( 0 )
{
}

WaveFile::~WaveFile ( ) {
    if ( ( fp_ == NULL ) || readNotWrite_ ) {
        return;
    }

    writeHeaders ( fp_, bitsPerSample_, samplerate_, channels_, samples_ );
}

std::string WaveFile::writeHeaders ( FILE *fp, unsigned long bitsPerSample, unsigned long samplerate, unsigned long channels, unsigned long long samplesWritten ) {
    if ( fseek ( fp, 0, SEEK_SET ) != 0 ) 	{
        return "Error positioning file to start of file.";
    }

    unsigned long bytesPerSample = bitsPerSample == 20 ? 3 : bitsPerSample / 8;

    uint64_t dataLength = static_cast<uint64_t> ( bytesPerSample )
                        * static_cast<uint64_t> ( channels )
                        * static_cast<uint64_t> ( samplesWritten );

    WAVFILE_HEADER wh;

    uint64_t mainLength = static_cast<uint64_t> ( sizeof ( wh.main_length ) )
                        + static_cast<uint64_t> ( sizeof ( WAVFILE_FMTCHUNK ) )
                        + static_cast<uint64_t> ( sizeof ( WAVFILE_CHUNK ) )
                        + static_cast<uint64_t> ( sizeof ( WAVFILE_DS64CHUNK ) )
                        + dataLength;

    const bool makeRf64 = mainLength > std::numeric_limits<uint32_t>::max() ? true : false;

    wh.main_chunk[0] = 'R';
    if ( makeRf64 )	{
        wh.main_chunk[1] = 'F';
        wh.main_chunk[2] = '6';
        wh.main_chunk[3] = '4';
        wh.main_length = static_cast<uint32_t> ( -1 );
    } else {
        wh.main_chunk[1] = 'I';
        wh.main_chunk[2] = 'F';
        wh.main_chunk[3] = 'F';
        wh.main_length = static_cast<unsigned long> ( mainLength );
    }
    wh.chunk_type[0] = 'W';
    wh.chunk_type[1] = 'A';
    wh.chunk_type[2] = 'V';
    wh.chunk_type[3] = 'E';

    if ( fwrite ( &wh, sizeof ( WAVFILE_HEADER ), 1, fp ) != 1 ) {
        fclose ( fp );
        return "Error writing Wave-Header to file.";
    }

    WAVFILE_DS64CHUNK ds64 = { 0 };
    ds64.ds64_length = sizeof ( WAVFILE_DS64CHUNK ) - sizeof ( WAVFILE_CHUNK );

    if ( makeRf64 ) {
        ds64.ds64_chunk[0] = 'd';
        ds64.ds64_chunk[1] = 's';
        ds64.ds64_chunk[2] = '6';
        ds64.ds64_chunk[3] = '4';
        ds64.riffSize = mainLength;
        ds64.dataSize = dataLength;
        ds64.sampleCount = samplesWritten;
    } else {
        ds64.ds64_chunk[0] = 'j';
        ds64.ds64_chunk[1] = 'u';
        ds64.ds64_chunk[2] = 'n';
        ds64.ds64_chunk[3] = 'k';
    }

    if ( fwrite ( &ds64, sizeof ( WAVFILE_DS64CHUNK ), 1, fp ) != 1 ) {
        fclose ( fp );
        if ( makeRf64 ) {
            return "Error writing ds64-Chunk to file.";
        } else {
            return "Error writing junk-Chunk to file.";
        }
    }

    WAVFILE_FMTCHUNK fmt = { 0 };

    fmt.fmt_chunk[0] = 'f';
    fmt.fmt_chunk[1] = 'm';
    fmt.fmt_chunk[2] = 't';
    fmt.fmt_chunk[3] = ' ';
    fmt.fmt_length = sizeof ( WAVFILE_FMTCHUNK ) - sizeof ( WAVFILE_CHUNK );
    fmt.formatTag = 1; // PCM = linear
    fmt.nChannels = static_cast<short> ( channels );
    fmt.nSamplesPerSec = static_cast<long> ( samplerate );
    fmt.nBitsPerSample = static_cast<short> ( bitsPerSample );
    fmt.nAvgBytesPerSec = fmt.nSamplesPerSec * fmt.nChannels * fmt.nBitsPerSample / 8;
    fmt.nBlockAlign = static_cast<short> ( bytesPerSample ) * fmt.nChannels;

    if ( fwrite ( &fmt, sizeof ( WAVFILE_FMTCHUNK ), 1, fp ) != 1 ) {
        fclose ( fp );
        return "Error writing Format-Chunk to file.";
    }

    WAVFILE_CHUNK data = { 0 };

    data.chunk[0] = 'd';
    data.chunk[1] = 'a';
    data.chunk[2] = 't';
    data.chunk[3] = 'a';
    if ( makeRf64 ) {
        data.length = static_cast<uint32_t> ( -1 );
    } else {
        data.length = static_cast<unsigned long> ( dataLength );
    }

    if ( fwrite ( &data, sizeof ( WAVFILE_CHUNK ), 1, fp ) != 1 ) {
        fclose ( fp );
        return "Error writing Data-Chunk-Header to file.";
    }

    long startOfData = ftell ( fp );

    if ( startOfData == -1 ) {
        fclose ( fp );
        return "Error retreiving file position.";
    }

    samplerate_ = samplerate;
    bitsPerSample_ = bitsPerSample;
    channels_ = channels;
    startOfData_ = startOfData;

    return "";
}

std::string WaveFile::readHeaders ( FILE* fp ) {
    if ( fseek ( fp, 0, SEEK_SET ) != 0 ) {
        return "Error positioning file to start of file.";
    }

    WAVFILE_HEADER wh;

    if ( fread ( &wh, sizeof ( WAVFILE_HEADER ), 1, fp ) != 1 ) {
        fclose ( fp );
        return "Error reading Wave-Header from file.";
    }

    if ( strncmp ( wh.chunk_type, "WAVE", sizeof ( wh.chunk_type ) ) != 0 ) {
        fclose ( fp );
        return "Error in Wave-Header: Chunk type incorrect.";
    }

    if ( strncmp ( wh.main_chunk, "RIFF", sizeof ( wh.main_chunk ) ) != 0 && strncmp ( wh.main_chunk, "RF64", sizeof ( wh.main_chunk ) ) != 0 ) {
        fclose ( fp );
        return "Error in Wave-Header: Main chunk incorrect.";
    }

    bool isRf64 = false;
    WAVFILE_DS64CHUNK ds64 = { 0 };
    if ( strncmp ( wh.main_chunk, "RF64", sizeof ( wh.main_chunk ) ) == 0 ) {
        isRf64 = true;

        std::string result = searchChunk ( fp, "ds64" );

        if ( !result.empty ( ) ) {
            return result;
        }

        if ( fread ( &ds64, sizeof ( WAVFILE_DS64CHUNK ), 1, fp ) != 1 ) {
            fclose ( fp );
            return "Error reading ds64-Chunk from file.";
        }
    }

    std::string result = searchChunk ( fp, "fmt " );

    if ( !result.empty ( ) ) {
        return result;
    }

    WAVFILE_FMTCHUNK fmt;

    if ( fread ( &fmt, sizeof ( WAVFILE_FMTCHUNK ), 1, fp ) != 1 ) {
        fclose ( fp );
        return "Error reading Format-Chunk from file.";
    }

    if ( fmt.formatTag != WAVE_FORMAT_PCM ) {
        if ( fmt.formatTag != WAVE_FORMAT_EXTENSIBLE ) {
            fclose ( fp );
            return "Error in Format-Chunk: No WAVE_FORMAT_PCM file.";
        }

        WAVFILEEXT_FMTCHUNKEXT fmtext;
        if ( fread ( &fmtext, sizeof ( WAVFILEEXT_FMTCHUNKEXT ), 1, fp ) != 1 ) {
            fclose ( fp );
            return "Error reading Format-Chunk from file.";
        }

        if ( memcmp ( &fmtext.SubFormat, &KSDATAFORMAT_SUBTYPE_PCM, sizeof ( WaveFile::GUID ) ) != 0 ) {
            fclose ( fp );
            return "Error in Format-Chunk: No WAVE_FORMAT_PCM file.";
        }
    }

    result = searchChunk ( fp, "data" );

    if ( !result.empty ( ) ) {
        return result;
    }

    WAVFILE_CHUNK data;

    if ( fread ( &data, sizeof ( WAVFILE_CHUNK ), 1, fp ) != 1 ) {
        fclose ( fp );
        return "Error reading Data-Chunk-Header from file.";
    }

    long startOfData = ftell ( fp );

    if ( startOfData == -1 ) {
        fclose ( fp );
        return "Error retreiving file position.";
    }

    samplerate_ = static_cast<unsigned long> ( fmt.nSamplesPerSec );
    bitsPerSample_ = static_cast<unsigned long> ( fmt.nBitsPerSample );
    channels_ = static_cast<unsigned long> ( fmt.nChannels );
    dataLength_ = isRf64 ? ds64.dataSize : data.length;
    startOfData_ = startOfData;

    return "";
}

std::string WaveFile::searchChunk ( FILE* fp, const char *identifier ) {
    struct DUMMY_HEADER {
        char			id[4];
        uint32_t	length;

    } dummyHeader;

    if ( fp == NULL ) {		// file is not open
        return "Error file is not open.";
    }

    long long currentPos = ftell ( fp );

    if ( currentPos == -1 ) {
        return "Error reading file position.";
    }

    if ( fseek ( fp, sizeof ( WAVFILE_HEADER ), SEEK_SET ) != 0 ) {
        return "Error setting file position.";
    }

    for ( ;; ) {
        if ( fread ( &dummyHeader, sizeof ( dummyHeader ), 1, fp ) != 1 ) {
            return "Error reading from file.";
        }

        if ( strncmp ( dummyHeader.id, identifier, sizeof ( dummyHeader.id ) ) == 0 ) {
            // we have found the chunk
            long long offset = 0LL - sizeof ( dummyHeader );
            if ( fseek ( fp, offset, SEEK_CUR ) != 0 ) {
                fseek ( fp, currentPos, SEEK_SET );
                return "Error setting file position.";
            }

            return "";
        }

        long long headerLength = dummyHeader.length;
        if ( fseek ( fp, headerLength, SEEK_CUR ) != 0 ) {
            fseek ( fp, currentPos, SEEK_SET );
            return "Error setting file position.";
        }
    }
}

std::string WaveFile::open ( std::filesystem::path filename, unsigned long samplerate, unsigned long bitsPerSample, unsigned long channels ) {
    FILE* fp = fopen ( filename.string().c_str(), "wb" );

    if ( fp == NULL ) {
        return "Error opening file \"" + filename.string() + "\".";
    }

    std::string result = writeHeaders ( fp, bitsPerSample, samplerate, channels, 0 );

    if ( !result.empty ( ) ) {
        return result;
    }

    fp_ = fp;
    filename_ = filename.string();
    samples_ = 0;
    readNotWrite_ = false;
    dataLength_ = 0;

    return "";
}

std::string WaveFile::open ( std::filesystem::path filename) {
    FILE* fp = fopen ( filename.string().c_str(), "rb" );

    if ( fp == NULL ) {
        return "Error opening file \"" + filename.string() + "\".";
    }

    std::string result = readHeaders ( fp );

    if ( !result.empty ( ) ) {
        return result;
    }

    fp_ = fp;
    filename_ = filename.string();
    samples_ = 0;
    readNotWrite_ = true;

    return "";
}

std::string WaveFile::close ( ) {
    if ( fp_ == NULL ) {
        return "";
    }

    if ( !readNotWrite_ ) {
        std::string result = writeHeaders ( fp_, bitsPerSample_, samplerate_, channels_, samples_ );

        if ( !result.empty ( ) ) {
            return result;
        }
    }

    if ( fclose ( fp_ ) != 0 ) {
        return "Error closing file \""s + filename_ + "\".";
    }

    fp_ = NULL;
    filename_ = "";
    samplerate_ = 0;
    bitsPerSample_ = 0;
    channels_ = 0;
    samples_ = 0;
    readNotWrite_ = true;
    dataLength_ = 0;
    startOfData_ = 0;

    return "";
}

std::string WaveFile::write ( void const *data, unsigned long samples ) {
    if ( fp_ == NULL ) {
        return "Error file not open.";
    }

    unsigned long bytesPerSample = getBytesPerSample ( );
    int blocksize = samples * bytesPerSample * channels_;

    if ( fwrite ( data, static_cast<size_t> ( blocksize ), 1, fp_ ) != 1 ) {
        return "Error writing data to file.";
    }

    samples_ += samples;
    dataLength_ = samples_ * bytesPerSample * channels_;

    return "";
}

unsigned long WaveFile::read ( unsigned long samples, void *data ) {
    if ( fp_ == NULL ) {
        return 0;
    }

    unsigned long bytesPerSample = getBytesPerSample ( );
    size_t blocksize = static_cast<size_t> ( bytesPerSample * channels_ );
    unsigned long long samplesInFile = getTotalSamples ( );
    assert(samplesInFile >= samples_);
    unsigned long long samplesToRead = samplesInFile - samples_;
    if (samplesInFile - samples_ > samples) {
        samplesToRead = samples;
    }

    unsigned long samplesRead = static_cast<unsigned long> ( fread ( data, blocksize, static_cast<size_t> ( samplesToRead ), fp_ ) );
    if (samplesRead == 0 ) {
        return 0;
    }

    samples_ += samplesRead;

    return samplesRead;
}

bool WaveFile::setPosition ( long long samples, int origin ) {
    if ( fp_ == NULL ) {
        return false;
    }

    if ( ( ( samples < 0 ) && ( origin == SEEK_SET ) ) ||
        ( ( samples > 0 ) && ( origin == SEEK_END ) ) ||
        ( static_cast<unsigned long long> ( samples ) > getTotalSamples ( ) ) ) {
        return false;
    }

    switch ( origin ) {
        default:
            return false;

        case SEEK_CUR:
            return ( setPositionRelative ( samples ) );

        case SEEK_SET:
            return ( setPositionAbsoluteForward ( static_cast<unsigned long long> ( samples ) ) );

        case SEEK_END:
            return ( setPositionAbsoluteBackward ( static_cast<unsigned long long> ( samples ) ) );
    }
}

bool WaveFile::setPositionRelative ( long long samples ) {
    if ( fp_ == NULL ) {
        return false;
    }

    if ( samples == 0 ) {
        return true;
    }

    unsigned long bytesPerSample = getBytesPerSample ( );
    size_t blocksize = static_cast<size_t> ( bytesPerSample * channels_ );
    long long samplesLeft = getTotalSamples ( ) - samples_;

    if ( samples < 0 ) {
        unsigned long long ulSamples = static_cast<unsigned long long> ( samples * -1 );
        if ( ulSamples > samples_ ) {
            return false;
        }

    } else {
        if ( static_cast<long long> ( samples ) > samplesLeft ) {
            return false;
        }
    }

    long long position = static_cast<long long> ( blocksize ) * samples;

    if ( fseek ( fp_, position, SEEK_CUR ) != 0 ) {
        return false;
    }

    samples_ += samples;

    return true;
}

bool WaveFile::setPositionAbsoluteForward ( unsigned long long samples ) {
    if ( fp_ == NULL ) {
        return false;
    }

    if ( samples == 0 ) {
        return true;
    }

    if ( ( samples < 0 ) || ( samples > getTotalSamples ( ) ) ) {
        return false;
    }

    unsigned long bytesPerSample = getBytesPerSample ( );
    size_t blocksize = static_cast<size_t> ( bytesPerSample * channels_ );

    long long position = static_cast<long long> ( blocksize ) * static_cast<long long> ( samples );

    if ( fseek ( fp_, startOfData_ + position, SEEK_SET ) != 0 ) {
        return false;
    }

    samples_ = samples;

    return true;
}

bool WaveFile::setPositionAbsoluteBackward ( unsigned long long samples ) {
    if ( fp_ == NULL ) {
        return false;
    }

    if ( samples == 0 ) {
        return true;
    }

    if ( ( samples < 0 ) || ( samples > getTotalSamples ( ) ) ) {
        return false;
    }

    unsigned long bytesPerSample = getBytesPerSample ( );
    size_t blocksize = static_cast<size_t> ( bytesPerSample * channels_ );

    long long position = static_cast<long long> ( blocksize ) * static_cast<long long> ( samples );

    if ( fseek ( fp_, startOfData_ + dataLength_ - position, SEEK_SET ) != 0 ) {
        return false;
    }

    samples_ = getTotalSamples ( ) - samples;

    return true;
}
