#pragma once

class StreamToFile final
{
public:
   explicit StreamToFile(const std::string& filename, std::ostream& redirectedStream = std::cerr)
      : _redirectedStream(redirectedStream), _bufferStream(redirectedStream.rdbuf())
   {
      if (!StreamToFile::OutFile)
         OutFile = std::make_shared<std::ofstream>(filename.c_str());
      _redirectedStream.rdbuf(StreamToFile::OutFile->rdbuf());
   }
   ~StreamToFile()
   {
      // восстанавливаем вывод в исходный буфер (вывод на консоль)
      _redirectedStream.rdbuf(_bufferStream);
   }

   // запрещаем не используемые операции
   StreamToFile() = delete;
   StreamToFile(const StreamToFile& streamToFile) = delete;
   StreamToFile& operator=(const StreamToFile& streamToFile) = delete;
   StreamToFile(StreamToFile&& streamToFile) = delete;
   StreamToFile& operator=(StreamToFile&& streamToFile) = delete;

private:
   // статический указатель дл¤ хранени¤ открытого файла
   static std::shared_ptr<std::ofstream> OutFile;

   // ссылка на перенаправл¤емый в файл поток
   std::ostream& _redirectedStream;
   // ссылка на исходный буфер перенаправл¤емого потока
   std::streambuf* _bufferStream;
};