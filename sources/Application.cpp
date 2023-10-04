#include "Application.h"		// Precompiled Header

//StreamToFile cerrToFile("cerr.log");          // раскоментировать, если нужно сохранить ошибки в файле
Shader shaderHSB;
glm::mat4 matMVP;
glm::mat3 matHSB;
glm::vec2 sizeImage;
glm::ivec2 screenResolution = glm::ivec2(1024, 768);
glm::vec3 changedHSB = glm::vec3(0.0f, 1.0f, 1.0f);
GLuint idVAO = 0;
GLuint idTexture = 0;

void UpdateMVP(int width, int height)
{
   // поскольку происходит вывод 2D-картинки, то создаем 2D-режим работы (камера лишь слегка сдвинута от холста)
   glm::mat4 matProj = glm::ortho(0.f, (float)width, 0.f, (float)height, -10.f, 10.f);
   glm::mat4 matView = glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, 0.0f, -1.0f));
   matMVP = matProj * matView;
}

glm::mat4 GetSaturationMatrix(float saturation)
{
   // "Matrix Operations for Image Processing", Paul Haeberli, 1993
   // https://www.graficaobscura.com/matrix/
   
   float lumR = 0.3086f;
   float lumG = 0.6094f;
   float lumB = 0.0820f;
   float one_minus_sat = 1.0f - saturation;
   float a = one_minus_sat * lumR + saturation;
   float b = one_minus_sat * lumR;
   float c = one_minus_sat * lumR;
   float d = one_minus_sat * lumG;
   float e = one_minus_sat * lumG + saturation;
   float f = one_minus_sat * lumG;
   float g = one_minus_sat * lumB;
   float h = one_minus_sat * lumB;
   float i = one_minus_sat * lumB + saturation;
   glm::mat4 mat = {
        a,      b,      c,      0.0f,
        d,      e,      f,      0.0f,
        g,      h,      i,      0.0f,
        0.0f,   0.0f,   0.0f,   1.0f,
   };
   return mat;
}

void KeyCallback(GLFWwindow* window, int key, int scancode, int action, int mods)
{
   if (action == GLFW_PRESS)
   {
      // закрываем приложение
      if (key == GLFW_KEY_ESCAPE)
         glfwSetWindowShouldClose(window, GL_TRUE);
   }
   // обрабатываем изменения по шкале HBS
   if (action == GLFW_PRESS || action == GLFW_REPEAT)
   {
      // HUE
      float stepH = 5.0f;
      float maxH = 180.0f;
      if (key == GLFW_KEY_Q)  changedHSB.x = glm::clamp(changedHSB.x - stepH, -maxH, maxH);
      if (key == GLFW_KEY_W)  changedHSB.x = 0.0f;
      if (key == GLFW_KEY_E)  changedHSB.x = glm::clamp(changedHSB.x + stepH, -maxH, maxH);
      // SATURATION
      float stepS = 0.1f;
      float maxS = 5.0f;
      if (key == GLFW_KEY_A)  changedHSB.y = glm::clamp(changedHSB.y - stepS, 0.0f, maxS);
      if (key == GLFW_KEY_S)  changedHSB.y = 1.0f;
      if (key == GLFW_KEY_D)  changedHSB.y = glm::clamp(changedHSB.y + stepS, 0.0f, maxS);
      // BRIGHTNESS
      float stepB = 0.1f;
      float maxB = 3.0f;
      if (key == GLFW_KEY_Z)  changedHSB.z = glm::clamp(changedHSB.z - stepB, 0.0f, maxB);
      if (key == GLFW_KEY_X)  changedHSB.z = 1.0f;
      if (key == GLFW_KEY_C)  changedHSB.z = glm::clamp(changedHSB.z + stepB, 0.0f, maxB);

      char buffer[100];
      sprintf(buffer, "Hue:%4.0f, Saturation:%4.1f, Brightness:%4.1f", changedHSB.x, changedHSB.y, changedHSB.z);
      std::cout << buffer << std::endl;
   }
}

void SetStaticUniforms(const Shader& shader)
{
   // переменные, которые устанавливаеются одноразово при компиляции шейдеров
   shader.SetUniformMat4("u_matMVP", matMVP);
   shader.SetUniformInt("u_texture0", 0);
   shader.SetUniformVec2("u_screenResolution", screenResolution);
   shader.SetUniformVec2("u_sizeImage", sizeImage);
   shader.SetUniformFloat("u_Pi", glm::pi<float>());
}

