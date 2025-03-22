#pragma once
#include <iostream>
#include <vector>
#include "glm/glm.hpp"
using namespace std;

// Struct for holding vertex data
struct SimpleVertex {
	glm::vec3 pos;
	glm::vec4 color;
};

// Struct for holding mesh data
template<typename T>
struct Mesh {
	vector<T> vertices {};
	vector<unsigned int> indices {};
};
