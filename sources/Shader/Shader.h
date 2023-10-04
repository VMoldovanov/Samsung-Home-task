#pragma once

class Shader final
{
public:
   Shader() : _idProgram(0), _timeVS(0), _timeFS(0)
   {}

   // запрещаем не используемые операции
   Shader(const Shader& shader) = delete;
   Shader& operator=(const Shader& shader) = delete;
   Shader(Shader&& shader) = delete;
   Shader& operator=(Shader&& shader) = delete;

   void Release()
   {
      // удаляем программу из GPU-памяти
      glDeleteProgram(_idProgram);
      // сбрасываем идентификатор
      _idProgram = 0;
   }

   void Load(const std::filesystem::path& pathVS,                       // локальный путь к вершинному шейдеру
      const std::filesystem::path& pathFS,                              // локальный путь к фрагментному шейдеру
      std::function<void(const Shader&)> SetStaticUniformsCallback)     // callback для инициализации не меняющихся uniform-переменных
   {
      // получаем код шейдера и флаг запроса на пересборку программы
      bool needBuildVS = LoadCodeShader(pathVS, _codeVS, _timeVS);
      bool needBuildFS = LoadCodeShader(pathFS, _codeFS, _timeFS);
      // если как-то шейдер обновился, то пересобираем программу
      if (needBuildVS || needBuildFS)
      {
         // сохраняем время сборки
         auto time = std::time(nullptr);
         auto formatedTime = std::put_time(std::localtime(&time), "[%Y-%m-%d %H:%M]");
         // обрабатываем секцию
         try
         {
            // компилируем шейдера
            unsigned int idVS = CompileShader(GL_VERTEX_SHADER, _codeVS.c_str(), pathVS);
            unsigned int idFS = CompileShader(GL_FRAGMENT_SHADER, _codeFS.c_str(), pathFS);
            // линкуем программу
            LinkProgram(idVS, idFS);
            // вызываем callback для инициализации uniform-переменных
            if (SetStaticUniformsCallback)
            {
               glUseProgram(_idProgram);
               SetStaticUniformsCallback(*this);
               glUseProgram(0);
            }
         }
         catch (std::string& logError)
         {
            // выводим сообщение об ошибке
            std::cerr << formatedTime << " Shader program build error!" << std::endl;
            std::cerr << logError;
            return;
         }
         // сообщаем об успешной сборке программы
         std::cout << formatedTime << " Shader program build success!" << std::endl;
      }
   }

   void Bind(std::function<void(const Shader&)> SetDynamicUniformsCallback = nullptr)
   {
      // активируем программный объект
      glUseProgram(_idProgram);

      // вызываем callback для переопределения часто меняющихся uniform-переменных
      if (_idProgram != 0 && SetDynamicUniformsCallback)
      {
         SetDynamicUniformsCallback(*this);
      }
   }
   void Unbind()
   {
      glUseProgram(0);
   }

   void SetUniformInt(const std::string& name, int value) const
   {
      glUniform1i(glGetUniformLocation(_idProgram, name.c_str()), value);
   }
   void SetUniformFloat(const std::string& name, float value) const
   {
      glUniform1f(glGetUniformLocation(_idProgram, name.c_str()), value);
   }
   void SetUniformVec2(const std::string& name, const glm::vec2& value) const
   {
      glUniform2fv(glGetUniformLocation(_idProgram, name.c_str()), 1, glm::value_ptr(value));
   }
   void SetUniformVec3(const std::string& name, const glm::vec3& value) const
   {
      glUniform3fv(glGetUniformLocation(_idProgram, name.c_str()), 1, glm::value_ptr(value));
   }
   void SetUniformMat3(const std::string& name, const glm::mat3& value) const
   {
      glUniformMatrix3fv(glGetUniformLocation(_idProgram, name.c_str()), 1, GL_FALSE, glm::value_ptr(value));
   }
   void SetUniformMat4(const std::string& name, const glm::mat4& value) const
   {
      glUniformMatrix4fv(glGetUniformLocation(_idProgram, name.c_str()), 1, GL_FALSE, glm::value_ptr(value));
   }

private:
   bool LoadCodeShader(const std::filesystem::path& pathShader,
      std::string& codeShader,                    // возвращаем исходный код шейдера
      std::time_t& timeLastWrite)                 // возвращаем время последнего обновления файла с исходным кодом
   {
      // проверка файла на существование
      if (!std::filesystem::exists(pathShader))
      {
         std::cerr << "Error! The required file does not exist: " << pathShader << std::endl;
         // обновления шейдера не требуется из-за ошибки
         return false;
      }
      // проверяем время последнего изменения файла или наличие кода
      auto time = GetFileWriteTime(pathShader);
      if (timeLastWrite != time || codeShader.empty())
      {
         try
         {
            // сохраняем время последнего изменения файла
            timeLastWrite = time;
            // открываем файл на чтение
            std::ifstream fileStader(pathShader, std::ios::in);
            // получаем содержимое файлового буфера
            std::stringstream streamShader;
            streamShader << fileStader.rdbuf();
            // сохраняем данные из потока в строку
            codeShader = streamShader.str();
            // закрываем файл
            fileStader.close();
         }
         catch (...)
         {
            std::cerr << "Error reading from a file: " << pathShader << std::endl;
            // обновления шейдера не требуется из-за ошибки
            return false;
         }
         // требуется пересобрать шейдер и программу
         return true;
      }
      // обновления шейдера не требуется
      return false;
   }