void SetDynamicUniforms(const Shader& shader)
{
   // вычисляем матрицы для Image Processing
   glm::mat4 matHue = glm::rotate(glm::mat4(1.0f), glm::radians(changedHSB.x), glm::normalize(glm::vec3(1.0f, 1.0f, 1.0f)));
   glm::mat4 matSaturate = GetSaturationMatrix(changedHSB.y);
   glm::mat4 matBrightness = glm::scale(glm::mat4(1.0f), glm::vec3(changedHSB.z, changedHSB.z, changedHSB.z));
   matHSB = glm::mat3(matBrightness * matSaturate * matHue);

   // переменные, которые задаются при каждой активации шейдера (т.е. обновление данных)
   shader.SetUniformMat3("u_matHSB", matHSB);
}

void InitData(GLFWwindow* window)
{
   // 1. инициализируем VAO
   float positions[] = {
      0.0f, 1.0f, 0.0f,			// верхняя левая 
      0.0f, 0.0f, 0.0f,			// нижняя левая		 
      1.0f, 1.0f, 0.0f,			// верхняя правая
      1.0f, 0.0f, 0.0f,			// нижняя правая
   };
   float texCoords[] = {
      0.0f,  1.0f,				// верхняя левая 
      0.0f,  0.0f,				// нижняя левая		 
      1.0f,  1.0f,				// верхняя правая
      1.0f,  0.0f,				// нижняя правая
   };
   unsigned int indices[] = { 0, 1, 2, 3 };
      
   glGenVertexArrays(1, &idVAO);
   GLuint idVBO;
   glGenBuffers(1, &idVBO);
   GLuint idEBO;
   glGenBuffers(1, &idEBO);

   glBindVertexArray(idVAO);
   {
      glBindBuffer(GL_ARRAY_BUFFER, idVBO);
      glBufferData(GL_ARRAY_BUFFER, sizeof(positions) + sizeof(texCoords), 0, GL_STATIC_DRAW);
      glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(positions), &positions);
      glBufferSubData(GL_ARRAY_BUFFER, sizeof(positions), sizeof(texCoords), &texCoords);

      glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, idEBO);
      glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);

      glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), 0);
      glEnableVertexAttribArray(0);
      glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), (void*)(sizeof(positions)));
      glEnableVertexAttribArray(1);
   }
   glBindVertexArray(0);

   glBindBuffer(GL_ARRAY_BUFFER, 0);
   glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
   glDeleteBuffers(1, &idVBO);
   glDeleteBuffers(1, &idEBO);

   // 2. загружаем текстуру из файла с фото-изображением
   glGenTextures(1, &idTexture);   
   glBindTexture(GL_TEXTURE_2D, idTexture);
   {
      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

      int widthImage, heightImage, channelsImage;
      stbi_set_flip_vertically_on_load(true);
      unsigned char* data = stbi_load("picture.png", &widthImage, &heightImage, &channelsImage, 0);
      if (data)
         glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, widthImage, heightImage, 0, GL_RGB, GL_UNSIGNED_BYTE, data);
      else
         std::cerr << "Failed to load file: 'picture.png'" << std::endl;
      stbi_image_free(data);

      // пересчитываем размеры картинки, чтобы они поместились в окно
      float newWidth = (float)widthImage;
      float newHeight = (float)heightImage;
      if (widthImage >= heightImage)
      {
         if (widthImage > screenResolution.x)
         {
            newWidth = (float)screenResolution.x;
            newHeight = newWidth * (float)heightImage / (float)widthImage;
         }
      }
      else
      {
         if (heightImage > screenResolution.y)
         {
            newHeight = (float)screenResolution.y;
            newWidth = newHeight * (float)widthImage / (float)heightImage;
         }
      }
      sizeImage = glm::vec2(newWidth, newHeight);
   }
   glBindTexture(GL_TEXTURE_2D, 0);

   // 3. обновляем матрицу MVP (обычно эта матрица активно меняется из-за изменения положения камеры, но в 2D-режиме это не нужно)
   UpdateMVP(screenResolution.x, screenResolution.y);

   // 4. загружаем шейдер
   shaderHSB.Load("shaders/HBS.vert", "shaders/HBS.frag", SetStaticUniforms);
}

