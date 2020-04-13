#pragma once

#include "mesh.h"
#include "imageLoader.hpp"
#include <glad/glad.h>

GLuint generateBuffer(Mesh &mesh);
unsigned int getTextureID(PNGImage* img);