   unsigned int CompileShader(GLenum shaderType,         // тип шейдера
      const char* codeShader,                            // код шейдера
      const std::filesystem::path& pathShader)           // путь к файлу шейдера (для отладочной информации)
   {
      // получаем уникальный идентификатор шейдерного объекта
      unsigned int idShader = glCreateShader(shaderType);
      // загружаем в шейдерный объект исходный код шейдера
      glShaderSource(idShader, 1, &codeShader, NULL);
      // компилируем исходный код
      glCompileShader(idShader);

      // проверяем на наличие ошибок
      GLint isCompiled = 0;
      glGetShaderiv(idShader, GL_COMPILE_STATUS, &isCompiled);
      if (isCompiled == GL_FALSE)
      {
         // получаем длину сообщения об ошибке
         GLint maxLength = 0;
         glGetShaderiv(idShader, GL_INFO_LOG_LENGTH, &maxLength);
         // получаем сообщение об ошибке
         std::vector<char> errorLog(maxLength);
         glGetShaderInfoLog(idShader, maxLength, &maxLength, &errorLog[0]);

         // удаляем шейдерный объект из GPU-памяти
         glDeleteShader(idShader);
         // удаляем программный объект из GPU-памяти
         Release();

         // формируем сообщение для исключения
         std::string logError = "\tSHADER_COMPILATION_ERROR: " + pathShader.string() + "\n\t" + std::string(&errorLog[0]);
         // генериуем исключение
         throw logError;
      }
      // возвращаем id шейдерного объекта
      return idShader;
   }

   void LinkProgram(unsigned int idVS, unsigned int idFS)
   {
      // удаляем старый программный объект из GPU-памяти
      Release();

      // получаем уникальный идентификатор шейдерной программы
      _idProgram = glCreateProgram();
      // присоединяем к программе шейдерные объекты
      glAttachShader(_idProgram, idVS);
      glAttachShader(_idProgram, idFS);
      // линкуем программу
      glLinkProgram(_idProgram);
      // удаляем шейдерные объекты из GPU-памяти
      glDeleteShader(idVS);
      glDeleteShader(idFS);

      // проверяем на наличие ошибок
      GLint isCompiled = 0;
      glGetProgramiv(_idProgram, GL_LINK_STATUS, &isCompiled);
      if (isCompiled == GL_FALSE)
      {
         // получаем длину сообщения об ошибке
         GLint maxLength = 0;
         glGetShaderiv(_idProgram, GL_INFO_LOG_LENGTH, &maxLength);
         // получаем сообщение об ошибке
         std::vector<GLchar> errorLog(maxLength);
         glGetShaderInfoLog(_idProgram, maxLength, &maxLength, &errorLog[0]);

         // удаляем программный объект из GPU-памяти
         Release();

         // формируем сообщение для исключения
         std::string logError = "\tPROGRAM_LINKING_ERROR:\n\t" + std::string(&errorLog[0]);
         // генериуем исключение
         throw logError;
      }
   }

   std::time_t GetFileWriteTime(const std::filesystem::path& filename)
   {
#if defined ( _WIN32 )
      {
         struct _stat64 fileInfo;
         if (_wstati64(filename.wstring().c_str(), &fileInfo) != 0)
         {
            std::cerr << "Error getting the time of the last file overwrite.";
            return 0;
         }
         return fileInfo.st_mtime;
      }
#else
      {
         auto fsTime = std::filesystem::last_write_time(filename);
         return decltype (fsTime)::clock::to_time_t(fsTime);
      }
#endif
   }

private:
   // уникальный идентификатор объекта шейдерной программы
   unsigned int _idProgram;

   // код шейдеров
   std::string _codeVS, _codeFS;

   // время последнего изменения файлов шейдеров
   std::time_t _timeVS, _timeFS;
};