void ReleaseData()
{
   shaderHSB.Release();
   glDeleteVertexArrays(1, &idVAO);
   glDeleteTextures(1, &idTexture);
}

void display(GLFWwindow* window)
{
   // ВНИМАНИЕ! 
   // Это отладочный механизм пересборки шейдерной программы "на лету", когда отслеживается время последней перезаписи файла.
   // Если отладка в реальном времени не нужна, то эту строчку нужно закоментировать!
   shaderHSB.Load("shaders/HBS.vert", "shaders/HBS.frag", SetStaticUniforms);

   // задаем окно просмотра
   glViewport(0, 0, screenResolution.x, screenResolution.y);
   // чистим фреймбуфер
   glClearColor(0.1f, 0.15f, 0.15f, 1.0f);
   glClear(GL_COLOR_BUFFER_BIT);

   // выводим геометрию
   glBindVertexArray(idVAO);
   glBindTexture(GL_TEXTURE_2D, idTexture);
   shaderHSB.Bind(SetDynamicUniforms);
   glDrawElements(GL_TRIANGLE_STRIP, 4, GL_UNSIGNED_INT, 0);
   shaderHSB.Unbind();
   glBindTexture(GL_TEXTURE_2D, 0);
   glBindVertexArray(0);
}

int main()
{
   // инициализируем оконную библиотеку
   if (!glfwInit())
      exit(EXIT_FAILURE);

   // задаем версию OpenGL|ES
   glfwWindowHint(GLFW_CLIENT_API, GLFW_OPENGL_ES_API);
   glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
   glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 2);
   glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

   // настройки окна
   glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);

   // инициализируем окно
   GLFWwindow* window = glfwCreateWindow(screenResolution.x, screenResolution.y, "Samsung R&D Institute - Home task by Vladimir Moldovanov (vlad.moldovanov@gmail.com), 8.Sep-10.Sep.2023", NULL, NULL);
   if (window == NULL)
   {
      std::cerr << "GLFW: Ошибка создания окна приложения." << std::endl;
      glfwTerminate();
      exit(EXIT_FAILURE);
   }     
            
   // выводим в консоли "шапку"
   std::cout << "--------------------------------------------------------------------------------------------------------------------\n";
   std::cout << "The following publicly available libraries are used in the project:\n";
   std::cout << " + GLAD, generated for the OpenGL|ES 3.2 core (https://glad.dav1d.de/)\n";
   std::cout << " + GLFW 3.3.8, API for creating windows, contextsand surfaces, getting input dat aand events (https://www.glfw.org/)\n";
   std::cout << " + stb_image 2.28, image file loader (https://github.com/nothings/stb/blob/master/stb_image.h)\n";
   std::cout << " + GLM 0.9.9.7, mathematics library on the GLSL specifications (https://github.com/g-truc/glm)\n";
   std::cout << "--------------------------------------------------------------------------------------------------------------------\n";
   std::cout << "Press to change the Hue:        Q (dec) - W (reset) - E (inc)\n";
   std::cout << "Press to change the Saturation: A (dec) - S (reset) - D (inc)\n";
   std::cout << "Press to change the Brightness: Z (dec) - X (reset) - C (inc)\n";
   std::cout << "Press 'Esc' to exit the program.\n";
   std::cout << "--------------------------------------------------------------------------------------------------------------------\n";

   // задаем текущий GL-контекст
   glfwMakeContextCurrent(window);

   // связываем адреса GL-методов с реализацией драйвера нужной версии OpenGL|ES
   if (!gladLoadGLES2Loader((GLADloadproc)glfwGetProcAddress))
   {
      std::cerr << "GLAD: Ошибка инициализации адресов" << std::endl;
      exit(EXIT_FAILURE);
   }

   // инициализируем данные в GL-контексте
   InitData(window);

   // задаем обработчик нажатий клавиш клавиатуры
   glfwSetKeyCallback(window, KeyCallback);

   // запускаем основной цикл
   while (!glfwWindowShouldClose(window))
   {
      // вывод во фреймбуфер
      display(window);
      // переключение буферов
      glfwSwapBuffers(window);
      // обработка событий GLFW
      glfwPollEvents();
   }

   // освобождаем наши ресурсы
   ReleaseData();

   // освобождаем ресурсы
   glfwDestroyWindow(window);
   glfwTerminate();
   exit(EXIT_SUCCESS);
}