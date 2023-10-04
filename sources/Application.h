#pragma once

// включаем _CRT_SECURE_NO_WARNINGS
#pragma warning( disable : 4996 )

// С библиотеки
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// STL библиотеки
#include <functional>
#include <filesystem>
#include <iostream>
#include <fstream>
#include <sstream>
#include <memory>
#include <chrono>

// сгенерированный для ядра OpenGL|ES 3.2
#include <glad/glad.h>
// API для создания окон, контекстов и поверхностей, получения входных данных и событий
#include <GLFW/glfw3.h>
#pragma comment(lib, "GLFW/glfw3.lib")
// загрузчик файлов-изображений
#define STB_IMAGE_IMPLEMENTATION
#include "STB/stb_image.h"
// математическая библиотека на спецификации GLSL
#include <glm/glm.hpp>
#include <glm/ext.hpp>

// файлы проекта
#include "Shader/Shader.h"
#include "StreamToFile/StreamToFile.h"
