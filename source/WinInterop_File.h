#pragma once
#include "Math.h"

#include <string>
#include <vector>

struct File {
    enum class Mode {
        Read,  //Read:   Open file for read access.
        Write, //Write:  Open and empty file for output.
        //Append,//Append: Open file for output.
    };

    bool m_handleIsValid     = false;
    bool m_textIsValid       = false;
    bool m_timeIsValid       = false;
    bool m_binaryDataIsValid = false;
    std::string m_filename;
    std::string         m_dataString;
    std::vector<u8>  m_dataBinary;
    u64 m_time = {};

    File(char const* fileName,        File::Mode fileMode, bool createIfNotFound);
    File(const std::string& fileName, File::Mode fileMode, bool createIfNotFound);
    ~File();

    bool Write(const std::string& text);
    bool Write(void* data, size_t sizeInBytes);
    bool Write(const void* data, size_t sizeInBytes);
    void GetData();
    void GetText();
    void GetTime();
    bool Delete();

private:

    void* m_handle;
    u32  m_accessType;
    u32  m_shareType;
    u32  m_openType;

    void GetHandle();
    void Init(const std::string& filename, File::Mode fileMode, bool createIfNotFound);
    bool FileDestructor();
};
